/* SPDX-License-Identifier: GPL-3.0-only */
/* Lossless FUN9 fields. Byte operations keep the on-flash layout independent of
 * compiler bitfield rules and avoid unaligned reads on the FM-1. */
#define PC_TRACK_BYTES (P_COUNT + 2u + NSTEP * 8u + NSTEP + NLOCK * 3u)
typedef struct {
    uint8_t p[P_COUNT], engine, preset;
    uint8_t step[NSTEP][8], mf[NSTEP], lock[NLOCK][3];
} packed_track_t;
_Static_assert(sizeof(packed_track_t) == PC_TRACK_BYTES, "packed track has no padding");

static int16_t pc_param_get(const packed_track_t *t, uint32_t id)
{
    return (int16_t)((int32_t)t->p[id] - 64);
}
static void pc_param_put(packed_track_t *t, uint32_t id, int32_t v)
{
    t->p[id] = (uint8_t)(clamp(v, -64, 191) + 64);
}
static void pc_step_put(uint8_t *d, const step_t *s)
{
    uint32_t n = s->n > 4u ? 4u : s->n, time = s->time > ST_REST ? ST_REST : s->time;
    d[0] = (uint8_t)((s->note[0] & 127u) | (s->note[1] & 1u) << 7);
    d[1] = (uint8_t)((s->note[1] & 127u) >> 1 | (s->note[2] & 3u) << 6);
    d[2] = (uint8_t)((s->note[2] & 127u) >> 2 | (s->note[3] & 7u) << 5);
    d[3] = (uint8_t)((s->note[3] & 127u) >> 3 | n << 4 | (time & 1u) << 7);
    d[4] = (uint8_t)(time >> 1 | (uint32_t)s->flags << 1);
    d[5] = (uint8_t)((uint32_t)s->flags >> 7 | (s->vel & 127u) << 1);
    d[6] = s->lvl; d[7] = s->rat;
}
static void pc_step_get(step_t *s, const uint8_t *d)
{
    s->note[0] = d[0] & 127u;
    s->note[1] = (uint8_t)((d[0] >> 7 | (uint32_t)d[1] << 1) & 127u);
    s->note[2] = (uint8_t)((d[1] >> 6 | (uint32_t)d[2] << 2) & 127u);
    s->note[3] = (uint8_t)((d[2] >> 5 | (uint32_t)d[3] << 3) & 127u);
    s->n = (d[3] >> 4) & 7u;
    s->time = (uint8_t)(d[3] >> 7 | (d[4] & 1u) << 1);
    s->flags = (uint8_t)(d[4] >> 1 | (uint32_t)d[5] << 7);
    s->vel = d[5] >> 1; s->lvl = d[6]; s->rat = d[7];
}
static void pc_lock_put(uint8_t *d, const plock_t *s)
{
    uint32_t step = s->step < NSTEP ? s->step : 63u;
    uint32_t id = s->step < NSTEP && s->param < P_COUNT ? s->param : 127u;
    uint32_t v = id==127u?64u:(uint32_t)(clamp(s->val, -64, 191) + 64);
    d[0] = (uint8_t)(step | id << 6);
    d[1] = (uint8_t)(id >> 2 | v << 5);
    d[2] = (uint8_t)(v >> 3);
}
static void pc_lock_get(plock_t *s, const uint8_t *d)
{
    uint32_t id = (d[0] >> 6 | (uint32_t)d[1] << 2) & 127u;
    s->step = id < P_COUNT ? d[0] & 63u : LOCK_FREE;
    s->param = id < P_COUNT ? (uint8_t)id : 0u;
    s->val = id < P_COUNT ? (int16_t)clamp((int32_t)((d[1] >> 5 | (uint32_t)d[2] << 3) & 255u) - 64, -64, 127) : 0;
}
