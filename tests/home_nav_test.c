/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
#include <assert.h>
static void turn(uint32_t role,int32_t delta) { encs[panel.enc[role]]=delta;frame(); }
static void init_page(uint32_t pg, uint32_t t)
{
    song.sel=t;ui.menu=0;ui.layer=LY_PLAY;ui.sheet=0;vis_on=0;
    ui.home=pg==NPAGES;if(pg<NPAGES)ui.page=pg;
    ui.force=1;frame();
}
int main(void)
{
    panel=PANEL_DEFAULT;layers_init();bank_resolve();host_tracks_init();palette_set(0);
    uint32_t checked=0;
    /* Every ordinary parameter page, including FX and the preset browser. */
    for(uint32_t pg=0;pg<=NPAGES;pg++) {
        init_page(pg,0);
        if(pg<NPAGES && (ui.page!=pg || ui.home || on_notes_page() || on_song_page() || on_drum_page()))continue;
        uint8_t engine[NTRK],preset[NTRK];int16_t sound[NTRK][P_COUNT];
        for(uint32_t t=0;t<NTRK;t++){engine[t]=trk[t].eng_req;preset[t]=trk[t].preset;memcpy(sound[t],trk[t].p,sizeof sound[t]);}
        turn(EN_PRESET,1);assert(song.sel==1);
        for(uint32_t t=0;t<NTRK;t++)assert(engine[t]==trk[t].eng_req&&preset[t]==trk[t].preset&&!memcmp(sound[t],trk[t].p,sizeof sound[t]));
        uint32_t n,old=preset_pos(&n);assert(n>1);turn(EN_ALGO,1);
        assert(song.sel==1&&preset_pos(&n)!=(old%n));checked++;
    }
    assert(checked>10);
    init_page(NPAGES,0);turn(EN_PRESET,-1);assert(song.sel==0);
    for(uint32_t t=0;t<NTRK+1;t++)turn(EN_PRESET,1);assert(song.sel==TRK_DRUM);
    int16_t kit=TDRUM->p[P_E0];turn(EN_ALGO,1);assert(song.sel==TRK_DRUM&&TDRUM->p[P_E0]==kit+1);
    turn(EN_ALGO,-1);assert(TDRUM->p[P_E0]==kit);
    uint32_t drum=0;
    for(uint32_t i=0;i<NPAGES;i++)if(PAGES[i].scope==SC_DRUM){drum=i;break;}
    init_page(drum,TRK_DRUM);kit=TDRUM->p[P_E0];turn(EN_ALGO,1);assert(TDRUM->p[P_E0]==kit+1);
    /* Continuing a track turn after DRUMS opens HOME must still select tracks, never sounds. */
    uint8_t eng=trk[1].eng_req,pre=trk[1].preset;
    encs[panel.enc[EN_PRESET]]=-1;enc_late[panel.enc[EN_PRESET]]=-1;frame();frame();
    assert(song.sel==1&&cur_fam()==FAM_TRK&&trk[1].eng_req==eng&&trk[1].preset==pre);
    for(uint32_t i=0;i<NPAGES;i++)if(!strcmp(PAGES[i].title,"NOTES")) {
        init_page(i,0);song.seq_mode=1;ui.zoom=0;turn(EN_PRESET,1);assert(song.sel==0&&ui.zoom==1);
        turn(EN_ALGO,1);assert(song.sel==1);break;
    }
    /* ALGORITHM on FX retains an active frozen insert while changing the source sound. */
    song.seq_mode=0;init_page(NPAGES,0);open_family(FAM_FX);frame();
    trk[0].p[P_ITYPE]=IT_GRAIN;grain_defaults(&trk[0]);trk[0].p[P_GFREEZE]=1;
    int16_t kept[P_COUNT];memcpy(kept,trk[0].p,sizeof kept);
    uint32_t n,old=preset_pos(&n);turn(EN_ALGO,1);assert(song.sel==0&&preset_pos(&n)!=old);
    for(uint32_t k=0;k<P_COUNT;k++)if(grain_param(k))assert(trk[0].p[k]==kept[k]);
    rec_wait=1;turn(EN_PRESET,1);assert(song.sel==1&&rec_wait);rec_wait=0;
    ft_on=1;turn(EN_PRESET,1);assert(song.sel==1);ft_on=0;
    printf("Navigation: %u pages PRESETS track / ALGORITHM sound, drums, late detents, NOTES zoom, frozen FX and record guard PASS\n",checked);return 0;
}
