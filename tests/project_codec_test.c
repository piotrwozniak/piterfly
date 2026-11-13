/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include "../firmware/src/project_codec.c"
#include <assert.h>

int main(void)
{
    packed_track_t t = {0};
    uint32_t seed = 17;
    for (uint32_t k = 0; k < P_E0; k++) {
        assert(TP[k].min >= -64 && TP[k].max <= 127);
        for (int32_t v = TP[k].min; v <= TP[k].max; v++) {
            pc_param_put(&t, k, v); assert(pc_param_get(&t, k) == v);
        }
    }
    for (uint32_t e = 0; e < NENGINES; e++)
        for (uint32_t k = 0; k < 8u; k++) {
            const param_desc_t *d = &ENGINES[e]->edit[k];
            assert(d->min >= -64 && d->max <= 127);
            for (int32_t v = d->min; v <= d->max; v++) {
                pc_param_put(&t, P_E0 + k, v); assert(pc_param_get(&t, P_E0 + k) == v);
            }
        }
    for (uint32_t n = 0; n < 200000u; n++) {
        step_t a, b;
        uint8_t packed[10] = {0x42,0,0,0,0,0,0,0,0,0x81};
        for (uint32_t k = 0; k < sizeof a; k++) {
            seed = seed * 1664525u + 1013904223u; ((uint8_t *)&a)[k] = (uint8_t)(seed >> 24);
        }
        for (uint32_t k = 0; k < 4u; k++) a.note[k] &= 127u;
        a.n %= 5u; a.time %= 3u; a.vel &= 127u;
        pc_step_put(packed + 1, &a); pc_step_get(&b, packed + 1);
        assert(!memcmp(&a, &b, sizeof a) && packed[0] == 0x42 && packed[9] == 0x81);
    }
    for (uint32_t step = 0; step < NSTEP; step++)
        for (uint32_t id = 0; id < P_COUNT; id++)
            for (int32_t v = -64; v <= 127; v++) {
                plock_t a = {(uint8_t)step, (uint8_t)id, (int16_t)v}, b;
                uint8_t packed[3];
                pc_lock_put(packed, &a); pc_lock_get(&b, packed);
                assert(a.step == b.step && a.param == b.param && a.val == b.val);
            }
    { plock_t a = {LOCK_FREE, 0, 0}, b; uint8_t packed[3];
      pc_lock_put(packed, &a); pc_lock_get(&b, packed); assert(b.step == LOCK_FREE);
      uint8_t other[3]; a.param=63; a.val=117; pc_lock_put(other,&a); assert(!memcmp(packed,other,3)); }
    printf("packed track: %zu bytes; parameter ranges, 200000 steps, every lock round-trip: ok\n", sizeof t);
    return 0;
}
