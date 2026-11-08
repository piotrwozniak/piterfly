/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/hal/fm1_flash_id.h"
#include "editor_host.c"
#include <assert.h>
int main(void)
{
    uint8_t b[MIDI_PART];
    uint32_t mask, i;
    host_tracks_init();
    midi_set[6] = 16;
    midi_set[8] = 12;
    for (mask = 0; mask < 16u; mask++) {
        midi_thru = (uint8_t)mask;
        midi_part_fill(b);
        midi_thru = 0;
        midi_part_read(b, sizeof b);
        assert(midi_thru == mask);
        assert(midi_set[6] == 16 && midi_set[8] == 12);
        for (i = 0; i < NTRK; i++)
            assert(song.g[G_THRU1 + i] == ((mask >> i) & 1u));
    }
    for (i = 0; i < NTRK; i++) song.g[G_THRU1 + i] = (i & 1u);
    sections_flush();
    assert(midi_thru == 10u);
    project_new();
    for (i = 0; i < NTRK; i++) assert(song.g[G_THRU1 + i] == (i & 1u));
    midi_part_read(b, 2u * ML_N + 4u);
    assert(midi_thru == 0);
    for (i = 0; i < NTRK; i++) assert(song.g[G_THRU1 + i] == 0);
    midi_part_fill(b);
    midi_part_read(b, sizeof b);
    assert(midi_thru == 0); /* old settings have no THRU bits */
    puts("MIDI THRU settings: all masks, channel preservation, old settings: PASS");
    return 0;
}
