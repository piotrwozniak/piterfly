/* SPDX-License-Identifier: GPL-3.0-only */
/* FUN9: seven synth parts and drums. Old FUN1..FUN7 files are read-only inputs. */
#include "project_legacy.c"
#include "project_codec.c"
#define PROJ_MAGIC 0x46554E39u
#define PROJ_CACHE_SLOTS 4u
typedef packed_track_t proj_trk_t;
typedef struct {
    uint32_t magic, size;
    int16_t g[PROJ_NG];
    uint8_t sel, drdly, rsv[2];
    proj_trk_t t[NTRK];
    uint8_t drum_tail[NSTEP * 2u];
    int16_t drum_tw[7];
    uint8_t pad[2];
    uint32_t sum;
} project_t;
_Static_assert(sizeof(project_t) <= 7936u, "project fits an 8 KiB journal record");
#define pt_get pc_param_get
#define pt_set pc_param_put
static int32_t pt_micro(const proj_trk_t *t, uint32_t i) { int32_t m=t->mf[i]&63; return m>=32?m-64:m; }
static uint32_t pt_fill(const proj_trk_t *t, uint32_t i) { return t->mf[i]>>6; }
static void pt_set_mf(proj_trk_t *t,uint32_t i,int32_t m,uint32_t f) { t->mf[i]=(uint8_t)(((uint32_t)m&63u)|(f&3u)<<6); }
static uint32_t proj_sum(const project_t *p) { return proj_hash(p,sizeof *p-4u); }
static int proj_ok(const project_t *p) { return p && p->magic==PROJ_MAGIC && p->size==sizeof *p && p->sum==proj_sum(p); }
static void pt_steps_put(project_t *p,uint32_t track,const void *steps)
{
    if(track==TRK_DRUM) {
        memcpy(p->t[track].step,steps,NSTEP*8u);
        memcpy(p->drum_tail,(const uint8_t *)steps+NSTEP*8u,NSTEP*2u);
    } else for(uint32_t k=0;k<NSTEP;k++) pc_step_put(p->t[track].step[k],(const step_t *)steps+k);
}
static void pt_steps_get(void *steps,const project_t *p,uint32_t track)
{
    if(track==TRK_DRUM) {
        memcpy(steps,p->t[track].step,NSTEP*8u);
        memcpy((uint8_t *)steps+NSTEP*8u,p->drum_tail,NSTEP*2u);
    } else for(uint32_t k=0;k<NSTEP;k++) pc_step_get((step_t *)steps+k,p->t[track].step[k]);
}
/* New parts in a four-track file start with the same sound as a fresh FM-1. */
UI_SMALL static void proj_default_sound(proj_trk_t *d,uint32_t engine)
{
    int16_t p[P_COUNT];
    const engine_t *e=ENGINES[engine]; const preset_t *pr=&e->presets[0];
    static const uint8_t fx[4]={0,24,28,36};
    for(uint32_t k=0;k<P_COUNT;k++) p[k]=k<P_E0?TP[k].def:e->edit[k-P_E0].def;
    for(uint32_t k=0;k<8u;k++) p[P_E0+k]=pr->e[k];
    for(uint32_t k=0;k<4u;k++) {
        p[P_ATK+k]=pr->env[k];
        p[P_DIST+k]=pr->fx[k]?pr->fx[k]-1:fx[k];
        p[P_AMODE+k]=pr->arp[k]?pr->arp[k]-1:TP[P_AMODE+k].def;
    }
    p[P_ED_FLT]=pr->fenv; p[P_ED_FX]=preset_trim(engine,0); p[P_VOICE]=pr->mono?V_LEGATO:V_POLY;
    preset_extras(p,pr);
    for(uint32_t k=0;k<P_COUNT;k++) pt_set(d,k,p[k]);
    d->engine=(uint8_t)engine; d->preset=0;
}
/* Legacy import runs at boot or from the UI; favor size over inlining. */
UI_SMALL static int proj_import(project_t *q,const void *b,int n)
{
    if(n==sizeof *q && proj_ok(b)) { if(q!=b) memcpy(q,b,sizeof *q); return 1; }
    project_v7_t old;
    if(!proj_legacy_import(&old,b,n)) return 0;
    memset(q,0,sizeof *q); q->magic=PROJ_MAGIC; q->size=sizeof *q;
    memcpy(q->g,old.g,sizeof q->g); q->sel=old.sel<3u?old.sel:old.sel==3u?TRK_DRUM:0u;
    q->drdly=old.drdly; memcpy(q->rsv,old.rsv,sizeof q->rsv);
    for(uint32_t t=0;t<NTRK;t++) {
        proj_trk_t *d=&q->t[t];
        uint32_t oi=t==TRK_DRUM?3u:t;
        const proj_trk_v7_t *s=oi<4u?&old.t[oi]:0;
        if(t>=3u && t!=TRK_DRUM) s=0;
        uint32_t e=s?s->engine:trk_def_engine(t);
        for(uint32_t k=0;k<P_COUNT;k++) pt_set(d,k,s?p7_get(s,k):k>=P_E0?ENGINES[e%NENGINES]->edit[k-P_E0].def:TP[k].def);
        d->engine=(uint8_t)e; d->preset=s?s->preset:0xFFu;
        if(!s) proj_default_sound(d,e);
        if(s) { pt_steps_put(q,t,s->step); memcpy(d->mf,s->mf,sizeof d->mf); }
        else { step_t rest={.time=ST_REST}; for(uint32_t k=0;k<NSTEP;k++) pc_step_put(d->step[k],&rest); }
        for(uint32_t k=0;k<NLOCK;k++) { plock_t l={LOCK_FREE,0,0}; if(s) { l.step=s->lock[k].step; l.param=s->lock[k].param; l.val=(int16_t)clamp(s->lock[k].val,-64,127); } pc_lock_put(d->lock[k],&l); }
    }
    memcpy(q->drum_tw,old.t[3].e+1,sizeof q->drum_tw);
    q->sum=proj_sum(q); return 1;
}
project_t proj_slot[PROJ_CACHE_SLOTS] __attribute__((section(".pool_noinit")));
#ifndef PROJ_HOST
/* Four editable live snapshots; the other patterns play directly from flash. */
static struct { uint32_t magic; uint8_t id[4]; } proj_cache __attribute__((section(".noinit")));
static uint8_t sec_dirty, song_dirty;
UI_SMALL static void project_cache_init(void)
{
    uint32_t i, seen = 0;
    int ok = proj_cache.magic == 0x50434331u;
    for (i = 0; ok && i < 4u; i++) {
        if (proj_cache.id[i] >= ARR_SCENES || (seen & (1u << proj_cache.id[i]))) ok = 0;
        else seen |= 1u << proj_cache.id[i];
    }
    if (!ok) {
        proj_cache.magic = 0x50434331u;
        for (i = 0; i < 4u; i++) proj_cache.id[i] = (uint8_t)i;
    }
}
#endif
static const project_t *project_at(uint32_t slot)
{
#ifndef PROJ_HOST
    uint32_t i;
    project_cache_init();
    for (i = 0; i < 4u; i++)
        if (proj_cache.id[i] == slot && proj_ok(&proj_slot[i])) return &proj_slot[i];
#if FELUCCA_FLASH && defined(ST_XIP_PTR)
    if (flash_ok && slot < ARR_SCENES && pb_length[slot] == sizeof(project_t)) return pb_xip(slot);
#endif
    return 0;
#else
    return slot < 4u ? &proj_slot[slot] : 0;
#endif
}
#ifndef PROJ_HOST
UI_SMALL static project_t *project_edit(uint32_t slot)
{
    uint32_t i;
    if (slot >= ARR_SCENES) return 0;
    project_cache_init();
    for (i = 0; i < 4u; i++)
        if (proj_cache.id[i] == slot) return &proj_slot[i];
    for (i = 0; i < 4u; i++)
        if (!(sec_dirty & (1u << i))) {
            proj_slot[i].magic = 0;
            proj_cache.id[i] = (uint8_t)slot;
            return &proj_slot[i];
        }
    return 0; /* All live snapshots await flash: stop playback before reusing one. */
}
#endif

/* ---- the working project <-> a project_t */
/* 2.6: GLO > TEXTURE in a project (a song section has its own): in the slots of two globals a project no longer
 * applies (G_DRCH: a setting of the FM-1 now; G_INFO: the CPU readout) and the spare bytes. rsv[0]: 0x80 (there) | SRC
 * | WHEN << 4 | DUCK << 5, rsv[1]: RATE, g[G_DRCH]: LEVEL | (TONE + 64) << 7, g[G_INFO]: MOVE. Without the 0x80 (a
 * project before it, converted): the defaults, OFF */
UI_SMALL static void proj_tx_put(project_t *p)
{
    p->rsv[0] = (uint8_t)(0x80u | ((uint32_t)song.g[G_TXSRC] & 15u) | ((uint32_t)song.g[G_TXWHEN] & 1u) << 4 |
                          ((uint32_t)song.g[G_TXDUCK] & 1u) << 5 | 0x40u);   /* (0x40: saved by 2.7 or later) */
    p->rsv[1] = (uint8_t)song.g[G_TXRATE];
    p->g[G_DRCH] = (int16_t)((song.g[G_TXLVL] & 127) | ((song.g[G_TXTONE] + 64) & 127) << 7);
    p->g[G_INFO] = song.g[G_TXMOVE];
}
static void proj_tx_get(const project_t *p)
{
    static const uint8_t ID[7] = {G_TXSRC, G_TXLVL, G_TXTONE, G_TXMOVE, G_TXRATE, G_TXWHEN, G_TXDUCK};
    int32_t v[7];
    uint32_t i;
    if (!(p->rsv[0] & 0x80u)) {
        for (i = 0; i < 7u; i++)
            v[i] = GP[ID[i]].def;
    } else {
        v[0] = p->rsv[0] & 15u, v[5] = (p->rsv[0] >> 4) & 1u, v[6] = (p->rsv[0] >> 5) & 1u, v[4] = p->rsv[1];
        v[1] = p->g[G_DRCH] & 127, v[2] = ((p->g[G_DRCH] >> 7) & 127) - 64, v[3] = p->g[G_INFO];
    }
    for (i = 0; i < 7u; i++)
        song.g[ID[i]] = (int16_t)clamp(v[i], GP[ID[i]].min, GP[ID[i]].max);
}

/* (2.7) the DRUMS screen's SOUND page in a project: each sound's TUNE (4 bits) and DECAY (3 bits), two's complement
 * (0: as the kit has it), 16 x 7 bits in the drum track's engine values E1..E7 (the drum track has no engine: they
 * were free), and a check of them in g[G_SAVE] (never loaded: SAVE is a GO button). An older SLOOP keeps neither
 * (it limits E1..E7 to an engine's ranges and saves G_SAVE as 0): its project opens with every sound as the kit has it */
#define DRUM_TW_MARK 0x5400
UI_SMALL static uint32_t drum_tw_sum(const int16_t *e)
{
    uint32_t i, s = 0x5Au;
    for (i = 0; i < 7u; i++)
        s += ((uint32_t)(uint16_t)e[i] & 0xFFu) * (i * 2u + 3u) + ((uint32_t)(uint16_t)e[i] >> 8) * (i * 2u + 4u);
    return s & 0xFFu;
}
UI_SMALL static void drum_tw_put(project_t *p)
{
    uint16_t w[7] = {0};
    uint32_t l, b;
    for (l = 0; l < DRUM_LANES; l++) {
        uint32_t v = ((uint32_t)drum_tune[l] & 15u) | ((uint32_t)drum_dec[l] & 7u) << 4;
        for (b = 0; b < 7u; b++)
            if ((v >> b) & 1u)
                w[(l * 7u + b) / 16u] |= (uint16_t)(1u << ((l * 7u + b) % 16u));
    }
    for (l = 0; l < 7u; l++)
        p->drum_tw[l] = (int16_t)w[l];
    p->g[G_SAVE] = (int16_t)(DRUM_TW_MARK | drum_tw_sum(p->drum_tw));
}
static void drum_tw_get(const project_t *p)
{
    const int16_t *e = p->drum_tw;
    uint32_t l, b, ok = (uint16_t)p->g[G_SAVE] == (DRUM_TW_MARK | drum_tw_sum(e));
    for (l = 0; l < DRUM_LANES; l++) {
        uint32_t v = 0;
        for (b = 0; ok && b < 7u; b++)
            if (((uint16_t)e[(l * 7u + b) / 16u] >> ((l * 7u + b) % 16u)) & 1u)
                v |= 1u << b;
        drum_tune[l] = (int8_t)((int32_t)((v & 15u) ^ 8u) - 8);
        drum_dec[l] = (int8_t)((int32_t)(((v >> 4) & 7u) ^ 4u) - 4);
    }
}

UI_SMALL static void proj_capture(project_t *p)        /* what is playing now, as a project */
{
    uint32_t i;
    memset(p, 0, sizeof *p);
    p->magic = PROJ_MAGIC;
    p->size = sizeof *p;
    for (i = 0; i < PROJ_NG; i++)
        p->g[i] = song.g[i];
    p->sel = song.sel;
    p->drdly = (uint8_t)song.g[G_DRDLY];
    proj_tx_put(p);
    /* FUN9's spare bytes keep old readers compatible; zero means legacy all-on. */
    for(i=0;i<NPART;i++) p->pad[0]|=(uint8_t)((!song.g[G_DUCK1+i])<<i);
    p->pad[1]=p->pad[0]?0xD1u:0u;
    for (i = 0; i < NTRK; i++) {
        uint32_t k;
        for (k = 0; k < P_COUNT; k++)
            pt_set(&p->t[i], k, trk[i].p[k]);
        p->t[i].engine = trk[i].eng_req;
        p->t[i].preset = trk[i].preset;
        pt_steps_put(p,i,trk[i].step);
        for (k = 0; k < NSTEP; k++)
            pt_set_mf(&p->t[i], k, trk[i].micro[k], (uint32_t)trk[i].fill[k / 4u] >> (2u * (k % 4u)));
        for(k=0;k<NLOCK;k++) pc_lock_put(p->t[i].lock[k],&trk[i].lock[k]);
    }
    drum_tw_put(p);                                     /* (2.7) the drum sounds' TUNE and DECAY */
    p->sum = proj_sum(p);
}

/* a project's tracks (and its globals, all: a load; or only the drum level / reverb / delay: a song
 * section) into the working one, every value back inside its range. The audio ISR must not run
 * meanwhile (the song sections: called from it; a load: IRQ off) */
static void proj_apply(const project_t *p, int all)
{
    gf_reset();
    uint32_t i, k;
    for (i = 0; i < PROJ_NG; i++)
        if (all ? i != G_SLOT && i != G_LOAD && i != G_SAVE && i != G_SYNC && i != G_MIDI && i != G_ROUTE && i != G_DRCH &&
                      i != G_INFO
                : i == G_DRLVL || i == G_DRREV)                 /* (2.6: the drum channel a setting of the FM-1, GLO > MIDI;
                                                                 * its slot and G_INFO's hold the TEXTURE) */
            song.g[i] = (int16_t)clamp(p->g[i], GP[i].min, GP[i].max);
    song.g[G_DRDLY] = (int16_t)clamp(p->drdly, GP[G_DRDLY].min, GP[G_DRDLY].max);   /* (a load and a section, as REV) */
    proj_tx_get(p);                                     /* GLO > TEXTURE (2.6): a load and a section */
    for(i=0;i<NPART;i++) song.g[G_DUCK1+i]=!(p->pad[1]==0xD1u && ((p->pad[0]>>i)&1u));
    drum_tw_get(p);                                     /* (2.7) the drum sounds' TUNE and DECAY: a load and a section */
    for (k = 0; k < NTRK; k++) {
        track_t *t = &trk[k];
        const proj_trk_t *s = &p->t[k];
        uint32_t e = k < NPART ? s->engine % NENGINES : 0u;
        t->eng_req = (uint8_t)e;
        t->user = 0;                                    /* (no user preset slot is saved) */
        t->lk_n = 0;                                    /* (the locks in force: the values come from the project) */
        for (i = 0; i < P_COUNT; i++) {                 /* every value back inside its range */
            const param_desc_t *d = k == TRK_DRUM && i == P_E0 ? &DRUM_KIT_DESC :   /* the drum kit */
                                    i >= P_E0 && i <= P_E7 ? &ENGINES[e]->edit[i - P_E0]
                                    : i >= P_IA && i <= P_IC ? ins_desc(t->p[P_ITYPE],i-P_IA) : &TP[i];
            t->p[i] = (int16_t)clamp(pt_get(s, i), d->min, d->max);
        }
        if (!(p->rsv[0] & 0x40u) && !t->p[P_AMODE])
            t->p[P_AHOLD] = 0;                          /* (2.7) saved before 2.7: HOLD with the arp OFF did nothing then; it
                                                         * is KBD HOLD now, so it starts OFF (no note held by surprise) */
        t->preset = (uint8_t)(ENGINES[e]->npresets ? (s->preset == 0xFFu ? 0u : s->preset) % ENGINES[e]->npresets : 0u);
        pt_steps_get(t->step,p,k);
        memset(t->fill, 0, sizeof t->fill);
        for (i = 0; i < NSTEP; i++) {                   /* the nudges and conditions (3 means normal: 0) */
            uint32_t c = pt_fill(s, i);
            t->micro[i] = (int8_t)clamp(pt_micro(s, i), MICRO_MIN, MICRO_MAX);
            if (c != 3u)
                t->fill[i / 4u] |= (uint8_t)(c << (2u * (i % 4u)));
        }
        for (i = 0; i < NLOCK; i++) {
            plock_t decoded; pc_lock_get(&decoded,s->lock[i]);
            const plock_t *l = &decoded;
            uint32_t id = l->param;
            if (l->step < NSTEP && id < P_COUNT && p_lockable(id)) {
                const param_desc_t *d = k == TRK_DRUM && id == P_E0 ? &DRUM_KIT_DESC :
                                        id >= P_E0 && id <= P_E7 ? &ENGINES[e]->edit[id - P_E0] : &TP[id];
                t->lock[i].step = l->step;
                t->lock[i].param = (uint8_t)id;
                t->lock[i].val = (int16_t)clamp(l->val, d->min, d->max);
            } else {
                t->lock[i].step = LOCK_FREE;            /* no such step or parameter: the slot is free */
                t->lock[i].param = 0;
                t->lock[i].val = 0;
            }
        }
        if (k != TRK_DRUM)
            for (i = 0; i < NSTEP; i++) {
                step_t *st = &t->step[i];
                uint32_t j;
                if (st->n > 4u)
                    st->n = 4;
                if (st->time > ST_REST)
                    st->time = ST_REST;
                st->vel &= 127u;                    /* (after 2.8.3: past 127 from a file, up to twice as loud) */
                for (j = 0; j < 4u; j++)
                    st->note[j] &= 127u;
            }
        fm6_track_loaded(t);                            /* FM6: the project keeps PTCH, not the patch: its slot's */
    }
    audio_prepare(); gf_layout();
}

#ifndef PROJ_HOST
#if FELUCCA_ARRANGER
#include "arranger_scene.c"
#endif

#if FELUCCA_FLASH
/* slot from flash into RAM (format 5, or an old one converted) */
static union {
    project_t v9;
    project_v7_t v7;
    project_v6_t v6;
    project_v5_t v5;
    project_v4_t v4;
    project_v3_t v3;
    project_v2_t v2;
    project_v1_t v1;
} proj_tmp;
UI_SMALL static void *up_stage(void) { project_stage_busy=0; return &proj_tmp; }   /* upreset.c: a user preset bank staged for a write (sloop-lab) */
UI_SMALL static void proj_fetch(uint32_t slot)
{
    project_stage_busy=0;
    project_t *q = project_edit(slot);
    int n;
    if (!q) return;
    n = pb_load(slot, &proj_tmp, sizeof proj_tmp);
    if (!proj_import(q, &proj_tmp, n))
        q->magic = 0;
}
#include "fm6_bank.c"                           /* the FM6 patch bank (eng_fm6.c PTCH B1..B27): staged in proj_tmp */
#endif

static const char *const SEC_NAME[24] = {"A1", "A2", "A3", "A4", "B1", "B2", "B3", "B4", "C1", "C2", "C3", "C4", "D1", "D2", "D3", "D4", "E1", "E2", "E3", "E4", "F1", "F2", "F3", "F4"};   /* (2.7: the slots named as the song's sections) */
UI_SMALL static void project_save(uint32_t slot)
{
    project_t *p;
#if FELUCCA_ARRANGER
    if (song.playing || transport_req) { ui_message("STOP BEFORE SAVE"); return; }
#endif
    p = project_edit(slot);
    if (!p) { ui_message("STOP TO SAVE MORE"); return; }
    proj_capture(p);
    sec_dirty |= (uint8_t)(1u << (p - proj_slot));
#if FELUCCA_FLASH
    if (flash_ok) {
        if (pb_save(slot, p, sizeof *p))
            ui_message("SAVE ERROR");
        else {
            sec_dirty &= (uint8_t)~(1u << (p - proj_slot));
            ui_say("SAVED PATTERN ", SEC_NAME[slot]);
        }
        return;
    }
#endif
    ui_message("SAVED (RAM)");
}

/* SAVE > PROJECT > ERASE (SLOOP 2.6, after a YouTube comment): the slot (section A..D) empty again. In flash as the
 * editor's restore leaves an empty slot (project_restore): an object of no bytes, which proj_fetch (older SLOOP too)
 * reads as no project. The song's steps that played that section are taken out (the default song if none is left),
 * so song mode still plays. Stopped only: a flash erase stops the audio ~50 ms */
UI_SMALL static void project_erase(uint32_t slot)
{
    if (slot >= ARR_SCENES) return;
#if FELUCCA_ARRANGER
    if (song.playing || transport_req) { ui_message("STOP BEFORE ERASE"); return; }
#endif
    if (!proj_ok(project_at(slot))) {
        ui_say("EMPTY SECTION ", SEC_NAME[slot]);
        return;
    }
#if FELUCCA_FLASH
    if (flash_ok && pb_save(slot, 0, 0)) {
        ui_message("ERASE ERROR");
        return;
    }
#endif
    fm1_irq_off();
    { uint32_t i; for (i = 0; i < 4u; i++) if (proj_cache.id[i] == slot) {
        proj_slot[i].magic = 0; sec_dirty &= (uint8_t)~(1u << i);
    } }
    if (live_sec == (int8_t)slot)
        live_sec = -1;
#if FELUCCA_ARRANGER
    if (arr_drop_scene(&arrangement, slot))
        song_dirty = 1;                                     /* (saved with the settings, when quiet) */
#endif
    fm1_irq_on();
    ui_say("ERASED SECTION ", SEC_NAME[slot]);
}

/* (2.8.4) a part whose sound is on an engine this firmware does not have (a project Clipper wrote: HYBRID,
 * FMX, VECTOR...; left in the flash when SLOOP is installed after Clipper): proj_apply wrapped the engine number round
 * onto another engine with the other one's values (Clipper's HYBRID played as 808 BOOM without its TRN -24: two
 * octaves high, until another sound was chosen). The part's default sound instead; its pattern and mix stay */
UI_SMALL static void proj_foreign_parts(const project_t *p)
{
    uint32_t k;
    for (k = 0; k < NPART; k++)
        if (p->t[k].preset != 0xFFu && p->t[k].engine >= NENGINES) {
            set_engine_of(&trk[k], TRK_DEF[k][0]);
            apply_preset_to(&trk[k], TRK_DEF[k][1]);
        }
}

/* a project into the working one: the transport stops, everything sounding is released */
UI_SMALL static void project_apply(const project_t *p)
{
    uint32_t k;
    transport_req = 2;
    panic_req = (1u << NTRK) - 1u;
    drum_mute = 0;                                      /* (the live mutes: a performance's, not the project's) */
    for (k = 0; k < NTRK; k++)
        ex_off[k] = 0;                                  /* (2.7.1) MIDI EXPRESSION: full again, as after CC 121 */
    fm1_irq_off();                                      /* the audio ISR must not see half a project */
    proj_apply(p, 1);
    song.sel = (uint8_t)(p->sel < NTRK ? p->sel : 0u);
    fm1_irq_on();
    for (k = 0; k < NPART; k++)                         /* a format 1 project: the default sounds of tracks 2, 3 */
        if (p->t[k].preset == 0xFFu) {
            apply_preset_to(&trk[k], TRK_DEF[k][1]);
            steps_clear(&trk[k]);
        }
    proj_foreign_parts(p);
    sync_reload = 1;
    ui.force = 1;
}

UI_SMALL static void project_load(uint32_t slot)
{
    const project_t *p;
    if (slot >= ARR_SCENES) return;
#if FELUCCA_ARRANGER
    if (song.playing || transport_req) { ui_message("STOP BEFORE LOAD"); return; }
#endif
    p = project_at(slot);
    if (!proj_ok(p)) { ui_say("EMPTY PATTERN ", SEC_NAME[slot]); return; }
    project_apply(p);
    ui_say("LOADED PATTERN ", SEC_NAME[slot]);
}

/* ---- the working project, kept in flash by itself: saved when it changed, the transport is stopped,
 * nothing sounds and the panel was not touched for AUTOSAVE_IDLE (a flash erase stops the audio for
 * ~50 ms: never while something plays); loaded at power-on (autosave_resume) */
#define AUTOSAVE_IDLE 2500u                    /* ms without input */
#define AUTOSAVE_GAP 20000u                    /* ms between two saves at least */
#define autosave_buf (proj_tmp.v9)
static uint32_t autosave_hash, autosave_ms, autosave_checked;

UI_SMALL static int audio_quiet(void)
{
    uint32_t p, i;
    for (p = 0; p < NPART; p++)
        for (i = 0; i < NVOICE; i++)
            if (voice_at(&trk[p],i)->active)
                return 0;
    for (i = 0; i < NDRUM; i++)
        if (drums.v[i].active)
            return 0;
    return 1;
}

UI_SMALL static void autosave_tick(void)                /* main loop */
{
#if FELUCCA_FLASH
    uint32_t h, now = fm1_ms;
    if (project_stage_locked(fm1_ms) || !flash_ok || song.playing || transport_req || rec_wait || ft_on || ui.menu ||
        now - ui_input_ms < AUTOSAVE_IDLE || now - autosave_ms < AUTOSAVE_GAP || now - autosave_checked < 1000u)
        return;
    autosave_checked = now;
    proj_capture(&autosave_buf);
    h = autosave_buf.sum;
    if (h == autosave_hash || !audio_quiet())
        return;
    if (pb_autosave_save(&autosave_buf, sizeof autosave_buf) == 0)
        autosave_hash = h;
    autosave_ms = fm1_ms;
#endif
}

static uint8_t drch_seed;                           /* a settings object as 2.5 wrote it: the drum channel was the
                                                     * project's then (autosave_resume takes it from a 2.5 autosave) */
UI_SMALL static void autosave_resume(void)              /* power-on: the project as it was left (felucca_init) */
{
#if FELUCCA_FLASH
    project_t *q = &autosave_buf;
    int n;
    if (!flash_ok)
        return;
    n = pb_load(pb_autosave_latest(), &proj_tmp, sizeof proj_tmp);
    if(n < 0) n = st_load(OBJ_AUTOSAVE, &proj_tmp, sizeof proj_tmp);
    if (!proj_import(q, &proj_tmp, n))
        return;
    autosave_hash = q->sum;
    if (drch_seed && !(q->rsv[0] & 0x80u) && q->g[G_DRCH] >= 0 && q->g[G_DRCH] <= 16 && q->g[G_DRCH] != midi_set[3]) {
        midi_set[3] = (uint8_t)q->g[G_DRCH];          /* (2.6) updated from 2.5: GLO > DRUMS > CH as it was there (a
                                                     * project's then, a setting of the FM-1 now), kept in the settings */
        settings_later = 1;
    }
    drch_seed = 0;
    proj_apply(q, 1);
    song.sel = (uint8_t)(q->sel < NTRK ? q->sel : 0u);
    proj_foreign_parts(q);                         /* (2.8.4) Clipper's working project: its own sounds, default */
    for (n = 0; n < NPART; n++)
        trk[n].engine = trk[n].eng_req;        /* (nothing sounds yet: no fade) */
#endif
}

/* settings + learned panel table: one flash object. The flash copy wins at
 * boot (the .noinit copies are garbage after a power-off). */
typedef struct {
    uint32_t magic, palette, lowcut, zoom;
    panel_t panel;
#if FELUCCA_ARRANGER
    arr_config_t arrangement;
#endif
    uint32_t lights;                               /* SLOOP 2.3: the backlight (panel.c lights_word); appended,
                                                    * so 2.2 still reads its part (st_load cuts at its size) */
} persist_t;
_Static_assert(sizeof(side_rec_t) == 80u && sizeof(side_rec_t) <= ST_SIDE_MAX, "the side record: theme, favourites");
static side_rec_t side_out;
#define PERSIST_SIZE_V22 __builtin_offsetof(persist_t, lights)   /* the settings as 2.2 wrote them (no lights) */
_Static_assert(sizeof(persist_t) == PERSIST_SIZE_V22 + 4u, "lights: the last word, no padding before it");
#if FELUCCA_ARRANGER
_Static_assert(__builtin_offsetof(persist_t, arrangement) == 48u && sizeof(arr_config_t) == 260u,
               "the web editor song order starts at byte 48");
#endif
#if FELUCCA_ARRANGER
#define PERSIST_MAGIC 0x50455234u                  /* "PER4": 128-position song order */
#else
#define PERSIST_MAGIC 0x50455232u
#endif
#if FELUCCA_FLASH
static persist_t persist_saved;
static void dsu_boot(void);
#endif

/* PER3 kept 16 entries and lights at byte 84; preserve panel, order and lights. */
UI_SMALL static int persist_import(persist_t *p, const void *raw, uint32_t n)
{
    uint32_t magic;
    if (n < 4u) return 0;
    memcpy(&magic, raw, 4);
#if FELUCCA_ARRANGER
    if (magic == 0x50455233u && n >= 84u) {
        persist_t old;
        memset(&old, 0, sizeof old);
        arr_defaults(&old.arrangement);
        memcpy(&old, raw, 84u);
        if (old.arrangement.count > 16u) return 0;
        if (n >= 88u) memcpy(&old.lights, (const uint8_t *)raw + 84u, 4u);
        old.magic = PERSIST_MAGIC;
        *p = old;
        return 1;
    }
#endif
    if (magic != PERSIST_MAGIC || (n != sizeof *p && n != PERSIST_SIZE_V22)) return 0;
    if ((const void *)p != raw) memcpy(p, raw, n);
    if (n == PERSIST_SIZE_V22) p->lights = 0;
    return 1;
}

UI_SMALL static void persist_boot(void)                    /* before settings_init / panel_init */
{
#if FELUCCA_ARRANGER
    arr_defaults(&arrangement);
#endif
#if FELUCCA_FLASH
    persist_t p;
    uint32_t f = irq_save();
    flash_ok = FL_JEDEC_OK(FL_FAR(fl_jedec_ram)());      /* a known 1 MiB part (fm1_flash_id.h), else stay RAM-only */
    irq_restore(f);
    if (!flash_ok)
        return;
    fl_plain_window_init();                        /* flash above 0x93000 reads as plaintext through XIP
                                                    * (user sample sets are played from there) */
    {
        uint32_t k;
        for (k = 0; k < SMP_USER_SLOTS; k++)
            smp_user_scan(k);
    }
    {
        int n = st_load(OBJ_SETTINGS, &p, sizeof p);
        if ((n >= 0 && persist_import(&p, &p, (uint32_t)n))
#if FELUCCA_ARRANGER
            || (n == (int)(16u + sizeof(panel_t)) && p.magic == 0x50455232u)
#endif
            ) {
            settings.magic = SETTINGS_MAGIC;
            settings.palette = p.palette;
            settings.lowcut = p.lowcut;
            settings.zoom = p.zoom;
            if (p.panel.magic == PANEL_MAGIC)
                panel = p.panel;
            if (p.magic == PERSIST_MAGIC)
                lights_from_word(p.lights);
            else
                p.lights = 0;
#if FELUCCA_ARRANGER
            if (p.magic == PERSIST_MAGIC && arr_valid(&p.arrangement, ARR_READY_MASK))
                arrangement = p.arrangement;
            else
                p.arrangement = arrangement;
#endif
            persist_saved = p;
            {   /* 2.7: the theme and the favourites, in the side record of the same copy (gfx.c, storage.c st_side) */
                side_rec_t r;
                if (st_side_load(OBJ_SETTINGS, &r, sizeof r) == 0)
                    side_take(&r);
            }
        } else if (n == (int)(8u + sizeof(panel_t)) && p.magic == 0x50455231u) {   /* "PER1": palette, panel */
            const uint32_t *w = (const uint32_t *)&p;
            panel_t old;
            memcpy(&old, w + 2, sizeof old);
            settings.magic = SETTINGS_MAGIC;
            settings.palette = w[1];
            settings.lowcut = 0;
            settings.zoom = 0;
            if (old.magic == PANEL_MAGIC)
                panel = old;
        }
    }
    project_cache_init();
    pb_scan();
    /* Journal migration is append-before-erase, and can restart after power loss. */
    for (uint32_t i = 0; i < ARR_SCENES; i++)
        if (pb_length[i] && pb_length[i] != sizeof(project_t)) {
            int n = pb_load(i, &proj_tmp, sizeof proj_tmp);
            if (proj_import(&autosave_buf, &proj_tmp, n))
                pb_save(i, &autosave_buf, sizeof autosave_buf);
        }
    /* Migrate A..D before reusing either of their old sectors. Repeat safely after a reset. */
    {
        uint32_t i;
        for (i = 0; i < 4u; i++)
            if (pb_sector[i] < 0) {
                int n = st_load(OBJ_PROJECT0 + i, &proj_tmp, sizeof proj_tmp);
                project_t *q = project_edit(i);
                if (q && (proj_ok(q) || proj_import(q, &proj_tmp, n))) {
                    if (pb_save(i, q, sizeof *q)) sec_dirty |= (uint8_t)(1u << (q - proj_slot));
                } else {
                    pb_save(i, 0, 0);
                }
            }
        for (i = 0; i < 4u; i++)
            if (proj_ok(&proj_slot[i])) {
                int n = pb_load(proj_cache.id[i], &proj_tmp, sizeof proj_tmp);
                if (n != (int)sizeof(project_t) || memcmp(&proj_tmp.v9, &proj_slot[i], sizeof(project_t)))
                    sec_dirty |= (uint8_t)(1u << i);
            }
    }
    dsu_boot();                                    /* SLOOP 2.5: the SYN kits (after the settings) */
    up_boot();                                     /* user presets */
    fm6_bank_boot();                               /* the FM6 patch bank (fm6_bank.c) */
#endif
}

UI_SMALL static int project_used(uint32_t slot) { return proj_ok(project_at(slot)); }

#if FELUCCA_FLASH
/* SLOOP 2.5: the settings object holds the SYN kits after persist_t (drum_synth.c dsu): firmware before 2.5 reads
 * its persist_t and leaves the rest (st_load cuts at its size); its own next save drops them. 2.6: then the MIDI LEARN
 * map (seq.c ml_tab, ML_N entries), only while something is learned; 2.5 reads an object with the map as one without
 * its SYN kits (their defaults), one without it as its own */
/* the settings object's third part (2.6): the MIDI LEARN map (seq.c ml_tab, ML_N entries, 2 bytes each, low first),
 * then GLO > MIDI: CH1 CH2 CH3, the drum channel | OTHER << 7, GLO > MIDI 2's BEND, 3 spare. Written only while it is not
 * the default (nothing learned, channels 1 2 3 10, OTHER SEL, BEND 2): else the object is exactly 2.5's. 2.6 test
 * builds wrote 32 (the map) or 36 bytes (and the channels) */
#define MIDI_PART_OLD (2u * ML_N + 8u)
#define MIDI_PART (MIDI_PART_OLD + 7u)
_Static_assert(sizeof(persist_t) + sizeof(dsu_bank_t) + MIDI_PART <= ST_PAYLOAD_MAX, "the settings object fits its sector");
static uint8_t midi_saved[MIDI_PART];
static uint8_t drch_seed;                           /* a settings object as 2.5 wrote it: the drum channel was the
                                                     * project's then (autosave_resume takes it from a 2.5 autosave) */               /* the part as flash has it (settings_save: written when it differs) */
UI_SMALL static void midi_part_fill(uint8_t *b)
{
    uint32_t k;
    for (k = 0; k < ML_N; k++)
        b[2u * k] = (uint8_t)ml_tab[k], b[2u * k + 1u] = (uint8_t)(ml_tab[k] >> 8);
    b[2u * ML_N] = midi_set[0], b[2u * ML_N + 1u] = midi_set[1], b[2u * ML_N + 2u] = midi_set[2];
    b[2u * ML_N + 3u] = (uint8_t)(midi_set[3] | (drum_preview != 0u) << 5 | (step_adv_off != 0u) << 6 | midi_set[4] << 7);
                                                    /* (2.7.1: bit 5 DRUM PREVIEW, bit 6 STEP ADVANCE OFF: bits 2.7 does not read) */
    b[2u * ML_N + 4u] = (uint8_t)(midi_set[5] | (fxfade > 15u ? 15u : fxfade) << 4);   /* BEND (bits 0-3); 2.7.2: FX FADE bits 4-7 (0 = none: the byte reads as 2.6's BEND) */
    b[2u * ML_N + 5u] = (uint8_t)(midi_set[6] | (midi_thru & 7u) << 5); /* FX CH in bits 0..4, THRU tracks 1..3 in bits 5..7 */
    b[2u * ML_N + 6u] = (uint8_t)(midi_set[7] | midi_set[9] << 1 |
                                  (flt_reso > 10u ? 10u : flt_reso) << 2);   /* (2.7) GLO > MIDI 3: DR MAP, bit 1 CLK OUT
                                                                      * (a spare byte to 2.6); 2.7.2: FILTER RESO in bits
                                                                      * 2..5 (0 = none: the byte then reads as 2.7's) */
    b[2u * ML_N + 7u] = (uint8_t)(midi_set[8] | (midi_thru >> TRK_DRUM & 1u) << 5); /* REMOTE in bits 0..4, THRU track 4 in bit 5 */
    for(k=0;k<4u;k++) b[MIDI_PART_OLD+k]=midi_extra[k];
    b[MIDI_PART_OLD+4u]=midi_thru;
    b[MIDI_PART_OLD+5u]=(uint8_t)ml_track_hi; b[MIDI_PART_OLD+6u]=(uint8_t)(ml_track_hi>>8);
}
UI_SMALL static int midi_part_default(const uint8_t *b)
{
    static const uint8_t def[10] = MIDI_SET_DEF;
    uint32_t k;
    for (k = 0; k < 2u * ML_N; k++)
        if (b[k])
            return 0;
    for(k=0;k<4u;k++) if(b[MIDI_PART_OLD+k]!=k+4u) return 0;
    if(b[MIDI_PART_OLD+4u] || b[MIDI_PART_OLD+5u] || b[MIDI_PART_OLD+6u]) return 0;
    return b[2u * ML_N] == def[0] && b[2u * ML_N + 1u] == def[1] && b[2u * ML_N + 2u] == def[2] &&
           b[2u * ML_N + 3u] == (uint8_t)(def[3] | def[4] << 7) && b[2u * ML_N + 4u] == def[5] && b[2u * ML_N + 5u] == def[6] &&
           b[2u * ML_N + 6u] == (uint8_t)(def[7] | def[9] << 1) && b[2u * ML_N + 7u] == def[8];
}
/* n: 32 (the map), 36 (and the channels) or MIDI_PART; every entry checked (a CC that learns, a parameter this knows, a
 * channel 0..16, a range 1..12) */
UI_SMALL static void midi_part_read(const uint8_t *b, uint32_t n)
{
    uint32_t k, e;
    fm1_irq_off();                                  /* Update the routing atomically with the channels. */
    midi_thru = n >= MIDI_PART_OLD ? (uint8_t)((b[2u * ML_N + 5u] >> 5) | ((b[2u * ML_N + 7u] >> 5 & 1u) << TRK_DRUM)) : 0;
    if(n>=MIDI_PART) midi_thru=b[MIDI_PART_OLD+4u];
    for(k=0;k<4u;k++) { midi_extra[k]=n>=MIDI_PART && b[MIDI_PART_OLD+k]<=16u?b[MIDI_PART_OLD+k]:(uint8_t)(k+4u); song.g[G_CH4+k]=midi_extra[k]; }
    for (k = 0; k < NTRK; k++)
        song.g[trk_thru_param(k)] = (midi_thru >> k) & 1u;
    ml_track_hi=n>=MIDI_PART?(uint16_t)(b[MIDI_PART_OLD+5u]|(uint32_t)b[MIDI_PART_OLD+6u]<<8):0;
    for (k = 0; k < ML_N; k++) {
        e = (uint32_t)b[2u * k] | (uint32_t)b[2u * k + 1u] << 8;
        ml_tab[k] = (uint16_t)((e >> 9) && ml_free_cc(e & 127u) && ml_id(e >> 9) < P_COUNT ? e : 0u);
        if(!ml_tab[k]) ml_track_hi&=(uint16_t)~(1u<<k);
#if NPART > 3
        else if(n<MIDI_PART && ((e>>7)&3u)==3u) ml_track_hi|=(uint16_t)(1u<<k);
#endif
    }
    if (n >= 2u * ML_N + 4u) {
        for (k = 0; k < 3u; k++)
            midi_set[k] = (uint8_t)(b[2u * ML_N + k] <= 16u ? b[2u * ML_N + k] : k + 1u);
        midi_set[3] = (uint8_t)((b[2u * ML_N + 3u] & 31u) <= 16u ? b[2u * ML_N + 3u] & 31u : 10u);
        midi_set[4] = (uint8_t)(b[2u * ML_N + 3u] >> 7);
        drum_preview = (uint8_t)((b[2u * ML_N + 3u] >> 5) & 1u);   /* (2.7.1) menu DRUM PREVIEW; older: OFF */
        step_adv_off = (uint8_t)((b[2u * ML_N + 3u] >> 6) & 1u);   /* (2.7.1) menu STEP ADVANCE; older: ON */
        song.g[G_CH1] = midi_set[0], song.g[G_CH2] = midi_set[1], song.g[G_CH3] = midi_set[2];
        song.g[G_DRCH] = midi_set[3], song.g[G_CHOTH] = midi_set[4];
    }
    if (n >= MIDI_PART_OLD) {
        midi_set[5] = (uint8_t)((b[2u * ML_N + 4u] & 15u) >= 1u && (b[2u * ML_N + 4u] & 15u) <= 12u ? b[2u * ML_N + 4u] & 15u : 2u);
        song.g[G_BEND] = midi_set[5];
        fxfade = (uint8_t)((b[2u * ML_N + 4u] >> 4) & 15u);         /* (2.7.2) FX FADE (bits 4-7); older: 0 */
        song.g[G_FXFADE] = fxfade;
        midi_set[6] = (uint8_t)((b[2u * ML_N + 5u] & 31u) <= 16u ? b[2u * ML_N + 5u] & 31u : 0u);   /* (2.7) FX CH */
        song.g[G_FXCH] = midi_set[6];
        midi_set[7] = (uint8_t)(b[2u * ML_N + 6u] & 1u);             /* (2.7) DR MAP */
        midi_set[9] = (uint8_t)((b[2u * ML_N + 6u] >> 1) & 1u);      /* (2.7) CLK OUT */
        flt_reso = (uint8_t)((b[2u * ML_N + 6u] >> 2) & 15u);        /* (2.7.2) FILTER RESO (bits 2..5); older: 0 */
        if (flt_reso > 10u)
            flt_reso = 10u;
        song.g[G_DRMAP] = midi_set[7];
        song.g[G_CLKOUT] = midi_set[9];
        midi_set[8] = (uint8_t)((b[2u * ML_N + 7u] & 31u) <= 16u ? b[2u * ML_N + 7u] & 31u : 0u);   /* (2.7) REMOTE */
        song.g[G_RMCH] = midi_set[8];
    }
    fm1_irq_on();
}
UI_SMALL static int settings_write(const persist_t *p)
{
    uint8_t mp[MIDI_PART];
    int rc;
    if (!dsu_valid(&dsu))
        dsu_defaults();
    midi_part_fill(mp);
    /* the third part only when it is not the default: without, the object is exactly 2.5's (2.5 reads it whole, SYN
     * kits included; with it, it takes the object for one without them). 2.7: the theme in the side record */
    side_fill(&side_out);
    st_side = &side_out;
    st_side_len = sizeof side_out;
    rc = st_save3(OBJ_SETTINGS, p, sizeof *p, &dsu, sizeof dsu, mp, midi_part_default(mp) ? 0u : sizeof mp);
    st_side = 0;
    if (rc)
        return -1;
    dsu_dirty = 0;
    fav_dirty = 0;
    memcpy(midi_saved, mp, sizeof midi_saved);
    return 0;
}
static void dsu_boot(void)                          /* the SYN kits (and the MIDI part) from the settings object, else */
{                                                   /* the defaults */
    st_hdr_t h;
    uint32_t off = sizeof(persist_t), len, k, magic;
    int found = st_current(OBJ_SETTINGS, &h);
    if (found >= 0 && h.len >= 4u) {
        memcpy(&magic, st_buf, 4u);
        if (magic == 0x50455233u) off = 88u;
    }
    len = off + sizeof(dsu_bank_t);
    dsu_defaults();
    ml_track_hi=0;
    for (k = 0; k < ML_N; k++)
        ml_tab[k] = 0;
    if (found >= 0 &&
        (h.len == len || h.len == len + 2u * ML_N || h.len == len + 2u * ML_N + 4u || h.len == len + MIDI_PART_OLD || h.len == len + MIDI_PART) &&
        dsu_valid((const dsu_bank_t *)(st_buf + off))) {
        memcpy(&dsu, st_buf + off, sizeof dsu);
        for (k = 0; k < DSU_N; k++)
            dsu_fix_kit(&dsu.k[k]);
        if (h.len > len)
            midi_part_read(st_buf + len, h.len - len);
        drch_seed = h.len == len;
    }
    midi_part_fill(midi_saved);
    dsu_dirty = 0;
}
#endif

UI_SMALL static void persist_fill(persist_t *p)              /* the settings as they are now */
{
    memset(p, 0, sizeof *p);
    p->magic = PERSIST_MAGIC;
    p->palette = settings.palette;
    p->lowcut = settings.lowcut;
    p->zoom = settings.zoom;
    p->panel = panel;
    p->lights = lights_word();
#if FELUCCA_ARRANGER
    p->arrangement = arrangement;
#endif
}

UI_SMALL static void settings_save(void)
{
#if FELUCCA_FLASH
    persist_t p;
    if (!flash_ok)
        return;
    persist_fill(&p);
    {
        uint8_t mp[MIDI_PART];
        midi_part_fill(mp);
        if (!memcmp(&p, &persist_saved, sizeof p) && !memcmp(mp, midi_saved, sizeof mp) && !fav_dirty) {
            song_dirty = 0;
            return;
        }                                /* unchanged: no erase cycle */
    }
    if (settings_write(&p) == 0) {
        persist_saved = p;
        song_dirty = 0;
    } else {
        song_dirty = 1;
        save_pending = 1;
        ui_message("SAVE ERROR: RETRYING");
    }
#endif
}

#if FELUCCA_FLASH
_Static_assert(sizeof(project_t) <= PB_PAYLOAD_MAX, "project fits the paired journal");

/* ---- backup restore (editor.c BK_PUT): each object checked as a load checks it, then written through the
 * same A/B commit as a save. rc: 0 ok, 2 not a valid object, 3 stop the song first, 4 flash */
UI_SMALL static int panel_valid(const panel_t *q)           /* a permutation of the buttons and of the knobs */
{
    uint32_t i, b = 0, e = 0;
    if (q->magic != PANEL_MAGIC)
        return 0;
    for (i = 0; i < NB; i++) {
        if (q->btn[i] >= 14u || (b >> q->btn[i]) & 1u)
            return 0;
        b |= 1u << q->btn[i];
    }
    for (i = 0; i < NE; i++) {
        if (q->enc[i] >= 7u || (e >> q->enc[i]) & 1u || (q->dir[i] != 1 && q->dir[i] != -1))
            return 0;
        e |= 1u << q->enc[i];
    }
    return 1;
}

UI_SMALL static uint32_t settings_restore(const void *raw, uint32_t n)
{
    persist_t p;
    if (n != sizeof p && n != PERSIST_SIZE_V22 && n != 84u && n != 88u)
        return 2;
    memset(&p, 0, sizeof p);
    if (!persist_import(&p, raw, n)) return 2;
    if (p.magic != PERSIST_MAGIC || p.palette >= NPALETTES || p.lowcut > 1u || p.zoom > 1u || !panel_valid(&p.panel))
        return 2;
#if FELUCCA_ARRANGER
    if (!arr_valid(&p.arrangement, ARR_READY_MASK))
        return 2;
    if ((song.playing || transport_req) && memcmp(&p.arrangement, &arrangement, sizeof arrangement))
        return 3;                                    /* SLOOP 2.5: a new song order only when stopped (as on the SONG screen) */
#endif
    if (!flash_ok || settings_write(&p))
        return 4;
    persist_saved = p;
    settings.palette = p.palette;
    settings.lowcut = p.lowcut;
    settings.zoom = p.zoom;
    panel = p.panel;
#if FELUCCA_ARRANGER
    arrangement = p.arrangement;
#endif
    lights_from_word(p.lights);
    song.g[G_SYNC] = (int16_t)lights_sync;
    song.g[G_MIDI] = (int16_t)lights_mout;
    song.g[G_ROUTE] = (int16_t)lights_min;
    palette_set(settings.palette);
    fx_lowcut = (uint8_t)(settings.lowcut != 0);
    ui.force = 1;
    return 0;
}

/* Pattern 0..23 (n 0: erase), or ARR_SCENES: the working loop. */
UI_SMALL static uint32_t project_restore(uint32_t slot, const void *raw, uint32_t n)
{
    if (song.playing || transport_req)
        return 3;
    if (slot > ARR_SCENES) return 2;
    if (slot < ARR_SCENES && !n) {
        if (!flash_ok || pb_save(slot, raw, 0))
            return 4;
        { uint32_t i; for (i = 0; i < 4u; i++) if (proj_cache.id[i] == slot) {
            proj_slot[i].magic = 0; sec_dirty &= (uint8_t)~(1u << i);
        } }
        return 0;
    }
    if (!proj_import(&autosave_buf, raw, (int)n))
        return 2;
    if (slot == ARR_SCENES) {
        project_apply(&autosave_buf);
        return 0;
    }
    if (!flash_ok || pb_save(slot, &autosave_buf, sizeof autosave_buf))
        return 4;
    { uint32_t i; for (i = 0; i < 4u; i++) if (proj_cache.id[i] == slot) {
        memcpy(&proj_slot[i], &autosave_buf, sizeof(project_t)); sec_dirty &= (uint8_t)~(1u << i);
    } }
    return 0;
}
#endif
#if FELUCCA_ARRANGER
UI_SMALL static void arrangement_save(void)
{
    if (song.playing || transport_req) { ui_message("STOP BEFORE SAVE"); return; }
    settings_save();
#if FELUCCA_FLASH
    if (flash_ok) {
        ui_message(memcmp(&persist_saved.arrangement, &arrangement, sizeof arrangement) ? "SAVE ERROR" : "SONG SAVED");
        return;
    }
#endif
    ui_message("SONG IN RAM ONLY");
}

/* ---- live sections (SAVE + key, ui_layers.c). A section is a project slot (A..D = 1..4): stored into RAM
 * at once (playing too), written to flash once the transport is stopped and nothing sounds (an erase
 * stops the audio for ~50 ms); a song recorded with SONG REC is saved the same way. */
UI_SMALL static int section_store(uint32_t s)
{
    project_t *p;
    if (s >= ARR_SCENES) return 0;
    fm1_irq_off();
    p = project_edit(s);
    if (p) {
        proj_capture(p);
        sec_dirty |= (uint8_t)(1u << (p - proj_slot));
        live_sec = (int8_t)s;
        save_pending = 1;
    }
    fm1_irq_on();
    if (!p) ui_message("STOP TO SAVE MORE");
    return p != 0;
}
UI_SMALL static void section_load(uint32_t s)
{
    const project_t *p = project_at(s);
    if (!proj_ok(p)) return;
    project_apply(p);
    live_sec = (int8_t)s;
}
UI_SMALL static void sections_write(void)                        /* the dirty sections and song into flash */
{
    uint32_t i;
#if FELUCCA_FLASH
    if (flash_ok)
        for (i = 0; i < 4u; i++)
            if (((sec_dirty >> i) & 1u) && pb_save(proj_cache.id[i], &proj_slot[i], sizeof proj_slot[i]) == 0)
                sec_dirty &= (uint8_t)~(1u << i);       /* (a failed write stays dirty: tried again later) */
    if (!flash_ok)
#endif
        sec_dirty = 0;
    (void)i;
    if (song_dirty || fav_dirty) {                      /* (2.7: the favourites too) */
        settings_save();
    }
    save_pending = sec_dirty || song_dirty || fav_dirty;
}
/* before an intentional reset (an update, UPDATE MODE, UBOOT from the host): the audio is stopped, so
 * whatever is only in RAM goes to flash now: the live sections, the song, the working project */
UI_SMALL static void persist_flush_now(void)
{
    project_stage_busy=0;
    sections_write();
#if FELUCCA_FLASH
    if (flash_ok && !arrangement_clock.running) {     /* (a song playing: the tracks hold a section) */
        proj_capture(&autosave_buf);
        if (autosave_buf.sum != autosave_hash && pb_autosave_save(&autosave_buf, sizeof autosave_buf) == 0)
            autosave_hash = autosave_buf.sum;
    }
#endif
}
UI_SMALL static void sections_flush(void)                        /* main loop */
{
    static uint32_t tried;
    save_pending = sec_dirty || song_dirty || fav_dirty || settings_later;
    if (srec_done) {
        song_dirty = srec_done != 0xFFu;
        if (song_dirty) {
            char b[8];
            fmt_int(b, srec_done);
            ui_say("SONG PARTS ", b);
        } else {
            ui_message("NO SONG");
        }
        srec_done = 0;
    }
    if ((uint32_t)song.g[G_SYNC] != lights_sync) {      /* GLO > SYSTEM > SYNC: kept with the settings */
        lights_sync = (uint8_t)song.g[G_SYNC];
        settings_later = 1;
    }
    if ((uint32_t)(song.g[G_MIDI] != 0) != lights_mout) {   /* GLO > SYSTEM > MIDI: the same */
        lights_mout = (uint8_t)(song.g[G_MIDI] != 0);
        settings_later = 1;
    }
    if ((uint32_t)song.g[G_ROUTE] != lights_min) {        /* GLO > SYSTEM > IN: the same (2.6: CH n too) */
        lights_min = (uint8_t)song.g[G_ROUTE];
        settings_later = 1;
    }
    {   /* 2.6: GLO > MIDI (CH1 CH2 CH3 OTHER) and GLO > DRUMS > CH: the same */
        static const uint8_t gid[10] = {G_CH1, G_CH2, G_CH3, G_DRCH, G_CHOTH, G_BEND, G_FXCH, G_DRMAP, G_RMCH, G_CLKOUT};
        uint32_t k;
        for (k = 0; k < 10u; k++)
            if ((uint32_t)song.g[gid[k]] != midi_set[k]) {
                midi_set[k] = (uint8_t)song.g[gid[k]];
                settings_later = 1;
            }
        for(k=0;k<4u;k++) if(midi_extra[k]!=(uint32_t)song.g[G_CH4+k]) { midi_extra[k]=(uint8_t)song.g[G_CH4+k]; settings_later=1; }
        uint32_t thru = 0;
        for (k = 0; k < NTRK; k++)
            thru |= (uint32_t)(song.g[trk_thru_param(k)] != 0) << k;
        if (thru != midi_thru) {
            midi_thru = (uint8_t)thru;
            settings_later = 1;
        }
        if ((uint32_t)song.g[G_FXFADE] != fxfade) {   /* (2.7.2) GLO > MASTER 2 > FADE: a setting of the FM-1 */
            fxfade = (uint8_t)song.g[G_FXFADE];
            settings_later = 1;
        }
    }
    if (settings_later) {                               /* the menu closed while playing */
        settings_later = 0;
        song_dirty = 1;                                 /* (settings_save when quiet, with the song) */
    }
    if ((!sec_dirty && !song_dirty) || song.playing || transport_req || !audio_quiet() || fm1_ms - ui_input_ms < 1500u ||
        fm1_ms - tried < 5000u)
        return;
    tried = fm1_ms;                                     /* (a failed write: again in 5 s, not every frame) */
    uint8_t patterns_pending = sec_dirty;
    sections_write();
    if (sec_dirty || song_dirty || fav_dirty)
        ui_message("SAVE ERROR: RETRYING");
    else if (patterns_pending && flash_ok)
        ui_message("PATTERNS SAVED");
}
#endif
#endif /* PROJ_HOST */
