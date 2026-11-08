/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/hal/fm1_flash_id.h"
#include "editor_host.c"
#include <assert.h>
int main(void)
{
    uint32_t i;
    harness_init();
    /* A device running the old four-pattern firmware, with sample bank 4 filled. */
    memset(nor, 0xFF, sizeof nor);
    memset(nor + 0xE7000, 0x52, 0x14000);
    for (i = 0; i < 4; i++) {
        trk[0].p[P_SLEN] = 64;
        trk[0].step[63].n = 1; trk[0].step[63].note[0] = (uint8_t)(70 + i);
        proj_capture(&proj_slot[i]);
        assert(st_save(OBJ_PROJECT0 + i, &proj_slot[i], sizeof(project_t)) == 0);
    }
    memset(proj_slot, 0, sizeof proj_slot); memset(&proj_cache, 0, sizeof proj_cache);
    sec_dirty = pb_ready = 0;
    persist_boot();
    for (i = 0; i < 4; i++) {
        assert(project_used(i));
        assert(project_at(i)->t[0].step[63].note[0] == 70 + i);
    }
    assert(!project_used(23));
    /* A saved live edit survives a warm reset before reaching flash. */
    song.playing = 1;
    trk[0].step[63].note[0] = 99;
    assert(section_store(23));
    song.playing = 0;
    persist_boot();
    assert(project_at(23)->t[0].step[63].note[0] == 99 && sec_dirty);
    sections_write();
    memset(proj_slot, 0, sizeof proj_slot); memset(&proj_cache, 0, sizeof proj_cache);
    pb_ready = 0; persist_boot();
    assert(project_at(23)->t[0].step[63].note[0] == 99);
    assert(project_at(0)->t[0].step[63].note[0] == 70);
    /* PER3 settings: old order, lights, SYN kits and MIDI routing all survive. */
    {
        persist_t p, restored;
        uint8_t old[88], mp[MIDI_PART];
        persist_fill(&p);
        memcpy(old, &p, 84);
        uint32_t magic = 0x50455233u, lights = lights_word();
        memcpy(old, &magic, 4); memcpy(old + 84, &lights, 4);
        midi_thru = 9; midi_part_fill(mp);
        dsu_bank_t kits = dsu;
        assert(st_save3(OBJ_SETTINGS, old, sizeof old, &kits, sizeof kits, mp, sizeof mp) == 0);
        memset(&arrangement, 0, sizeof arrangement); midi_thru = 0;
        persist_boot();
        assert(arrangement.count == p.arrangement.count);
        assert(!memcmp(arrangement.entry, p.arrangement.entry, 32));
        assert(lights_word() == lights && midi_thru == 9 && !memcmp(&dsu, &kits, sizeof kits));
        assert(persist_import(&restored, old, 88) && restored.magic == PERSIST_MAGIC);
        assert(settings_restore(old, 88) == 0);
        arrangement.count = 128;
        for (i = 0; i < 128; i++) arrangement.entry[i] = (arr_entry_t){i % 24, 64};
        settings_save();
        memset(&arrangement, 0, sizeof arrangement);
        persist_boot();
        assert(arrangement.count == 128 && arrangement.entry[127].scene == 7 && arrangement.entry[127].bars == 64);
        assert(midi_thru == 9 && !memcmp(&dsu, &kits, sizeof kits));
    }
    assert(fl_bad == 0);
    puts("24 patterns: old A-D migration, occupied USR4, warm/cold restart: PASS");
    return 0;
}
