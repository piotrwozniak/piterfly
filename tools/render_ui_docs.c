/* SPDX-License-Identifier: GPL-3.0-only */
/* Reproduce current documentation frames from the actual UI; no hardware access.
 * cc -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal tools/render_ui_docs.c -lm -o build/render_ui_docs
 * mkdir -p build/ui-docs && build/render_ui_docs build/ui-docs
 */
#define UI_PAGES_HARNESS_ONLY 1
#include "../tests/ui_pages_test.c"

static void init_docs(uint32_t palette)
{
    panel=PANEL_DEFAULT; layers_init(); settings.palette=palette; palette_set(palette);
    host_tracks_init(); song.g[G_BPM]=120; song.sel=0;
    for(uint32_t i=0;i<NPART;i++) {
        set_engine_of(&trk[i],TRK_DEF[i][0]); apply_preset_to(&trk[i],TRK_DEF[i][1]);
        trk[i].engine=trk[i].eng_req;
    }
    TDRUM->p[P_E0]=DRUM_DEFAULT_KIT;
    song.playing=0; song.rec=0; rec_wait=0;
}

static void mixed_lengths(int mixed)
{
    const uint32_t lengths[4]={64,32,12,8};
    for(uint32_t t=0;t<NTRK;t++) {
        uint32_t n=mixed?lengths[t]:64;
        steps_clear(&trk[t]); trk[t].p[P_SLEN]=n;
        for(uint32_t k=0;k<n;k++) {
            if(t==TRK_DRUM) {
                if(k%4==0)dstep_set(&trk[t].dstep[k],0,LV_HARD,0);
                if(k%4==2)dstep_set(&trk[t].dstep[k],4,LV_HARD,0);
            } else if((k+t*2)%(t+3)==0 || k%16==15) {
                trk[t].step[k].n=1; trk[t].step[k].time=ST_NOTE;
                trk[t].step[k].note[0]=48+k%12;
            }
        }
        trk[t].seq_idx=57%n;
    }
}

static void capture_page(const char *title,const char *name)
{
    uint32_t p;
    for(p=0;p<NPAGES;p++)if(!strcmp(PAGES[p].title,title))break;
    assert(p<NPAGES); open_family(PAGES[p].fam); ui.page=(uint8_t)p;
    ui.force=1; ui_draw(); ppm(name);
}

/* Schematic comparison only: current GPL icons/renderer, legacy 16-cell layout.
 * No historical screenshots, atlas, headers or firmware binaries are loaded. */
static void legacy_home_layout(void)
{
    for (uint32_t t=0;t<NTRK;t++) {
        uint32_t len=trk_len(&trk[t]), base=trk[t].seq_idx/16u*16u;
        uint32_t y=40+t*36+21;
        lcd_fill(34,40+t*36+18,160,16,C_BLACK);
        for(uint32_t j=0;j<16;j++) {
            uint32_t step=base+j;
            lcd_fill(34+j*10,y,8,9,step>=len?C_BLACK:trk_step_on(&trk[t],step)?TE_COL[t]:TE_G1);
            if(step<len && step==trk[t].seq_idx)lcd_fill(34+j*10,y+10,8,1,C_WHITE);
        }
    }
    ppm("layout-home-before");
}
static void legacy_options_layout(void)
{
    lcd_fill(94,0,58,16,C_BLACK); /* old options did not have the top page indicators */
    lcd_fill(0,Y_FOOT,240,18,C_BLACK);
    for(uint32_t j=0;j<16;j++) {
        uint32_t x=4+j*14+j/4*4;
        lcd_fill(x,Y_FOOT+1,12,9,trk_step_on(TSEL,j)?TE_COL[song.sel]:TE_G1);
        if(j==TSEL->seq_idx)lcd_fill(x,Y_FOOT+11,12,1,C_WHITE);
    }
    ppm("layout-options-before");
}

int main(int argc,char **argv)
{
    assert(argc==2); outdir=argv[1];
    init_docs(4); mixed_lengths(1); song.playing=1;
    go_home(); ui.force=1; ui_draw(); ppm("piterfly-home-after"); legacy_home_layout();
    mixed_lengths(0); ui.force=1; ui_draw(); ppm("home-64");
    song.playing=0; rec_wait=1; ui.force=1; ui_draw(); ppm("rec-ready-64");

    init_docs(0);
    for(uint32_t t=0;t<NTRK;t++){steps_clear(&trk[t]);trk[t].p[P_SLEN]=64;}
    for(uint32_t k=0;k<64;k++)if(k%4==0 || k%7==2){
        trk[0].step[k].n=1; trk[0].step[k].time=ST_NOTE; trk[0].step[k].note[0]=60+k%12;
    }
    song.playing=1; trk[0].seq_idx=38;
    capture_page("MASTER","piterfly-options-after"); ppm("glo-64"); legacy_options_layout();

    init_docs(0);
    song.g[G_CH1]=1; song.g[G_CH2]=2; song.g[G_CH3]=3; song.g[G_DRCH]=10;
    song.g[G_ROUTE]=0; song.g[G_CHOTH]=1; song.g[G_MIDI]=1;
    for(uint32_t t=0;t<NTRK;t++) {
        song.g[G_THRU1+t]=t==0; steps_clear(&trk[t]); trk[t].p[P_SLEN]=16;
    }
    trk[0].p[P_LEVEL]=0;
    capture_page("MIDI","piterfly-midi-channels");
    capture_page("MIDI 2","piterfly-midi-input");
    capture_page("MIDI THRU","piterfly-midi-thru");
    go_home(); ui.force=1; ui_draw(); ppm("piterfly-midi-home");
    return 0;
}
