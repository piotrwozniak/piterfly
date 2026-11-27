/* SPDX-License-Identifier: GPL-3.0-only */
/* Run the real USB code against the AC79 register and DMA interface. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#define __attribute__(x)
#define FELUCCA_USB_HOST 1
#define FELUCCA_UAC 1
#define FELUCCA_OTA 1
#define HALF_FRAMES 128u
#define RING_PUBLISH() ((void)0)
#include "../firmware/hal/fm1_usb.h"

static volatile uint32_t fm1_ms;
static uint8_t regs[32], setup[8];
static uint8_t *control_dma;
static uint32_t read_reg, line_state = 1, host_starts, device_starts;
static uint32_t sent[16], sent_count, sends;
static uint8_t address, hub, port_reset, port_power, port_enable, midi_alt, probing_port, hub_error;
static uint8_t attached = 1, changed, expected_pid[8], sent_to, sent_pid;
static uint32_t delivered[8][128], delivered_n[8];
static void mock_wdt(void);

static const uint8_t midi_config[] = {
    9, 2, 59, 0, 3, 1, 0, 0x80, 50,
    9, 4, 0, 0, 0, 1, 1, 0, 0,
    9, 4, 1, 0, 1, 1, 3, 0, 0,
    7, 5, 0x81, 2, 64, 0, 0,            /* skip an input-only interface */
    9, 4, 2, 0, 0, 1, 3, 0, 0,
    9, 4, 2, 1, 1, 1, 3, 0, 0,
    7, 5, 0x02, 2, 64, 0, 0,
};
static const uint8_t hub_config[] = {9, 2, 9, 0, 1, 1, 0, 0xC0, 0};
static void mock_reset(void)
{
    memset(regs, 0, sizeof regs);
    address = port_reset = port_power = port_enable = midi_alt = probing_port = 0;
    memset(expected_pid, 0, sizeof expected_pid);
}
static void mock_host_attach(void *p) { control_dma = p; host_starts++; }
static void mock_device_attach(void *p) { control_dma = p; device_starts++; }
static void mock_setup(void *p, uint32_t n) { assert(n == 8); memcpy(setup, p, 8); }
static void mock_send(uint32_t ep, void *p, uint32_t n)
{
    uint8_t *bytes = p;
    assert(ep == 1 && n && n <= sizeof sent && !(n % 4));
    sent_to = address;
    sent_pid = regs[18] & 1u;
    assert(address < 8u);
    sent_count = n / 4;
    for (uint32_t i = 0; i < sent_count; i++, bytes += 4)
        sent[i] = bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
    sends++;
}
static void mock_control_in(void)
{
    uint8_t descriptor[8] = {18, 1, 0, 2, 0, 0, 0, 64};
    uint8_t hub_descriptor[9] = {9, 0x29, hub, 0, 0, 1};
    uint32_t bit = 1u << (setup[4] ? (setup[4] - 1u) % 8u : 0u);
    uint8_t status[4] = {0};
    unsigned len = setup[6] | (unsigned)setup[7] << 8;
    const uint8_t *data = NULL;
    if (setup[1] == 6 && setup[3] == 1) {
        descriptor[4] = hub && !probing_port ? 9 : 0;
        data = descriptor;
    } else if (setup[1] == 6 && setup[3] == 2) {
        data = hub && address == 1 ? hub_config : midi_config;
    } else if (setup[1] == 6 && setup[3] == 0x29) {
        data = hub_descriptor;
    } else if (setup[0] == 0xA3 && setup[1] == 0) {
        assert(port_power & bit);
        status[0] = (attached & bit ? 1u : 0u) | (port_enable & bit ? 2u : 0u);
        status[2] = (changed & bit ? 1u : 0u) | (port_reset & bit ? 0x10u : 0u);
        data = status;
    }
    assert(data && len <= 64);
    memcpy(control_dma, data, len);
    regs[22] = (uint8_t)len;
}
static void mock_write(uint32_t reg, uint32_t val)
{
    if (reg == 0)
        address = (uint8_t)val;
    if (reg == 17 && regs[14] == 0) {
        if (val == 0x20) {
            if (hub_error && setup[0] == 0xA3) {
                val = 0x10;
            } else {
                mock_control_in();
                val = 1;
            }
        } else if (val == 0x60) {
            if (setup[0] == 0x01 && setup[1] == 11) {
                assert(setup[2] == 1 && setup[4] == 2);
                midi_alt = setup[2];
            }
            if (setup[0] == 0x23 && setup[4]) {
                uint8_t bit = (uint8_t)(1u << (setup[4] - 1u));
                if (setup[1] == 3 && setup[2] == 8)
                    port_power |= bit;
                if (setup[1] == 3 && setup[2] == 4) {
                    port_reset |= bit;
                    port_enable |= bit;
                    probing_port = setup[4];
                    expected_pid[probing_port + 1] = 0;
                }
                if (setup[1] == 1 && setup[2] == 16)
                    changed &= (uint8_t)~bit;
                if (setup[1] == 1 && setup[2] == 20)
                    port_reset &= (uint8_t)~bit;
            }
            val = 1;
        } else if (val == 0x0A || val == 0x42) {
            val = 0;
        }
    }
    regs[reg] = (uint8_t)val;
}

#define fm1_usb_reset() mock_reset()
#define fm1_usb_off() mock_reset()
#define fm1_usb_attach(p) mock_device_attach(p)
#define fm1_usb_host_attach(p) mock_host_attach(p)
#define fm1_usb_line() line_state
#define fm1_usb_sie_on() 1u
#define fm1_usb_sie_done() 1u
#define fm1_usb_sie_wr_start(reg, val) mock_write(reg, val)
#define fm1_usb_sie_rd_start(reg) (read_reg = (reg))
#define fm1_usb_sie_data() regs[read_reg]
#define fm1_usb_ep0_buf(p) ((void)(p))
#define fm1_usb_ep0_send(p, n) mock_setup(p, n)
#define fm1_usb_ep_txbuf(ep, p) ((void)(p))
#define fm1_usb_ep_rxbuf(ep, p) ((void)(p))
#define fm1_usb_ep_send(ep, p, n) mock_send(ep, p, n)
#define fm1_usb_ep4_send(p, n) assert(0 && "unexpected record packet during USB handover test")
#define fm1_usb_ep_enable(mask) ((void)(mask))
#define fm1_usb_sof_take() 0u
#define fm1_usb_rx_sync() ((void)0)
#define fm1_delay_ms(ms) (fm1_ms += (ms))
#define fm1_wdt_feed() mock_wdt()
#include "../firmware/src/usb.c"

static uint32_t ota_now_ms(void) { return fm1_ms; }
static void ota_idle(void) { fm1_ms++; }

static void device_payload_test(void)
{
    const uint8_t message[] = {0xF0, 1, 0xF7};
    const int32_t samples[] = {123, -456};
    assert(!uh.on && usb.up);
    usb.config = 1;
    uac_stream(1);
    uac.flowing = 1;
    uac_render_start();
    uint32_t w = ua_w;
    uac_tap(samples, 1);
    assert(ua_w == w + 1 && ua_ring[w % UA_N] == (123u | (uint32_t)(uint16_t)-456 << 16));
    assert(ota_wire_send(message, sizeof message) == 0 && sx_out_q[(so_w - 1) % SXQ] == 0xF701F007u);
    sx_frame[0] = 0x39;
    sx_ready = usb.ota_req = 1;                         /* neither may outlive the device role */
    midi_out_track(4, 0x643C9009u);
    assert(midi_out_q[(mo_w - 1) % MQ] == 0x643C9009u);  /* a device packet uses cable zero */
}

static void host_payload_test(void)
{
    uh_link_t before[UH_PORTS];
    const uint8_t message[] = {0xF0, 1, 0xF7}, *frame;
    uint32_t n;
    const int32_t samples[] = {123, -456};
    assert(uh.on && !usb.config && !usb.up && !usb.ota_req && !uac.alt && !uac.feed);
    memcpy(before, uh_links, sizeof before);
    assert(ota_wire_send(message, sizeof message) == -1 && !ota_frame_get(&frame, &n));
    uac_render_start(); uac_tap(samples, 1); uac_service();
    assert(!memcmp(before, uh_links, sizeof before));
}

static void connect_host(void)
{
    for (unsigned i = 0; i < 20 && !uh.run; i++) {
        fm1_ms += 20;
        usb_host_task(fm1_ms);
        usb_retry(fm1_ms);
    }
    assert(uh.run && uh.msg == UH_MSG_MIDI && midi_alt == 1);
    usb_poll();                                        /* finish admission after reconnect cleanup, if empty */
}
static void accept_packet(void)
{
    if (uh.multi && uh.hub_addr) {
        assert(sent_to >= 2 && sent_pid == expected_pid[sent_to]);
        expected_pid[sent_to] ^= 1;
    }
    for (uint32_t i = 0; i < sent_count; i++) {
        assert(delivered_n[sent_to] < 128);
        delivered[sent_to][delivered_n[sent_to]++] = sent[i];
    }
    regs[S_TXCSR1] = 0;
}
static void acknowledge(void)
{
    assert(uh.tx_count);
    accept_packet();
    usb_poll();
}
static void mock_wdt(void)
{
    fm1_ms++;
    if (uh.tx_count) {
        accept_packet();
        usb_poll();
    }
}
static void drain(void)
{
    for (uint32_t i = 0; i < 32; i++) {
        if (uh.tx_count)
            accept_packet();
        usb_poll();
    }
    assert(!uh.tx_count);
}
static void rescan(void)
{
    for (uint32_t i = 0; i < 8; i++) {
        fm1_ms += 40;
        usb_host_task(fm1_ms);
        drain();
    }
}
static void multi_test(void)
{
    hub = 7;
    attached = 0x7F;
    uh.multi = 1;
    usb_host_switch(1);
    connect_host();
    drain();
    assert(uh.active == 0x3F && port_power == 0x7F && !uh_links[0].cleanup);
    memset(delivered_n, 0, sizeof delivered_n);
    /* Six tracks all use channel 1. USB cable tags are private to the router. */
    for (uint32_t track = 0; track < 6; track++)
        midi_out_track(track, 0x64009009u | (60u + track) << 16);
    drain();
    for (uint32_t i = 0; i < 6; i++) {
        assert(delivered_n[i + 2] == 1);
        assert(delivered[i + 2][0] == (0x64009009u | (60u + i) << 16));
    }
    midi_out_event(0xFA0Fu);
    midi_out_event(0xF80Fu);
    drain();
    for (uint32_t i = 2; i < 8; i++) {
        assert(delivered_n[i] == 3);
        assert(delivered[i][1] == 0xFA0Fu && delivered[i][2] == 0xF80Fu);
    }
    /* A stalled port records the uncertain packet and leaves the other links running. */
    midi_out_event(0x64529029u);
    usb_poll();
    assert(sent_to == 4);
    attached &= (uint8_t)~4u;
    port_enable &= (uint8_t)~4u;
    changed |= 4u;
    midi_out_event(0x64539039u);
    regs[S_TXCSR1] = 0x20;
    usb_poll();
    drain();
    assert(uh.active == 0x3B && uh.run && host_starts == 4);
    assert(delivered_n[4] == 3 && delivered[5][3] == 0x64539009u);
    rescan();
    attached |= 4u;
    changed |= 4u;
    rescan();
    assert(uh.active == 0x3F && uh.run);
    assert(delivered_n[4] == 6 && delivered[4][3] == 0xFC0Fu && delivered[4][4] == 0x003E8008u && delivered[4][5] == 0x00528008u);
    /* Reconnected port gets its own notes again. Other ports did not receive its cleanup. */
    midi_out_event(0x64549029u);
    midi_out_event(0x64559069u);                         /* the seventh physical port is unassigned */
    drain();
    assert(delivered[4][6] == 0x64549009u && delivered_n[4] == 7 && delivered_n[5] == 4);
    uint32_t before[8];
    memcpy(before, delivered_n, sizeof before);
    usb_host_switch(0);
    drain();
    for (uint32_t i = 2; i < 8; i++) {
        uint32_t first = before[i];
        if (i == 4) {
            assert(delivered_n[i] == first + 1 && delivered[i][first] == 0x00548008u);
        } else {
            assert(delivered_n[i] >= first + 2 && delivered[i][first] == 0xFC0Fu);
            assert(delivered[i][first + 1] == (0x8008u | (58u + i) << 16));
        }
    }
    usb_host_task(fm1_ms);
    usb_retry(fm1_ms);
    assert(!uh.on && usb.up);

    usb_host_switch(1);
    connect_host();
    hub_error = 1;
    fm1_ms += 40;
    usb_host_task(fm1_ms);
    assert(uh.fault && uh.run);                         /* failed hub control while MIDI was active */
    usb_host_switch(0);
    usb_host_task(fm1_ms);
    usb_retry(fm1_ms);
    assert(!uh.on && !uh.run && usb.up);
}
int main(void)
{
    /* Configuration parsing tries a later output alternate setting and rejects truncated descriptors. */
    uh_midi_t midi;
    assert(sizeof midi_config == midi_config[2]);
    assert(uh_parse_midi(midi_config, sizeof midi_config, &midi) && midi.iface == 2 && midi.alt == 1);
    assert(!uh_parse_midi(midi_config, sizeof midi_config - 1, &midi));

    usb_start();
    assert(usb.up && device_starts == 1 && !uh.on);
    device_payload_test();
    usb_host_switch(1);
    connect_host();
    assert(host_starts == 1 && device_starts == 1 && !usb.up);
    host_payload_test();

    midi_out_event(0x643C9009u);
    usb_poll();
    assert(sent_count == 1 && sent[0] == 0x643C9009u && uh_links[0].read != uh_links[0].write);
    regs[S_TXCSR1] = 0x80;                             /* NAK: keep the in-flight packet */
    usb_poll();
    assert(sends == 1 && uh_links[0].read != uh_links[0].write);
    acknowledge();
    assert(uh_links[0].read == uh_links[0].write);

    midi_out_event(0xFA0Fu);                            /* a running external clock also needs Stop */
    midi_out_event(0x647F9F09u);                         /* last note/channel in the cleanup bitmap */
    usb_poll();
    acknowledge();
    usb_host_switch(0);
    usb_host_task(fm1_ms);
    assert(uh.on);
    usb_poll();
    assert(sent_count == 3 && sent[0] == 0xFC0Fu && sent[1] == 0x003C8008u && sent[2] == 0x007F8F08u);
    drain();
    usb_host_task(fm1_ms);
    usb_retry(fm1_ms);
    assert(!uh.on && device_starts == 2 && usb.up);
    device_payload_test();

    hub = 1;
    usb_host_switch(1);
    connect_host();
    assert(port_power && port_enable && uh_links[0].addr == 2);
    midi_out_event(0x643E9009u);
    usb_poll();
    regs[S_TXCSR1] = 0x20;                              /* stall with an uncertain Note On ACK */
    usb_poll();
    assert(!uh.run && uh.fault);
    usb_host_task(fm1_ms);
    for (unsigned i = 0; i < 20; i++)
        fm1_ms += 30;
    connect_host();
    assert(sent_count == 1 && sent[0] == 0x003E8008u);
    acknowledge();

    midi_out_event(0x643D9009u);
    usb_poll();
    acknowledge();
    for (unsigned i = 0; i <= MQ; i++)
        midi_out_event(0xF80Fu);
    assert(uh_links[0].cleanup);
    usb_poll();
    assert(sent_count == 1 && sent[0] == 0x003D8008u && uh_links[0].read == uh_links[0].write);
    acknowledge();
    assert(!uh_links[0].cleanup);
    usb_host_switch(0);
    drain();
    usb_host_task(fm1_ms);
    multi_test();
    usb_detach();
    usb_host_task(fm1_ms + 1000);
    usb_retry(fm1_ms + 1000);
    assert(!uh.run && device_starts == 4);
    puts("USB MIDI host: direct output, six ports, independent toggles, reconnect, cleanup and shared RAM handover PASS");
    return 0;
}
