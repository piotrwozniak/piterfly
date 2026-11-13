/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/hal/fm1_flash_id.h"
#include "editor_host.c"
#include <assert.h>
static void legacy(project_v6_t *v, uint32_t id)
{
    project_t p; proj_capture(&p); memset(v,0,sizeof *v);
    v->magic=PROJ_MAGIC_V6;v->size=sizeof *v;
    memcpy(v->g,p.g,sizeof v->g);v->sel=p.sel;v->drdly=p.drdly;memcpy(v->rsv,p.rsv,sizeof v->rsv);
    for(uint32_t t=0;t<NTRK;t++) {
        proj_trk_v6_t *d=&v->t[t];const track_t *s=&trk[t];
        memcpy(d->p,s->p,sizeof d->p); memcpy(d->e,s->p+P_E0,sizeof d->e); for(uint32_t k=0;k<10;k++) d->x[k]=(uint8_t)s->p[P_ITYPE+k];
        d->engine=s->engine;d->preset=s->preset;memcpy(d->step,s->step,sizeof d->step);memcpy(d->mf,p.t[t].mf,sizeof d->mf);
        memcpy(d->lock,s->lock,sizeof d->lock);
    }
    v->t[0].step[63].n=1;v->t[0].step[63].note[0]=(uint8_t)(40+id);
    v->t[0].lock[0].step=17;v->t[0].lock[0].param=63;v->t[0].lock[0].val=64;
    v->sum=proj_hash(v,sizeof *v-4u);
}
int main(void)
{
    harness_init();
    memset(nor,255,sizeof nor);pb_ready=0;pb_scan();
    project_v6_t old;
    for(uint32_t i=0;i<ARR_SCENES;i++){legacy(&old,i);assert(!pb_save(i,&old,sizeof old));}
    memset(proj_slot,0,sizeof proj_slot);memset(&proj_cache,0,sizeof proj_cache);
    persist_boot();
    for(uint32_t i=0;i<ARR_SCENES;i++) {
        const project_t *p=project_at(i);assert(proj_ok(p));
        assert(stored_note(p,0,63)==40+i);
        { plock_t l; pc_lock_get(&l,p->t[0].lock[0]); assert(l.param==P_E0); }
        for(uint32_t k=P_GPOS;k<=P_GFREEZE;k++)assert(pt_get(&p->t[0],k)==TP[k].def);
    }
    uint32_t serial=pb_serial;persist_boot();assert(pb_serial==serial);
    /* Interrupted conversion leaves the old journal record recoverable. */
    for(uint32_t i=0;i<ARR_SCENES;i++){legacy(&old,i);assert(!pb_save(i,&old,sizeof old));}
    memset(proj_slot,0,sizeof proj_slot);memset(&proj_cache,0,sizeof proj_cache);
    fl_fail_at=1;persist_boot();assert(pb_length[0]==sizeof(project_v6_t));
    memset(proj_slot,0,sizeof proj_slot);memset(&proj_cache,0,sizeof proj_cache);
    persist_boot();
    for(uint32_t i=0;i<ARR_SCENES;i++) {const project_t *p=project_at(i);assert(proj_ok(p));assert(stored_note(p,0,63)==40+i);}
    trk[1].p[P_ITYPE]=IT_GRAIN;grain_defaults(&trk[1]);trk[1].p[P_GSPREAD]=99;trk[1].p[P_GFREEZE]=1;trk[1].p[P_IB]=127;
    project_t saved;proj_capture(&saved);trk[1].p[P_GSPREAD]=0;proj_apply(&saved,1);
    assert(trk[1].p[P_GSPREAD]==99 && trk[1].p[P_GFREEZE]==1 && trk[1].p[P_ITYPE]==IT_GRAIN);
    assert(trk[1].p[P_IB]==60);
    assert(!gf[1].valid); /* settings persist; volatile audio never does */
    printf("granular_project: all 24 FUN6 slots migrate, engine locks move, restart idempotent, settings round trip PASS\n");
    return 0;
}
