/* SPDX-License-Identifier: GPL-3.0-only */
/* Named snapshots pin immutable journal records; a root commits last. */
UI_SMALL static int song_has_notes(const project_t *p)
{
    if(!proj_ok(p)) return 0;
    for(uint32_t t=0;t<NTRK;t++) for(uint32_t k=0;k<NSTEP;k++) {
        if(t<NPART) { step_t s; pc_step_get(&s,p->t[t].step[k]); if(s.time==ST_NOTE && s.n) return 1; }
        else {
            const uint8_t *head=(const uint8_t *)p->t[t].step;
            const uint8_t *tail=t==NPART?p->drum2_tail:p->drum_tail;
            uint32_t off=k*10u;
            if((off<512u?head[off]:tail[off-512u]) || (off+1u<512u?head[off+1u]:tail[off+1u-512u])) return 1;
        }
    }
    return 0;
}
UI_SMALL static __attribute__((noinline)) uint32_t song_library_free(void) { if(!pb_ready) pb_scan(); uint32_t n=pb_free(); return (n?n-1u:0u)*8u; }
UI_SMALL static __attribute__((noinline)) void song_library_name(uint32_t slot,char *dst)
{
    if(!pb_ready) pb_scan();
    const song_root_t *r=slot<SONG_SLOTS?pb_root(26u+slot):0;
    if(!r) { str_cpy(dst,"EMPTY",16); return; }
    for(uint32_t i=0;i<15u;i++) { char c=r->name[i]; dst[i]=c>=32 && c<=126?c:0; }
    dst[15]=0;
}
UI_SMALL static __attribute__((noinline)) uint32_t song_library_used(uint32_t slot)
{
    if(!pb_ready) pb_scan(); return slot<SONG_SLOTS && pb_root(26u+slot)!=0;
}
/* rc: 0 OK, 1 invalid, 2 stop first, 3 empty, 4 no flash, 5 full,
 * 6 I/O error, 7 editor busy, 8 damaged snapshot. */
UI_SMALL static __attribute__((noinline)) uint32_t song_library_save(uint32_t slot,const char *name)
{
    song_root_t r; uint32_t notes=0;
    if(slot>=SONG_SLOTS || !name || !name[0]) return 1;
    if(song.playing || transport_req) return 2;
    if(!flash_ok) return 4;
    if(project_stage_busy) return 7;
    memset(&r,0,sizeof r); memset(r.ref,255,sizeof r.ref);
    str_cpy(r.name,name,sizeof r.name);
    uint32_t visible=0; for(uint32_t i=0;r.name[i];i++) { if(r.name[i]<32 || r.name[i]>126) return 1; visible|=r.name[i]!=' '; }
    if(!visible) return 1;
    r.order=arrangement; r.mode=arrangement_enabled;
    if(!arr_valid(&r.order,ARR_READY_MASK)) return 1;
    for(uint32_t i=0;i<4u;i++) if(sec_dirty&(1u<<i)) {
        if(pb_save(proj_cache.id[i],&proj_slot[i],sizeof(project_t))) return pb_full?5u:6u;
        sec_dirty&=(uint8_t)~(1u<<i);
    }
    for(uint32_t i=0;i<ARR_SCENES;i++) if(song_has_notes(project_at(i))) {
        if(pb_sector[i]<PB_PAIR_ID) return 8;
        r.ref[i]=(uint8_t)(pb_sector[i]-PB_PAIR_ID); notes++;
    }
    proj_capture(&proj_tmp.v9);
    if(!notes && !song_has_notes(&proj_tmp.v9)) return 3;
    if(pb_autosave_save(&proj_tmp.v9,sizeof(project_t))) return pb_full?5u:6u;
    r.ref[24]=(uint8_t)(pb_sector[pb_autosave_latest()]-PB_PAIR_ID);
    if(!pb_root(SONG_ACTIVE)) {
        st_hdr_t h; r.settings_seq=st_current(OBJ_SETTINGS,&h)<0?1u:h.seq+1u;
        if(!r.settings_seq || pb_save(SONG_ACTIVE,&r,sizeof r)) return pb_full?5u:6u;
    }
    r.settings_seq=0;
    if(pb_save(26u+slot,&r,sizeof r)) return pb_full?5u:6u;
    autosave_hash=proj_tmp.v9.sum; settings_save();
    return 0;
}
UI_SMALL static __attribute__((noinline)) uint32_t song_library_delete(uint32_t slot)
{
    if(slot>=SONG_SLOTS) return 1;
    if(song.playing || transport_req) return 2;
    if(!flash_ok) return 4;
    if(project_stage_busy) return 7;
    if(!song_library_used(slot)) return 1;
    return pb_save(26u+slot,0,0)?(pb_full?5u:6u):0u;
}
UI_SMALL static __attribute__((noinline)) uint32_t song_library_load(uint32_t slot)
{
    if(slot>=SONG_SLOTS) return 1;
    if(song.playing || transport_req) return 2;
    if(!flash_ok) return 4;
    if(project_stage_busy) return 7;
    const song_root_t *old=pb_root(26u+slot);
    if(!pb_root_valid(old)) return 8;
    song_root_t r=*old;
    if(r.ref[24]>=PB_PAIRS || !proj_ok((const project_t *)ST_XIP_PTR(pb_off(PB_PAIR_ID+r.ref[24])+ST_PAYLOAD_OFF))) return 8;
    for(uint32_t i=0;i<24u;i++) if(r.ref[i]!=255u && !proj_ok((const project_t *)ST_XIP_PTR(pb_off(PB_PAIR_ID+r.ref[i])+ST_PAYLOAD_OFF))) return 8;
    st_hdr_t h; r.settings_seq=st_current(OBJ_SETTINGS,&h)<0?1u:h.seq+1u;
    if(!r.settings_seq) return 6;
    if(pb_save(SONG_ACTIVE,&r,sizeof r)) return pb_full?5u:6u;
    pb_mount(&r,pb_seq[SONG_ACTIVE]);
    for(uint32_t i=0;i<4u;i++) proj_slot[i].magic=0;
    sec_dirty=0; live_sec=-1; song_kept=0;
    if(pb_load(24u,&proj_tmp,sizeof proj_tmp)!=(int)sizeof(project_t) || !proj_ok(&proj_tmp.v9)) return 8;
    project_apply(&proj_tmp.v9);
    arrangement=r.order; arrangement_enabled=(uint8_t)(r.mode!=0); memset(&arrangement_clock,0,sizeof arrangement_clock);
    autosave_hash=proj_tmp.v9.sum; settings_save(); sync_reload=1; ui.force=1;
    return 0;
}
UI_SMALL static __attribute__((noinline)) void song_library_message(uint32_t rc)
{
    static const char *const text[]={"SONG SAVED","INVALID SONG","STOP FIRST","EMPTY SONG","NO FLASH","NO SPACE: EXPORT/DELETE","SAVE ERROR","EDITOR BUSY","DAMAGED SONG"};
    ui_message(text[rc<9u?rc:6u]);
}
/* An export is read directly from pinned records, with a root-sequence guard.
 * item 0 is the manifest, 1 the working loop, 2..25 are patterns 0..23. */
UI_SMALL static const uint8_t *song_library_part(uint32_t slot,uint32_t item,uint32_t *len,uint32_t *seq)
{
    *len=0; *seq=0;
    if(slot>=SONG_SLOTS || item>25u || !flash_ok) return 0;
    const song_root_t *r=pb_root(26u+slot);
    if(!r) return 0;
    *seq=pb_seq[26u+slot];
    if(!item) { *len=sizeof *r; return (const uint8_t *)r; }
    uint32_t index=item==1u?24u:item-2u;
    if(r->ref[index]==255u) return (const uint8_t *)r;
    st_hdr_t h; if(pb_ref(r->ref[index],index,&h)) return 0;
    *len=h.len; return ST_XIP_PTR(pb_off(PB_PAIR_ID+r->ref[index])+ST_PAYLOAD_OFF);
}
