/* SPDX-License-Identifier: GPL-3.0-only */
/* piterfly boot splash: the logo (tools/gen_logo.py -> sloop_logo.h: 16 colours, RLE runs of
 * (run - 1) << 4 | colour), drawn into the canvas in bands of at most 120 rows. */
#ifndef UI_SMALL                 /* (2.7) compiled for size: not the audio path (gfx.c UI_SMALL); the host tests: nothing */
#define UI_SMALL
#endif
#ifndef FELUCCA_LOGO_TEXT       /* 1: text-only piterfly in the track colours; build.py uses it if flash is full. */
#define FELUCCA_LOGO_TEXT 0
#endif
#if FELUCCA_LOGO_TEXT
UI_SMALL static void sloop_logo_draw(uint32_t y0)
{
    static const char L[] = "piterfly";
    uint32_t i, w = text_w(&FONT_L, L), k = w * 3u <= 232u ? 3u : 2u, h = 32u, off, y, x;
    int32_t px = 0;
    char c[2] = {0, 0};
    /* Text fallback uses the native font directly, with no oversized in-place scaling. */
    CV_BANDS(240, 64, C_BLACK) {
        px = 0;
        off = (240u - w) / 2u;
        for (i = 0; i < sizeof L - 1u; i++) {
            c[0] = L[i];
            px = cv_text((int32_t)off + px, 16, &FONT_L, c, i ? TE_COL_DEF[(i - 1u) & 3u] : C_WHITE) - (int32_t)off;
        }
        cv_blit(0, y0 + 60u);
    }
}
#else
#include "sloop_logo.h"
UI_SMALL static void sloop_logo_draw(uint32_t y0)
{
    uint32_t r0;
    for (r0 = 0; r0 < SLOOP_SPLASH_H; r0 += CV_ROWS) {
        uint32_t rows = SLOOP_SPLASH_H - r0 > CV_ROWS ? CV_ROWS : SLOOP_SPLASH_H - r0;
        uint32_t lo = r0 * SLOOP_SPLASH_W, hi = (r0 + rows) * SLOOP_SPLASH_W, pos = 0, i;
        cv_begin(SLOOP_SPLASH_W, rows, C_BLACK);
        for (i = 0; i < sizeof SLOOP_SPLASH_RLE && pos < hi; i++) {
            uint32_t run = (SLOOP_SPLASH_RLE[i] >> 4) + 1u, c = SLOOP_SPLASH_RLE[i] & 15u, p;
            if (c && pos + run > lo)
                for (p = pos < lo ? lo : pos; p < pos + run && p < hi; p++)
                    cv_px[p - lo] = swap16(SLOOP_SPLASH_PAL[c]);
            pos += run;
        }
        cv_blit((240u - SLOOP_SPLASH_W) / 2u, y0 + (188u - SLOOP_SPLASH_H) / 2u + r0);
    }
}
#endif

/* the boot screen: the logo, the version, where it comes from */
UI_SMALL static void sloop_splash(void)
{
    lcd_fill(0, 0, 240, 240, C_BLACK);
    sloop_logo_draw(10);
    const char *v = FELUCCA_VERSION;
    if (v[0] == 'S' && v[1] == 'L' && v[5] == ' ')
        v += 6;                                         /* "SLOOP 1.0" -> "1.0" under the wordmark */
    CV_BANDS(240, 36, C_BLACK) {
    cv_text(120 - text_w(&FONT_S, v) / 2, 0, &FONT_S, v, RGB(196, 196, 204));
    cv_text(120 - text_w(&FONT_S, "based on felucca") / 2, 18, &FONT_S, "based on felucca", RGB(96, 96, 104));
    cv_blit(0, 202);
    }
}
