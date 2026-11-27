/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio)
 * Copyright (C) 2026 Adrien Tibi (Recursive Field Instruments)
 * Ported from Melodee; source revisions and MPL permission: see THIRD_PARTY_NOTICES.md. */
/* Select the first USB MIDI 1.0 streaming interface with an OUT endpoint. */
#pragma once
#include <stdint.h>

typedef struct {
    uint8_t cfg;                 /* bConfigurationValue */
    uint8_t iface, alt;          /* its interface and alternate setting */
    uint8_t out_ep;              /* endpoint number, with the direction bit removed */
    uint8_t out_type, out_interval;
    uint16_t out_mps;
} uh_midi_t;

/* d / len: the configuration descriptor as read (possibly cut short: a descriptor that does not fit is ignored) */
static int uh_parse_midi(const uint8_t *d, uint32_t len, uh_midi_t *m)
{
    uint32_t i = 0;
    uint8_t cfg = 0, in_midi = 0;
    *m = (uh_midi_t){0};
    while (i + 2u <= len) {
        uint32_t l = d[i];
        if (l < 2u || i + l > len)
            break;
        if (d[i + 1] == 2u && l >= 9u) {
            cfg = d[i + 5];
        } else if (d[i + 1] == 4u && l >= 9u) {
            m->cfg = cfg;
            m->iface = d[i + 2];
            m->alt = d[i + 3];
            in_midi = d[i + 5] == 1u && d[i + 6] == 3u && d[i + 7] == 0u;
        } else if (d[i + 1] == 5u && l >= 7u && in_midi) {
            uint8_t ea = d[i + 2], type = d[i + 3] & 3u;
            uint16_t mps = (uint16_t)(d[i + 4] | d[i + 5] << 8);
            /* A bulk endpoint declaring a high-speed size (128..512) at full speed: 64 (the most a full-speed bulk
             * packet holds on the wire). Elektron's Digitone II answers a full-speed host with its high-speed
             * descriptor (512) and stalls the device qualifier: "NO MIDI" before (the user, 2026-11-09) */
            if (type == 2u && (mps == 128u || mps == 256u || mps == 512u))
                mps = 64u;
            /* Full-speed USB-MIDI 1.0 events are four bytes. The buffers hold at most 64 bytes. */
            if ((ea & 15u) && !(ea & 0xF0u) && mps >= 4u && mps <= 64u &&
                ((type == 2u && (mps == 8u || mps == 16u || mps == 32u || mps == 64u)) ||
                 (type == 3u && d[i + 6]))) {
                m->out_ep = ea;
                m->out_type = type;
                m->out_interval = d[i + 6];
                m->out_mps = mps & ~3u;              /* send whole event packets only */
                return 1;
            }
        }
        i += l;
    }
    return 0;
}
