/* SPDX-License-Identifier: GPL-3.0-only */
#define FELUCCA_SYNTH_PARTS 6
#include "editor_host.c"
int main(void)
{
    harness_init(); bank_resolve(); go_home();
    uint32_t total, count=NBANK+up_count();
    for(uint32_t n=0;n<count;n++) {
        assert(!preset_go_to(&trk[0],n));
        assert(sound_position(&trk[0],&total)==n && total==count);
    }
    assert(!preset_go_to(&trk[0],23) && !preset_go_to(&trk[2],23));
    assert(sound_position(&trk[0],&total)==sound_position(&trk[2],&total));
    song.sel=0; ui.force=1; studio_tracks_draw();
    uint32_t x=1+23u*232u/(count-1u);
    assert(swap16(screen[36*240])==TE_G3);
    assert(swap16(screen[36*240+x])==TE_COL[0]);
    assert(swap16(screen[36*240+239])==TE_G3);
    for(uint32_t y=38;y<39;y++) for(uint32_t col=0;col<240;col++) assert(swap16(screen[y*240+col])==C_BLACK);
    assert(swap16(screen[34*240])==C_BLACK && swap16(screen[34*240+239])==C_BLACK);
    uint16_t before=screen[36*240+x];
    assert(!preset_go_to(&trk[0],0));
    ui.force=0; studio_tracks_draw();
    assert(swap16(screen[36*240+1])==TE_COL[0] && screen[36*240+x]!=before);
    assert(!preset_go_to(&trk[0],count-1));
    ui.force=0; studio_tracks_draw(); assert(swap16(screen[36*240+233])==TE_COL[0]);
    for(uint32_t t=6;t<8;t++) {
        song.sel=(uint8_t)t; trk[t].p[P_E0]=7;
        assert(sound_position(&trk[t],&total)==7 && total==DRUM_KITS);
        ui.force=1; studio_tracks_draw();
        assert(song.sel==t && swap16(screen[(40+(t&3u)*36)*240])==TE_COL[t&3u]);
    }

    puts("HOME sound: factory identities, same sound on other tracks, live redraw, rail endpoints, both drum kits, original 36-pixel tracks and header gap: PASS");
}
