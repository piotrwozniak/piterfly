/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Independent GM drum parts: the SAMPLE engine's KIT set (General MIDI percussion map,
 * tools/gen_samples.py GM_KIT), played by its step pattern, the keys when track 4
 * is selected, and its own MIDI channel (GLO -> DRUMS, default 10). Its own voices
 * (outside the parts' voice budget). One-shots: note-offs are ignored; a closed or
 * pedal hi-hat chokes the open one. LEVEL / REV / DLY: GLO > DRUMS (G_DRLVL, G_DRREV, G_DRDLY);
 * PAN and MUTE: the drum track's P_PAN / P_MUTE. Rendered from the audio ISR. */
#ifndef NDRUM                    /* (a host may give the drums more voices: Kraken) */
#define NDRUM 6
#endif
/* a host's own treatment of each drum voice before the mix (Kraken: a level, a pan, a filter per sound);
 * on the FM-1, nothing */
#ifndef DRUM_VOICE_FX
#define DRUM_VOICE_FX(k, s) (s)
#define DRUM_VOICE_GL(k, g) (g)
#define DRUM_VOICE_GR(k, g) (g)
#endif
#include "drum_synth.c"       /* synthesised kits (DS_KITS) */
/* P_E0 was unused on the drum track: it holds the kit. 0..4: the GM sample kit and its four
 * treatments (as before: old projects keep their kit), 5..: the synthesised kits, then (2.4) USR1..USR4:
 * a user sample slot as a kit (the web editor's DRUM KIT: a sound per lane, zone lo..hi = the lane's note),
 * and USR3+4: one kit over the slots USR3 and USR4 (a lane's sound in either) */
#define DRUM_SAMPLED 5u
#define DRUM_USR (DRUM_SAMPLED + DS_NKITS)
#define DRUM_PAIR (DRUM_USR + 4u)   /* USR3+4: one kit in two slots (the editor splits it), ~15 s */
#define DRUM_SYN (NPART==6?DRUM_USR:DRUM_PAIR + 1u)                /* SLOOP 2.5: SYN1..SYN4, your synthesised kits (drum_synth.c dsu) */
#define DRUM_KITS (DRUM_SYN + DSU_N)
static const char *const DRUM_KIT_NAMES[] = {"ACOUSTIC", "DEEP", "TIGHT", "BRIGHT", "DUST", DS_KIT_NAME_LIST,
#if NPART != 6
                                             "USR1", "REMOVED", "REMOVED", "REMOVED", "REMOVED",
#endif
                                             "SYN1", "SYN2", "SYN3", "SYN4"};
static const char *const DRUM_KIT_STYLES[] = {"STUDIO", "SOFT", "PUNCHY", "BRIGHT", "DUSTY", DS_KIT_STYLE_LIST,
#if NPART != 6
                                              "YOUR KIT", "YOUR KIT", "YOUR KIT", "YOUR KIT", "BIG KIT",
#endif
                                              "YOUR SYNTH", "YOUR SYNTH", "YOUR SYNTH", "YOUR SYNTH"};
_Static_assert(sizeof(DRUM_KIT_NAMES) / sizeof(DRUM_KIT_NAMES[0]) == DRUM_KITS, "a name per kit");
static uint32_t drum_kit_for(const track_t *t) { return (uint32_t)clamp(t->p[P_E0], 0, DRUM_KITS - 1); }
#define DRUM_DEFAULT_KIT DRUM_SAMPLED   /* power-on: 808 */

typedef struct {
    voice_t v[NDRUM];
    uint32_t age;
    int16_t set;                 /* SMP_SETS index of "PERC" (GM map), -1 = none */
    int32_t tail;                /* declick: the last output of cut voices, decaying */
    int32_t peak;                /* largest |output| since the UI last looked (TRACKS meter) */
    uint8_t kit[NDRUM];
    int32_t filter[NDRUM], env[NDRUM], denv[NDRUM];   /* denv (2.7): the DECAY's fade, Q15 */
    uint16_t dk[NDRUM];          /* (2.7) its factor a sample (DECAY_K), 0: none */
    uint8_t synth[NDRUM];        /* the voice plays a synthesised kit (ds[]) */
    dsv_t ds[NDRUM];
    volatile uint16_t hits;      /* bit per lane hit since the UI last looked (pads, key LEDs) */
    volatile uint8_t kick;       /* a kick was hit (fx.c DUCK) */
    int32_t a0, a1;              /* the drum track's mute / solo attenuation over this block (fx.c), Q15, 0 = heard */
    int32_t glv, ggl, ggr;       /* (2.7) LEVEL and PAN as last mixed: a change glides (~3 ms, fx.c mix_glide) */
    uint8_t gon;
} drum_state_t;
static drum_state_t drum_states[NDRUM_TRACKS];
#define drums drum_states[NDRUM_TRACKS - 1]
static drum_state_t *drum_state(const track_t *t) { return &drum_states[t - trk - NPART]; }



/* (2.7) LEVEL and PAN glide: a knob's detent (or a new sound's trim) moves a quarter of the way each block of CTL
 * samples (0.73 ms), ~3 ms in all, instead of a step a detent (after JIANT FM 0.6.8, github.com/juanjiant-bit/JIANT:
 * "LEVEL and PAN glide (~3 ms) instead of stepping at each knob move"). The last few units: at once */
static int32_t mix_glide(int32_t from, int32_t to)
{
    int32_t d = to - from;
    return d <= 8 && d >= -8 ? to : from + d / 4;
}

static int32_t drum_set(void)
{
    static int16_t set = -2;
    uint32_t i;
    if (set == -2) {
        set = -1;
        for (i = 0; i < SMP_NSETS; i++)
            if (str_eq(SMP_SETS[i].name, "PERC"))
                set = (int16_t)i;
    }
    return set;
}

/* ------------------------------------------------------------ lanes --- */
/* The drum track's 16 sounds, one per white key, F3 (kick) .. G5 (cowbell): kicks, snare and clap,
 * the hi-hats, rim and a second snare, the toms, the cymbals, the percussion. Each plays a GM note
 * (the sampled kits: their samples; the synthesised kits: drum_synth.c DS_MAP). A black key plays
 * the lane of the white key left of it (two fingers on one sound). */
static const uint8_t LANE_NOTE[DRUM_LANES] = {36, 35, 38, 39, 42, 46, 44, 37, 40, 43, 48, 49, 51, 70, 63, 56};
static const char *const LANE_NAME[DRUM_LANES] = {
    "KICK", "KICK 2", "SNARE", "CLAP", "HAT", "OPEN HAT", "PEDAL", "RIM",
    "SNARE 2", "LOW TOM", "HI TOM", "CRASH", "RIDE", "SHAKER", "CONGA", "COWBELL"};
static const char *const LANE_SHORT[DRUM_LANES] = {           /* 5 characters: tiles, dials */
    "kick", "kick2", "snare", "clap", "hat", "open", "pedal", "rim",
    "snr 2", "tom l", "tom h", "crash", "ride", "shake", "conga", "bell"};
/* a GM note (MIDI in, old projects) -> its lane: the nearest sound of the 16 (35..81; below: kick, above: shaker) */
static const uint8_t LANE_OF_GM[81 - 35 + 1] = {
    /* 35 */ 1, 0, 7, 2, 3, 8, 9, 4, 9, 6,
    /* 45 */ 9, 5, 10, 10, 11, 10, 12, 11, 12, 13,
    /* 55 */ 11, 15, 11, 13, 12, 14, 14, 14, 14, 14,
    /* 65 */ 14, 14, 15, 15, 13, 13, 15, 15, 13, 13,
    /* 75 */ 7, 7, 7, 14, 14, 15, 15};
/* SLOOP 2.7, the DRUMS screen's SOUND page: each of the 16 sounds' TUNE (-8 .. +7 semitones) and DECAY (-4 .. +3,
 * 0 as the kit has it), on every kit; kept with the project and each section (project.c: in the drum track's free
 * engine values) */
#define DRUM_TUNE_MIN (-8)
#define DRUM_TUNE_MAX 7
#define DRUM_DEC_MIN (-4)
#define DRUM_DEC_MAX 3
static int8_t drum_tunes[NDRUM_TRACKS][DRUM_LANES], drum_decs[NDRUM_TRACKS][DRUM_LANES];
#define drum_tune drum_tunes[NDRUM_TRACKS - 1]
#define drum_dec drum_decs[NDRUM_TRACKS - 1]
static int8_t *drum_tune_for(const track_t *t) { return drum_tunes[t - trk - NPART]; }
static int8_t *drum_dec_for(const track_t *t) { return drum_decs[t - trk - NPART]; }
static const uint8_t DK_SMP[4] = {47, 60, 74, 87};   /* a sample's DECAY -4 .. -1: -60 dB in 60, 120, 250, 500 ms */
#define DRUM_CLICK(n) ((n) == 76u || (n) == 77u)    /* (the click's wood block: no lane, never tuned) */
static uint32_t lane_of_note(uint32_t note);
static int drum_tweaked_for(const track_t *t)
{
    uint32_t l;
    for (l = 0; l < DRUM_LANES; l++)
        if (drum_tune_for(t)[l] || drum_dec_for(t)[l])
            return 1;
    return 0;
}
static uint32_t lane_of_note(uint32_t note)
{
    return note < 35u ? 0u : note > 81u ? 13u : LANE_OF_GM[note - 35u];
}
/* the lane of key k (0 = F3 .. 26 = G5): white keys in order, a black key the white key left of it */
static uint32_t lane_of_key(uint32_t k)
{
    static const int8_t W[12] = {0, -1, 1, -1, 2, -1, 3, 4, -1, 5, -1, 6};   /* from F */
    int32_t i = W[k % 12u];
    if (i < 0)
        i = W[(k - 1u) % 12u];
    return (uint32_t)((int32_t)(k / 12u) * 7 + i) & 15u;
}
/* the white key of lane l (key index), for the key LEDs */
static uint32_t key_of_lane(uint32_t l)
{
    static const uint8_t K[7] = {0, 2, 4, 6, 7, 9, 11};      /* F G A B C D E */
    return (l / 7u) * 12u + K[l % 7u];
}

/* drum steps: a lane's bit, level, ratchet */
static int dstep_has(const dstep_t *s, uint32_t l) { return (s->on[(l >> 3) & 1u] >> (l & 7u)) & 1u; }
static uint32_t dstep_lvl(const dstep_t *s, uint32_t l) { return (s->lvl[(l >> 2) & 3u] >> ((l & 3u) * 2u)) & 3u; }
static uint32_t dstep_rat(const dstep_t *s, uint32_t l) { return (s->rat[(l >> 2) & 3u] >> ((l & 3u) * 2u)) & 3u; }
static uint32_t dstep_mask(const dstep_t *s) { return (uint32_t)s->on[0] | (uint32_t)s->on[1] << 8; }
static void dstep_set(dstep_t *s, uint32_t l, uint32_t lvl, uint32_t rat)   /* lane on, with its level and ratchet */
{
    uint32_t sh = (l & 3u) * 2u, b = (l >> 2) & 3u;
    s->on[(l >> 3) & 1u] |= (uint8_t)(1u << (l & 7u));
    s->lvl[b] = (uint8_t)((s->lvl[b] & ~(3u << sh)) | (lvl & 3u) << sh);
    s->rat[b] = (uint8_t)((s->rat[b] & ~(3u << sh)) | (rat & 3u) << sh);
}
static void dstep_clr(dstep_t *s, uint32_t l)                                /* lane off */
{
    uint32_t sh = (l & 3u) * 2u, b = (l >> 2) & 3u;
    s->on[(l >> 3) & 1u] &= (uint8_t)~(1u << (l & 7u));
    s->lvl[b] &= (uint8_t)~(3u << sh);
    s->rat[b] &= (uint8_t)~(3u << sh);
}

/* the zone a sampled drum voice plays (drum_on: v->s[4]): SMP_ZONES[i], or a user kit's, DZ_USR + slot * 16 + zone */
#define DZ_USR (1 << 16)                 /* (above every SMP_ZONES index) */
static const smp_zone_t *drum_zone_of(const voice_t *v)
{
    int32_t z = v->s[4];
#if SMP_USER_SLOTS == 0
    return &SMP_ZONES[(uint32_t)z<NELEM(SMP_ZONES)?(uint32_t)z:0u];
#else
    return z >= DZ_USR ? &usr_zone[((uint32_t)(z - DZ_USR) >> 4) % SMP_USER_SLOTS][(uint32_t)z & 15u] : &SMP_ZONES[(uint32_t)z];
#endif
}
/* a user kit (KIT USR1..3): the zone of slot k for the lane note ln (the lane's GM note), -1 = none */
static int32_t drum_usr_zone(uint32_t k, uint32_t ln)
{
    uint32_t j;
    if (k >= SMP_USER_SLOTS) return -1;
    for (j = 0; j < usr_nz[k] && j < 16u; j++)
        if (ln >= usr_zone[k][j].lo && ln <= usr_zone[k][j].hi)
            return DZ_USR + (int32_t)(k * 16u + j);
    return -1;
}

/* the velocity of a level: as played (LV_NORM: vel), ghost, soft, hard */
static uint32_t lvl_vel(uint32_t lvl, uint32_t vel)
{
    static const uint8_t V[4] = {0, 42, 72, 127};
    return lvl & 3u ? V[lvl & 3u] : vel;
}
/* a MIDI velocity -> the level it records as (keys play 100: LV_NORM) */
static uint32_t vel_lvl(uint32_t vel)
{
    return vel < 56u ? LV_GHOST : vel < 88u ? LV_SOFT : vel < 116u ? LV_NORM : LV_HARD;
}

/* the metronome (GLO > GLOBAL > CLICK, and the REC count-in), 2.4.1: a voice of its own, mixed after the drum
 * track (fx.c mix_block), so it is heard with every kit (a user kit has no wood block: it was silent), whatever
 * the drum track's mute, a solo, its filter or slicer. A short sine tick: 1568 Hz on a bar's first beat, 1047 Hz
 * on the others, about 12 ms to fade by 1/e; its level follows GLO > DRUMS > LVL (100: as loud as the old
 * wood block). It costs a few hundred cycles a block while it sounds, nothing after */
static struct { uint32_t ph, inc; int32_t env, gain; } click;   /* env: Q30, 0 = silent */
static uint32_t click_n;                         /* clicks so far, and the last one's accent (the host tests count them) */
static uint8_t click_acc;
static void click_on(int accent)
{
    click.ph = 0;
    click.inc = accent ? (uint32_t)(((uint64_t)1568u << 32) / FS) : (uint32_t)(((uint64_t)1047u << 32) / FS);
    click.gain = accent ? 20000 : 12000;
    click.env = 1 << 30;
    click_acc = (uint8_t)(accent != 0);
    click_n++;
}
static void click_render(int32_t *ml, int32_t *mr, uint32_t n)
{
    uint32_t i;
    int32_t g;
    if (!click.env)
        return;
    g = (int32_t)((int64_t)click.gain * song.g[G_DRLVL] / 100);
    for (i = 0; i < n; i++) {
        int32_t v = (int32_t)(((int64_t)sine_i(click.ph) * (click.env >> 15) >> 15) * g >> 15);
        ml[i] += v;
        mr[i] += v;
        click.ph += click.inc;
        click.env -= click.env >> 9;                 /* about 12 ms to 1/e */
    }
    if (click.env < (1 << 18))
        click.env = 0;                                /* (-72 dB: done, about 100 ms) */
}

/* (2.8.3) the voice for a drum hit: a free voice; all busy, the sound's own voice if it still sounds (it starts over,
 * as on a drum machine: a 16th hi-hat no longer takes every voice with its own tails); else the oldest, a kick last.
 * Up to 2.8.2 the oldest went whatever it was: on a busy beat (16th hats, ride, crash, snare and clap) the kick lost its
 * voice after a step or two and was cut, or not heard at all (a tester's report, "Heads Will Roll") */
static voice_t *drum_pick(track_t *t, uint32_t note)
{
    drum_state_t *d = drum_state(t);
    voice_t *v = 0;
    uint32_t i, best = 0xFFFFFFFFu;
    for (i = 0; i < NDRUM; i++)
        if (!d->v[i].active)
            return &d->v[i];
    for (i = 0; i < NDRUM; i++)
        if (d->v[i].note == note)
            return &d->v[i];
    for (i = 0; i < NDRUM; i++) {                   /* the oldest that is not a kick; all kicks: the oldest */
        uint32_t kick = d->v[i].note == 35u || d->v[i].note == 36u;
        uint32_t key = (kick ? 0x80000000u : 0u) | (d->v[i].age & 0x7FFFFFFFu);
        if (key < best) {
            best = key;
            v = &d->v[i];
        }
    }
    return v ? v : &d->v[0];
}
/* Start a voice from the selected factory or user kit, stealing a voice when needed. */
static void __attribute__((noinline)) drum_synth_on(track_t *t, uint32_t note, uint32_t vel, uint32_t kit, const dsu_kit_t *user, int tweak)
{
    drum_state_t *d = drum_state(t);
    voice_t *v = &d->v[0];
    uint32_t i;
    if (note == 42u || note == 44u)                 /* hi-hat choke */
        for (i = 0; i < NDRUM; i++)
            if (d->v[i].active && d->v[i].note == 46u) {
                d->v[i].active = 0;
                d->tail += d->v[i].s[7];
            }
    v = drum_pick(t, note);
    if (v->active)
        d->tail += v->s[7];
    i = (uint32_t)(v - d->v);
    v->note = (uint8_t)note;
    v->vel = (uint8_t)vel;
    v->active = 1;
    v->s[7] = 0;
    v->age = ++d->age;
    d->synth[i] = 1;
    d->kit[i] = (uint8_t)kit;
    {
        uint32_t ln = lane_of_note(note), lane, crush;
        const dsnd_t *sound;
        int32_t semi;
        lane = ds_lane(note, &semi);
        if (user) {
            sound = &user->s[lane];
            crush = user->crush;
        } else {
            const dkit_t *factory = &DS_KITS[kit - DRUM_SAMPLED];
            sound = ds_factory_sound(factory, lane);
            crush = factory->crush;
        }
        tweak = tweak && !DRUM_CLICK(note);
        ds_on(&d->ds[i], sound, crush, note, semi, vel, tweak ? drum_tune_for(t)[ln] * 16 : 0,
              tweak ? (uint32_t)(drum_dec_for(t)[ln] + 4) : 4u);
    }
}
/* the editor's audition (editor_dsyn.c DSYN_PLAY): queued, played by the audio ISR (drum_audition_poll) */
static volatile uint8_t dsu_aud_k, dsu_aud_lane, dsu_aud_vel;
static void drum_audition_poll(void)
{
    uint32_t vel = dsu_aud_vel;
    if (!vel)
        return;
    dsu_aud_vel = 0;
    {
        const dsu_kit_t *u = dsu_kit(dsu_aud_k);
        drum_synth_on(TDRUM_UI, DS_LANE_NOTE[dsu_aud_lane % DS_LANES], vel, DRUM_SYN + dsu_aud_k % DSU_N, u, 0);   /* (as designed) */
    }
}

static void drum_on_for(track_t *t, uint32_t note, uint32_t vel)
{
    drum_state_t *d = drum_state(t);
    int32_t si = drum_set();
    const smp_set_t *set;
    voice_t *v = &d->v[0];
    uint32_t i, zi = 0xFFFFu, kit = drum_kit_for(t);
    int32_t zid = -1;
    if (note != 76u && note != 77u)                 /* the pads and key LEDs (not the click's wood block) */
        d->hits |= (uint16_t)(1u << lane_of_note(note));
    if (note == 35u || note == 36u)
        d->kick = 1;                             /* (DUCK) */
    if (kit >= DRUM_SYN) {                          /* SYN1..SYN4: your synthesised kit */
        const dsu_kit_t *u = dsu_kit(kit - DRUM_SYN);
        drum_synth_on(t, note, vel, kit, u, 1);
        return;
    }
    if (kit >= DRUM_USR) {                          /* a user kit: the lane's sound, as its note (the hat choke) */
        if (note == 76u || note == 77u)
            return;                                 /* (the click's wood block: not a lane) */
        note = LANE_NOTE[lane_of_note(note)];
        if (kit == DRUM_PAIR) {                     /* USR3+4: the lane's sound in USR3, else in USR4 */
            zid = drum_usr_zone(2, note);
            if (zid < 0)
                zid = drum_usr_zone(3, note);
        } else {
            zid = drum_usr_zone(kit - DRUM_USR, note);
        }
        if (zid < 0)
            return;                                 /* (an empty slot, or no sound on this lane) */
    } else if (kit >= DRUM_SAMPLED) {               /* synthesised kit */
        drum_synth_on(t, note, vel, kit, 0, 1);
        return;
    }
    if (zid < 0) {                                  /* the GM sample kit */
        if (si < 0)
            return;
        set = &SMP_SETS[si];
        for (i = 0; i < set->nz; i++)
            if (note >= SMP_ZONES[set->z0 + i].lo && note <= SMP_ZONES[set->z0 + i].hi)
                zi = set->z0 + i;
        if (zi == 0xFFFFu)
            return;
        zid = (int32_t)zi;
    }
    if (note == 42u || note == 44u)                 /* hi-hat choke */
        for (i = 0; i < NDRUM; i++)
            if (d->v[i].active && d->v[i].note == 46u) {
                d->v[i].active = 0;
                d->tail += d->v[i].s[7];          /* fade what it was playing, not a step */
            }
    v = drum_pick(t, note);                            /* (2.8.3: a free one, its own, else the oldest but a kick) */
    if (v->active)
        d->tail += v->s[7];                      /* stolen voice: fade its last value */
    v->note = (uint8_t)note;
    v->vel = (uint8_t)vel;
    v->active = 1;
    v->s[7] = 0;
    v->age = ++d->age;
    v->s[4] = zid;
    v->ph[0] = v->ph[1] = 0;
    v->s[0] = v->s[1] = v->s[2] = 0;
    {
        const smp_zone_t *z = drum_zone_of(v);
        v->s[3] = sample_next(z, v, 0);
        /* (after 2.8.3) clamped: a user kit zone's root is not checked (a typed root of 300 shifted by 2^20) */
        v->s[5] = (int32_t)((pow2_q16(clamp((int32_t)note * 16 - z->root16, -1536, 2047)) >> 8) * (z->rate >> 8));
    }
    {
        uint32_t vi = (uint32_t)(v - d->v);
        uint32_t ln = lane_of_note(v->note), tw = !DRUM_CLICK(v->note);
        int32_t shift = kit == 1u ? (note <= 36u ? -5 : -2) : kit == 3u ? 2 : kit == 4u ? -1 : 0;
        d->kit[vi] = (uint8_t)kit;
        d->synth[vi] = 0;
        d->filter[vi] = 0;
        d->env[vi] = 32767;
        d->denv[vi] = 32767;                     /* (2.7) DECAY below 0: a fade to -60 dB in 60 .. 500 ms (DK_SMP) */
        d->dk[vi] = tw && drum_dec_for(t)[ln] < 0 ? DECAY_K[DK_SMP[drum_dec_for(t)[ln] + 4]] : 0u;
        shift = shift * 16 + (tw ? drum_tune_for(t)[ln] * 16 : 0);    /* (2.7) TUNE: the sound's semitones */
        if (shift) {
            const smp_zone_t *z = drum_zone_of(v);
            v->s[5] = (int32_t)((pow2_q16(clamp((int32_t)note * 16 + shift - z->root16, -1536, 2047)) >> 8) * (z->rate >> 8));
        }
    }
}

/* adds the drums into the dry mix and the reverb and delay sends; mono != 0: into mono instead, before
 * the pan and the sends (the SLICER, slicer.c slicer_drums, does those after it) */
static inline void drums_mix(track_t *t, int32_t *ml, int32_t *mr, int32_t *rev, int32_t *dly, int32_t *mono, uint32_t n)
{
    drum_state_t *d = drum_state(t);
    int32_t ds_buf[CTL];
    uint32_t k, i;
    int32_t lvl = expr_gain((t == TDRUM ? song.g[G_DRLVL] : t->p[P_LEVEL]) * 200, (uint32_t)(t-trk)), send = send_thr((t == TDRUM ? song.g[G_DRREV] : t->p[P_REV]) * 258, thr_rev),
            dsend = send_thr((t == TDRUM ? song.g[G_DRDLY] : t->p[P_DLY]) * 258, thr_dly), pk = d->peak;   /* (2.6: the THROWs) */
    int32_t pan = t->p[P_PAN], gl = 4096 - (pan > 0 ? pan * 64 : 0), gr = 4096 + (pan < 0 ? pan * 64 : 0);
    if (!d->gon)                                 /* (2.7) LEVEL and PAN glide, block by block (mix_glide) */
        d->glv = lvl, d->ggl = gl, d->ggr = gr, d->gon = 1;
    lvl = d->glv = mix_glide(d->glv, lvl);
    gl = d->ggl = mix_glide(d->ggl, gl);
    gr = d->ggr = mix_glide(d->ggr, gr);
    for (i = 0; i < n && d->tail; i++) {         /* declick tail, ~0.4 ms */
        if (mono) {
            mono[i] += d->tail;
        } else {
            ml[i] += d->tail;
            mr[i] += d->tail;
        }
        d->tail -= d->tail / 16 + (d->tail > 0 ? 1 : d->tail < 0 ? -1 : 0);
    }
    for (k = 0; k < NDRUM; k++) {               /* synthesised voices: render, then as below */
        voice_t *v = &d->v[k];
        uint32_t m = n < CTL ? n : CTL;             /* (the mix runs in blocks of CTL) */
        if (!v->active || !d->synth[k])
            continue;
        if (!ds_render(&d->ds[k], ds_buf, m))
            v->active = 0;
        for (i = 0; i < m; i++) {
            int32_t s = mulq15(ds_buf[i], mulq15(lvl, 32767 - d->a0 - (((d->a1 - d->a0) * (int32_t)i) >> CTL_LOG2)));
            s = DRUM_VOICE_FX(k, s);
            v->s[7] = s;
            if (s > pk || -s > pk)
                pk = s < 0 ? -s : s;
            if (mono) {
                mono[i] += s;
                continue;
            }
            ml[i] += (s * DRUM_VOICE_GL(k, gl)) >> 12;
            mr[i] += (s * DRUM_VOICE_GR(k, gr)) >> 12;
            if (send)
                rev[i] += mulq15(s, send);
            if (dsend)
                dly[i] += mulq15(s, dsend);
        }
        if (!v->active) {
            d->tail += v->s[7];                  /* ended: no step at the end */
            v->s[7] = 0;
        }
    }
    for (k = 0; k < NDRUM; k++) {
        voice_t *v = &d->v[k];
        const smp_zone_t *z;
        if (d->synth[k])
            continue;
        z = drum_zone_of(v);
        uint32_t frac = v->ph[1], stepq = (uint32_t)v->s[5];   /* Q16 source samples per output (drum_on) */
        int32_t g;
        if (!v->active)
            continue;
        g = mulq15(lvl, v->vel * 258);
        g += g * 3 >> 2;                           /* x1.75 (+5 dB): as loud as the synthesised kits */
        for (i = 0; i < n; i++) {
            int32_t s;
            frac += stepq;
            while (frac >= 65536u) {
                frac -= 65536u;
                v->s[2] = v->s[3];
                if (v->ph[0] >= z->n) {
                    v->active = 0;
                    break;
                }
                v->s[3] = sample_next(z, v, 0);
            }
            if (!v->active)
                break;
            s = v->s[2] + (((v->s[3] - v->s[2]) * (int32_t)(frac >> 1)) >> 15);
            s = mulq15(s, mulq15(g, 32767 - d->a0 - (((d->a1 - d->a0) * (int32_t)i) >> CTL_LOG2)));
            if (d->kit[k] == 1u || d->kit[k] == 4u) {
                d->filter[k] += (s - d->filter[k]) >> (d->kit[k] == 1u ? 2 : 1);
                s = d->filter[k];
                if (d->kit[k] == 4u) s = (s >> 8) * 256;
            } else if (d->kit[k] == 2u) {
                s = mulq15(s, d->env[k]);
                d->env[k] -= (d->env[k] >> 11) + 1;
                if (d->env[k] <= 0) v->active = 0;
            }
            if (d->dk[k]) {                      /* (2.7) a shorter DECAY: the sample fades out */
                s = mulq15(s, d->denv[k]);
                d->denv[k] = (int32_t)(((uint32_t)d->denv[k] * d->dk[k]) >> 16);
                if (d->denv[k] < 8) v->active = 0;
            }
            s = DRUM_VOICE_FX(k, s);
            v->s[7] = s;
            if (s > pk || -s > pk)
                pk = s < 0 ? -s : s;
            if (mono) {
                mono[i] += s;
                continue;
            }
            ml[i] += (s * DRUM_VOICE_GL(k, gl)) >> 12;
            mr[i] += (s * DRUM_VOICE_GR(k, gr)) >> 12;
            if (send)
                rev[i] += mulq15(s, send);
            if (dsend)
                dly[i] += mulq15(s, dsend);
        }
        v->ph[1] = frac;
    }
    d->peak = pk;
}
static void drums_render_for(track_t *t, int32_t *ml, int32_t *mr, int32_t *rev, int32_t *dly, uint32_t n) { drums_mix(t, ml, mr, rev, dly, 0, n); }
static void drums_render_mono_for(track_t *t, int32_t *mono, uint32_t n) { drums_mix(t, 0, 0, 0, 0, mono, n); }

/* Legacy one-drum host fixtures. Production routing passes the owning track. */
static uint32_t drum_kit(void) { return drum_kit_for(TDRUM); }
static int drum_tweaked(void) { return drum_tweaked_for(TDRUM); }
static void drum_on(uint32_t note, uint32_t vel) { drum_on_for(TDRUM, note, vel); }

static void drums_render(int32_t *l,int32_t *r,int32_t *v,int32_t *d,uint32_t n) { drums_render_for(TDRUM,l,r,v,d,n); }
