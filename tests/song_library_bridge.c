/* SPDX-License-Identifier: GPL-3.0-only */
#define FELUCCA_SYNTH_PARTS 6
#include "editor_host.c"
int main(void) {
 harness_init(); project_stage_busy=0; song.playing=0; transport_req=0;
 for(uint32_t t=0;t<NTRK;t++) steps_clear(&trk[t]);
 trk[0].step[0]=(step_t){{48},1,ST_NOTE,0,100,0,0}; project_save(0);
 assert(!song_library_save(0,"FIRST"));
 trk[0].step[0].note[0]=60; project_save(0); assert(!song_library_save(1,"SECOND"));
 char line[4096];
 while(fgets(line,sizeof line,stdin)) {
  uint8_t frame[700]; uint32_t n=0; char *p=line,*end;
  while(*p && n<sizeof frame) { unsigned long x=strtoul(p,&end,16); if(end==p) break; frame[n++]=(uint8_t)x; p=end; }
  if(n<4) continue;
  fm1_ms+=10; transport_req=0;
  ed_handle(frame,n);
  for(uint32_t i=0;i<ed_n;i++) printf("%02x%s",ed_out[i],i+1==ed_n?"\n":" "); fflush(stdout);
 }
 return 0;
}
