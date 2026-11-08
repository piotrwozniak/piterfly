/* SPDX-License-Identifier: GPL-3.0-only */
#include "editor_host.c"
#include <assert.h>
static uint32_t history_hash(void)
{
    uint32_t h=2166136261u;
    for(uint32_t k=0;k<gf_capacity(0);k++)h=(h^*gf_byte(&gf[0],k))*16777619u;
    return h;
}
static int16_t kept[P_COUNT];
static uint32_t wp,valid,hash;
static void check_kept(void)
{
    for(uint32_t k=0;k<P_COUNT;k++)if(grain_param(k))assert(trk[0].p[k]==kept[k]);
    gf_layout();assert(gf[0].wp==wp && gf[0].valid==valid && history_hash()==hash);
}
static void check_frozen_audio(void)
{
    int32_t out[CTL*2];int64_t energy=0;
    for(uint32_t b=0;b<512;b++) {
        mix_block(out,CTL);
        for(uint32_t k=0;k<CTL*2;k++)energy+=abs(out[k]);
    }
    assert(energy>1000);check_kept();
}
int main(void)
{
    harness_init();bank_resolve();song.sel=0;track_t *t=&trk[0];
    /* A user sound with a different engine and no insert must not replace the active cloud. */
    set_engine_of(t,1);t->engine=t->eng_req;t->p[P_ITYPE]=IT_FLANGER;assert(!up_store(0, "Test"));
    t->p[P_ITYPE]=IT_GRAIN;grain_defaults(t);t->p[P_IMIX]=127;
    gf_layout();gf_reset();int32_t b[CTL];uint32_t sample=0;
    for(uint32_t block=0;block<1500;block++) {
        for(uint32_t k=0;k<CTL;k++,sample++)b[k]=(int32_t)(sin(sample*6.283185307179586*220/FS)*16000);
        gf_insert(t,b,0,0,0,CTL);
    }
    t->p[P_GFREEZE]=1;memcpy(kept,t->p,sizeof kept);
    wp=gf[0].wp;valid=gf[0].valid;hash=history_hash();assert(valid>1000);
    /* Factory preset, engine selector and the unified preset browser all retain the history. */
    for(uint32_t e=0;e<NENGINES;e++) {
        set_engine_of(t,e);check_kept();check_frozen_audio();
        apply_preset_to(t,ENGINES[e]->npresets-1);check_kept();check_frozen_audio();
    }
    for(uint32_t n=0;n<NBANK;n+=17) {assert(!preset_go_to(t,n));check_kept();check_frozen_audio();}
    assert(!up_load_to(t,0));assert(t->eng_req==1 && t->user==1);check_frozen_audio();
    /* Future factory extras must not briefly turn off or overwrite an active insert. */
    preset_t pr={0};pr.x[0]=P_ITYPE+1;pr.x[1]=0;pr.x[2]=P_GFREEZE+1;pr.x[3]=0;
    preset_extras(t->p,&pr);check_kept();
    /* LIVE retains its history and keeps capturing the newly selected sound. */
    t->p[P_GFREEZE]=0;uint32_t oldwp=gf[0].wp;
    apply_preset_to(t,0);assert(t->p[P_ITYPE]==IT_GRAIN && !t->p[P_GFREEZE]);
    int32_t out[CTL*2];for(uint32_t k=0;k<100;k++)mix_block(out,CTL);
    assert(gf[0].wp!=oldwp && gf[0].valid>=valid);
    assert(!up_load_to(t,0));assert(t->p[P_ITYPE]==IT_GRAIN && !t->p[P_GFREEZE]);
    /* With GRANUL off, loading a user sound still restores that sound's stored insert. */
    t->p[P_ITYPE]=0;assert(!up_load_to(t,0));assert(t->p[P_ITYPE]==IT_FLANGER);
    /* Kit changes never touch the drum track's independent insert. */
    song.sel=TRK_DRUM;TDRUM->p[P_ITYPE]=IT_GRAIN;grain_defaults(TDRUM);TDRUM->p[P_GFREEZE]=1;
    go_home();encs[panel.enc[EN_ALGO]]=1;ui_input();
    assert(TDRUM->p[P_ITYPE]==IT_GRAIN && TDRUM->p[P_GFREEZE]==1);
    puts("granular_presets: factory/user/engine changes keep LIVE/FREEZE, history and wet audio PASS");return 0;
}
