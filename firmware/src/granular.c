/* SPDX-License-Identifier: GPL-3.0-only */
/* Live granular INSERT. Mono 22.05 kHz, independently addressable 8-bit mu-law.
 * Up to 72 KiB of history is paged; the eight-track build borrows unused
 * engine/effect pages and relocates history before those pages are reclaimed. Only core 0 changes page ownership,
 * before handing tracks to core 1. No flash writes, allocation or RNG sharing. */
#ifdef GF_CHECK_HISTORY
#include <assert.h>
#endif
#define GF_PAGE 1024u
#define GF_PAGES 72u
#define GF_GRAINS 4u
#define GF_Q 4096u
#if NPART <= 3
static uint8_t gf_ram[32768];
static uint8_t gf_pool[40960] __attribute__((section(".pool")));
#endif
typedef struct { uint32_t pos, step, phase, inc; } gf_grain_t;
typedef struct {
    gf_grain_t grain[GF_GRAINS];
    uint8_t page[GF_PAGES], np, half, firw, transition, frozen, leaving, sleeping;
    int16_t fir[23];
    uint32_t wp, valid, countdown, rng;
    int32_t mix, last, old, out[4];
    uint32_t calls, ticks, peak_ticks;
} gf_state_t;
static gf_state_t gf[NTRK] __attribute__((section(".lowram")));
static uint32_t gf_mask;
static const uint16_t GF_SIZE[16] = {10,15,20,30,40,60,80,100,130,170,220,280,350,450,600,800};
/* Windowed-sinc anti-alias filter, unity DC; cutoff 9 kHz at 44.1 kHz. */
static const int16_t GF_FIR[23] = {76, 26, -151, -230, 225, 813, 161, -1739, -1892, 2632, 9792, 13342, 9792, 2632, -1892, -1739, 161, 813, 225, -230, -151, 26, 76};

static uint8_t gf_encode(int32_t x)
{
    uint32_t a, sign = x < 0 ? 0x7fu : 0xffu;
    a = (uint32_t)(x < 0 ? -x : x);
    if (a > 32635u) a = 32635u;
    a += 132u;
    uint32_t seg = 24u - (uint32_t)__builtin_clz(a | 255u);
    return (uint8_t)(((seg << 4) | ((a >> (seg + 3u)) & 15u)) ^ sign);
}
static int32_t gf_decode(uint32_t u)
{
    u = (~u) & 255u;
    int32_t t = (int32_t)(((u & 15u) << 3) + 132u) << ((u >> 4) & 7u);
    return u & 128u ? 132 - t : t - 132;
}
/* Symmetric taps: twelve products give exactly the original 23-tap result. */
static int32_t gf_capture(const gf_state_t *s)
{
    uint32_t a = s->firw, b = a ? a - 1u : 22u;
    int32_t sum = 0;
    for (uint32_t k = 0; k < 11u; k++) {
        sum += ((int32_t)s->fir[a] + s->fir[b]) * GF_FIR[k];
        if (++a == 23u) a = 0;
        b = b ? b - 1u : 22u;
    }
    return sum + s->fir[a] * GF_FIR[11];
}
static uint8_t *gf_byte(const gf_state_t *s, uint32_t idx)
{
#if NPART > 3
    return audio_page(s->page[idx>>10])+(idx&1023u);
#else
    uint32_t p = s->page[idx >> 10], off = ((p & 31u) << 10) | (idx & 1023u);
    return p < 32u ? &gf_ram[off] : &gf_pool[((p - 32u) << 10) | (idx & 1023u)];
#endif
}
static uint32_t gf_capacity(uint32_t track) { return (uint32_t)gf[track % NTRK].np * GF_PAGE; }
static uint32_t gf_buffer_ms(uint32_t track) { return gf_capacity(track) * 1000u / (FS / 2u); }
static void gf_reset(void)
{
    for (uint32_t t = 0; t < NTRK; t++) {
        gf[t].valid = gf[t].countdown = 0;
        memset(gf[t].fir, 0, sizeof gf[t].fir); gf[t].firw = gf[t].half = 0;
        gf[t].sleeping = 0;
        memset(gf[t].grain, 0, sizeof gf[t].grain);
        gf[t].old = gf[t].last; gf[t].transition = 128;
    }
}
#if NPART > 3
static __attribute__((noinline)) void gf_layout(void)
{
    static uint8_t was[AUDIO_PAGES] __attribute__((section(".noinit.audio")));
    uint32_t mask=0,count=0,available=0;
    uint8_t map[NTRK][GF_PAGES],used[AUDIO_PAGES];
    for(uint32_t t=0;t<NTRK;t++) if(trk[t].p[P_ITYPE]==IT_GRAIN) mask|=1u<<t,count++;
    if(mask==gf_mask && !memcmp(was,audio_blocked,sizeof was)) return;
    memcpy(used,audio_blocked,sizeof used); memcpy(was,audio_blocked,sizeof was);
    for(uint32_t p=0;p<AUDIO_PAGES;p++) available+=!used[p];
    if(available>GF_PAGES) available=GF_PAGES;
    uint32_t np=count?available/count:0;
    memset(map,255,sizeof map);
    for(uint32_t t=0;t<NTRK;t++) {
        gf_state_t *s=&gf[t];
        if(!(mask&(1u<<t))) continue;
        uint32_t keep=s->np<np?s->np:np;
        for(uint32_t age=0;age<keep;age++) {
            uint32_t page=s->page[((s->wp>>10)+s->np-age)%s->np];
            map[t][(np-age)%np]=(uint8_t)page;
            if(!audio_blocked[page]) used[page]=1;
        }
        uint32_t valid=keep?(keep-1u)*GF_PAGE+(s->wp&1023u):0;
        if(s->valid>valid) s->valid=valid;
        s->wp&=1023u;
    }
    uint32_t free=0;
    for(uint32_t t=0;t<NTRK;t++) {
        gf_state_t *s=&gf[t]; s->old=s->last; s->transition=128;
        memset(s->grain,0,sizeof s->grain); s->countdown=0;
        s->leaving=!(mask&(1u<<t)) && s->np && s->mix;
        if(!(mask&(1u<<t))) { s->np=0; s->valid=0; continue; }
        for(uint32_t p=0;p<np;p++) {
            uint32_t old=map[t][p];
            if(old==255u || audio_blocked[old]) {
                while(free<AUDIO_PAGES && used[free]) free++;
                if(old!=255u) memcpy(audio_page(free),audio_page(old),GF_PAGE);
                map[t][p]=(uint8_t)free; used[free]=1;
            }
        }
        memcpy(s->page,map[t],np); s->np=(uint8_t)np;
        if(!s->rng) s->rng=0x193e2a71u+t*12345u;
    }
    gf_mask=mask;
}
#else
static __attribute__((noinline)) void gf_layout(void)
{
    uint32_t mask = 0, count = 0;
    uint8_t map[NTRK][GF_PAGES], used[GF_PAGES] = {0};
    for (uint32_t t = 0; t < NTRK; t++)
        if (trk[t].p[P_ITYPE] == IT_GRAIN) mask |= 1u << t, count++;
    if (mask == gf_mask) return;
    memset(map, 255, sizeof map);
    uint32_t np = count ? GF_PAGES / count : 0;
    for (uint32_t t = 0; t < NTRK; t++) {
        gf_state_t *s = &gf[t];
        if (!(mask & (1u << t))) continue;
        uint32_t keep = s->np < np ? s->np : np;
        for (uint32_t age = 0; age < keep; age++) {
            uint32_t page = s->page[((s->wp >> 10) + s->np - age) % s->np];
            map[t][(np - age) % np] = (uint8_t)page; used[page] = 1;
        }
        uint32_t valid = keep ? (keep - 1u) * GF_PAGE + (s->wp & 1023u) : 0;
        if (s->valid > valid) s->valid = valid;
        s->wp &= 1023u;
    }
    uint32_t free = 0;
    for (uint32_t t = 0; t < NTRK; t++) {
        gf_state_t *s = &gf[t];
        s->old = s->last; s->transition = 128;
        memset(s->grain, 0, sizeof s->grain); s->countdown = 0;
        s->leaving = !(mask & (1u << t)) && s->np && s->mix;
        if (!(mask & (1u << t))) { s->np = 0; s->valid = 0; continue; }
        for (uint32_t p = 0; p < np; p++) {
            if (map[t][p] == 255u) {
                while (free < GF_PAGES && used[free]) free++;
                map[t][p] = (uint8_t)free; used[free] = 1;
            }
        }
        memcpy(s->page, map[t], np);
        s->np = (uint8_t)np;
        if (!s->rng) s->rng = 0x193e2a71u + t * 12345u;
    }
    gf_mask = mask;
}
#endif
static uint32_t gf_rand(gf_state_t *s) { s->rng = s->rng * 1664525u + 1013904223u; return s->rng; }
static uint32_t gf_birth(gf_state_t *s, const int16_t *p)
{
    uint32_t cap = (uint32_t)s->np * GF_PAGE, valid = s->valid;
    if (valid < 64u) return 0;
    gf_grain_t *g = 0;
    for (uint32_t k = 0; k < GF_GRAINS; k++) if (!s->grain[k].inc) { g = &s->grain[k]; break; }
    if (!g) return 0;                       /* bounded overlap: never steal a sounding grain */
    int32_t pitch = clamp(p[P_IC],0,127) * 25 / 128 - 12;
    uint32_t step = pow2_q16(pitch * 16) >> 5; /* source is at half output rate; Q12 */
    uint32_t frames = GF_SIZE[(uint32_t)clamp(p[P_IA],0,127) >> 3] * FS / 1000u;
    /* Bound relative motion against the writer, not the sum of both speeds.
     * At unity pitch LIVE follows at a fixed lag; FREEZE must hold the full span. */
    int32_t relative = (int32_t)step - (p[P_GFREEZE] ? 0 : (int32_t)GF_Q / 2);
    uint32_t motion = (uint32_t)(relative < 0 ? -relative : relative);
    if (motion) {
        uint32_t room = relative > 0 ? valid : cap;
        uint32_t maxframes = (room - 8u) * GF_Q / motion;
        if (frames > maxframes) frames = maxframes;
    }
    if (frames < 32u) return 0;
    uint32_t travel = (frames * motion + GF_Q - 1u) >> 12;
    uint32_t lo = relative > 0 ? travel + 4u : 4u;
    uint32_t hi = relative < 0 ? cap - travel - 4u : cap - 4u;
    if (hi > valid - 4u) hi = valid - 4u;
    if (lo > hi) return 0;
    int32_t age = (int32_t)((uint32_t)clamp(p[P_GPOS],0,127) * valid / 127u);
    int32_t spread = (int32_t)((uint32_t)clamp(p[P_GSPREAD],0,127) * valid / 127u);
    if (spread) age += (int32_t)(gf_rand(s) % (uint32_t)(spread + 1)) - spread / 2;
    age = clamp(age, (int32_t)lo, (int32_t)hi);
    uint32_t pos = s->wp + cap - (uint32_t)age;
    if (pos >= cap) pos -= cap;
    g->pos = pos * GF_Q; g->step = step; g->phase = 0;
    g->inc = ((256u << 16) + frames - 1u) / frames;
    return frames;
}
static int32_t gf_wet(gf_state_t *s)
{
    int32_t sum = 0;
    uint32_t cap = (uint32_t)s->np * GF_PAGE;
    for (uint32_t k = 0; k < GF_GRAINS; k++) {
        gf_grain_t *g = &s->grain[k];
        if (!g->inc) continue;
        uint32_t at = g->pos >> 12, next = at + 1u;
        if (next == cap) next = 0;
#ifdef GF_CHECK_HISTORY
        uint32_t age = (s->wp + cap - at) % cap;
        assert(age >= 2u && age <= s->valid);
#endif
        int32_t a = gf_decode(*gf_byte(s, at)), b = gf_decode(*gf_byte(s, next));
        a += ((b - a) * (int32_t)(g->pos & (GF_Q - 1u))) >> 12;
        uint32_t w = g->phase >> 16;
        int32_t env = GR_HANN[w] + (((GR_HANN[w + 1u] - GR_HANN[w]) * (int32_t)(g->phase & 65535u)) >> 16);
        sum += mulq15(a, env);
        g->pos += g->step;
        if (g->pos >= cap * GF_Q) g->pos -= cap * GF_Q;
        g->phase += g->inc;
        if (g->phase >= (256u << 16)) g->inc = 0;
    }
    return clamp(sum / 2, -32768, 32767) * 8; /* four overlapping Hann windows, capture headroom restored */
}
/* One instance for the stereo drum track, never four unrelated captures of its buses. */
static void gf_insert(track_t *t, int32_t *l, int32_t *r, int32_t *rev, int32_t *dly, uint32_t n)
{
    gf_state_t *s = &gf[(uint32_t)(t - trk)];
    if (!s->np || !n) return;
    uint32_t t0 = cpu1_ticks(), cap = (uint32_t)s->np * GF_PAGE;
    if (s->frozen != !!t->p[P_GFREEZE]) {
        s->frozen = !!t->p[P_GFREEZE]; s->old = s->last; s->transition = 128;
        memset(s->grain, 0, sizeof s->grain); s->countdown = 0;
    }
    int32_t target = ins_mixq(t->p[P_IMIX]);
    if (!target && !s->mix) {
        /* MIX 0 parks capture after the fade; keep history, including FREEZE. */
        memset(s->grain, 0, sizeof s->grain);
        s->countdown = 0; s->last = 0; s->transition = 0; s->sleeping = 1;
        t->ins_run = 0;
        s->out[0] = l[n - 1u];
        if (r) { s->out[1] = r[n - 1u]; s->out[2] = rev[n - 1u]; s->out[3] = dly[n - 1u]; }
        return;
    }
    if (s->sleeping) {
        /* The source may have changed during bypass: do not filter across the gap. */
        memset(s->fir, 0, sizeof s->fir); s->firw = s->half = 0;
        s->sleeping = 0;
    }
    if (!s->countdown && target) {
        uint32_t frames = gf_birth(s, t->p);
        s->countdown = FS / (uint32_t)clamp(t->p[P_IB], 1, 60);
        /* Spread bounded overlap instead of bunching four long grains together. */
        if (frames / GF_GRAINS > s->countdown) s->countdown = frames / GF_GRAINS;
    }
    for (uint32_t i = 0; i < n; i++) {
        int32_t input = r ? (l[i] / 2 + r[i] / 2) : l[i];
        s->fir[s->firw] = (int16_t)clamp(input / 8, -32768, 32767);
        if (++s->firw == 23) s->firw = 0;
        if ((s->half ^= 1u) == 0u && !t->p[P_GFREEZE]) {
            int32_t filtered = gf_capture(s);
            *gf_byte(s, s->wp) = gf_encode(clamp(filtered >> 15, -32768, 32767));
            if (++s->wp == cap) s->wp = 0;
            if (s->valid < cap) s->valid++;
        }
        if (s->countdown) s->countdown--;
        int32_t wet = target || s->mix ? gf_wet(s) : 0;
        if (s->transition) {
            wet = (s->old * s->transition + wet * (128 - s->transition)) / 128;
            s->transition--;
        }
        s->last = wet;
        s->mix += s->mix < target ? (target - s->mix > 256 ? 256 : target - s->mix)
                               : (target - s->mix < -256 ? -256 : target - s->mix);
        l[i] = ins_mix(l[i], wet, s->mix); s->out[0] = l[i];
        if (r) {
            r[i] = ins_mix(r[i], wet, s->mix);
            rev[i] = ins_mix(rev[i], mulq16(wet, (uint32_t)song.g[G_DRREV] * 516u), s->mix);
            dly[i] = ins_mix(dly[i], mulq16(wet, (uint32_t)song.g[G_DRDLY] * 516u), s->mix);
            s->out[1] = r[i]; s->out[2] = rev[i]; s->out[3] = dly[i];
        }
    }
    t->ins_run = target != 0;
    uint32_t ticks = cpu1_ticks() - t0;
    if (s->calls >= 65536u) s->ticks = s->calls = 0;
    s->ticks += ticks; s->calls++;
    if (ticks > s->peak_ticks) s->peak_ticks = ticks;
}

/* TYPE changes ramp from the last output rather than cutting a frozen cloud. */
static void gf_leave(track_t *t, int32_t *l, int32_t *r, int32_t *rev, int32_t *dly, uint32_t n)
{
    gf_state_t *s = &gf[(uint32_t)(t - trk)];
    if (!s->leaving) return;
    int32_t *b[4] = {l,r,rev,dly};
    for (uint32_t i = 0; i < n && s->transition; i++, s->transition--)
        for (uint32_t k = 0; k < (r ? 4u : 1u); k++)
            b[k][i] = (s->out[k] * s->transition + b[k][i] * (128 - s->transition)) / 128;
    if (!s->transition) s->leaving = 0;
    t->ins_run |= s->leaving;
}
