/* SPDX-License-Identifier: GPL-3.0-only */
/* Frozen HOME layout from public 1.2.0 (also fe74608), before the 1.2.2 strip.
 * RGB565 hashes exclude only the three new indicator rows (35..37). */
#define FELUCCA_SYNTH_PARTS 6
#include "editor_host.c"
static const uint32_t original[64] = {
0x0af87f89u, 0xf4bb1376u, 0x98577b3bu, 0x6e70df00u,
0x5b15f6f8u, 0x846d0255u, 0x450ca96cu, 0x99a3e351u,
0x0a5c4000u, 0xe14684ccu, 0x296feb4du, 0x1fe849f8u,
0xee36810fu, 0x0a4c5bc6u, 0xe073c56fu, 0x99127ee6u,
0x911bd6bfu, 0x4efe002cu, 0xeef29cc9u, 0x7290da7au,
0xefbd0ed3u, 0x7ae64895u, 0x34666a54u, 0x86bee31fu,
0x8fdb7ff7u, 0x6a9cfb14u, 0x4a6546a1u, 0x820c3f8eu,
0x2eba27e2u, 0x26281679u, 0x32acec74u, 0x882b681bu,
0x0af87f89u, 0xf4bb1376u, 0x98577b3bu, 0x6e70df00u,
0x5b15f6f8u, 0x846d0255u, 0x450ca96cu, 0x99a3e351u,
0x0a5c4000u, 0xe14684ccu, 0x296feb4du, 0x1fe849f8u,
0xee36810fu, 0x0a4c5bc6u, 0xe073c56fu, 0x99127ee6u,
0x911bd6bfu, 0x4efe002cu, 0xeef29cc9u, 0x7290da7au,
0xefbd0ed3u, 0x7ae64895u, 0x34666a54u, 0x86bee31fu,
0x8fdb7ff7u, 0x6a9cfb14u, 0x4a6546a1u, 0x820c3f8eu,
0x2eba27e2u, 0x26281679u, 0x32acec74u, 0x882b681bu,
 };
static uint32_t layout_hash(void)
{
    uint32_t h=2166136261u;
    for(uint32_t y=0;y<240;y++) if(y<35 || y>37)
        for(uint32_t x=0;x<240;x++) h=(h^swap16(screen[y*240+x]))*16777619u;
    return h;
}
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
            ui.force=1;studio_tracks_draw();
            assert(layout_hash()==original[n++]);
        }
    }
    assert(n==64);
    puts("original HOME layout: all pixels outside the 3-row indicator unchanged; 8 tracks, 2 palettes, stopped/play/REC/PENDING/SOLO: PASS");
}
