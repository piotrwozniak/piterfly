/* SPDX-License-Identifier: GPL-3.0-only */
/* 6+2: 44 sector pairs shared by current patterns and eight named snapshots.
 * All four former user-sample banks are reclaimed. Other layouts keep USR1.
 * Snapshot records are immutable; new roots commit after their payloads. */
#if NPART == 6 && defined(ST_XIP_PTR)
#define PB_COUNT 35u
#define PB_PAIRS 44u
#define SONG_SLOTS 8u
#define SONG_ACTIVE 34u
typedef struct { uint32_t settings_seq, mode; char name[16]; uint8_t ref[26]; arr_config_t order; } song_root_t;
static uint8_t pb_full;
#else
#define PB_COUNT 26u
#define PB_PAIRS 34u
#endif
#define PB_SECTORS 28u
#define PB_PAYLOAD_MAX (2u * ST_SECTOR - ST_PAYLOAD_OFF)
#define PB_MAGIC 0x50415431u
#define PB_MAGIC_PAIR 0x50415432u
#define PB_PAIR_ID 64u
static int8_t pb_sector[PB_COUNT];
static uint32_t pb_seq[PB_COUNT], pb_length[PB_COUNT], pb_serial;
static uint8_t pb_ready;
UI_SMALL static __attribute__((noinline)) uint32_t pb_off(uint32_t s)
{
    if (s < PB_PAIR_ID) return s < 8u ? 0x97000u + s * ST_SECTOR : 0xE7000u + (s - 8u) * ST_SECTOR;
    s -= PB_PAIR_ID;
    if(s>=34u) return 0xA0000u+(s-34u)*8192u;
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
#if NPART == 6 && defined(ST_XIP_PTR)
UI_SMALL static const song_root_t *pb_root(uint32_t id)
{
    return id<PB_COUNT && pb_sector[id]>=0 && pb_length[id]==sizeof(song_root_t) ?
        (const song_root_t *)ST_XIP_PTR(pb_off((uint32_t)pb_sector[id])+ST_PAYLOAD_OFF):0;
}
UI_SMALL static int pb_ref(uint32_t pair,uint32_t slot,st_hdr_t *h)
{
    if(pair>=PB_PAIRS || st_read(pb_off(PB_PAIR_ID+pair),h,sizeof *h) || h->magic!=PB_MAGIC_PAIR ||
       h->slot!=pair || (slot<24u?h->type!=slot:(h->type<24u || h->type>25u)) ||
       h->len>PB_PAYLOAD_MAX || h->hcrc!=st_crc32(h,sizeof *h-4u)) return -1;
    uint32_t crc; return pb_crc(pb_off(PB_PAIR_ID+pair)+ST_PAYLOAD_OFF,h->len,&crc) || crc!=h->crc ? -1:0;
}
UI_SMALL static int pb_root_valid(const song_root_t *root)
{
    if(!root || !arr_valid(&root->order,ARR_READY_MASK)) return 0;
    st_hdr_t h;
    for(uint32_t i=0;i<26u;i++) if(root->ref[i]!=255u && pb_ref(root->ref[i],i,&h)) return 0;
    return 1;
}
UI_SMALL static void pb_mount(const song_root_t *root,uint32_t seq)
{
    st_hdr_t h;
    for(uint32_t i=0;i<26u;i++) if(pb_seq[i]<=seq) {
        pb_sector[i]=-1; pb_seq[i]=pb_length[i]=0;
        if(root->ref[i]!=255u && !pb_ref(root->ref[i],i,&h)) {
            pb_sector[i]=(int8_t)(PB_PAIR_ID+root->ref[i]); pb_seq[i]=h.seq; pb_length[i]=h.len;
        }
    }
}
UI_SMALL static void pb_used(uint8_t *used)
{
    memset(used,0,PB_PAIRS);
    for(uint32_t i=0;i<PB_COUNT;i++) if(pb_sector[i]>=0) {
        uint32_t s=(uint32_t)pb_sector[i];
        used[s>=PB_PAIR_ID?s-PB_PAIR_ID:s<8u?s/2u:24u+(s-8u)/2u]=1;
        if(i>=26u && i<34u) {
            const song_root_t *r=pb_root(i);
            if(r) for(uint32_t j=0;j<26u;j++) if(r->ref[j]<PB_PAIRS) used[r->ref[j]]=1;
        }
    }
}
UI_SMALL static uint32_t pb_free(void)
{
    uint8_t used[PB_PAIRS]; pb_used(used);
    uint32_t n=0; for(uint32_t k=0;k<PB_PAIRS;k++) n+=!used[k]; return n;
}
#endif
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
#if NPART == 6 && defined(ST_XIP_PTR)
        /* Incompatible projects must not occupy invisible working slots. */
        if(h.type<26u && h.len && *(const uint32_t *)ST_XIP_PTR(pb_off(s)+ST_PAYLOAD_OFF)!=0x46554E42u) continue;
#endif
        if (h.seq > pb_serial) pb_serial = h.seq;
        if (pb_sector[h.type] < 0 || h.seq > pb_seq[h.type]) {
            pb_sector[h.type] = (int8_t)s; pb_seq[h.type] = h.seq; pb_length[h.type] = h.len;
        }
    }
#if NPART == 6 && defined(ST_XIP_PTR)
    const song_root_t *root=pb_root(SONG_ACTIVE);
    if(root) pb_mount(root,pb_seq[SONG_ACTIVE]);
#endif
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
#if NPART == 6 && defined(ST_XIP_PTR)
    pb_full=0;
    if(slot<24u && !len && pb_sector[slot]<0) return 0;
    if(slot<PB_COUNT && pb_sector[slot]>=0 && pb_length[slot]==len && (!len || !memcmp(pb_xip(slot),src,len))) return 0;
#endif
    if (slot >= PB_COUNT || len > PB_PAYLOAD_MAX || (!src && len) || pb_serial == 0xFFFFFFFFu) return -1;
#if NPART == 6 && defined(ST_XIP_PTR)
    pb_used(used);
    uint32_t free=0; for(k=0;k<PB_PAIRS;k++) free+=!used[k];
    /* Keep the last block for deleting a song even when normal saves are full. */
    if(free<2u && !(slot>=26u && slot<34u && !len) && !(slot==SONG_ACTIVE && pb_sector[SONG_ACTIVE]>=0)) { pb_full=1; return -1; }
#else
    for (k = 0; k < PB_COUNT; k++) if (pb_sector[k] >= 0) {
        uint32_t id = (uint32_t)pb_sector[k];
        used[id >= PB_PAIR_ID ? id - PB_PAIR_ID : id < 8u ? id / 2u : 24u + (id - 8u) / 2u] = 1;
    }
    for (k = 0; k < 4u; k++) if (pb_sector[k] < 0) used[k] = 1;
#endif
    for (k = 0; k < PB_PAIRS; k++) { s = (pb_serial + k) % PB_PAIRS; if (!used[s]) break; }
    if (k == PB_PAIRS) {
#if NPART == 6 && defined(ST_XIP_PTR)
        pb_full=1;
#endif
        return -1;
    }
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
static int pb_autosave_save(const void *src, uint32_t n) {
#if NPART == 6 && defined(ST_XIP_PTR)
    if(!pb_ready) pb_scan();
    uint32_t id=pb_autosave_latest();
    if(pb_sector[id]>=0 && pb_length[id]==n && !memcmp(pb_xip(id),src,n)) return 0;
#endif
    return pb_save(pb_autosave_latest() == 24u ? 25u : 24u, src, n);
}

static const char *pb_error(void)
{
#if NPART == 6 && defined(ST_XIP_PTR)
    if(pb_full) return "NO SPACE: EXPORT/DELETE";
#endif
    return "SAVE ERROR";
}
