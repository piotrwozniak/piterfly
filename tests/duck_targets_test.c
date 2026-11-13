/* SPDX-License-Identifier: GPL-3.0-only */
#define FELUCCA_SYNTH_PARTS 7
#include "editor_host.c"

typedef struct { uint64_t hash, energy; } render_result_t;

/* Fork before starting the host render thread: every comparison starts at boot. */
static render_result_t render_case(uint32_t track,int depth,int enabled)
{
    int fds[2]; assert(!pipe(fds)); pid_t pid=fork(); assert(pid>=0);
    if(!pid) {
        close(fds[0]); harness_init();
        host_preset(&trk[track],2,0);
        track_t *t=&trk[track];
        t->p[P_ATK]=0; t->p[P_DEC]=0; t->p[P_SUS]=100; t->p[P_LEVEL]=60;
        t->p[P_AMODE]=t->p[P_CHOR]=t->p[P_DLY]=t->p[P_REV]=0;
        song.g[G_DUCK]=(int16_t)depth; song.g[G_DUCK1+track]=(int16_t)enabled;
        int32_t out[CTL*2];
        for(uint32_t b=0;b<16;b++) mix_block(out,CTL); /* finish boot/preset panic before the played note */
        trk_note_on(t,60,100);
        for(uint32_t b=0;b<128;b++) mix_block(out,CTL);
        drums.kick=1;
        render_result_t r={1469598103934665603ull,0};
        for(uint32_t b=0;b<128;b++) {
            mix_block(out,CTL);
            for(uint32_t i=0;i<CTL*2;i++) {
                r.hash=(r.hash^(uint32_t)out[i])*1099511628211ull;
                r.energy+=(uint64_t)((int64_t)out[i]*out[i]);
            }
        }
        assert(write(fds[1],&r,sizeof r)==sizeof r); close(fds[1]); _exit(0);
    }
    close(fds[1]); render_result_t r; assert(read(fds[0],&r,sizeof r)==sizeof r); close(fds[0]);
    int status; assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status)); return r;
}

int main(int argc,char **argv)
{
    for(uint32_t t=0;t<NPART;t++) {
        render_result_t dry=render_case(t,0,1), excluded=render_case(t,127,0), ducked=render_case(t,127,1);
        assert(dry.energy && dry.hash==excluded.hash && dry.energy==excluded.energy);
        assert(ducked.energy<dry.energy/2u);
    }
    puts("all seven parts: excluded audio exactly matches depth zero; enabled audio ducks: PASS");

    harness_init();
    uint32_t page=0; while(page<NELEM(PAGES) && !(PAGES[page].scope==SC_TRACK && PAGES[page].id[0]==P_TFLT)) page++;
    assert(page<NELEM(PAGES)); ui.home=0; ui.page=(uint8_t)page;
    for(uint32_t t=0;t<NPART;t++) {
        track_select(t); int16_t *value; const param_desc_t *d=page_desc(cur_page(),1,&value);
        assert(d && !strcmp(d->label,"DUCK") && *value==1);
        uint8_t lock=ui.lock_par; edit_param(1,-1); assert(song.g[G_DUCK1+t]==0 && ui.lock_par==lock);
        edit_param(1,1); assert(song.g[G_DUCK1+t]==1); ui.force=1; ui_draw();
    }
    track_select(TRK_DRUM); int16_t *value; assert(!page_desc(cur_page(),1,&value));
    puts("FILTER knob 2 edits only its synth target; drums have no self-duck control: PASS");

    assert(sizeof(project_t)==6016u);
    for(uint32_t mask=0;mask<128;mask++) {
        for(uint32_t t=0;t<NPART;t++) song.g[G_DUCK1+t]=!((mask>>t)&1u);
        proj_capture(&proj_tmp.v9); assert(proj_ok(&proj_tmp.v9));
        assert(!pb_save(mask%ARR_SCENES,&proj_tmp.v9,sizeof(project_t)));
        for(uint32_t t=0;t<NPART;t++) song.g[G_DUCK1+t]=2; /* force a restore rather than retaining RAM */
        memset(proj_slot,0,sizeof proj_slot); memset(&proj_cache,0,sizeof proj_cache); pb_ready=0;
        assert(pb_load(mask%ARR_SCENES,&proj_tmp,sizeof proj_tmp)==sizeof(project_t));
        proj_apply(&proj_tmp.v9,mask&1u); /* full load and song-section load */
        for(uint32_t t=0;t<NPART;t++) assert(song.g[G_DUCK1+t]==!((mask>>t)&1u));
    }
    proj_tmp.v9.pad[0]=0x7Fu; proj_tmp.v9.pad[1]=0; proj_tmp.v9.sum=proj_sum(&proj_tmp.v9);
    proj_apply(&proj_tmp.v9,1);
    for(uint32_t t=0;t<NPART;t++) assert(song.g[G_DUCK1+t]==1);
    project_v7_t old; legacy_capture(&old); assert(proj_import(&proj_tmp.v9,&old,sizeof old));
    proj_apply(&proj_tmp.v9,1);
    for(uint32_t t=0;t<NPART;t++) assert(song.g[G_DUCK1+t]==1);
    proj_capture(&proj_tmp.v9); assert(!proj_tmp.v9.pad[0] && !proj_tmp.v9.pad[1]);
    puts("all 128 target masks persist without project growth; old FUN9/FUN7 restore all-on: PASS");

    track_select(6); song.g[G_DUCK1+6]=0; proj_capture(&proj_tmp.v9);
    if(argc>1) { FILE *f=fopen(argv[1],"wb"); assert(f && fwrite(&proj_tmp.v9,1,sizeof(project_t),f)==sizeof(project_t)); fclose(f); }
    assert(!pb_autosave_save(&proj_tmp.v9,sizeof(project_t)));
    for(uint32_t t=0;t<NPART;t++) song.g[G_DUCK1+t]=1;
    memset(proj_slot,0,sizeof proj_slot); memset(&proj_cache,0,sizeof proj_cache); pb_ready=0;
    autosave_resume(); assert(!song.g[G_DUCK1+6]);
    song.g[G_DUCK]=127;
    for(uint32_t t=0;t<NPART;t++) song.g[G_DUCK1+t]=1;
    input_on(TDRUM,36,100); assert(drums.kick); audio_blocks(1); assert(duck.g1<32767);
    song.g[G_DUCK1+6]=0; audio_blocks(1); assert((duck.prev&64u) && !(duck.now&64u));
    audio_blocks(1); assert(!(duck.prev&64u)); song.g[G_DUCK1+6]=1; audio_blocks(1);
    assert(!(duck.prev&64u) && (duck.now&64u));
    puts("live kick triggers ducking; target toggles publish ramp endpoints before core dispatch: PASS");

    for(uint32_t t=0;t<NPART;t++) {
        track_t *p=&trk[t]; steps_clear(p); p->p[P_SLEN]=16;
        p->step[0]=(step_t){{(uint8_t)(48+t),0,0,0},1,ST_NOTE,0,90,0,0};
        p->p[P_AMODE]=0;
    }
    steps_clear(TDRUM); dstep_set(&TDRUM->dstep[0],0,LV_NORM,0);
    seq_start(); song.rec=64u; rec_begin(); audio_blocks(2);
    input_on(&trk[6],77,100);
    uint32_t recorded=0;
    for(uint32_t s=0;s<NSTEP;s++) for(uint32_t n=0;n<trk[6].step[s].n;n++) recorded|=trk[6].step[s].note[n]==77;
    assert(recorded && undo.valid && undo.trk==6);
    for(uint32_t b=0;b<128;b++) { if(b%16u==0) song.g[G_DUCK1]=!song.g[G_DUCK1]; audio_blocks(1); assert(voices_busy()<=NVOICE); }
    for(uint32_t t=0;t<NPART;t++) assert(trk[t].seq_abs!=SEQ_NONE);
    input_off(&trk[6],77); song.rec=0; seq_stop();
    puts("eight-track playback and live overdub continue while duck targets change: PASS");

    /* Capture a readable native FILTER page for visual review. */
    song.g[G_DUCK1+6]=0; ui.home=0; ui.page=(uint8_t)page; ui.force=1; ui_draw();
    if(argc>2) { FILE *f=fopen(argv[2],"wb"); assert(f); fprintf(f,"P6\n240 240\n255\n");
        for(uint32_t i=0;i<240*240;i++) { uint16_t p=swap16(screen[i]); uint8_t c[3]={(uint8_t)(((p>>11)&31)*255/31),(uint8_t)(((p>>5)&63)*255/63),(uint8_t)((p&31)*255/31)}; fwrite(c,1,3,f); } fclose(f); }
    return 0;
}
