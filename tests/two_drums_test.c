/* SPDX-License-Identifier: GPL-3.0-only */
#define FELUCCA_SYNTH_PARTS 6
#include "editor_host.c"
int main(void) {
 harness_init();
 assert(NPART==6 && NTRK==8 && NDRUM_TRACKS==2 && TRK_DRUM==7);
 track_t *a=&trk[6], *b=&trk[7];
 a->p[P_E0]=DRUM_DEFAULT_KIT; b->p[P_E0]=DRUM_DEFAULT_KIT+1;
 assert(is_drum(a) && is_drum(b) && !is_drum(&trk[5]));
 trk_note_on(a,46,110); trk_note_on(b,42,110);
 assert(drum_state(a)->v[0].active && drum_state(a)->v[0].note==46);
 assert(drum_state(b)->v[0].active && drum_state(b)->v[0].note==42);
 dstep_set(&a->dstep[63],15,LV_HARD,3); dstep_set(&b->dstep[62],2,LV_HARD,1);
 drum_tune_for(a)[15]=-8; drum_dec_for(a)[15]=3;
 drum_tune_for(b)[15]=7; drum_dec_for(b)[15]=-4;
 proj_capture(&proj_tmp.v9); assert(proj_ok(&proj_tmp.v9));
 memset(a->dstep,0,sizeof a->dstep); memset(b->dstep,0,sizeof b->dstep);
 memset(drum_tunes,0,sizeof drum_tunes); memset(drum_decs,0,sizeof drum_decs);
 proj_apply(&proj_tmp.v9,1);
 assert(dstep_has(&a->dstep[63],15) && !dstep_has(&b->dstep[63],15));
 assert(dstep_has(&b->dstep[62],2) && !dstep_has(&a->dstep[62],2));
 assert(drum_tune_for(a)[15]==-8 && drum_dec_for(a)[15]==3);
 assert(drum_tune_for(b)[15]==7 && drum_dec_for(b)[15]==-4);
 uint64_t energy=0; for(int j=0;j<250;j++) { int32_t out[CTL*2]; mix_block(out,CTL); for(int i=0;i<CTL*2;i++) energy+=llabs(out[i]); }
 assert(energy); printf("6+2 independent voices, hi-hat choke, patterns, tuning, persistence and audio: PASS (%zu-byte project)\n",sizeof(project_t));
 for(uint32_t slot=0;slot<24;slot++) {
  a->p[P_E0]=(int16_t)(slot%DRUM_KITS); b->p[P_E0]=(int16_t)((slot+5)%DRUM_KITS);
  proj_capture(&proj_tmp.v9); assert(!pb_save(slot,&proj_tmp.v9,sizeof(project_t)));
 }
 for(uint32_t slot=0;slot<24;slot++) {
  memset(a->dstep,0,sizeof a->dstep); memset(b->dstep,0,sizeof b->dstep);
  assert(pb_load(slot,&proj_tmp,sizeof proj_tmp)==sizeof(project_t)); assert(proj_ok(&proj_tmp.v9));
  proj_apply(&proj_tmp.v9,1);
  assert(a->p[P_E0]==slot%DRUM_KITS && b->p[P_E0]==(slot+5)%DRUM_KITS);
  assert(dstep_has(&a->dstep[63],15) && dstep_has(&b->dstep[62],2));
 }
 song.sel=0;
 uint8_t get[]={ED_HDR0,ED_HDR1,ED_HDR2,ED_DRUM_STEP,63,6};
 ed_handle(get,sizeof get); assert(song.sel==0 && ed_out[5]==63 && ed_out[8]==2);
 song.sel=7; ed_w.st[61]=12345;
 uint8_t put[]={ED_HDR0,ED_HDR1,ED_HDR2,ED_DRUM_STEP,61,1,0,0,3,0,0,0,0,0,0,0,0,0,6};
 ed_handle(put,sizeof put);
 assert(dstep_has(&a->dstep[61],0) && !dstep_has(&b->dstep[61],0) && ed_w.st[61]==12345 && song.sel==7);
 uint8_t tw[]={12,15,10,5,6}; ed_begin(ED_LIVE); ed_live(tw,sizeof tw);
 assert(drum_tune_for(a)[15]==2 && drum_dec_for(a)[15]==1 && drum_tune_for(b)[15]==7);
 assert(proj_import(&proj_tmp.v9,&(project_v7_t){0},sizeof(project_v7_t))==0);
 uint64_t hash=1469598103934665603ull;
 song.g[G_CH7]=9; song.g[G_DRCH]=10;
 assert(midi_own(8)==a && midi_own(9)==b);
 midi_cc(a,7,90); assert(a->p[P_LEVEL]==90 && song.g[G_DRLVL]!=90);
 for(uint32_t effect=0;effect<=IT_GRAIN;effect++) {
  a->p[P_ITYPE]=b->p[P_ITYPE]=(int16_t)effect;
  if(effect==IT_GRAIN) { grain_defaults(a); grain_defaults(b); }
  a->p[P_SLCR]=b->p[P_SLCR]=SL_STUT;
  for(uint32_t t=0;t<NPART;t++) { trk[t].p[P_AMODE]=0; trk[t].p[P_LEVEL]=30; trk_note_on(&trk[t],48+t,80); }
  for(uint32_t j=0;j<96;j++) {
   if(j%24==0) { trk_note_on(a,36,100); trk_note_on(b,38,100); }
   int32_t out[CTL*2]; mix_block(out,CTL);
   for(uint32_t k=0;k<CTL*2;k++) hash=(hash^(uint32_t)out[k])*1099511628211ull;
   uint8_t used[AUDIO_PAGES]={0};
   for(uint32_t t=0;t<NTRK;t++) for(uint32_t k=0;k<gf[t].np;k++) {
    uint32_t page=gf[t].page[k]; assert(page<AUDIO_PAGES && !audio_blocked[page] && !used[page]); used[page]=1;
   }
  }
 }
 printf("all drum inserts, eight stutters, granular ownership, MIDI channels/volume; audio hash %016llx\n",(unsigned long long)hash);
 return 0;
}
