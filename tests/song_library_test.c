/* SPDX-License-Identifier: GPL-3.0-only */
#define FELUCCA_SYNTH_PARTS 6
#include "editor_host.c"
static void pattern(uint32_t slot,uint32_t note) {
 for(uint32_t t=0;t<NTRK;t++) steps_clear(&trk[t]);
 trk[0].step[0]=(step_t){{(uint8_t)note},1,ST_NOTE,0,100,0,0};
 project_save(slot); assert(!sec_dirty || pb_full);
}
static void cold(void) {
 memset(proj_slot,0,sizeof proj_slot); memset(&proj_cache,0,sizeof proj_cache); sec_dirty=0;
 pb_ready=0; memset(pb_sector,0,sizeof pb_sector); pb_scan();
}
static void clear_tracks_and_reboot(void) {
 harness_init(); song.playing=0; transport_req=0; project_stage_busy=0;
 for(uint32_t t=0;t<NTRK;t++) steps_clear(&trk[t]);
 studio_open(SC_TRK);
 song.sel=NPART+1u;
 persist_flush_now(); /* The older autosave contains the empty loop. */
 trk[NPART-1u].step[63]=(step_t){{60},1,ST_NOTE,0,100,0,0};
 dstep_set(&trk[NPART].dstep[0],0,LV_NORM,1);
 dstep_set(&trk[NPART+1u].dstep[63],15,LV_HARD,1);
 persist_flush_now();
 uint32_t rec=1u<<panel.btn[B_REC];
 for(uint32_t t=NPART-1u;t<NTRK;t++) {
  song.sel=(uint8_t)t;
  fm1_in.buttons=rec;
  holds_input(rec,fm1_ms);
  fm1_ms+=700u; holds_input(0,fm1_ms);
  fm1_ms+=HOLD_CLEAR_MS; holds_input(0,fm1_ms);
  assert(!trk_step_on(&trk[t],t==NPART?0u:63u));
  fm1_in.buttons=0; holds_input(0,fm1_ms);
 }
 ui_input_ms=autosave_ms=fm1_ms; autosave_checked=fm1_ms-1000u;
 fl_fail_at=1;
 autosave_tick();
 assert(autosave_clear && !strcmp(ui.msg,"SAVE ERROR"));
 fm1_ms+=1000u;
 autosave_tick();
 assert(!autosave_clear && !strcmp(ui.msg,"CLEAR SAVED"));
 cold(); autosave_resume();
 assert(!trk_step_on(&trk[NPART-1u],63) && !trk_step_on(&trk[NPART],0) && !trk_step_on(&trk[NPART+1u],63));
 puts("REC clears the last synth and both drum tracks and survives power cycle PASS");
}
static void press_rec(void) {
 edges_btn=fm1_in.buttons=1u<<panel.btn[B_REC];
 ui_input();
 fm1_in.buttons=0;
 ui_input();
}
static void new_song(void) {
 harness_init(); song.playing=0; transport_req=0; project_stage_busy=0; rec_wait=0;
 pattern(23,48);
 arrangement.count=1; arrangement.entry[0]=(arr_entry_t){23,5}; arrangement_enabled=1;
 assert(!song_library_save(0,"KEEP"));
 studio_open(SC_SONG);
 encs[panel.enc[EN_ALGO]]=1; song_screen_input(0,0); assert(song_library_open);
 song_screen_draw();
 press_rec();
 assert(song_library_armed==4 && project_at(23) && trk[0].step[0].n);
 fl_fail_at=1; press_rec();
 assert(song_library_open && project_at(23) && trk[0].step[0].n && arrangement_enabled);
 steps_clear(&trk[0]);
 cold(); persist_boot(); autosave_resume();
 assert(project_at(23) && trk[0].step[0].n && arrangement.entry[0].scene==23);
 press_rec(); press_rec();
 assert(!song_library_open && !strcmp(ui.msg,"NEW SONG") && !undo.valid);
 assert(!project_at(23) && !trk[0].step[0].n && !arrangement_enabled && arrangement.count==4);
 assert(song_library_used(0));
 cold(); persist_boot(); autosave_resume();
 assert(!project_at(23) && !trk[0].step[0].n && !arrangement_enabled && arrangement.count==4);

 /* With identical settings, a second NEW still supersedes patterns saved after the first. */
 transport_req=0; pattern(4,60); persist_flush_now();
 assert(!song_library_new());
 cold(); persist_boot(); autosave_resume();
 assert(!project_at(4) && !trk[0].step[0].n);
 transport_req=0;
 assert(!song_library_load(0) && project_at(23) && trk[0].step[0].note[0]==48);
 puts("NEW SONG: confirmation, interrupted write, repeat after edits, cold start and named snapshot preservation PASS");
}
static void incompatible_records(void) {
 harness_init(); song.playing=0; transport_req=0; project_stage_busy=0;
 proj_capture(&proj_tmp.v9); proj_tmp.v9.magic=0x46554E41u; proj_tmp.v9.sum=proj_sum(&proj_tmp.v9);
 assert(!pb_save(0,&proj_tmp.v9,sizeof(project_t))); cold();
 assert(pb_sector[0]<0 && !project_at(0) && song_library_free()==344u);
 puts("incompatible project records: rejected and reclaimed rather than hidden capacity loss PASS");
}
static void panel_library(void) {
 harness_init(); song.playing=0; transport_req=0; project_stage_busy=0;
 for(uint32_t t=0;t<NTRK;t++) steps_clear(&trk[t]);
 dstep_set(&trk[6].dstep[63],15,LV_HARD,1);
 project_save(2); arrangement.count=1; arrangement.entry[0]=(arr_entry_t){2,8};
 studio_open(SC_SONG); encs[panel.enc[EN_ALGO]]=1; song_screen_input(0,0); assert(song_library_open);
 song_screen_draw(); layer_tap(LY_SONG); assert(song_library_used(0));
 ui.msg_t=0; song_screen_draw();
 FILE *screenfile=fopen("build/host/song-library.ppm","wb"); assert(screenfile);
 fprintf(screenfile,"P6\n240 240\n255\n");
 for(uint32_t i=0;i<240u*240u;i++) { uint16_t c=swap16(screen[i]); uint8_t rgb[3]={(uint8_t)((c>>11)*255/31),(uint8_t)(((c>>5)&63)*255/63),(uint8_t)((c&31)*255/31)}; fwrite(rgb,1,3,screenfile); }
 fclose(screenfile);
 uint32_t seq=pb_seq[26]; layer_tap(LY_SONG); assert(pb_seq[26]==seq && song_library_armed==1);
 layer_tap(LY_SONG); assert(pb_seq[26]==seq); /* identical confirmed save needs no new record */
 encs[panel.enc[EN_K1]]=7; song_screen_input(0,0); song_screen_draw(); assert(song_library_slot==7);
 encs[panel.enc[EN_K2]]=14; encs[panel.enc[EN_K3]]=1; song_screen_input(0,0); song_screen_draw();
 assert(song_name_pos==14 && song_name[14]=='A');
 song_library_pick(0);
 uint32_t load=1u<<panel.btn[B_OCTUP]; song_screen_input(load,0); assert(song_library_armed==2);
 song_screen_input(load,0); assert(trk[6].dstep[63].on && !project_at(0) && project_at(2));
 transport_req=0; persist_boot(); autosave_resume();
 assert(dstep_has(&trk[6].dstep[63],15) && arrangement.entry[0].scene==2 && arrangement.entry[0].bars==8);
 transport_req=0;
 uint32_t del=1u<<panel.btn[B_OCTDN]; song_screen_input(del,0); assert(song_library_used(0));
 song_screen_input(del,0); assert(!song_library_used(0) && dstep_has(&trk[6].dstep[63],15)); song_screen_draw();
 song_screen_input(0,1); assert(!song_library_open);
 puts("panel library: real SAVE tap, sparse drum-tail pattern, naming, load/delete confirmation, draw bounds and boot resume PASS");
}
static void interrupted_load(void) {
 harness_init(); song.playing=0; transport_req=0; project_stage_busy=0;
 pattern(0,48); arrangement.count=1; arrangement.entry[0]=(arr_entry_t){0,3};
 assert(!song_library_save(0,"FIRST"));
 pattern(4,60); arrangement.entry[0]=(arr_entry_t){4,7};
 assert(!song_library_save(1,"SECOND")); assert(!song_library_load(0)); transport_req=0;
 uint8_t *image=malloc(sizeof nor); assert(image); memcpy(image,nor,sizeof nor);
 for(int fail=1;fail<18;fail++) {
  memcpy(nor,image,sizeof nor); fl_fail_at=-1; persist_boot(); autosave_resume(); transport_req=0;
  fl_fail_at=fail; uint32_t rc=song_library_load(1); fl_fail_at=-1;
  persist_boot(); autosave_resume(); transport_req=0;
  assert(arrangement.entry[0].scene==(rc?0u:4u) && arrangement.entry[0].bars==(rc?3u:7u));
  assert(trk[0].step[0].note[0]==(rc?48u:60u));
  assert(!!project_at(4)==!rc);
 }
 free(image); puts("interrupted load: working loop, pattern map and arrangement recover together after restart PASS");
}
static void interrupted_overwrite(void) {
 harness_init(); transport_req=0; project_stage_busy=0; pattern(0,48);
 arrangement.count=1; arrangement.entry[0]=(arr_entry_t){0,4};
 assert(!song_library_save(0,"ORIGINAL"));
 uint8_t *image=malloc(sizeof nor); assert(image); memcpy(image,nor,sizeof nor);
 for(int fail=1;fail<45;fail++) {
  memcpy(nor,image,sizeof nor); fl_fail_at=-1; cold(); transport_req=0; project_stage_busy=0;
  assert(!song_library_load(0)); transport_req=0; trk[0].step[0].note[0]=60;
  fl_fail_at=fail; uint32_t rc=song_library_save(0,"REPLACED"); fl_fail_at=-1;
  cold(); transport_req=0; assert(!song_library_load(0));
  assert(trk[0].step[0].note[0]==(rc?48u:60u));
 }
 free(image); puts("interrupted overwrite: previous complete song or committed replacement survives every injected write failure PASS");
}
int main(void) {
 harness_init(); assert(PB_PAIRS==44 && SMP_USER_SLOTS==0);
 song.playing=0; transport_req=0; project_stage_busy=0;
 pattern(0,48); arrangement.count=1; arrangement.entry[0]=(arr_entry_t){0,4};
 assert(!song_library_save(0,"FIRST"));
 uint32_t first=pb_free();
 assert(!song_library_save(0,"FIRST") && pb_free()==first);
 pattern(0,60); pattern(4,67); arrangement.entry[0]=(arr_entry_t){4,8};
 assert(!song_library_save(1,"SECOND"));
 assert(!song_library_load(0));
 assert(project_at(0) && !project_at(4));
 step_t s; pc_step_get(&s,project_at(0)->t[0].step[0]); assert(s.note[0]==48);
 assert(arrangement.entry[0].scene==0 && arrangement.entry[0].bars==4);
 cold(); transport_req=0; assert(!song_library_load(1));
 pc_step_get(&s,project_at(0)->t[0].step[0]); assert(s.note[0]==60);
 assert(project_at(4) && arrangement.entry[0].scene==4);
 uint32_t len,seq; const uint8_t *part=song_library_part(0,2,&len,&seq);
 assert(part && len==sizeof(project_t) && seq && proj_ok((const project_t *)part));
 transport_req=0; assert(!song_library_load(1)); transport_req=0;
 uint32_t before=pb_free(); assert(!song_library_delete(0)); assert(pb_free()>before);
 cold(); assert(!song_library_used(0) && song_library_used(1));
 // Fill with distinct snapshots until capacity, while preserving earlier songs.
 transport_req=0; uint32_t full=0;
 for(uint32_t songid=2;songid<8;songid++) {
  for(uint32_t slot=0;slot<24;slot++) {
   pattern(slot,40+songid+slot);
   if(pb_full) break;
  }
  uint32_t rc=song_library_save(songid,"FILL");
  if(rc==5u) { full=1; break; }
  assert(!rc);
 }
 assert(full && song_library_used(1));
 assert(!song_library_new() && !project_at(4) && song_library_used(1));
 transport_req=0;
 assert(!song_library_delete(1));
 cold(); assert(!song_library_used(1));
 puts("named songs: sparse snapshots, independent load, cold scan, export reads, delete and full-space recovery PASS");
 interrupted_overwrite();
 panel_library();
 interrupted_load();
 incompatible_records();
    clear_tracks_and_reboot();
    new_song();
 return 0;
}
