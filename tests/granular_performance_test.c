/* SPDX-License-Identifier: GPL-3.0-only */
#define FELUCCA_SYNTH_PARTS 7
#include "editor_host.c"

static void filter_exact(void)
{
    for (int32_t x = INT16_MIN; x <= INT16_MAX; x++) {
        uint32_t a = (uint32_t)(x < 0 ? -x : x), seg = 0;
        if (a > 32635u) a = 32635u;
        a += 132u;
        for (uint32_t t = a >> 8; t; t >>= 1) seg++;
        uint32_t reference = ((seg << 4) | ((a >> (seg + 3u)) & 15u)) ^ (x < 0 ? 0x7fu : 0xffu);
        assert(gf_encode(x) == reference);
    }
    gf_state_t s = {0};
    uint32_t seed = 17;
    for (uint32_t run = 0; run < 5000u; run++) {
        for (uint32_t k = 0; k < 23u; k++) {
            seed = seed * 1664525u + 1013904223u;
            s.fir[k] = run < 2u ? (run ? INT16_MAX : INT16_MIN) : (int16_t)(seed >> 16);
        }
        for (s.firw = 0; s.firw < 23u; s.firw++) {
            int64_t reference = 0;
            for (uint32_t k = 0, j = s.firw; k < 23u; k++) {
                reference += (int32_t)s.fir[j] * GF_FIR[k];
                if (++j == 23u) j = 0;
            }
            assert(reference >= INT32_MIN && reference <= INT32_MAX);
            assert(gf_capture(&s) == reference);
        }
    }
    puts("capture: all 65536 codec inputs and 115000 FIR rings bit-exact");
}

static void balance_check(void)
{
    uint32_t seed = 11;
    for (uint32_t run = 0; run < 2000u; run++) {
        uint32_t total = 0, largest = 0;
        for (uint32_t t = 0; t < NTRK; t++) {
            seed = seed * 1664525u + 1013904223u;
            unit_ticks[t] = seed % 100000u + 1u;
            total += unit_ticks[t];
            if (unit_ticks[t] > largest) largest = unit_ticks[t];
        }
        for (uint32_t eligible = 0; eligible <= U_ALL; eligible++) {
            uint32_t mask = units_balance(eligible), worker = 0;
            assert(!(mask & ~eligible));
            for (uint32_t t = 0; t < NTRK; t++) if (mask & (1u << t)) worker += unit_ticks[t];
            if (eligible == U_ALL) {
                uint32_t difference = worker > total - worker ? 2u * worker - total : total - 2u * worker;
                assert(difference <= largest);
            }
        }
    }
    memset(unit_ticks, 0, sizeof unit_ticks);
    unit_ticks[0] = unit_ticks[1] = unit_ticks[2] = 1000;
    uint32_t mask = units_balance(U_ALL);
    assert((mask & 7u) && (mask & 7u) != 7u);
    puts("scheduler: 512000 eligibility/cost cases; heavy first three parts use both cores");
}

static uint64_t render_scene(int32_t mask, uint32_t scene)
{
    host_tracks_init(); fm6_init();
    cpu1_on(); cpu1_units = mask;
    for (uint32_t t = 0; t < NPART; t++) {
        host_preset(&trk[t], scene == 3u ? 5u : scene ? (t * 2u + scene) % NENGINES : 0u, 0);
        trk[t].p[P_ITYPE] = IT_GRAIN; grain_defaults(&trk[t]);
        trk[t].p[P_IMIX] = 90; trk[t].p[P_SUS] = 127;
        trk[t].p[P_AMODE] = 0; trk[t].p[P_AHOLD] = 0;
        trk[t].p[P_LWAVE] = 4; trk[t].p[P_LRATE] = 123;
    }
    TDRUM->p[P_ITYPE] = IT_GRAIN; grain_defaults(TDRUM); TDRUM->p[P_IMIX] = 85;
    int32_t out[CTL * 2]; uint64_t hash = 14695981039346656037ull;
    for (uint32_t block = 0; block < 3200u; block++) {
        uint32_t t = block / 43u % NPART;
        if (block % 43u == 0) {
            trk_all_off(&trk[t]); trk_note_on(&trk[t], 48 + block / 43u % 24u, 100);
        }
        if (block % 83u == 0) drum_on(36u, 110);
        if (block == 500u) for (uint32_t k = 0; k < NTRK; k++) trk[k].p[P_GFREEZE] = 1;
        if (block == 700u) for (uint32_t k = 0; k < NTRK; k++) trk[k].p[P_IMIX] = 0;
        if (block == 900u) for (uint32_t k = 0; k < NTRK; k++) trk[k].p[P_IMIX] = 100;
        if (block == 1200u) for (uint32_t k = 0; k < NTRK; k++) trk[k].p[P_GFREEZE] = 0;
        if (block == 1400u) host_preset_req(&trk[0], ENGI_FM6, 1);
        if (block == 1800u) trk[3].p[P_SLCR] = SL_STUT;
        if (block == 2100u) trk[3].p[P_ITYPE] = 0;
        if (block == 2400u) trk[3].p[P_ITYPE] = IT_GRAIN;
        mix_block(out, CTL);
        for (uint32_t k = 0; k < CTL * 2u; k++) hash = (hash ^ (uint32_t)out[k]) * 1099511628211ull;
        assert(voices_busy() <= NVOICE);
    }
    return hash;
}

static uint64_t child_scene(int32_t mask, uint32_t scene)
{
    int fd[2]; assert(!pipe(fd)); pid_t pid = fork(); assert(pid >= 0);
    if (!pid) {
        close(fd[0]); uint64_t hash = render_scene(mask, scene);
        assert(write(fd[1], &hash, sizeof hash) == sizeof hash); _exit(0);
    }
    close(fd[1]); uint64_t hash = 0;
    assert(read(fd[0], &hash, sizeof hash) == sizeof hash); close(fd[0]);
    int status; assert(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    return hash;
}

static void sleeping_check(void)
{
    host_tracks_init(); track_t *t = &trk[0]; gf_state_t *s = &gf[0];
    t->p[P_ITYPE] = IT_GRAIN; grain_defaults(t); t->p[P_IMIX] = 100;
    audio_prepare(); gf_layout();
    int32_t b[CTL]; uint32_t phase = 0;
    for (uint32_t block = 0; block < 1000u; block++) {
        for (uint32_t i = 0; i < CTL; i++) b[i] = osc_sine(phase += 20000000u) / 2;
        gf_insert(t, b, 0, 0, 0, CTL);
    }
    t->p[P_IMIX] = 0;
    for (uint32_t block = 0; block < 8u; block++) { memset(b, 0, sizeof b); gf_insert(t, b, 0, 0, 0, CTL); }
    assert(s->sleeping && !s->mix && !t->ins_run && s->valid);
    gf_state_t parked = *s;
    for (uint32_t block = 0; block < 100u; block++) {
        for (uint32_t i = 0; i < CTL; i++) b[i] = (int32_t)i * 321 - 4000;
        gf_insert(t, b, 0, 0, 0, CTL);
        for (uint32_t i = 0; i < CTL; i++) assert(b[i] == (int32_t)i * 321 - 4000);
        assert(s->wp == parked.wp && s->valid == parked.valid && s->half == parked.half);
        assert(!memcmp(s->fir, parked.fir, sizeof s->fir));
    }
    t->p[P_GFREEZE] = 1; t->p[P_IMIX] = 100;
    uint32_t energy = 0;
    for (uint32_t block = 0; block < 1000u; block++) {
        memset(b, 0, sizeof b); gf_insert(t, b, 0, 0, 0, CTL);
        assert(s->wp == parked.wp && s->valid == parked.valid);
        for (uint32_t i = 0; i < CTL; i++) energy |= (uint32_t)b[i];
    }
    assert(energy && !s->sleeping);
    t->p[P_GFREEZE] = 0;
    memset(b, 0, sizeof b); gf_insert(t, b, 0, 0, 0, CTL);
    assert(s->wp != parked.wp);
    puts("MIX 0: dry bypass parks capture; history survives and FREEZE/LIVE resume");
}

int main(void)
{
    /* Fork before the parent creates a worker or touches the renderer. */
    for (uint32_t scene = 0; scene < 4u; scene++) {
        uint64_t serial = child_scene(0, scene);
        assert(serial == child_scene(-1, scene));
        assert(serial == child_scene(0x55, scene));
        assert(serial == child_scene(U_ALL, scene));
    }
    puts("eight granular instances: serial, automatic and explicit splits give identical audio");
    filter_exact(); balance_check(); sleeping_check();
    puts("granular performance checks PASS");
    return 0;
}
