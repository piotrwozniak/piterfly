/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/hal/fm1_flash_id.h"
#include "editor_host.c"
#include <assert.h>
int main(void)
{
    uint32_t i;
    harness_init();
    assert(SMP_USER_SLOTS == 3 && ARR_SCENES == 24 && NSTEP == 64);
    pb_scan();
    /* Fill every slot with a distinct full-length four-track pattern. */
    for (i = 0; i < 24; i++) {
        trk[0].step[63].n = 1;
        trk[0].step[63].note[0] = (uint8_t)(40 + i);
        trk[0].p[P_SLEN] = 64;
        project_save(i);
        assert(project_used(i));
    }
    assert(arrangement_ready() == ARR_READY_MASK);
    { /* Real SysEx protocol, not the browser mock. */
        uint8_t query[] = {ED_HDR0, ED_HDR1, ED_HDR2, ED_PROJECT, 2, 23};
        ed_inject(query, sizeof query);
        assert(rx_n >= 8 && rx_last[5] == 2 && rx_last[6] == 23 && rx_last[7] == 1);
        uint32_t len;
        assert(ed_bk_obj(83, &len) == (const uint8_t *)project_at(23) && len == sizeof(project_t));
        const uint8_t *blob = ed_bk_obj(83, &len);
        memcpy(&proj_tmp, blob, len);
        assert(project_restore(22, &proj_tmp, len) == 0);
        assert(project_at(22)->t[0].step[63].note[0] == 63);
        project_load(22); audio_blocks(1);
        trk[0].step[63].note[0] = 62; project_save(22);
    }
    /* Cold cache; every pattern still exists and can be played from flash. */
    memset(proj_slot, 0, sizeof proj_slot);
    pb_scan();
    for (i = 0; i < 24; i++) {
        project_load(i);
        assert(trk[0].step[63].note[0] == 40 + i && trk[0].p[P_SLEN] == 64);
        audio_blocks(1);
        arrangement_apply(i);
        assert(trk[0].step[63].note[0] == 40 + i);
    }
    /* Slot F4 also switches through the real next-bar path, directly from flash. */
    trk[0].step[63].note[0] = 1;
    song.playing = 1; clk_beat = 4; live_bar = 0xFFFFFFFFu; live_req = 23;
    live_block();
    assert(live_sec == 23 && trk[0].step[63].note[0] == 63);
    song.playing = 0;
    /* Old sample bank cannot be addressed by either a drum or the upload protocol. */
    assert(drum_usr_zone(3, 36) < 0);
    assert(ed_bk_obj(35, &i) == 0);
    audio_blocks(1);
    assert(project_restore(23, 0, 0) == 0);
    assert(!project_used(23) && project_used(22));
    /* Four live edits are protected against eviction before transport stops. */
    sec_dirty = 0;
    song.playing = 1;
    for (i = 4; i < 8; i++) assert(section_store(i));
    assert(!section_store(8));
    { uint8_t store[] = {ED_HDR0, ED_HDR1, ED_HDR2, ED_LIVE, 14, 8};
      ed_inject(store, sizeof store);
      assert(rx_last[rx_n - 2u] == 3u); /* Full cache is reported even if slot 8 already existed. */
      store[5] = 24; ed_inject(store, sizeof store);
      assert(rx_last[rx_n - 2u] == 2u);
    }
    song.playing = 0;
    sections_write();
    assert(sec_dirty == 0 && section_store(8));
    sections_write();
    memset(proj_slot, 0, sizeof proj_slot);
    assert(project_used(8));
    /* Native third-row DELETE reaches F3 and keeps the working loop intact. */
    section_bank = 5;
    uint8_t current_note = trk[0].step[63].note[0];
    assert(project_used(22));
    layer_key(LY_SONG, key_of_white(10), 1);
    assert(project_used(22) && sec_erase_armed == 23);
    layer_key(LY_SONG, key_of_white(10), 1);
    assert(!project_used(22) && project_used(21));
    assert(trk[0].step[63].note[0] == current_note);
    assert(fl_bad == 0);
    puts("24 patterns: full-length save/load/play, reboot, delete, live cache, USR4 disabled: PASS");
    return 0;
}
