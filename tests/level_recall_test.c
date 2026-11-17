/* SPDX-License-Identifier: GPL-3.0-only */
#define FELUCCA_SYNTH_PARTS 6
#include "editor_host.c"
static int16_t *level(uint32_t t) { return is_drum(&trk[t]) ? drum_global(&trk[t],G_DRLVL) : &trk[t].p[P_LEVEL]; }
int main(void)
{
    harness_init(); go_home();
    for(uint32_t t=0;t<NTRK;t++) *level(t)=100;
    project_save(0);
    for(uint32_t t=0;t<NTRK;t++) *level(t)=80;
    project_save(1); audio_blocks(1);
    for(uint32_t t=0;t<NTRK;t++) {
        project_load(0); ui_frame(); track_select(t); go_home();
        ui.enc_t[EN_K2]=ui.enc_t[EN_K3]=ui.enc_t[EN_K4]=0;
        int16_t old_steps=trk[t].p[P_SLEN], old_pan=trk[t].p[P_PAN];
        encs[panel.enc[EN_K2]]=-1;
        encs[panel.enc[EN_K3]]=1; encs[panel.enc[EN_K4]]=1;
        ui_frame();
        int16_t steps=trk[t].p[P_SLEN], pan=trk[t].p[P_PAN];
        assert(steps==old_steps+1 && pan==old_pan+1);
        int16_t value=*level(t);
        assert(value<100);
        project_load(1); ui_frame();
        project_load(0); ui_frame();
        assert(*level(t)==value && trk[t].p[P_SLEN]==steps && trk[t].p[P_PAN]==pan);
        /* The same knob edits also survive switching while playing. */
        song.playing=1; live_bar=0; live_req=1;
        clk_beat=4u*section_bars(ARR_SCENES); live_block(); assert(live_sec==1);
        live_req=0; clk_beat=4u*section_bars(ARR_SCENES); live_block(); assert(live_sec==0);
        assert(*level(t)==value && trk[t].p[P_SLEN]==steps && trk[t].p[P_PAN]==pan);
        song.playing=0; sections_write();
    }
    /* HOME macros and the main LEVEL dial must address the same drum part. */
    song.sel=6; ui.home=1;
    int16_t *value; home_param(0,&value);
    assert(value==&trk[6].p[P_LEVEL]);
    song.sel=7; home_param(0,&value); assert(value==&song.g[G_DRLVL]);
    puts("main LEVEL, STEPS and PAN knobs: all eight tracks retain values through LOAD and live switching; independent drum HOME routing: PASS");
}
