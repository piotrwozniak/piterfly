/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/hal/fm1_flash_id.h"
#include "editor_host.c"
#include <assert.h>
int main(void)
{
    harness_init();
    settings_save();
    assert(persist_saved.arrangement.count == 4);
    arrangement.count = 5; song_dirty = 1; fav_dirty = 0; sec_dirty = 0;
    fl_fail_at = 1; sections_write();
    assert(song_dirty && save_pending && persist_saved.arrangement.count == 4);
    sections_write();
    assert(!song_dirty && !save_pending && persist_saved.arrangement.count == 5);
    project_save(0); project_save(1);
    fm1_ms = 1000; sec_armed = 0;
    layer_key(LY_SONG, key_of_white(4), 1);
    assert(sec_armed == 1 && !sec_dirty);
    chain_release(); assert(!sec_armed);
    layer_key(LY_SONG, key_of_white(1), 1);
    layer_key(LY_SONG, key_of_white(4), 1);
    assert(sec_armed == 1 && !sec_dirty);
    layer_key(LY_SONG, key_of_white(1), 1); assert(!sec_armed);
    layer_key(LY_SONG, key_of_white(4), 1); assert(sec_armed == 1 && !sec_dirty);
    layer_key(LY_SONG, key_of_white(4), 1);
    assert(sec_dirty && save_pending && !strncmp(ui.msg, "PENDING", 7));
    uint8_t state[] = {ED_HDR0, ED_HDR1, ED_HDR2, ED_LIVE, 0};
    ed_inject(state, sizeof state); assert(rx_last[7] & 16u);
    song.playing = 1; sections_flush(); assert(sec_dirty && save_pending);
    song.playing = 0; transport_req = 0; ui_input_ms = 0; fm1_ms = 10000;
    sections_flush();
    assert(!sec_dirty && !save_pending && !strcmp(ui.msg, "PATTERNS SAVED"));
    ed_inject(state, sizeof state); assert(!(rx_last[7] & 16u));
    puts("save safety: flash retry, fresh overwrite confirmation, pending vs durable save PASS");
    return 0;
}
