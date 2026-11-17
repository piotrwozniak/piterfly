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

    /* Check actual rendered number and engine pixels, including inactive tracks. */
    static uint16_t rendered[240*240];
    for(uint32_t t=0;t<NTRK;t++) {
        song.sel=(uint8_t)((t&~3u)+((t+1u)&3u));
        song.rec=0;song.solo=0;song.playing=0;ui.force=1;
        trk[t].p[P_MUTE]=0;trk[t].p[P_LEVEL]=100;song.g[G_DRLVL]=100;
        studio_tracks_draw();memcpy(rendered,screen,sizeof rendered);
        uint32_t n=sound_position(&trk[t],&total)+1u;
        char label[5]={'#',(char)('0'+n/100u),(char)('0'+n/10u%10u),(char)('0'+n%10u),0};
        cv_begin(240,36,C_BLACK);cv_text(198,1,&FONT_S,label,TE_G3);cv_blit(0,40+(t&3u)*36u);
        for(uint32_t y=1;y<17;y++) for(uint32_t col=198;col<236;col++)
            assert(screen[(40+(t&3u)*36u+y)*240+col]==rendered[(40+(t&3u)*36u+y)*240+col]);
        char name[24],engine[16];
        if(is_drum(&trk[t])) {str_cpy(name,DRUM_KIT_NAMES[drum_kit_for(&trk[t])],sizeof name);te_lower(engine,"drums",sizeof engine);}
        else {trk_short_name(t,name);te_lower(engine,ENGINES[trk[t].eng_req%NENGINES]->name,sizeof engine);}
        name[13]=0;
        while(str_len(name) && 34+text_w(&FONT_S,name)+6+text_w(&FONT_S,engine)>194) name[str_len(name)-1u]=0;
        int32_t ex=34+text_w(&FONT_S,name)+6;
        cv_begin(240,36,C_BLACK);cv_text(ex,1,&FONT_S,engine,TE_G3);cv_blit(0,40+(t&3u)*36u);
        for(uint32_t y=1;y<17;y++) for(uint32_t col=(uint32_t)ex;col<194;col++)
            assert(screen[(40+(t&3u)*36u+y)*240+col]==rendered[(40+(t&3u)*36u+y)*240+col]);
    }

    puts("HOME sound: factory identities, same sound on other tracks, live redraw, rail endpoints, both drum kits, original 36-pixel tracks and header gap: PASS");
}
