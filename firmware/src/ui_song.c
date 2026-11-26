/* SPDX-License-Identifier: GPL-3.0-only */
/* Compact 240 x 240 song-order editor. All changes and flash saves require
 * stopped transport. Up to 128 entries reference the 24 stored patterns. */
static uint8_t song_cursor, song_store_armed, song_load_armed;
static uint32_t song_store_t;   /* when REC or OCT+ asked for its confirmation (ms) */
UI_SMALL static int on_song_page(void) { return !ui.home && cur_page()->scope == SC_SONG; }
UI_SMALL static void song_sane(void)                         /* a bad chain (blank / damaged settings): defaults */
{
    uint32_t i, ok = arrangement.count >= 1u && arrangement.count <= ARR_STEPS;
    for (i = 0; ok && i < arrangement.count; i++)
        ok = arrangement.entry[i].scene < ARR_SCENES && arrangement.entry[i].bars >= 1u && arrangement.entry[i].bars <= 64u;
    if (!ok)
        arr_defaults(&arrangement);
    if (song_cursor >= arrangement.count)
        song_cursor = (uint8_t)(arrangement.count - 1u);
}

#if NPART == 6
static uint32_t song_library_free(void), song_library_used(uint32_t slot);
static void song_library_name(uint32_t slot,char *dst), song_library_message(uint32_t rc);
static uint32_t song_library_save(uint32_t slot,const char *name), song_library_load(uint32_t slot), song_library_delete(uint32_t slot);
static uint32_t song_library_new(void);
static uint8_t song_library_open, song_library_slot, song_name_pos, song_library_armed;
static char song_name[16];
UI_SMALL static __attribute__((noinline)) void song_library_pick(uint32_t slot)
{
    song_library_slot=(uint8_t)slot; song_library_armed=0;
    song_library_name(slot,song_name);
    if(!song_library_used(slot)) { str_cpy(song_name,"SONG 01",sizeof song_name); song_name[6]=(char)('1'+slot); }
    ui.force=1;
}
UI_SMALL static __attribute__((noinline)) void song_library_draw(void)
{
    char b[30]; uint32_t start=song_library_slot>3u?song_library_slot-3u:0u;
    CV_BANDS(240,40,C_BLACK) {
        cv_text(4,2,&FONT_L,"SONGS",C_WHITE);
        fmt_int(b,(int32_t)song_library_free()); str_cpy(b+str_len(b)," KiB FREE",sizeof b-str_len(b));
        cv_text(112,12,&FONT_S,b,TE_G3); cv_blit(0,0);
    }
    for(uint32_t row=0;row<4u;row++) { uint32_t slot=start+row;
        CV_BANDS(240,30,C_BLACK) {
            if(slot<8u) { song_library_name(slot,b); cv_text(24,6,&FONT_S,b,slot==song_library_slot?C_WHITE:TE_G3);
                if(slot==song_library_slot) cv_rect(2,3,5,24,TE_COL[slot&3u]); }
            cv_blit(0,40+row*30u);
        }
    }
    CV_BANDS(240,80,C_BLACK) {
        cv_text(4,2,&FONT_S,"NAME:",TE_G3); cv_text(48,2,&FONT_S,song_name,C_WHITE);
        cv_rect(48+song_name_pos*8u,19,8,2,TE_G2);
        cv_text(4,26,&FONT_S,"K1 slot K2 pos K3 letter",TE_G3);
        cv_text(4,43,&FONT_S,"SAVE save  OCT+ load",TE_G3);
        cv_text(4,60,&FONT_S,"OCT- del REC new ALGO back",TE_G3);
        if(ui.msg_t) { cv_rect(0,20,240,32,C_WHITE); cv_text(4,28,&FONT_S,ui.msg,C_BLACK); }
        cv_blit(0,160);
    }
}
UI_SMALL static __attribute__((noinline)) void song_library_input(uint32_t pressed,uint32_t home)
{
    int32_t step=panel_enc(EN_ALGO);
    if(home==1u || step) { song_library_open=0; ui.force=1; return; }
    step=panel_enc(EN_K1)+panel_enc(EN_PRESET);
    if(step) song_library_pick((uint32_t)clamp(song_library_slot+step,0,7));
    step=panel_enc(EN_K2); if(step) { song_name_pos=(uint8_t)clamp(song_name_pos+step,0,14); song_library_armed=0; }
    step=panel_enc(EN_K3);
    if(step) {
        static const char chars[]=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"; uint32_t i=0;
        while(chars[i] && chars[i]!=song_name[song_name_pos]) i++;
        if(!chars[i]) i=0;
        uint32_t n=(uint32_t)clamp((int32_t)i+step,0,sizeof chars-2u);
        for(i=0;i<song_name_pos;i++) if(!song_name[i]) song_name[i]=' ';
        song_name[song_name_pos]=chars[n]; song_name[15]=0; song_library_armed=0;
    }
    panel_enc(EN_K4); panel_enc(EN_SELECT);
    uint32_t op=(pressed>>panel.btn[B_SAVE]&1u)?1u:(pressed>>panel.btn[B_OCTUP]&1u)?2u:
                (pressed>>panel.btn[B_OCTDN]&1u)?3u:(pressed>>panel.btn[B_REC]&1u)?4u:0u;
    if(!op) return;
    if(song.playing || transport_req) { ui_message("STOP FIRST"); return; }
    if((op!=1u || song_library_used(song_library_slot)) &&
       (song_library_armed!=op || fm1_ms-song_store_t>3000u)) {
        song_library_armed=(uint8_t)op; song_store_t=fm1_ms;
        ui_message(op==1u?"SAVE AGAIN: REPLACE":op==2u?"OCT+ AGAIN: LOAD":
                   op==3u?"OCT- AGAIN: DELETE":"REC AGAIN: CLEAR SONG"); return;
    }
    song_library_armed=0;
    uint32_t rc=op==1u?song_library_save(song_library_slot,song_name):op==2u?song_library_load(song_library_slot):
                op==3u?song_library_delete(song_library_slot):song_library_new();
    if(rc || op==1u) song_library_message(rc);
    else if(op==2u) ui_message("SONG LOADED");
    else if(op==3u) { song_library_pick(song_library_slot); ui_message("SONG DELETED"); }
    else { song_library_open=0; song_cursor=0; ui_message("NEW SONG"); }
}
#endif

/* SONG: the chain of sections (A..D in the track colours), bars of each, where the song is */
UI_SMALL static void song_screen_draw(void)
{
    song_sane();
#if NPART == 6
    if(song_library_open) { song_library_draw(); return; }
#endif
    uint32_t i, start = song_cursor > 3u ? song_cursor - 3u : 0u;
    static uint32_t previous;
    const uint16_t *SC = TE_COL;                    /* the tracks' colours (2.7.2: your theme's, when it is on) */
    uint32_t sig = song_cursor + 17u * arrangement_enabled + 37u * song.playing +
                   71u * arrangement_clock.index + 127u * arrangement_clock.bar +
                   257u * arrangement.count + 509u * song.g[G_BPM];
    char b[30];
    for (i = 0; i < arrangement.count; i++)
        sig = sig * 31u + arrangement.entry[i].scene + 7u * arrangement.entry[i].bars;
    for (i = 0; i < ARR_SCENES; i++) sig = sig * 3u + (uint32_t)project_used(i);
    if (!ui.force && !ui.msg_t && sig == previous) return;
    previous = ui.msg_t ? ~sig : sig;               /* redraw after a message expires */
    /* Draw one small band at a time: never exceed the 124-row canvas. */
    CV_BANDS(240, 40, C_BLACK) {
    cv_text(4, 4, &FONT_L, "SONG", C_WHITE);
    cv_text(76, 20, &FONT_S, arrangement_enabled ? "song mode" : "loop mode", arrangement_enabled ? SC[3] : TE_G3);
    fmt_int(b, song.g[G_BPM]);
    cv_text(236 - text_w(&FONT_S, b) - 28, 4, &FONT_S, b, C_WHITE);
    cv_text(236 - 24, 4, &FONT_S, "bpm", TE_G3);
    cv_text(236 - text_w(&FONT_S, song.playing ? "playing" : "stopped"), 20, &FONT_S,
            song.playing ? "playing" : "stopped", song.playing ? SC[1] : TE_G3);
    cv_rect(0, 39, 240, 1, TE_G1);
    cv_blit(0, 0);
    }
    for (i = 0; i < 5u; i++) {
        uint32_t pos = start + i;
        CV_BANDS(240,26,C_BLACK) {
        if (pos < arrangement.count) {
            const arr_entry_t *e = &arrangement.entry[pos];
            int selected = pos == song_cursor, used = project_used(e->scene);
            uint16_t sc = SC[e->scene & 3u];
            int32_t w;
            fmt_int(b, (int32_t)pos + 1);
            cv_text(4, 5, &FONT_S, b, selected ? C_WHITE : TE_G3);
            cv_rect(30, 3, 28, 20, used ? sc : TE_G1);   /* the section tile */
            str_cpy(b, N_SECT[e->scene], sizeof b);
            cv_text(35, 5, &FONT_S, b, used ? C_BLACK : sc);
            w = e->bars * 44 / 64 + 4;                 /* its length as a bar */
            cv_rect(60, 9, w, 8, used ? (selected ? sc : TE_G2) : TE_G1);
            fmt_int(b, e->bars);
            str_cpy(b + str_len(b), e->bars == 1 ? " bar" : " bars", 8);
            cv_text(180, 5, &FONT_S, b, selected ? C_WHITE : TE_G3);
            cv_text(112, 5, &FONT_S, used ? "ready" : "empty", used ? TE_G3 : TE_G2);
            if (arrangement_clock.running && arrangement_clock.index == pos) {
                cv_rect(60, 19, (int32_t)(arrangement_clock.bar + 1u) * w / (e->bars ? e->bars : 1), 2, C_WHITE);
                cv_rect(226, 9, 9, 9, SC[1]);
            }
            if (selected) cv_outline(0, 0, 240, 26, sc);
        }
        cv_blit(0, 42 + 26u * i);
        }
    }
    CV_BANDS(240, 68, C_BLACK) {
    if (ui.msg_t) {
        cv_rect(0, 4, 240, 30, C_WHITE);
        cv_text((240 - text_w(&FONT_S, ui.msg)) / 2, 11, &FONT_S, ui.msg, C_BLACK);
    } else {
        static const char *const L[4] = {"entry", "section", "bars", "length"};
        for (i = 0; i < 4u; i++) {
            cv_rect((int32_t)i * 60 + 4, 6, 52, 3, SC[i]);
            cv_text((int32_t)i * 60 + 30 - text_w(&FONT_S, L[i]) / 2, 12, &FONT_S, L[i], TE_G3);
        }
    }
    cv_text(4, 34, &FONT_S, "rec: store   save: chain", TE_G4);
#if NPART == 6
    cv_text(4, 50, &FONT_S, "ALGO: named songs", TE_G3);
    fmt_int(b, arrangement.entry[song_cursor].bars);
    str_cpy(b + str_len(b), arrangement.entry[song_cursor].bars == 1 ? " bar" : " bars", 8);
    cv_text(180, 50, &FONT_S, b, RGB(255,255,0));
#else
    cv_text(4, 50, &FONT_S, "oct-: loop/song  oct+ x2: load", TE_G3);
#endif
    cv_blit(0, 172);
    }
}

UI_SMALL static void song_screen_input(uint32_t pressed, uint32_t home)
{
    uint32_t k, b;
    int32_t steps;
    song_sane();
#if NPART == 6
    if(song_library_open) { song_library_input(pressed,home); return; }
    if(panel_enc(EN_ALGO)) { song_library_open=1; song_library_pick(song_library_slot); return; }
#endif
    if (home == 1u) { go_home(); return; }       /* BT_TAP */
    for (k = 0; k < NB; k++) {
        if (!((pressed >> panel.btn[k]) & 1u)) continue;
        b = k;
        if ((b == B_PLAY || b == B_REC) && ft_owns_press())
            continue;                            /* (they close a free take: seq.c) */
        if (b == B_PLAY) {
            if (!song.playing && arrangement_enabled && !arr_valid(&arrangement, arrangement_ready())) {
                ui_message("EMPTY SECTION: REC");
            } else transport_req = song.playing ? 2 : 1;
        } else if (b == B_SEQ) {
            open_family(FAM_SEQ);
        } else if (b == B_SAVE || b == B_REC || b == B_OCTDN || b == B_OCTUP) {
            uint32_t scene = arrangement.entry[song_cursor].scene;
            if (song.playing || transport_req) { ui_message("STOP FIRST"); continue; }
            if (b == B_SAVE) {
                arrangement_save();
                song_store_armed = 0;
            } else if (b == B_OCTDN) {
                arrangement_enabled ^= 1u;
                ui_message(arrangement_enabled ? "SONG MODE" : "LOOP MODE");
            } else if (b == B_OCTUP) {                   /* load: a second press within 3 s (unsaved work goes) */
                if (song_load_armed && fm1_ms - song_store_t <= 3000u) {
                    song_load_armed = 0;
                    project_load(scene);
                } else {
                    song_load_armed = 1;
                    song_store_armed = 0;
                    song_store_t = fm1_ms;
                    ui_message("OCT+ AGAIN: LOAD");
                }
            } else if (project_used(scene) &&
                       (!song_store_armed || fm1_ms - song_store_t > 3000u)) {
                song_store_armed = 1;
                song_store_t = fm1_ms;
                ui_message("REC AGAIN: REPLACE");
            } else {
                project_save(scene);
                settings_save();
                song_store_armed = 0;
            }
        }
    }
    for (k = 0; k < 4u; k++) {
        steps = panel_enc(EN_K1 + k);
        if (!steps) continue;
        song_store_armed = 0;
        song_load_armed = 0;
        if (k == 0) {
            song_cursor = (uint8_t)clamp(song_cursor + steps, 0, arrangement.count - 1);
            continue;
        }
        if (song.playing || transport_req) { ui_message("STOP FIRST"); continue; }
        if (k == 1) arrangement.entry[song_cursor].scene = (uint8_t)clamp(arrangement.entry[song_cursor].scene + steps, 0, ARR_SCENES - 1);
        if (k == 2) arrangement.entry[song_cursor].bars = (uint8_t)clamp(arrangement.entry[song_cursor].bars + steps, 1, 64);
        if (k == 3) {
            arrangement.count = (uint8_t)clamp(arrangement.count + steps, 1, ARR_STEPS);
            if (song_cursor >= arrangement.count) song_cursor = arrangement.count - 1;
        }
    }
    /* Unused encoders have no hidden action on this screen. */
    panel_enc(EN_SELECT); panel_enc(EN_ALGO); panel_enc(EN_PRESET);
}
