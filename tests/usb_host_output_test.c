/* SPDX-License-Identifier: GPL-3.0-only */
/* Run the real USB code against the AC79 register and DMA interface. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#define __attribute__(x)
#define FELUCCA_USB_HOST 1
#define RING_PUBLISH() ((void)0)
#include "../firmware/hal/fm1_usb.h"

static volatile uint32_t fm1_ms;
static uint8_t regs[32], setup[8];
static uint8_t *control_dma;
static uint32_t read_reg, line_state = 1, host_starts, device_starts;
static uint32_t sent[16], sent_count, sends;
static uint8_t address, hub, port_reset, port_power, midi_alt;

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
    address = port_reset = port_power = midi_alt = 0;
}
static void mock_host_attach(void *p) { control_dma = p; host_starts++; }
static void mock_device_attach(void *p) { control_dma = p; device_starts++; }
static void mock_setup(void *p, uint32_t n) { assert(n == 8); memcpy(setup, p, 8); }
static void mock_send(uint32_t ep, void *p, uint32_t n)
{
    uint8_t *bytes = p;
    assert(ep == 1 && n && n <= sizeof sent && !(n % 4));
    sent_count = n / 4;
    for (uint32_t i = 0; i < sent_count; i++, bytes += 4)
        sent[i] = bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
    sends++;
}
static void mock_control_in(void)
{
    uint8_t descriptor[8] = {18, 1, 0, 2, 0, 0, 0, 64};
    uint8_t hub_descriptor[9] = {9, 0x29, 1, 0, 0, 1};
    uint8_t status[4] = {3, 0, port_reset ? 0x10 : 0, 0};
    unsigned len = setup[6] | (unsigned)setup[7] << 8;
    const uint8_t *data = NULL;
    if (setup[1] == 6 && setup[3] == 1) {
        descriptor[4] = hub && !port_reset ? 9 : 0;
        data = descriptor;
    } else if (setup[1] == 6 && setup[3] == 2) {
        data = hub && address == 1 ? hub_config : midi_config;
    } else if (setup[1] == 6 && setup[3] == 0x29) {
        data = hub_descriptor;
    } else if (setup[0] == 0xA3 && setup[1] == 0) {
        assert(port_power);
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
            mock_control_in();
            val = 1;
        } else if (val == 0x60) {
            if (setup[0] == 0x01 && setup[1] == 11) {
                assert(setup[2] == 1 && setup[4] == 2);
                midi_alt = setup[2];
            }
            if (setup[0] == 0x23 && setup[1] == 3) {
                if (setup[2] == 8)
                    port_power = 1;
                if (setup[2] == 4)
                    port_reset = 1;
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
#define fm1_usb_ep_enable(mask) ((void)(mask))
#define fm1_usb_sof_take() 0u
#define fm1_usb_rx_sync() ((void)0)
#define fm1_delay_ms(ms) (fm1_ms += (ms))
#define fm1_wdt_feed() (fm1_ms++)
#include "../firmware/src/usb.c"

static void connect_host(void)
{
    for (unsigned i = 0; i < 20 && !uh.run; i++) {
        fm1_ms += 20;
        usb_host_task(fm1_ms);
        usb_retry(fm1_ms);
    }
    assert(uh.run && uh.msg == UH_MSG_MIDI && regs[S_TXTYPE] == 0x22 && midi_alt == 1);
    usb_poll();                                        /* finish admission after reconnect cleanup, if empty */
}
static void acknowledge(void)
{
    regs[S_TXCSR1] = 0;
    usb_poll();
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
    usb_host_switch(1);
    connect_host();
    assert(host_starts == 1 && device_starts == 1 && !usb.up);

    midi_out_event(0x643C9009u);
    usb_poll();
    assert(sent_count == 1 && sent[0] == 0x643C9009u && mo_r != mo_w);
    regs[S_TXCSR1] = 0x80;                             /* NAK: keep the in-flight packet */
    usb_poll();
    assert(sends == 1 && mo_r != mo_w);
    acknowledge();
    assert(mo_r == mo_w);

    midi_out_event(0xFA0Fu);                            /* a running external clock also needs Stop */
    midi_out_event(0x647F9F09u);                         /* last note/channel in the cleanup bitmap */
    usb_poll();
    acknowledge();
    usb_host_switch(0);
    usb_host_task(fm1_ms);
    assert(uh.on);
    usb_poll();
    assert(sent_count == 1 && sent[0] == 0xFC0Fu);
    acknowledge();
    assert(sent_count == 2 && sent[0] == 0x003C8008u && sent[1] == 0x007F8F08u);
    acknowledge();
    usb_host_task(fm1_ms);
    usb_retry(fm1_ms);
    assert(!uh.on && device_starts == 2 && usb.up);

    hub = 1;
    usb_host_switch(1);
    connect_host();
    assert(port_power && port_reset && address == 2);
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
    assert(uh.cleanup);
    usb_poll();
    assert(sent_count == 1 && sent[0] == 0x003D8008u && mo_r == mo_w);
    acknowledge();
    assert(!uh.cleanup);
    usb_detach();
    usb_host_task(fm1_ms + 1000);
    usb_retry(fm1_ms + 1000);
    assert(!uh.run && device_starts == 2);
    puts("USB host output: roles, MIDI OUT, powered hub, retry and note cleanup PASS");
    return 0;
}
