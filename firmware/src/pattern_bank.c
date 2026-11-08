/* SPDX-License-Identifier: GPL-3.0-only */
/* 24 patterns in 28 sectors: the former four A/B pairs and former USR4.
 * Write payload first and commit header last. Never erase a current record.
 * Old A/B sectors stay protected until their slot has a journal record. */
#define PB_COUNT 24u
#define PB_SECTORS 28u
#define PB_MAGIC 0x50415431u
static int8_t pb_sector[PB_COUNT];
static uint32_t pb_seq[PB_COUNT], pb_length[PB_COUNT], pb_serial;
static uint8_t pb_ready;
static uint32_t pb_off(uint32_t s) { return s < 8u ? 0x97000u + s * ST_SECTOR : 0xE7000u + (s - 8u) * ST_SECTOR; }
UI_SMALL static void pb_scan(void)
{
    uint32_t s;
    st_hdr_t h;
    memset(pb_sector, -1, sizeof pb_sector);
    memset(pb_seq, 0, sizeof pb_seq);
    memset(pb_length, 0, sizeof pb_length);
    pb_serial = 0;
    for (s = 0; s < PB_SECTORS; s++) {
        if (st_read(pb_off(s), &h, sizeof h) || h.magic != PB_MAGIC || h.type >= PB_COUNT || h.slot != s ||
            h.len > ST_PAYLOAD_MAX || h.hcrc != st_crc32(&h, sizeof h - 4u) ||
            st_read(pb_off(s) + ST_PAYLOAD_OFF, st_buf, h.len) || st_crc32(st_buf, h.len) != h.crc)
            continue;
        if (h.seq > pb_serial) pb_serial = h.seq;
        if (pb_sector[h.type] < 0 || h.seq > pb_seq[h.type]) {
            pb_sector[h.type] = (int8_t)s;
            pb_seq[h.type] = h.seq;
            pb_length[h.type] = h.len;
        }
    }
    pb_ready = 1;
}
UI_SMALL static int pb_load(uint32_t slot, void *dst, uint32_t max)
{
    uint32_t n;
    if (!pb_ready) pb_scan();
    if (slot >= PB_COUNT || pb_sector[slot] < 0) return -1;
    n = pb_length[slot] < max ? pb_length[slot] : max;
    return st_read(pb_off((uint32_t)pb_sector[slot]) + ST_PAYLOAD_OFF, dst, n) ? -1 : (int)n;
}
#ifdef ST_XIP_PTR
UI_SMALL static const void *pb_xip(uint32_t slot)
{
    if (!pb_ready) pb_scan();
    return slot < PB_COUNT && pb_sector[slot] >= 0 && pb_length[slot] ?
        ST_XIP_PTR(pb_off((uint32_t)pb_sector[slot]) + ST_PAYLOAD_OFF) : 0;
}
#endif
UI_SMALL static int pb_save(uint32_t slot, const void *src, uint32_t len)
{
    uint32_t s, k, used = 0, off;
    st_hdr_t h;
    if (!pb_ready) pb_scan();
    if (slot >= PB_COUNT || len > ST_PAYLOAD_MAX || pb_serial == 0xFFFFFFFFu) return -1;
    for (k = 0; k < PB_COUNT; k++)
        if (pb_sector[k] >= 0) used |= 1u << (uint32_t)pb_sector[k];
    for (k = 0; k < 4u; k++)
        if (pb_sector[k] < 0) used |= 3u << (k * 2u); /* legacy data is not migrated yet */
    for (k = 0; k < PB_SECTORS; k++) {
        s = (pb_serial + k) % PB_SECTORS;
        if (!(used & (1u << s))) break;
    }
    if (k == PB_SECTORS) return -1;
    if (len && src != st_buf) memcpy(st_buf, src, len);
    memset(&h, 0, sizeof h);
    h.magic = PB_MAGIC; h.type = (uint16_t)slot; h.slot = (uint16_t)s;
    h.seq = pb_serial + 1u; h.len = len; h.crc = st_crc32(st_buf, len);
    h.hcrc = st_crc32(&h, sizeof h - 4u);
    if (st_erase(pb_off(s))) return -1;
    for (off = 0; off < len; off += 256u) {
        uint32_t n = len - off > 256u ? 256u : len - off;
        if (st_prog(pb_off(s) + ST_PAYLOAD_OFF + off, st_buf + off, n)) return -1;
    }
    /* Verify payload before publishing the new record. Source may live in XIP. */
#ifdef ST_XIP_PTR
    if (st_crc32(ST_XIP_PTR(pb_off(s) + ST_PAYLOAD_OFF), len) != h.crc) return -1;
#endif
    if (st_prog(pb_off(s), &h, sizeof h)) return -1;
    pb_serial = h.seq;
    pb_sector[slot] = (int8_t)s; pb_seq[slot] = h.seq; pb_length[slot] = len;
    return 0;
}
