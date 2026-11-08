/* SPDX-License-Identifier: GPL-3.0-only */
#define main storage_original_main
#include "storage_test.c"
#undef main
#include <assert.h>
static uint8_t snapshot[sizeof nor], payload[3816], result[3816], new_record[ST_SECTOR];
static void verify(uint32_t slot, uint8_t value)
{
    uint32_t i;
    assert(pb_load(slot, result, sizeof result) == sizeof result);
    for (i = 0; i < sizeof result; i++) assert(result[i] == value);
}
int main(void)
{
    uint32_t i, n;
    memset(nor, 0xFF, sizeof nor);
    memset(nor + 0xA0000, 0x6D, 3u * 0x14000u);
    for (i = 0; i < 4; i++) {
        memset(payload, (int)i + 1, sizeof payload);
        assert(st_save(OBJ_PROJECT0 + i, payload, sizeof payload) == 0);
    }
    pb_scan();
    for (i = 0; i < 4; i++) {
        assert(st_load(OBJ_PROJECT0 + i, payload, sizeof payload) == sizeof payload);
        assert(pb_save(i, payload, sizeof payload) == 0);
        /* Every not-yet-migrated legacy slot must still be readable. */
        for (n = i + 1; n < 4; n++) assert(st_load(OBJ_PROJECT0 + n, result, sizeof result) == sizeof result);
    }
    for (i = 4; i < 24; i++) {
        memset(payload, (int)i + 1, sizeof payload);
        assert(pb_save(i, payload, sizeof payload) == 0);
    }
    memcpy(snapshot, nor, sizeof nor);
    for (n = 0; n <= 16; n++) {
        memcpy(nor, snapshot, sizeof nor);
        pb_scan();
        memset(payload, 99, sizeof payload);
        fail_after = (int)n;
        int rc = pb_save(23, payload, sizeof payload);
        fail_after = -1;
        pb_scan(); /* simulate loss of all RAM state */
        for (i = 0; i < 23; i++) verify(i, (uint8_t)(i + 1));
        verify(23, rc ? 24 : 99);
        for (i = 0xA0000; i < 0xDC000; i++) assert(nor[i] == 0x6D);
    }
    /* Power may disappear partway through programming the commit header itself. */
    memcpy(nor, snapshot, sizeof nor); pb_scan();
    memset(payload, 99, sizeof payload);
    assert(pb_save(23, payload, sizeof payload) == 0);
    uint32_t target = pb_off((uint32_t)pb_sector[23]);
    memcpy(new_record, nor + target, ST_SECTOR);
    for (n = 0; n < sizeof(st_hdr_t); n++) {
        memcpy(nor, snapshot, sizeof nor);
        memset(nor + target, 0xFF, ST_SECTOR);
        memcpy(nor + target + ST_PAYLOAD_OFF, new_record + ST_PAYLOAD_OFF, sizeof payload);
        memcpy(nor + target, new_record, n);
        pb_scan();
        for (i = 0; i < 24; i++) verify(i, (uint8_t)(i + 1));
    }
    for (n = 0; n < 200; n++) {
        i = n % 24;
        memset(payload, (int)i + 1, sizeof payload);
        assert(pb_save(i, payload, sizeof payload) == 0);
    }
    pb_scan();
    for (i = 0; i < 24; i++) verify(i, (uint8_t)(i + 1));
    assert(pb_save(23, 0, 0) == 0);
    pb_scan(); assert(pb_load(23, result, sizeof result) == 0);
    assert(pb_save(24, payload, sizeof payload) < 0);
    puts("24 pattern journal: migration, full capacity, every torn-write page, rotation, tombstones, USR1-3: PASS");
    return 0;
}
