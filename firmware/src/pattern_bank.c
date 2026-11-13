/* SPDX-License-Identifier: GPL-3.0-only */
/* 24 patterns and two alternating working-project roots in 34 sector pairs.
 * USR1 is untouched; USR2/3 supply 20 pairs, the old journal another 14.
 * Legacy records remain protected until their replacement commits. */
#define PB_COUNT 26u
#define PB_SECTORS 28u
#define PB_PAIRS 34u
#define PB_PAYLOAD_MAX (2u * ST_SECTOR - ST_PAYLOAD_OFF)
#define PB_MAGIC 0x50415431u
#define PB_MAGIC_PAIR 0x50415432u
#define PB_PAIR_ID 64u
static int8_t pb_sector[PB_COUNT];
static uint32_t pb_seq[PB_COUNT], pb_length[PB_COUNT], pb_serial;
static uint8_t pb_ready;
static uint32_t pb_off(uint32_t s)
{
    if (s < PB_PAIR_ID) return s < 8u ? 0x97000u + s * ST_SECTOR : 0xE7000u + (s - 8u) * ST_SECTOR;
    s -= PB_PAIR_ID;
    return s < 4u ? 0x97000u + s * 8192u : s < 24u ? 0xB4000u + (s - 4u) * 8192u : 0xE7000u + (s - 24u) * 8192u;
}
static int pb_crc(uint32_t off, uint32_t n, uint32_t *out)
{
    uint8_t b[256];
    uint32_t crc = 0xFFFFFFFFu;
    while (n) {
        uint32_t m = n > sizeof b ? sizeof b : n;
        if (st_read(off, b, m)) return -1;
        crc=st_crc32_more(crc,b,m);
        off += m; n -= m;
    }
    *out = ~crc; return 0;
}
UI_SMALL static void pb_scan(void)
{
    st_hdr_t h;
    memset(pb_sector, -1, sizeof pb_sector);
    memset(pb_seq, 0, sizeof pb_seq); memset(pb_length, 0, sizeof pb_length);
    pb_serial = 0;
    for (uint32_t k = 0; k < PB_SECTORS + PB_PAIRS; k++) {
        uint32_t s = k < PB_SECTORS ? k : PB_PAIR_ID + k - PB_SECTORS, crc;
        uint32_t pair = s >= PB_PAIR_ID;
        if (st_read(pb_off(s), &h, sizeof h) || h.magic != (pair ? PB_MAGIC_PAIR : PB_MAGIC) ||
            h.type >= (pair ? PB_COUNT : 24u) || h.slot != (pair ? s - PB_PAIR_ID : s) ||
            h.len > (pair ? PB_PAYLOAD_MAX : ST_PAYLOAD_MAX) || h.hcrc != st_crc32(&h, sizeof h - 4u) ||
            pb_crc(pb_off(s) + ST_PAYLOAD_OFF, h.len, &crc) || crc != h.crc) continue;
        if (h.seq > pb_serial) pb_serial = h.seq;
        if (pb_sector[h.type] < 0 || h.seq > pb_seq[h.type]) {
            pb_sector[h.type] = (int8_t)s; pb_seq[h.type] = h.seq; pb_length[h.type] = h.len;
        }
    }
    pb_ready = 1;
}
UI_SMALL static int pb_load(uint32_t slot, void *dst, uint32_t max)
{
    if (!pb_ready) pb_scan();
    if (slot >= PB_COUNT || pb_sector[slot] < 0) return -1;
    uint32_t n = pb_length[slot] < max ? pb_length[slot] : max;
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
    uint8_t used[PB_PAIRS] = {0}, b[256];
    st_hdr_t h, verify;
    uint32_t s, k, crc;
    if (!pb_ready) pb_scan();
    if (slot >= PB_COUNT || len > PB_PAYLOAD_MAX || (!src && len) || pb_serial == 0xFFFFFFFFu) return -1;
    for (k = 0; k < PB_COUNT; k++) if (pb_sector[k] >= 0) {
        uint32_t id = (uint32_t)pb_sector[k];
        used[id >= PB_PAIR_ID ? id - PB_PAIR_ID : id < 8u ? id / 2u : 24u + (id - 8u) / 2u] = 1;
    }
    for (k = 0; k < 4u; k++) if (pb_sector[k] < 0) used[k] = 1;
    for (k = 0; k < PB_PAIRS; k++) { s = (pb_serial + k) % PB_PAIRS; if (!used[s]) break; }
    if (k == PB_PAIRS) return -1;
    uint32_t off = pb_off(PB_PAIR_ID + s);
    memset(&h, 0, sizeof h);
    h.magic = PB_MAGIC_PAIR; h.type = (uint16_t)slot; h.slot = (uint16_t)s;
    h.seq = pb_serial + 1u; h.len = len; h.crc = st_crc32(src, len); h.hcrc = st_crc32(&h, sizeof h - 4u);
    if (st_erase(off) || st_erase(off + ST_SECTOR)) return -1;
    for (k = 0; k < len; k += sizeof b) {
        uint32_t n = len - k > sizeof b ? sizeof b : len - k;
        memcpy(b, (const uint8_t *)src + k, n);
        if (st_prog(off + ST_PAYLOAD_OFF + k, b, n)) return -1;
    }
    if (pb_crc(off + ST_PAYLOAD_OFF, len, &crc) || crc != h.crc ||
        st_prog(off, &h, sizeof h) || st_read(off, &verify, sizeof verify) || memcmp(&h, &verify, sizeof h)) return -1;
    pb_serial = h.seq; pb_sector[slot] = (int8_t)(PB_PAIR_ID + s);
    pb_seq[slot] = h.seq; pb_length[slot] = len;
    return 0;
}
static uint32_t pb_autosave_latest(void) { return pb_sector[25] >= 0 && pb_seq[25] > pb_seq[24] ? 25u : 24u; }
static int pb_autosave_save(const void *src, uint32_t n) { return pb_save(pb_autosave_latest() == 24u ? 25u : 24u, src, n); }
