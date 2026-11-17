/* SPDX-License-Identifier: GPL-3.0-only */
/* Frozen public 1.2.0 HOME renderer for pixel comparisons. */
UI_SMALL static void baseline_studio_tracks_draw(void)
{
    static uint32_t head, rows[NTRK], footer;
    uint32_t i, j, base=song.sel&~3u;
    int32_t sect = song.playing && live_sec >= 0 ? live_sec : (int32_t)song.g[G_SLOT] - 1;   /* (2.7.2) the section playing */
    te_header("tracks", TE_G3, &head, sect);
    for (i = base; i < base+4u; i++) {
        track_t *t = &trk[i];
        char b[24], e[16];
        uint32_t selected = song.sel == i, len = (uint32_t)clamp(t->p[P_SLEN], 1, 64);
        uint32_t level = i == TRK_DRUM ? song.g[G_DRLVL] : t->p[P_LEVEL];
        uint32_t silent = trk_silent(t) || !level, pos = t->seq_idx % len;
        uint32_t rec = (song.playing && ((song.rec >> i) & 1u)) || (ft_on && ft_trk % NTRK == i);
        uint32_t solo = (song.solo >> i) & 1u, h;
        uint16_t col = TE_COL[i&3u], dim = TE_DIM[i&3u];
        if (is_drum(t)) {
            str_cpy(b, DRUM_KIT_NAMES[drum_kit_for(t)], sizeof b);
            te_lower(e, "drums", sizeof e);
        } else {
            trk_short_name(i, b);
            te_lower(e, ENGINES[t->eng_req % NENGINES]->name, sizeof e);
        }
        b[13] = 0;
        h = studio_hash(selected + level * 7u + silent * 997u + rec * 1999u + solo * 4999u + len * 37u +
                        song.playing * 7u + pos * 71u, b);
        for (j = 0; j < len; j++) h = h * 31u + (uint32_t)trk_step_on(t, j);
        if (!ui.force && h == rows[i]) continue;
        rows[i] = h;
        CV_BANDS(240, 36, C_BLACK) {
        cv_rect(2, 3, 26, 30, selected ? col : dim);   /* the track tile */
        {
            char n[2] = {(char)('1' + i), 0};
            cv_text(11, 10, &FONT_S, n, selected ? C_BLACK : col);
        }
        cv_text(34, 1, &FONT_S, b, selected ? C_WHITE : TE_G4);
        if (34 + text_w(&FONT_S, b) + 6 + text_w(&FONT_S, e) < 196)
            cv_text(34 + text_w(&FONT_S, b) + 6, 1, &FONT_S, e, selected ? col : TE_G3);
        if (rec) {
            cv_rect(200, 1, 36, 14, TE_RED);
            cv_text(206, 1, &FONT_S, "rec", C_WHITE);
        } else if (solo) {
            cv_rect(200, 2, 36, 15, C_WHITE);
            cv_text(202, 1, &FONT_S, "solo", C_BLACK);
        } else if (silent) {
            cv_text(204, 1, &FONT_S, "mute", TE_G3);
        }
        /* Each row is one real group of 16 steps, including while stopped. */
        uint32_t pages = (len + 15u) / 16u;
        int32_t pitch = pages == 1u ? 0 : 16 / (int32_t)pages;
        int32_t height = pages == 1u ? 9 : pitch - 1;
        for (j = 0; j < pages * 16u; j++) {
            int32_t x = 34 + (int32_t)(j % 16u) * 10;
            int32_t y = pages == 1u ? 21 : 18 + (int32_t)(j / 16u) * pitch;
            int on = j < len && trk_step_on(t, j);
            cv_rect(x, y, 8, height, j >= len ? C_BLACK :
                    on ? (silent ? TE_G3 : selected ? col : dim) : TE_G1);
            if (on && !selected && !silent)
                cv_rect(x, y, 8, 1, col);
            if (song.playing && j == pos)
                cv_outline(x, y, 8, height, C_WHITE);
        }
        cv_rect(198, 23, 38, 5, TE_G1);                /* the level */
        if (!silent)
            cv_rect(198, 23, (int32_t)level * 38 / 127, 5, selected ? col : TE_G3);
        if (selected || rec)
            cv_outline(0, 0, 240, 36, rec ? TE_RED : col);
        cv_blit(0, 40 + (i-base) * 36);
    }
    }
    {   /* KNOB 1 swing (the global groove), 2 level, 3 steps, 4 pan of the selected track */
        track_t *t = TSEL;
        static char v[4][8];
        static const char *const lab[4] = {"swing", "level", "steps", "pan"};
        const char *val[4] = {v[0], v[1], v[2], v[3]};
        int32_t ratio[4];
        uint32_t lvl = is_drum(t) ? song.g[G_DRLVL] : t->p[P_LEVEL];
        swing_str(v[0], song.g[G_SWING]);
        fmt_int(v[1], t->p[P_MUTE] ? 0 : (int32_t)lvl * 100 / 127);
        fmt_int(v[2], t->p[P_SLEN]);
        fmt_int(v[3], t->p[P_PAN]);
        ratio[0] = song.g[G_SWING] * 10;
        ratio[1] = t->p[P_MUTE] ? 0 : (int32_t)lvl * 1000 / 127;
        ratio[2] = (t->p[P_SLEN] - 1) * 1000 / 63;
        ratio[3] = (t->p[P_PAN] + 64) * 1000 / 127;
        te_dials(184, lab, val, ratio, song.sel, &footer);
    }
}

