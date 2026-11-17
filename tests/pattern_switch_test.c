/* SPDX-License-Identifier: GPL-3.0-only */
#define FELUCCA_SYNTH_PARTS 6
#include "editor_host.c"

static void bar(uint32_t n) { clk_beat = n * 4u; live_block(); }
int main(void)
{
    harness_init();
    for (uint32_t k=0;k<NTRK;k++) { trk[k].p[P_SLEN]=16; trk[k].p[P_SDIV]=4; }
    /* Use the actual default 1/16 division rather than assuming its index. */
    for (uint32_t d=0;d<NDIV_STEP;d++) if (div_units(d)==BEAT_U/4u)
        for(uint32_t k=0;k<NTRK;k++) trk[k].p[P_SDIV]=(int16_t)d;
    trk[0].p[P_SLEN]=64;
    assert(section_store(0));
    trk[0].p[P_SLEN]=16;
    assert(section_store(1));
    section_load(0);
    /* Edit a sound and both independent kits after loading A1. */
    set_engine_of(&trk[0],1);
    apply_preset_to(&trk[0],3);
    trk[0].p[P_LEVEL]=71;
    trk[6].p[P_E0]=2; trk[7].p[P_E0]=3;
    song.playing=1; live_bar=0; live_req=1;
    for(uint32_t n=1;n<4;n++) { bar(n); assert(live_sec==0 && live_req==1); }
    trk[0].p[P_LEVEL]=15; /* A temporary step lock must not replace the edited base. */
    trk[0].lk_n=1; trk[0].lk_param[0]=P_LEVEL;
    trk[0].lk_base[0]=72; trk[0].lk_set[0]=15;
    bar(4); assert(live_sec==1 && live_req==-1 && trk[0].p[P_SLEN]==16);
    live_req=0; bar(1);
    assert(live_sec==0 && trk[0].p[P_SLEN]==64);
    assert(trk[0].eng_req==1 && trk[0].preset==3 && trk[0].p[P_LEVEL]==72);
    assert(trk[6].p[P_E0]==2 && trk[7].p[P_E0]==3);
    /* Stopped navigation preserves edits too, including same-slot selection. */
    song.playing=0;
    trk[0].p[P_LEVEL]=83; section_load(0); assert(trk[0].p[P_LEVEL]==83);
    section_load(1); section_load(0); assert(trk[0].p[P_LEVEL]==83);
    sections_write(); memset(proj_slot,0,sizeof proj_slot); pb_scan(); live_sec=-1;
    section_load(0); assert(trk[0].eng_req==1 && trk[0].preset==3 && trk[0].p[P_LEVEL]==83);
    /* Unchanged navigation must not fill all four pending-edit slots. */
    section_load(1); section_load(0); song.playing=0; sections_write();
    for(uint32_t n=0;n<12;n++) { section_load(n%2); assert(live_sec==(int8_t)(n%2)); }
    assert(sec_dirty==0);
    section_load(0);
    /* A 32-step drum track at 1/8 takes four bars even with short synths. */
    trk[0].p[P_SLEN]=16; trk[6].p[P_SLEN]=32;
    for(uint32_t d=0;d<NDIV_STEP;d++) if(div_units(d)==BEAT_U/2u) trk[6].p[P_SDIV]=d;
    assert(section_bars(ARR_SCENES)==4);
    song.playing=1; live_bar=0; live_req=1;
    bar(1); bar(2); bar(3); assert(live_sec==0 && live_req==1);
    bar(4); assert(live_sec==1);
    song.playing=0;
    for(uint32_t k=0;k<NTRK;k++) { trk[k].p[P_SLEN]=16; trk[k].p[P_SDIV]=2; }
    trk[0].p[P_SLEN]=32; assert(section_bars(ARR_SCENES)==2);
    trk[0].p[P_SLEN]=17; assert(section_bars(ARR_SCENES)==2);
    trk[0].p[P_SLEN]=64; trk[0].p[P_SDIV]=8;
    assert(section_bars(ARR_SCENES)==128); /* 64 steps at 2BAR, no 64-bar truncation. */
    trk[0].p[P_SLEN]=16; trk[0].p[P_SDIV]=2;
    /* Full dirty cache must refuse navigation, keeping the working sound intact. */
    song.playing=0; sections_write();
    for(uint32_t s=4;s<8;s++) assert(section_store(s));
    song.playing=1; live_sec=0; live_req=1; live_bar=0;
    int16_t cutoff=trk[0].p[P_LEVEL]; bar(4);
    assert(live_sec==0 && trk[0].p[P_LEVEL]==cutoff);
    song.playing=0;
    section_bank=0;
    layer_key(LY_SONG,key_of_white(1),1);
    assert(live_sec==0 && !strcmp(ui.msg,"STOP TO SAVE MORE"));
    sections_write();
    project_new(); assert(live_sec==-1);
    puts("pattern switching: 64 steps, drum division, sound retention, flash reload, full-cache refusal: PASS");
}
