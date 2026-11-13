/* SPDX-License-Identifier: GPL-3.0-only */
/* Fixed engine/SLICER addresses, with unused pages loaned to the live granular
 * insert. Core 0 changes ownership before dispatching either render core. */
#if NPART > 3
#define AUDIO_PAGE 1024u
#define AUDIO_PAGES 137u
#define AUDIO_ENGINE_PAGES 9u
static uint8_t audio_ram[32u * AUDIO_PAGE] __attribute__((aligned(4)));
static uint8_t audio_pool[(127u - 32u) * AUDIO_PAGE] __attribute__((section(".pool"),aligned(4)));
static uint8_t audio_extra[10u * AUDIO_PAGE] __attribute__((aligned(4)));
static uint8_t audio_blocked[AUDIO_PAGES] __attribute__((section(".noinit.audio")));
static uint8_t *audio_page(uint32_t p) { return p < 32u ? audio_ram + p*AUDIO_PAGE : p>=127u?audio_extra+(p-127u)*AUDIO_PAGE:audio_pool + (p-32u)*AUDIO_PAGE; }
static uint32_t audio_sl_page(uint32_t t) { return t < 3u ? t*8u : t==TRK_DRUM ? 24u : 95u+(t-3u)*8u; }
static void audio_mark(uint32_t p,uint32_t n) { for(uint32_t i=0;i<n;i++) audio_blocked[p+i]=1; }
static void audio_prepare(void);
#else
static void audio_prepare(void) {}
#endif
