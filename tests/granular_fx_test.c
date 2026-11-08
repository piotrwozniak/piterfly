/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static void activate(uint32_t mask)
{
    for (uint32_t t=0;t<NTRK;t++) trk[t].p[P_ITYPE] = mask & (1u<<t) ? IT_GRAIN : 0;
    gf_layout();
    uint8_t used[GF_PAGES]={0};
    for (uint32_t t=0;t<NTRK;t++) for(uint32_t p=0;p<gf[t].np;p++) {
        assert(gf[t].page[p]<GF_PAGES); assert(!used[gf[t].page[p]]++);
    }
}
static uint8_t history(uint32_t t,uint32_t age)
{
    gf_state_t *s=&gf[t]; uint32_t cap=gf_capacity(t);
    assert(age && age<=s->valid);
    return *gf_byte(s,(s->wp+cap-age)%cap);
}
static int split_render(void)
{
    host_tracks_init();
    for(uint32_t t=0;t<NTRK;t++) {
        trk[t].p[P_ITYPE]=IT_GRAIN;grain_defaults(&trk[t]);trk[t].p[P_IMIX]=96;
        if(t<NPART) { trk[t].p[P_E0]=3;input_on(&trk[t],(int32_t)(48+t*7),100); }
    }
    uint32_t hash=2166136261u; int32_t out[CTL*2];
    for(uint32_t block=0;block<7000;block++) {
        if(block%80==0)drum_on(36,100);
        if(block==3000)trk[0].p[P_GFREEZE]=1;
        if(block==4000)trk[2].p[P_ITYPE]=0;
        if(block==5000)trk[2].p[P_ITYPE]=IT_GRAIN;
        mix_block(out,CTL);
        for(uint32_t i=0;i<CTL*2;i++)hash=(hash^(uint32_t)out[i])*16777619u;
    }
    printf("granular split hash %08x\n",hash);return 0;
}
int main(void)
{
    if(getenv("GF_SPLIT")) return split_render();
    host_tracks_init();
    int32_t prev=-40000;
    for(int32_t x=-32768;x<=32767;x++) {
        int32_t y=gf_decode(gf_encode(x));
        assert(y>=prev && abs(y-x)<=644); prev=y;
    }
    int32_t tl[CTL],tr[CTL];
    texture_gen(TX_USR1 + SMP_USER_SLOTS,tl,tr,CTL);
    for(uint32_t i=0;i<CTL;i++)assert(!tl[i] && !tr[i]);
    activate(1); assert(gf_buffer_ms(0)==3343);
    /* Layout resizing preserves every retained sample, including partial pages. */
    static uint8_t old[NTRK][GF_PAGES*GF_PAGE];
    uint32_t seed=1;
    for(uint32_t run=0;run<400;run++) {
        uint32_t valid[NTRK];
        for(uint32_t t=0;t<NTRK;t++) {
            gf_state_t *s=&gf[t]; uint32_t cap=gf_capacity(t);
            if(cap) {
                for(uint32_t p=0;p<cap;p++) *gf_byte(s,p)=(uint8_t)(p*17u+t*59u+run);
                seed=seed*1664525u+1013904223u; s->wp=seed%cap; s->valid=cap;
            }
            valid[t]=s->valid;
            for(uint32_t a=1;a<=s->valid;a++) old[t][a-1]=history(t,a);
        }
        seed=seed*1664525u+1013904223u; activate((seed>>16)&15u);
        for(uint32_t t=0;t<NTRK;t++) {
            assert(gf[t].valid<=valid[t]);
            for(uint32_t a=1;a<=gf[t].valid;a++) assert(history(t,a)==old[t][a-1]);
        }
    }
    activate(1); gf_reset();
    track_t *t=&trk[0]; t->p[P_IA]=56; t->p[P_IB]=40; t->p[P_IC]=64;
    t->p[P_IMIX]=127;t->p[P_GPOS]=64;t->p[P_GSPREAD]=32;t->p[P_GFREEZE]=0;
    int32_t b[CTL],peak=0; uint32_t sample=0;
    for(uint32_t block=0;block<5000;block++) {
        for(uint32_t i=0;i<CTL;i++,sample++) b[i]=(int32_t)(sin(sample*6.283185307179586*220/FS)*16000);
        gf_insert(t,b,0,0,0,CTL);
        if(block>1000) for(uint32_t i=0;i<CTL;i++) if(abs(b[i])>peak)peak=abs(b[i]);
    }
    assert(peak>1000 && peak<60000);
    t->p[P_GFREEZE]=1; uint32_t wp=gf[0].wp,valid=gf[0].valid; peak=0;
    for(uint32_t block=0;block<5000;block++) {
        memset(b,0,sizeof b);gf_insert(t,b,0,0,0,CTL);
        assert(gf[0].wp==wp && gf[0].valid==valid);
        if(block>1000)for(uint32_t i=0;i<CTL;i++)if(abs(b[i])>peak)peak=abs(b[i]);
    }
    assert(peak>1000);
    activate(15);
    for(uint32_t block=0;block<8000;block++) for(uint32_t j=0;j<NTRK;j++) {
        track_t *t=&trk[j]; t->p[P_IA]=(block&1)?0:127; t->p[P_IB]=60;
        t->p[P_IC]=(block&2)?0:127;t->p[P_GPOS]=(block&4)?0:127;
        t->p[P_GSPREAD]=127;t->p[P_GFREEZE]=(block%700)>350;t->p[P_IMIX]=127;
        for(uint32_t i=0;i<CTL;i++)b[i]=(int32_t)((seed=seed*1664525u+1013904223u)>>14)-131072;
        gf_insert(t,b,0,0,0,CTL);
        for(uint32_t i=0;i<CTL;i++)assert(abs(b[i])<=262144);
    }
    t->p[P_IMIX]=0;
    for(uint32_t i=0;i<20;i++){memset(b,0,sizeof b);gf_insert(t,b,0,0,0,CTL);}
    for(uint32_t i=0;i<CTL;i++)b[i]=(int32_t)i*1234-20000;
    gf_insert(t,b,0,0,0,CTL);
    for(uint32_t i=0;i<CTL;i++)assert(b[i]==(int32_t)i*1234-20000);
    if (!getenv("GF_UNIT_ONLY")) {
    /* A frozen cloud must outlive its synth voices through the complete mixer. */
    host_tracks_init();activate(1);gf_reset();t=&trk[0];grain_defaults(t);
    t->p[P_IMIX]=127;t->p[P_REL]=0;t->p[P_E0]=3;input_on(t,60,100);
    int32_t out[CTL*2];
    for(uint32_t block=0;block<1200;block++)mix_block(out,CTL);
    t->p[P_GFREEZE]=1;input_off(t,60);peak=0;
    for(uint32_t block=0;block<4000;block++) {
        mix_block(out,CTL);
        if(block>3000)for(uint32_t i=0;i<CTL*2;i++)if(abs(out[i])>peak)peak=abs(out[i]);
    }
    for(uint32_t i=0;i<NVOICE;i++)assert(!t->v[i].active);
    assert(peak>50);
    }
    printf("granular_fx: codec, 400 dynamic layouts, LIVE/FREEZE, extremes, bounded output and bit-exact dry PASS\n");
    return 0;
}
