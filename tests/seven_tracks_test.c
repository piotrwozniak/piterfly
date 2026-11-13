/* SPDX-License-Identifier: GPL-3.0-only */
#define FELUCCA_SYNTH_PARTS 7
#include "editor_host.c"

static void quiet_reset(void)
{
    memset(trk,0,sizeof trk); memset(voice_pool,0,sizeof voice_pool); memset(voice_owner,0,sizeof voice_owner);
    memset(&voice_silent,0,sizeof voice_silent); memset(sl,0,sizeof sl); memset(ins,0,sizeof ins);
    memset(gf,0,sizeof gf); gf_mask=0; memset(eng_arena_own,0,sizeof eng_arena_own);
    song.playing=0; song.rec=song.solo=0; transport_req=panic_req=0;
    host_tracks_init(); fm6_init(); audio_prepare(); gf_layout();
}
static void check_pages(void)
{
    uint8_t used[AUDIO_PAGES]={0};
    for(uint32_t t=0;t<NTRK;t++) for(uint32_t p=0;p<gf[t].np;p++) {
        uint32_t id=gf[t].page[p]; assert(id<AUDIO_PAGES && !audio_blocked[id] && !used[id]); used[id]=1;
    }
}
static uint32_t energy(uint32_t t,uint32_t engine)
{
    quiet_reset();
    host_preset(&trk[t],engine,0);
    trk[t].p[P_LEVEL]=100; trk[t].p[P_ATK]=0;
    trk[t].p[P_AMODE]=0; trk[t].p[P_AHOLD]=0;
    trk_note_on(&trk[t],60,110);
    uint32_t peak=0;
    for(uint32_t b=0;b<350u;b++) {
        int32_t out[CTL*2]; mix_block(out,CTL);
        for(uint32_t i=0;i<CTL*2u;i++) { uint32_t a=(uint32_t)(out[i]<0?-out[i]:out[i]); if(a>peak) peak=a; }
        assert(voices_busy()<=NVOICE && !voice_silent.active); check_pages();
    }
    return peak;
}
static void pattern_fill(void)
{
    quiet_reset();
    for(uint32_t t=0;t<NPART;t++) {
        host_preset(&trk[t],t%NENGINES,0);
        trk[t].p[P_LEVEL]=(int16_t)(50u+t); trk[t].p[P_SLEN]=64;
        step_t *s=&trk[t].step[63];
        *s=(step_t){{(uint8_t)(40u+t),64,67,71},4,ST_NOTE,255,127,0xE4,0x39};
        trk[t].micro[63]=(int8_t)((int32_t)t-3); step_fill_set(&trk[t],63,FC_NOFILL);
        lock_set(&trk[t],63,P_PAN,(int32_t)t-32);
    }
    dstep_set(&TDRUM->dstep[63],15,LV_HARD,3); TDRUM->p[P_SLEN]=64;
    drum_tune[15]=-8; drum_dec[15]=3; song.sel=TRK_DRUM;
}
static void pattern_check(void)
{
    assert(song.sel==TRK_DRUM && drum_tune[15]==-8 && drum_dec[15]==3);
    for(uint32_t t=0;t<NPART;t++) {
        assert(trk[t].p[P_LEVEL]==50+t && trk[t].p[P_SLEN]==64);
        step_t *s=&trk[t].step[63]; assert(s->n==4 && s->note[0]==40+t && s->flags==255 && s->vel==127 && s->lvl==0xE4 && s->rat==0x39);
        assert(trk[t].micro[63]==(int32_t)t-3 && step_fill(&trk[t],63)==FC_NOFILL);
        int32_t l=lock_find(&trk[t],63,P_PAN,0); assert(l>=0 && trk[t].lock[l].val==(int32_t)t-32);
    }
    assert(dstep_has(&TDRUM->dstep[63],15) && dstep_rat(&TDRUM->dstep[63],15)==3);
}
static void relocation_check(void)
{
    quiet_reset();
    uint8_t tail[NTRK][256];
    for(uint32_t t=0;t<NTRK;t++) trk[t].p[P_ITYPE]=IT_GRAIN;
    audio_prepare(); gf_layout();
    for(uint32_t t=0;t<NTRK;t++) {
        gf_state_t *g=&gf[t]; uint32_t cap=gf_capacity(t);
        for(uint32_t i=0;i<cap;i++) *gf_byte(g,i)=(uint8_t)(i*7u+t*31u);
        g->wp=cap-7u; g->valid=cap-1u;
        for(uint32_t i=0;i<256u;i++) tail[t][i]=*gf_byte(g,(g->wp+cap-i-1u)%cap);
    }
    for(uint32_t t=3;t<NPART;t++) trk[t].engine=trk[t].eng_req=ENGI_PHYS;
    for(uint32_t t=0;t<NTRK;t++) trk[t].p[P_SLCR]=SL_STUT;
    audio_prepare(); gf_layout(); check_pages();
    for(uint32_t t=0;t<NTRK;t++) {
        gf_state_t *g=&gf[t]; uint32_t cap=gf_capacity(t); assert(g->valid>=256u);
        for(uint32_t i=0;i<256u;i++) assert(tail[t][i]==*gf_byte(g,(g->wp+cap-i-1u)%cap));
    }
    puts("granular history survives engine and SLICER page reclamation: ok");
}
static void voice_stress(void)
{
    quiet_reset();
    uint32_t seed=9;
    for(uint32_t t=0;t<NPART;t++) host_preset(&trk[t],t%7u,0);
    for(uint32_t b=0;b<2500u;b++) {
        seed=seed*1664525u+1013904223u; uint32_t t=(seed>>16)%NPART;
        if(b%13u==0) trk_all_off(&trk[t]);
        else if(b%3u==0) trk_note_on(&trk[t],36u+(seed>>24)%48u,100);
        if(b%127u==0) { host_preset_req(&trk[t],(seed>>20)%NENGINES,0); }
        int32_t out[CTL*2]; mix_block(out,CTL);
        if(voices_busy()>NVOICE || voice_silent.active) {
            fprintf(stderr,"block %u busy %u silent %u\n",b,voices_busy(),voice_silent.active);
            for(uint32_t a=0;a<NPART;a++) for(uint32_t v=0;v<NVOICE;v++) if(voice_at(&trk[a],v)->active)
                fprintf(stderr,"track %u voice %u mode %u gate %u stage %u note %u slot %u\n",a,v,trk[a].p[P_VOICE],voice_at(&trk[a],v)->gate,voice_at(&trk[a],v)->stage,voice_at(&trk[a],v)->note,trk[a].v[v]);
            abort();
        }
        check_pages();
        for(uint32_t a=0;a<NPART;a++) for(uint32_t v=0;v<NVOICE;v++) if(trk[a].v[v]) {
            assert(trk[a].v[v]<=VOICE_SLOTS);
            assert(voice_owner[trk[a].v[v]-1u]==a*NVOICE+v+1u);
        }
    }
    puts("shared voice allocation, stealing and engine changes across all seven parts: ok");
}
static void legacy_check(void)
{
    harness_init();
    int16_t defaults[4][P_COUNT];
    for(uint32_t k=3;k<NPART;k++) memcpy(defaults[k-3u],trk[k].p,sizeof defaults[0]);
    song.sel=TRK_DRUM; trk[0].step[63]=(step_t){{61,0,0,0},1,ST_NOTE,0,100,0,0};
    dstep_set(&TDRUM->dstep[63],15,LV_HARD,3);
    project_v7_t old; legacy_capture(&old); project_t converted;
    assert(proj_import(&converted,&old,sizeof old) && converted.sel==TRK_DRUM);
    quiet_reset(); proj_apply(&converted,1);
    assert(trk[0].step[63].note[0]==61 && dstep_has(&TDRUM->dstep[63],15));
    for(uint32_t k=3;k<NPART;k++) {
        assert(!memcmp(defaults[k-3u],trk[k].p,sizeof defaults[0]));
        for(uint32_t i=0;i<NSTEP;i++) assert(!trk[k].step[i].n && trk[k].step[i].time==ST_REST);
    }
    uint8_t legacy_midi[MIDI_PART_OLD]={0}; uint32_t entry=22u|3u<<7|(P_PAN+1u)<<9;
    legacy_midi[0]=(uint8_t)entry; legacy_midi[1]=(uint8_t)(entry>>8);
    legacy_midi[32]=1; legacy_midi[33]=2; legacy_midi[34]=3; legacy_midi[35]=10; legacy_midi[36]=2;
    midi_part_read(legacy_midi,sizeof legacy_midi); assert(ml_track(0)==TRK_DRUM);
    assert(midi_learned(22,127) && TDRUM->p[P_PAN]==63 && trk[3].p[P_PAN]==defaults[0][P_PAN]);
    ml_clear_all();
    puts("legacy drum, MIDI Learn and default sounds migrate to the eight-track layout: ok");
}
int main(void)
{
    legacy_check();
    harness_init();
    assert(NPART==7 && NTRK==8 && TRK_DRUM==7 && SMP_USER_SLOTS==1 && sizeof(project_t)==6016);
    assert(sizeof(project_v7_t)==3832 && sizeof(project_v6_t)==3816);
    for(uint32_t t=0;t<NPART;t++) for(uint32_t e=0;e<NENGINES;e++) {
        uint32_t peak=energy(t,e);
        if(!peak) { fprintf(stderr,"silent track %u engine %u\n",t,e); return 1; }
    }
    puts("all 12 engines produce internal audio on each of seven synth tracks: ok");
    relocation_check(); voice_stress();
    quiet_reset();
    for(uint32_t t=0;t<NTRK;t++) {
        if(t<NPART) trk[t].engine=trk[t].eng_req=ENGI_PHYS;
        trk[t].p[P_SLCR]=SL_STUT; trk[t].p[P_ITYPE]=IT_GRAIN;
    }
    for(uint32_t i=0;i<NPART+4u;i++) { ins[i].type=IT_CHORUS; ins[i].w=30000; }
    audio_prepare(); gf_layout(); check_pages();
    for(uint32_t t=0;t<NTRK;t++) assert(gf[t].np>=1u);
    for(uint32_t i=0;i<NPART+4u;i++) { ins[i].line=insert_line(&ins[i]); memset(ins[i].line,0,IL_LEN*sizeof(int16_t)); }
    for(uint32_t t=0;t<NPART;t++) trk_note_on(&trk[t],48u+t,100);
    for(uint32_t b=0;b<500u;b++) { int32_t out[CTL*2]; mix_block(out,CTL); check_pages(); }
    puts("all engine arenas, eight stutters, granular inserts and chorus fade tails fit simultaneously: ok");
    pattern_fill();
    for(uint32_t s=0;s<24;s++) { proj_capture(&proj_tmp.v9); assert(!pb_save(s,&proj_tmp.v9,sizeof(project_t))); }
    assert(!pb_autosave_save(&proj_tmp.v9,sizeof(project_t)));
    memset(proj_slot,0,sizeof proj_slot); quiet_reset(); pb_scan(); autosave_resume(); pattern_check();
    for(uint32_t s=0;s<24;s++) { assert(pb_load(s,&proj_tmp,sizeof proj_tmp)==sizeof(project_t)); assert(proj_ok(&proj_tmp.v9)); }
    puts("24 full eight-track patterns and working project survive loss of RAM: ok");
    track_select(6); song.solo=128; song.rec=128; TDRUM->p[P_MUTE]=1;
    uint8_t a[3]={0}; ed_begin(ED_LIVE); ed_live(a,1);
    assert(ed_out[ed_n-1u]==7u);
    TDRUM->p[P_MUTE]=0; ed_begin(ED_LIVE); ed_live(a,1); assert(ed_out[ed_n-1u]==6u);
    a[0]=15; a[1]=6; a[2]=1; ed_begin(ED_LIVE); ed_live(a,3); assert(song.solo==192);
    ml_clear_all(); assert(ml_learn(22,6,P_LEVEL)==1); trk[6].p[P_LEVEL]=0;
    assert(midi_learned(22,100) && trk[6].p[P_LEVEL]==100);
    uint8_t midi[MIDI_PART]; midi_thru=0x81u; midi_extra[0]=12; midi_part_fill(midi);
    midi_thru=0; midi_extra[0]=0; midi_part_read(midi,sizeof midi);
    trk[6].p[P_LEVEL]=0; assert(midi_learned(22,99) && trk[6].p[P_LEVEL]==99);
    assert(midi_extra[0]==12 && song.g[G_CH4]==12 && song.g[G_THRU4]==1 && song.g[G_THRU1]==1 && midi_thru==0x81u);
    for(uint32_t t=0;t<NTRK;t++) { song.g[trk_ch_param(t)]=(int16_t)(t+1u); assert(midi_own(t)==&trk[t]); }
    song.playing=0; song.rec=0; for(uint32_t t=0;t<NTRK;t++) trk_all_off(&trk[t]); audio_blocks(3000);
    ed_begin(ED_BK_LIST); assert(ed_backup(ED_BK_LIST,a,0)); assert(project_stage_locked(fm1_ms));
    uint32_t staged=st_crc32(&proj_tmp,sizeof(project_t)); trk[0].p[P_LEVEL]++;
    fm1_ms+=AUTOSAVE_GAP+1u; project_stage_ms=fm1_ms; ui_input_ms=0; autosave_ms=0; autosave_checked=0;
    autosave_tick(); assert(st_crc32(&proj_tmp,sizeof(project_t))==staged);
    project_stage_busy=0;
    puts("extended MIDI settings, routing and USB backup staging ownership: ok");
    for(uint32_t t=0;t<NTRK;t++) { track_select(t); studio_open(SC_TRK); ui.force=1; ui_draw(); }
    for(uint32_t t=0;t<NTRK;t++) { song.sel=(uint8_t)t; vis_style=4; ui.force=1; vis_draw(); }
    puts("all eight native track views and USB high mask bits: ok");
    return 0;
}
