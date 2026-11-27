/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio)
 * Copyright (C) 2026 Adrien Tibi (Recursive Field Instruments)
 * Ported from Melodee; source revisions and MPL permission: see THIRD_PARTY_NOTICES.md. */
/* USB MIDI output in host mode. The main loop owns EP0 and switches roles.
 * TIMER5 owns MIDI transfers while connected. USB starts in the device role.
 * The FM-1 needs an externally powered USB hub/adapter for bus-powered synths. */
#include "usb_host_desc.h"

enum { S_DEVCTL = 15, S_TXTYPE = 24, S_TXINTERVAL = 25 };
enum { UH_MSG_NONE, UH_MSG_ON, UH_MSG_OFF, UH_MSG_MIDI, UH_MSG_NOMIDI, UH_MSG_LOW, UH_MSG_LOST };
#define UH_FIRST_ADDR 1u
#define UH_MAX_ERRORS 16u
#define UH_ROOT_TRIES 5u                                /* resets of a device that does not answer at all (uh_connect) */

#define uh_links (usb_payload.host)

static struct {
    volatile uint8_t want, on, run, fault, control, tx_count, active, discovered;
    uint8_t multi, seen, next_addr, dev, tried;
    uint8_t hub_addr, hub_mps, hub_ports, hub_poll_port;
    uint8_t current, round, tx_errors, line_idle, tx_cleanup, msg, answered;
    uint32_t t_ms, retry_ms, tx_ms, hub_poll_ms;
} uh;

static void usb_host_switch(uint32_t on) { if (!usb.detached) uh.want = on != 0; }

/* ------------------------------------------------ main loop: control --- */
static void uh_wait(uint32_t ms)                        /* main loop only */
{
    uint32_t t = fm1_ms;
    while (fm1_ms - t < ms)
        fm1_wdt_feed();
}

/* CSR0 (host) until want_set bits are all set and want_clr are all clear: >= 0 the CSR0, < 0 a stall / error / timeout */
static int uh_csr0(uint32_t want_set, uint32_t want_clr)
{
    uint32_t t = fm1_ms, v;
    for (;;) {
        v = sie_rd(S_CSR0);
        if (v & 0x04u) {                                /* RxStall */
            sie_wr(S_CSR0, 0);
            return -2;
        }
        if (v & 0x10u) {                                /* Error: no answer */
            sie_wr(S_CSR0, 0);
            return -1;
        }
        if ((v & want_set) == want_set && !(v & want_clr))
            return (int)v;
        if (fm1_ms - t > 500u)
            return -3;
        fm1_wdt_feed();
    }
}

/* One control transfer to `addr`: SETUP, the IN data stage if any, the status stage. Bytes read, or < 0.
 * Only requests without an OUT data stage (every one used here). */
static int uh_ctl_locked(uint8_t addr, uint8_t rt, uint8_t rq, uint16_t val, uint16_t idx, uint16_t len, uint8_t *out,
                  uint8_t mps)
{
    uint32_t got = 0, n, i;
    int r;
    sie_wr(S_FADDR, addr);
    sie_wr(S_INDEX, 0);
    ep0buf[0] = rt;
    ep0buf[1] = rq;
    ep0buf[2] = (uint8_t)val;
    ep0buf[3] = (uint8_t)(val >> 8);
    ep0buf[4] = (uint8_t)idx;
    ep0buf[5] = (uint8_t)(idx >> 8);
    ep0buf[6] = (uint8_t)len;
    ep0buf[7] = (uint8_t)(len >> 8);
    fm1_usb_ep0_send(ep0buf, 8);
    sie_wr(S_CSR0, 0x0Au);                              /* SetupPkt | TxPktRdy */
    r = uh_csr0(0, 0x02u);
    if (r < 0)
        return r;
    if (len && (rt & 0x80u)) {
        while (got < len) {
            sie_wr(S_CSR0, 0x20u);                      /* ReqPkt */
            r = uh_csr0(0x01u, 0);
            if (r < 0)
                return r;
            n = sie_rd(S_COUNT0);
            if (n > 64u)
                n = 64u;
            fm1_usb_rx_sync();
            for (i = 0; i < n && got + i < len; i++)
                out[got + i] = ep0buf[i];
            sie_wr(S_CSR0, 0);                          /* RxPktRdy done */
            got += n;
            if (n < mps)
                break;
        }
        if (got > len)
            got = len;
        sie_wr(S_CSR0, 0x42u);                          /* status: StatusPkt | TxPktRdy (zero-length OUT) */
        r = uh_csr0(0, 0x02u);
    } else {
        sie_wr(S_CSR0, 0x60u);                          /* status: StatusPkt | ReqPkt (zero-length IN) */
        r = uh_csr0(0x01u, 0);
        if (r >= 0)
            sie_wr(S_CSR0, 0);
    }
    return r < 0 ? r : (int)got;
}

/* Main-loop control transfers and TIMER5 share the address and endpoint index. */
static void uh_take(void)
{
    uh.control = 1;
    RING_PUBLISH();
    while (uh.tx_count && !uh.fault)
        fm1_wdt_feed();
}
static void uh_give(void)
{
    RING_PUBLISH();
    uh.control = 0;
}

static int uh_ctl(uint8_t addr, uint8_t rt, uint8_t rq, uint16_t val, uint16_t idx, uint16_t len,
                  uint8_t *out, uint8_t mps)
{
    uh_take();
    int r = uh.fault ? -1 : uh_ctl_locked(addr, rt, rq, val, idx, len, out, mps);
    uh_give();
    return r;
}

static void uh_bind(uint32_t slot, const uh_midi_t *m)
{
    uh_link_t *d = &uh_links[slot];
    uh_take();
    d->addr = uh.dev;
    d->mps = (uint8_t)m->out_mps;
    d->type = (uint8_t)(m->out_type << 4 | m->out_ep);
    d->interval = m->out_type == 3u ? m->out_interval : (uh.multi ? 4u : 0u);
    d->toggle = 0;
    d->cleanup_at = 0;
    uh.active |= (uint8_t)(1u << slot);
    uh_give();
}

/* ---------------------------------------------------- enumeration --- */
static int uh_hub_probe(uint8_t addr, uint8_t mps, uint8_t cfgval, uh_midi_t *m);

/* The device at address 0 (just reset): give it an address, read its configuration, and if it has a USB-MIDI
 * streaming interface select it. 1 = found, 0 = no (including a hub below depth 0 and a device that fails). */
static int uh_probe(uint8_t depth, uh_midi_t *m)
{
    uint8_t cfg[320];                                   /* a configuration, cut short if longer */
    uint8_t d8[8], mps, cls, addr = uh.next_addr++;
    uint32_t total;
    int r;
    r = uh_ctl(0, 0x80, 6, 0x0100, 0, 8, d8, 8);        /* GET_DESCRIPTOR(device), first 8 bytes */
    if (r < 8)
        return 0;
    if (!depth)
        uh.answered = 1;
    mps = d8[7] == 16u || d8[7] == 32u || d8[7] == 64u ? d8[7] : 8u;
    cls = d8[4];
    if (uh_ctl(0, 0x00, 5, addr, 0, 0, 0, mps) < 0)     /* SET_ADDRESS */
        return 0;
    uh_wait(10);                                        /* the device settles to its new address */
    r = uh_ctl(addr, 0x80, 6, 0x0200, 0, 9, cfg, mps);
    if (r < 9)
        return 0;
    total = (uint32_t)(cfg[2] | cfg[3] << 8);
    if (total > sizeof cfg)
        total = sizeof cfg;
    if (total > 9u) {
        r = uh_ctl(addr, 0x80, 6, 0x0200, 0, (uint16_t)total, cfg, mps);
        if (r < 9)
            return 0;
        total = (uint32_t)r;
    }
    if (cls == 9u)                                      /* a hub */
        return depth == 0u ? uh_hub_probe(addr, mps, cfg[5], m) : 0;
    if (!uh_parse_midi(cfg, total, m))
        return 0;
    if (uh_ctl(addr, 0x00, 9, m->cfg, 0, 0, 0, mps) < 0)   /* SET_CONFIGURATION */
        return 0;
    if (m->alt && uh_ctl(addr, 0x01, 11, m->alt, m->iface, 0, 0, mps) < 0)   /* SET_INTERFACE */
        return 0;
    uh.dev = addr;
    return 1;
}

/* Probe only this downstream port; keep the hub and the other devices configured. Each port reuses its
 * address on reconnect, so repeated hot-plugs cannot exhaust the USB address space. */
static int uh_hub_port_probe(uint8_t p, uint32_t st, uh_midi_t *m)
{
    uint8_t b[4], addr = uh.hub_addr, mps = uh.hub_mps;
    uint32_t t;
    if (!(st & 0x0001u) || (st & 0x0200u))              /* nothing there, or low speed */
        return 0;
    if (uh_ctl(addr, 0x23, 3, 4, p, 0, 0, mps) < 0)    /* SET_FEATURE(PORT_RESET) */
        return 0;
    t = fm1_ms;
    for (;;) {
        if (uh_ctl(addr, 0xA3, 0, 0, p, 4, b, mps) >= 4 && (b[2] & 0x10u))   /* C_PORT_RESET */
            break;
        if (fm1_ms - t > 500u)
            return 0;
        uh_wait(10);
    }
    uh_ctl(addr, 0x23, 1, 20, p, 0, 0, mps);            /* CLEAR_FEATURE(C_PORT_RESET) */
    if (uh_ctl(addr, 0xA3, 0, 0, p, 4, b, mps) < 4 || !(b[0] & 0x02u))   /* enabled? */
        return 0;
    uh_wait(20);                                        /* reset recovery */
    uh.next_addr = (uint8_t)(UH_FIRST_ADDR + p);
    return uh_probe(1, m);
}

/* Read and acknowledge connection changes, including disconnects and unsupported devices. An unresponsive
 * hub is left alone until root unplug/replug, rather than repeating a blocking control timeout forever. */
static int uh_hub_status(uint8_t p, uint8_t *b)
{
    if (uh_ctl(uh.hub_addr, 0xA3, 0, 0, p, 4, b, uh.hub_mps) < 4 ||
        ((b[2] & 1u) && uh_ctl(uh.hub_addr, 0x23, 1, 16, p, 0, 0, uh.hub_mps) < 0)) {
        return 0;
    }
    return 1;
}

/* Configure and power the root hub once. After the initial scan, idle polling uses connection-change bits. */
static int uh_hub_probe(uint8_t addr, uint8_t mps, uint8_t cfgval, uh_midi_t *m)
{
    uint8_t b[16], ports, p;                            /* (the hub descriptor is 9 bytes with <= 7 ports) */
    uint32_t good;
    if (uh_ctl(addr, 0x00, 9, cfgval, 0, 0, 0, mps) < 0)
        return 0;
    if (uh_ctl(addr, 0xA0, 6, 0x2900, 0, 9, b, mps) < 7)   /* hub descriptor */
        return 0;
    ports = b[2] > 7u ? 7u : b[2];
    good = (uint32_t)b[5] * 2u;                         /* bPwrOn2PwrGood, in 2 ms */
    for (p = 1; p <= ports; p++)
        uh_ctl(addr, 0x23, 3, 8, p, 0, 0, mps);         /* SET_FEATURE(PORT_POWER) */
    uh_wait((good < 100u ? 100u : good) + 100u);        /* power good, then the devices announce themselves */
    uh.hub_addr = addr;
    uh.hub_mps = mps;
    uh.hub_ports = ports;
    uh.hub_poll_port = 1;
    uh.hub_poll_ms = fm1_ms;
    for (p = 1; p <= ports; p++) {
        if (uh.multi && p > UH_PORTS)
            break;
        if (!uh_hub_status(p, b))
            break;
        if (b[0] & 1u)
            uh.discovered |= (uint8_t)(1u << (p - 1u));
        if (uh_hub_port_probe(p, (uint32_t)(b[0] | b[1] << 8), m)) {
            uh_bind(uh.multi ? p - 1u : 0u, m);
            if (!uh.multi)
                break;
        }
    }
    return uh.active != 0;
}

static void uh_start_ep(void)
{
    sie_wr(S_INDEX, 1);
    fm1_usb_ep_txbuf(1, ep1tx);
    sie_wr(S_TXMAXP, 0xFF);
    sie_wr(S_TXCSR1, 0x48);                             /* clear data toggle and flush FIFO */
    fm1_usb_ep_enable(1u << 1);
    uh.line_idle = uh.tx_errors = uh.round = 0;
    RING_PUBLISH();
    uh.run = 1;
}

static void uh_track_packet(uh_link_t *d, uint32_t uncertain)
{
    for (uint32_t i = 0; i < 4u * uh.tx_count; i += 4u) {
        uint32_t st = ep1tx[i + 1], kind = st & 0xF0u;
        if (st == 0xFAu || st == 0xFBu) {
            d->clock = 1;
        } else if (st == 0xFCu && !uncertain) {
            d->clock = 0;
        } else if (kind == 0x90u || kind == 0x80u) {
            uint32_t note = ep1tx[i + 2] & 127u;
            uint32_t *word = &d->notes[(st & 15u) * 4u + note / 32u];
            uint32_t bit = 1u << (note & 31u);
            if (kind == 0x90u && ep1tx[i + 3])
                *word |= bit;
            else if (!uncertain)
                *word &= ~bit;
        }
    }
}

static void uh_drop(uint32_t slot)
{
    uh_link_t *d = &uh_links[slot];
    uh.active &= (uint8_t)~(1u << slot);
    d->read = d->write;
    d->cleanup_at = 0;
    d->cleanup = 1;
}

/* Keep note ownership when an OUT packet may have reached the synth. */
static void uh_session_end(void)
{
    uh.run = 0;
    RING_PUBLISH();
    uh_track_packet(&uh_links[uh.current], 1);
    uh.tx_count = 0;
    uh.discovered = 0;
    for (uint32_t i = 0; i < UH_PORTS; i++)
        uh_drop(i);
    mo_r = mo_w;
}

/* the controller's port as a host with nothing attached yet */
static void uh_host_up(void)
{
    usb.timeouts = 0;
    fm1_usb_reset();
    uh_wait(25);
    fm1_usb_host_attach(ep0buf);
    uh_wait(25);
    sie_wr(S_INTRUSBE, 0);
    sie_wr(S_INTRTX1E, 0);
    sie_wr(S_INTRTX2E, 0);
    sie_wr(S_INTRRX1E, 0);
    sie_wr(S_INTRRX2E, 0);
    fm1_usb_ep_enable(0x1Fu);
    sie_wr(S_FADDR, 0);
    sie_wr(S_DEVCTL, 0x03u);                            /* session + host request */
    uh.seen = 0;
    uh.line_idle = 0;
    uh.hub_addr = 0;
}

/* a device is on the line: reset the bus and find the MIDI endpoint */
static void uh_connect(void)
{
    uh_midi_t m;
    uint32_t t;
    int ok = 0;
    if (sie_rd(S_DEVCTL) & 0x20u) {                     /* low speed (FSDev is bit 6, LSDev bit 5) */
        if (!uh.tried)
            uh.msg = UH_MSG_LOW;                        /* said once per attach, as NO MIDI is */
        uh.tried = 1;
        return;
    }
    /* A root reset of 50 ms (USB 2.0 TDRSTR), then the device's recovery. A device that does not answer its first request
     * at all is reset again with a longer recovery (100, 200, 400, 800, 1600 ms): an Elektron Digitone II answers nothing
     * after 30 ms of reset and 30 ms of recovery. One that answered and failed later is not reset again. (Melodee's
     * 6e615760 does the same for hubs slow to start.) */
    for (t = 0; t < UH_ROOT_TRIES; t++) {
        sie_wr(S_POWER, 0x08u);                         /* bus reset */
        uh_wait(50);
        sie_wr(S_POWER, 0);
        uh_wait(100u << t);
        uh.next_addr = UH_FIRST_ADDR;
        uh.answered = 0;
        if ((ok = uh_probe(0, &m)) || uh.answered)
            break;
    }
    if (ok) {
        if (!uh.hub_addr)
            uh_bind(0, &m);
        uh_start_ep();
        uh.msg = UH_MSG_MIDI;
        uh.tried = 0;
    } else {
        if (!uh.tried)
            uh.msg = UH_MSG_NOMIDI;                     /* said once per attach */
        uh.tried = 1;
    }
}

/* Main loop. usb_host_switch requests the mode; this finds and keeps a controller. */
static void usb_host_task(uint32_t now)
{
    if (usb.detached)
        return;
    if (uh.want != uh.on) {
        if (!uh.want && uh.run && !uh.fault) {          /* let TIMER5 end sounding notes while the bus responds */
            return;
        }
        usb.up = 0;                                     /* TIMER5 leaves the SIE to this context */
        RING_PUBLISH();
        usb.config = 0;
        usb.suspended = 0;
        if (uh.on)
            uh_session_end();                          /* only the host owns these bytes now */
        uh.fault = 0;
        uh.tried = 0;
        if (uh.want) {
#if FELUCCA_UAC
            uac_stream(0);
            uac.feed = 0;
#endif
#if FELUCCA_CDC
            cdc.dtr = cdc.e0_rx = cdc.rx_pend = 0;
#endif
#if FELUCCA_OTA
            so_r = so_w;
            sx_ready = sx_collect = sx_busy = 0;
            usb.ota_req = 0;
#endif
            usb.sx_on = 0;
            fm1_usb_off();
            mo_r = mo_w;
            memset(uh_links, 0, sizeof uh_links);
            RING_PUBLISH();
            uh.on = 1;
            RING_PUBLISH();
            uh_host_up();
            uh.retry_ms = now;
            uh.msg = UH_MSG_ON;
        } else {
            fm1_usb_off();
            memset(&usb_payload.device, 0, sizeof usb_payload.device);
            RING_PUBLISH();
            uh.on = 0;
            usb_retry_ms = now - 1000u;                 /* usb_retry makes the port a device again at once */
            uh.msg = UH_MSG_OFF;
        }
        return;
    }
    if (!uh.on)
        return;
    if (uh.fault) {                                     /* stall, or the controller stopped answering (TIMER5 cleared run) */
        uh_session_end();
        uh.fault = 0;
        uh_host_up();
        uh.retry_ms = now + 500u;
        uh.msg = UH_MSG_LOST;
        return;
    }
    if (uh.run && (!uh.multi || !uh.hub_addr || !uh.want))
        return;
    if ((int32_t)(now - uh.t_ms) < 20 || (int32_t)(now - uh.retry_ms) < 0)
        return;
    uh.t_ms = now;
    if (!fm1_usb_line()) {
        uh.seen = 0;
        if (uh.line_idle < 5u) uh.line_idle++;
        if (uh.line_idle >= 5u && uh.tried) {           /* debounce root disconnect while idle too */
            uh.tried = 0;
            uh.hub_addr = 0;
            uh.msg = UH_MSG_ON;                        /* waiting for a fresh connection */
        }
        return;
    }
    uh.line_idle = 0;
    if (uh.hub_addr && (uh.tried || uh.multi)) {
        if (uh.hub_ports && (uint32_t)(now - uh.hub_poll_ms) >= 40u) {
            uint8_t b[4], p = uh.hub_poll_port;
            uint32_t slot = uh.multi ? p - 1u : 0u;
            uint32_t bit = 1u << (p - 1u);
            uh_midi_t m;
            uh.hub_poll_ms = now;
            uh.hub_poll_port = p >= uh.hub_ports || (uh.multi && p == UH_PORTS) ? 1u : (uint8_t)(p + 1u);
            if (uh_hub_status(p, b)) {
                uint32_t probe = (b[0] & 1u) && (b[2] & 1u);
                if (uh.multi) {
                    uh_take();
                    if (!(b[0] & 1u) || (b[2] & 1u)) {
                        uh_drop(slot);
                        uh.discovered &= (uint8_t)~bit;
                    }
                    probe = (b[0] & 1u) && !(uh.discovered & bit);
                    if (probe)
                        uh.discovered |= (uint8_t)bit;
                    uh_give();
                }
                if (probe && !uh.fault) {
                    if (uh_hub_port_probe(p, (uint32_t)(b[0] | b[1] << 8), &m)) {
                        uh_bind(slot, &m);
                        if (!uh.run)
                            uh_start_ep();
                        uh.msg = UH_MSG_MIDI;
                        uh.tried = 0;
                    }
                }
            } else {
                uh.fault = 1;
            }
        }
        return;
    }
    if (uh.tried)
        return;                                        /* direct failures wait for root unplug/replug */
    if (uh.seen < 5u) {                                 /* 100 ms of a steady line before the bus reset */
        uh.seen++;
        return;
    }
    uh.seen = 0;
    uh_connect();
}

/* ------------------------------------------------------ TIMER5 ISR --- */
static void uh_fail(void)
{
    uh.run = 0;
    uh.fault = 1;
}

static void uh_enqueue(uint32_t slot, uint32_t packet)
{
    uh_link_t *d = &uh_links[slot];
    if (!(uh.active & (1u << slot)) || d->cleanup)
        return;
    if ((uint8_t)(d->write - d->read) >= MQ) {
        d->cleanup = 1;
        return;
    }
    d->queue[d->write % MQ] = packet & ~0xF0u;
    RING_PUBLISH();
    d->write++;
}

static void uh_midi_out(uint32_t packet)
{
    if (!uh.run || !uh.want)
        return;
    if (!uh.multi || !uh.hub_addr) {
        uh_enqueue(0, packet);
    } else if ((packet & 15u) == 15u) {                  /* clock and transport are shared */
        for (uint32_t i = 0; i < UH_PORTS; i++)
            uh_enqueue(i, packet);
    } else if ((packet >> 4 & 15u) < UH_PORTS) {
        uh_enqueue(packet >> 4 & 15u, packet);
    }
}

/* ACK advances the ring and the next DATA PID. Reprogram that PID when sharing EP1. */
static void uh_tx_poll(void)
{
    uh_link_t *d = &uh_links[uh.current];
    uint32_t n = 0;
    if (uh.tx_count) {
        uint32_t csr = sie_rd(S_TXCSR1);
        uint32_t shared = uh.multi && uh.hub_addr;
        if ((csr & 0x20u) || fm1_ms - uh.tx_ms >= (shared ? 8u : 500u) ||
            ((csr & 4u) && ++uh.tx_errors >= UH_MAX_ERRORS)) {
            if (!shared) {
                uh_fail();
                return;
            }
            uh_track_packet(d, 1);
            sie_wr(S_TXCSR1, 8u);                       /* abort this FIFO before selecting a different address */
            uh_drop(uh.current);
            uh.discovered &= (uint8_t)~(1u << uh.current);
        } else if (csr & 0x84u) {
            sie_wr(S_TXCSR1, 1u);                       /* retry the same packet and data toggle */
            return;
        } else if (csr & 3u) {
            return;
        } else {
            uh_track_packet(d, 0);
            d->toggle ^= 1u;
            if (!uh.tx_cleanup)
                d->read += uh.tx_count;
        }
        uh.tx_count = 0;
    }
    if (uh.control)
        return;
    if (!uh.active) {
        uh.run = 0;
        return;
    }
    uint32_t slot = uh.round;
    for (uint32_t i = 0; i < UH_PORTS; i++) {
        d = &uh_links[slot];
        if ((uh.active & (1u << slot)) && (!uh.want || d->cleanup || d->read != d->write))
            break;
        if (++slot == UH_PORTS)
            slot = 0;
    }
    if (!((uh.active & (1u << slot)) && (!uh.want || d->cleanup || d->read != d->write)))
        return;
    uh.round = (uint8_t)(slot + 1u == UH_PORTS ? 0u : slot + 1u);
    uh.tx_cleanup = d->cleanup || !uh.want;
    while (n + 4u <= d->mps) {
        uint32_t p;
        if (uh.tx_cleanup) {
            d->read = d->write;
            p = 0;
            if (!n && d->clock) {
                p = 0xFC0Fu;
            } else while (d->cleanup_at < 2048u) {
                uint32_t at = d->cleanup_at++;
                uint32_t word = d->notes[at / 32u];
                if (word & (1u << (at & 31u))) {
                    p = 0x8008u | (at / 128u) << 8 | (at & 127u) << 16;
                    break;
                }
                if (!word)
                    d->cleanup_at = (uint16_t)((at + 32u) & ~31u);
            }
            if (!p)
                break;
        } else {
            if ((uint8_t)(d->read + n / 4u) == d->write)
                break;
            p = d->queue[(d->read + n / 4u) % MQ];
        }
        ep1tx[n++] = (uint8_t)p;
        ep1tx[n++] = (uint8_t)(p >> 8);
        ep1tx[n++] = (uint8_t)(p >> 16);
        ep1tx[n++] = (uint8_t)(p >> 24);
    }
    if (!n) {
        d->cleanup_at = d->cleanup = 0;
        if (!uh.want) {
            uh.active &= (uint8_t)~(1u << slot);
            uh.run = uh.active != 0;
        }
        return;
    }
    uh.current = (uint8_t)slot;
    sie_wr(S_FADDR, d->addr);
    sie_wr(S_TXTYPE, d->type);
    sie_wr(S_TXINTERVAL, d->interval);
    sie_wr(S_TXCSR2, uh.multi && uh.hub_addr ? 0x22u | d->toggle : 0x20u);
    uh.tx_errors = 0;
    uh.tx_count = (uint8_t)(n / 4u);
    uh.tx_ms = fm1_ms;
    fm1_usb_ep_send(1, ep1tx, n);
    sie_wr(S_TXCSR1, 1u);
}

static void uh_poll(void)
{
    if (uh.fault || (uh.control && !uh.tx_count))
        return;
    if (!fm1_usb_line()) {
        /* EOP also drives SE0 briefly. Require 8 ms of idle samples. */
        if (++uh.line_idle >= 16u) uh_fail();
        return;
    }
    uh.line_idle = 0;
    sie_wr(S_INDEX, 1);
    uh_tx_poll();
}
