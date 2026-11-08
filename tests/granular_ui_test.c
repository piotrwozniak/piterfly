/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_DAMAGE_TEST 1
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
#include <assert.h>
static void damage_reset(void) { memset(lcd_damage,0,sizeof lcd_damage);lcd_fill_calls=0; }
static void edit_clean(uint32_t slot,int32_t delta)
{
    damage_reset();edit_param(slot,delta);assert(!ui.force);ui_draw();
    assert(!lcd_fill_calls); /* no blanking before a value is redrawn */
    for(uint32_t y=0;y<240;y++)for(uint32_t x=0;x<240;x++)
        if(y<24 || y>=200)assert(!lcd_damage[y*240+x]);
}
static int status_blank(void)
{
    for(uint32_t y=Y_GRAPH+110;y<Y_GRAPH+124;y++)for(uint32_t x=6;x<238;x++)
        if((x<118 || x>=126) && swap16(screen[y*240+x])!=C_BLACK)return 0;
    return 1;
}
static uint16_t chor_grid[240*240];
static void grid_matches_chor(void)
{
    for(uint32_t y=24;y<70;y++)for(uint32_t x=59;x<240;x+=60)
        assert(screen[y*240+x]==chor_grid[y*240+x]);
    for(uint32_t y=Y_GRAPH;y<Y_GRAPH+H_GRAPH;y++)
        assert(screen[y*240+119]==chor_grid[y*240+119]);
    for(uint32_t x=6;x<234;x++)
        assert(screen[(Y_GRAPH+62)*240+x]==chor_grid[(Y_GRAPH+62)*240+x]);
}
int main(int argc,char **argv)
{
    outdir=argc>1?argv[1]:"build/host";panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(0);
    song.sel=0;trk[0].p[P_SLEN]=64;steps_clear(&trk[0]);
    for(uint32_t k=0;k<64;k+=4){trk[0].step[k].n=1;trk[0].step[k].note[0]=60;trk[0].step[k].time=ST_NOTE;}
    open_family(FAM_FX);
    for(uint32_t k=0;k<NPAGES;k++)if(PAGES[k].graph==GR_INS&&PAGES[k].id[0]==P_ITYPE)ui.page=(uint8_t)k;
    uint32_t insert=ui.page;
    song.playing=1;trk[0].seq_idx=38;
    trk[0].p[P_ITYPE]=IT_GRAIN-1;ui.force=1;ui_draw();
    memcpy(chor_grid,screen,sizeof chor_grid);ppm("chor-reference");edit_param(0,1);
    assert(on_grain_page() && trk[0].p[P_IB]==20 && trk[0].p[P_IC]==64);
    midi_cc_set(&trk[0],P_IB,127);assert(trk[0].p[P_IB]==60);
    midi_cc_set(&trk[0],P_IB,0);assert(trk[0].p[P_IB]==1);trk[0].p[P_IB]=20;
    gf_layout();assert(gf_buffer_ms(0)==3343);
    for(uint32_t k=0;k<NPAGES;k++)if(PAGES[k].graph==GR_INS&&PAGES[k].id[0]==P_IMIX)assert(!page_shown(&PAGES[k]));
    song.playing=1;trk[0].seq_idx=38;ui.force=1;ui_draw();grid_matches_chor();ppm("grain-pair-1");
    int32_t density=trk[0].p[P_IB];edit_clean(3,1);assert(trk[0].p[P_IB]>density);
    trk[0].p[P_IB]=20;ui_draw();
    edit_clean(2,1);assert(ui.lock_par==P_IA && trk[0].p[P_IA]!=32);
    /* Only SIZE's column and large cell change; TYPE, PARAM and DENSITY stay untouched. */
    for(uint32_t y=24;y<70;y++)for(uint32_t x=0;x<240;x++)if(x<124 || x>=179)assert(!lcd_damage[y*240+x]);
    for(uint32_t y=Y_GRAPH;y<Y_GRAPH+62;y++)for(uint32_t x=0;x<240;x++)assert(!lcd_damage[y*240+x]);
    trk[0].p[P_IA]=32;ui_draw();
    for(uint32_t pair=1;pair<4;pair++) {
        int16_t before[P_COUNT];memcpy(before,trk[0].p,sizeof before);
        edit_clean(1,1);assert(ui.grain_pair==pair && !memcmp(before,trk[0].p,sizeof before));
        uint32_t id=GRAIN_IDS[pair][0];edit_clean(2,1);if(p_lockable(id))assert(ui.lock_par==id);
        if(pair<3) {
            id=GRAIN_IDS[pair][1];int32_t old=trk[0].p[id];
            edit_clean(3,1);assert(trk[0].p[id]>old);if(p_lockable(id))assert(ui.lock_par==id);
        }
        if(pair==3) {
            assert(trk[0].p[P_GFREEZE]==1);
            memcpy(before,trk[0].p,sizeof before);edit_clean(3,100);
            assert(!memcmp(before,trk[0].p,sizeof before));
            assert(status_blank());
            edit_clean(1,-1);assert(!status_blank());
            edit_clean(1,1);assert(status_blank());
        }
        char name[24];snprintf(name,sizeof name,"grain-pair-%u",pair+1);grid_matches_chor();ppm(name);
    }
    int32_t bpm=song.g[G_BPM];assert(page_walk(1));assert(ui.page!=insert && song.g[G_BPM]==bpm);
    assert(page_walk(-1));assert(ui.page==insert && ui.grain_pair==3);
    for(uint32_t t=1;t<NTRK;t++)trk[t].p[P_ITYPE]=IT_GRAIN;
    gf_layout();assert(gf_buffer_ms(0)==835);
    ui.force=1;ui_draw();ppm("grain-four");
    edit_param(0,-1);assert(trk[0].p[P_ITYPE]==IT_GRAIN-1 && !on_grain_page() && ui.force);
    ui_draw();
    /* Every pair exposes TYPE directly; all parameters stay in range under large turns. */
    for(uint32_t pair=0;pair<4;pair++) {
        edit_param(0,1);ui.grain_pair=(uint8_t)pair;
        for(uint32_t slot=1;slot<4;slot++){edit_param(slot,10000);edit_param(slot,-10000);}
        ui.grain_pair=(uint8_t)pair;edit_param(0,-1);assert(!on_grain_page());
    }
    /* MIDI-learned TYPE has no ui.force side effect: the layout still changes both ways. */
    ui_draw();assert(!ui.force);midi_cc_set(&trk[0],P_ITYPE,127);
    damage_reset();ui_draw();assert(on_grain_page() && lcd_fill_calls);
    midi_cc_set(&trk[0],P_ITYPE,0);damage_reset();ui_draw();
    assert(!on_grain_page() && lcd_fill_calls);
    printf("granular_ui: INSERT pairs, SELECT navigation, partial redraw, FREEZE, capacity, TYPE exit PASS\n");return 0;
}
