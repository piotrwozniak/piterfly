/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static void block(void) { int32_t out[CTL * 2]; mix_block(out, CTL); }
static void note(uint32_t ch, uint32_t n, uint32_t vel)
{
    midi_in_event((vel ? 9u : 8u) | ((vel ? 0x90u : 0x80u) | ch) << 8 | n << 16 | vel << 24);
    block();
}
static void expect(uint32_t ch, uint32_t n, uint32_t vel)
{
    uint32_t p;
    assert(mo_w - mo_r == 1);
    p = midi_out_q[mo_r++ % MQ];
    assert((p >> 8 & 255u) == ((vel ? 0x90u : 0x80u) | ch));
    assert((p >> 16 & 127u) == n);
    assert((p >> 24 & 127u) == vel);
}
int main(void)
{
    uint32_t i;
    host_tracks_init();
    song.g[G_ROUTE] = 0;
    song.g[G_CHOTH] = 0;
    song.g[G_CH1] = 1;
    song.g[G_CH2] = 2;
    song.g[G_CH3] = 3;
    song.g[G_DRCH] = 10;
    song.g[G_FXCH] = 0;
    block();
    usb.config = 1;
    mo_r = mo_w;
    song.g[G_THRU1] = 1;
    block();
    song.sel = 0;
    note(3, 60, 100); expect(0, 60, 100);
    song.sel = 1;
    note(3, 60, 0); expect(0, 60, 0);
    for (i = 1; i < 4; i++) {
        song.sel = i;
        note(3, 62, 90);
        note(3, 62, 0);
        assert(mo_w == mo_r);
    }
    song.sel = 0;
    song.g[G_THRU1] = 0; block();
    note(0, 64, 100); note(0, 64, 0);
    assert(mo_w == mo_r); /* THRU OFF: input notes are not echoed. */
    song.g[G_THRU1] = 1; block();
    note(3, 65, 80); expect(0, 65, 80);
    song.g[G_CH1] = 5;
    block(); expect(0, 65, 0);
    note(3, 65, 0); assert(mo_w == mo_r);
    note(3, 67, 70); expect(4, 67, 70);
    midi_in_lost = 1;
    block(); expect(4, 67, 0);
    /* Every track, a different input channel, and channel-filter mode. */
    for (i = 0; i < NTRK; i++) {
        song.g[G_THRU1 + i] = 1;
        song.g[G_ROUTE] = 9; /* IN = CH 8 */
        song.sel = i;
        block();
        note(7, 70, 95); expect(trk_midi_ch(i), 70, 95);
        song.sel = (i + 1u) % NTRK;
        note(7, 70, 0); expect(trk_midi_ch(i), 70, 0);
    }
    song.sel = 2;
    note(6, 71, 90); assert(mo_w == mo_r); /* wrong input channel */
    note(7, 71, 90); expect(2, 71, 90);
    song.g[G_THRU3] = 0; block(); expect(2, 71, 0);
    note(7, 71, 0); assert(mo_w == mo_r);
    song.g[G_THRU3] = 1; block();
    /* USB and TRS may hold the same note independently. */
    note(7, 72, 80); expect(2, 72, 80);
    midi_in_event(0x19u | 0x97u << 8 | 72u << 16 | 90u << 24); block(); expect(2, 72, 90);
    note(7, 72, 0); expect(2, 72, 0);
    midi_in_event(0x18u | 0x87u << 8 | 72u << 16); block(); expect(2, 72, 0);
    song.g[G_CH3] = 0; block();
    note(7, 73, 90); note(7, 73, 0); assert(mo_w == mo_r);
    puts("MIDI THRU: routing, velocity, track switch, THRU OFF, channel change, overflow, USB/TRS, all tracks: PASS");
    return 0;
}
