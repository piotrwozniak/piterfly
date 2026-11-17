/* SPDX-License-Identifier: GPL-3.0-only */
/* HOME screenshot from the actual firmware UI and sample state. Host only.
 * cc -O1 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal tools/preview_home_sound.c -lm -o build/home-sound-preview
 * build/home-sound-preview build/home-sound-preview.ppm
 */
#define FELUCCA_SYNTH_PARTS 6
#include "../tests/editor_host.c"
int main(int argc,char **argv)
{
    assert(argc>=2 && argc<=4);harness_init();bank_resolve();
    for(uint32_t t=0;t<NTRK;t++) {
        if(is_drum(&trk[t])) trk[t].p[P_E0]=(int16_t)(t-4u);
        else preset_go_to(&trk[t],t==0||t==2?23:t==1?4:35);
        trk[t].p[P_SLEN]=argc>3?(int16_t)atoi(argv[3]):t==0?64:16; trk[t].p[P_LEVEL]=t==0?91:85;
        for(uint32_t s=0;s<(uint32_t)trk[t].p[P_SLEN];s+=t+3) {
            if(is_drum(&trk[t])) { dstep_set(&trk[t].dstep[s],s%4u,LV_NORM,0); continue; }
            trk[t].step[s].n=1;trk[t].step[s].time=ST_NOTE;
            trk[t].step[s].note[0]=(uint8_t)(48+t*4);trk[t].step[s].vel=90;
        }
    }
    song.sel=argc>2?(uint8_t)atoi(argv[2]):0;assert(song.sel<NTRK);song.playing=0;song.g[G_BPM]=120;save_pending=0;ui.msg_t=0;ui.force=1;
    go_home(); studio_tracks_draw();
    FILE *f=fopen(argv[1],"wb");assert(f);fprintf(f,"P6\n240 240\n255\n");
    for(uint32_t i=0;i<240*240;i++) {
        uint16_t p=swap16(screen[i]);uint8_t rgb[3]={(p>>11)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(rgb,1,3,f);
    }
    assert(!fclose(f));
}
