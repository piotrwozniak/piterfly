/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"

static void ball(int32_t x,int32_t y,uint16_t colour)
{
    static const char *const shape[]={"01110","11111","11111","11111","01110"};
    for(int32_t dy=-2;dy<=2;dy++)for(int32_t dx=-2;dx<=2;dx++) {
        assert(x+dx>=0 && x+dx<240 && y+dy>=0 && y+dy<240);
        uint16_t pixel=swap16(screen[(y+dy)*240+x+dx]);
        if ((pixel==colour)!=(shape[dy+2][dx+2]=='1')) {
            fprintf(stderr,"ball at %d,%d offset %d,%d got %04x want %04x\n",x,y,dx,dy,pixel,colour);
            abort();
        }
    }
}

int main(int argc,char **argv)
{
    outdir=argc>1?argv[1]:"build/host";
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();
    for(uint32_t theme=0;theme<2;theme++) {
        palette_set(theme?4:0);
        for(uint32_t c=0;c<4;c++) {
            ui.force=1;
            /* Constant text: position changes must invalidate the column cache on their own. */
            for(int32_t ratio=0;ratio<=1000;ratio++) {
                draw_column(c,"","","",TE_COL[c],ratio,ICON_NONE);
                ball((int32_t)c*60+6+ratio*48/1000,Y_GAUGE+1,TE_COL[c]);
                ui.force=0;
            }
            draw_column(c,"","","",TE_COL[c],-1,ICON_NONE);
            for(uint32_t y=Y_GAUGE-1;y<=Y_GAUGE+3;y++)for(uint32_t x=c*60+4;x<c*60+59;x++)
                assert(swap16(screen[y*240+x])==C_BLACK);
        }
        for(int32_t value=0;value<=127;value++) {
            for(uint32_t c=0;c<4;c++)trk[0].p[P_DIST+c]=(int16_t)value;
            CV_BANDS(240,H_GRAPH,C_BLACK) {graph_fx(&trk[0],C_WHITE);cv_blit(0,Y_GRAPH);}
            for(uint32_t c=0;c<4;c++)ball((int32_t)c*60+28,Y_GRAPH+90-value*80/127,C_WHITE);
        }
        song.sel=0;ui.hot_t=0;ui.force=1;
        for(int32_t value=0;value<=127;value++) {
            for(uint32_t c=0;c<NTRK;c++)trk[c].p[P_LEVEL]=(int16_t)value;
            song.g[G_DRLVL]=value;draw_tracks();
            int32_t fy=TS_H-3-value*(TS_H-5)/127;
            for(uint32_t c=0;c<NTRK;c++)ball((int32_t)c*60+7,TS_Y+fy,!value?C_DIM:c==song.sel?C_HI:C_GRAY);
            ui.force=0;
        }
    }
    palette_set(4);song.sel=0;ui.force=1;
    trk[0].p[P_DIST]=0;trk[0].p[P_CHOR]=42;trk[0].p[P_DLY]=84;trk[0].p[P_REV]=127;
    open_family(FAM_FX);ui.force=1;ui_draw();ppm("rounded-fx");
    puts("slider handles: all gauge positions, FX and TRACKS levels, zero/max, cache redraw, two themes PASS");
    return 0;
}
