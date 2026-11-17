/* SPDX-License-Identifier: GPL-3.0-only */
/* Frozen HOME layout from public 1.2.0 (also fe74608), before the 1.2.2 strip.
 * Compare all pixels except the indicator and explicitly changed track labels. */
#define FELUCCA_SYNTH_PARTS 6
#include "editor_host.c"
#include "home_layout_baseline.h"
static uint16_t expected[240*240];
int main(void)
{
    harness_init(); bank_resolve(); go_home();
    for(uint32_t t=0;t<NTRK;t++) {
        trk[t].p[P_SLEN]=t%3u==0?64:t%3u==1?32:16;
        trk[t].p[P_LEVEL]=104;
        for(uint32_t s=0;s<(uint32_t)trk[t].p[P_SLEN];s+=3u+t) {
            if(is_drum(&trk[t])) dstep_set(&trk[t].dstep[s],s%4u,LV_NORM,0);
            else { trk[t].step[s].n=1;trk[t].step[s].time=ST_NOTE;trk[t].step[s].note[0]=48+t; }
        }
        trk[t].seq_idx=(uint32_t)trk[t].p[P_SLEN]-1u;
    }
    song.g[G_DRLVL]=104; song.g[G_BPM]=120;
    uint32_t n=0;
    for(uint32_t palette=0;palette<2;palette++) {
        settings.palette=palette?4:0;palette_set(settings.palette);
        for(uint32_t t=0;t<NTRK;t++) for(uint32_t state=0;state<4;state++) {
            song.sel=(uint8_t)t; song.playing=state!=0;
            song.rec=state==2?1u<<t:0;save_pending=state==2;song.solo=state==3?1u<<t:0;
            ui.force=1;baseline_studio_tracks_draw();
            memcpy(expected,screen,sizeof expected);
            ui.force=1;studio_tracks_draw();
            for(uint32_t y=0;y<240;y++) for(uint32_t x=0;x<240;x++) {
                if(y>=35 && y<=37) continue;
                /* Only track labels and their new numbers may differ. */
                if(y>=40 && y<184 && (y-40)%36>=1 && (y-40)%36<17) {
                    if(x>=34 && x<194) continue;
                    uint32_t row=song.sel/4u*4u+(y-40)/36u;
                    int status=(row==t && (state==2 || state==3));
                    if(!status && x>=198 && x<236) continue;
                }
                if(screen[y*240+x]!=expected[y*240+x]) {fprintf(stderr,"mismatch t=%u state=%u x=%u y=%u\n",t,state,x,y);abort();}
            }
            n++;
        }
    }
    assert(n==64);
    puts("original HOME layout: geometry, grids, levels, status and footer unchanged outside approved labels; 8 tracks, 2 palettes, stopped/play/REC/PENDING/SOLO: PASS");
}
