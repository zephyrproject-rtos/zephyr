/*
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>

#include <toolchain.h>
#include <zephyr/types.h>
#include <soc.h>
#include <drivers/clock_control.h>

#include "hal/cpu.h"
#include "hal/cntr.h"
#include "hal/ccm.h"
#include "hal/radio.h"

#include "util/memq.h"

#include "lll.h"
#include "lll_internal.h"

#include "ll_test.h"

#include "hal/debug.h"
#include "common/log.h"

#define CNTR_MIN_DELTA 3

static const uint32_t test_sync_word = 0x71764129;
static uint8_t        test_phy;
static uint8_t        test_phy_flags;
static uint16_t       test_num_rx;
static bool        started;

/* NOTE: The PRBS9 sequence used as packet payload.
 * The bytes in the sequence are in the right order, but the bits of each byte
 * in the array are reverse from that found by running the PRBS9 algorithm. This
 * is done to transmit MSbit first on air.
 */

static const uint8_t prbs9[] = {
	0xFF, 0xC1, 0xFB, 0xE8, 0x4C, 0x90, 0x72, 0x8B,
	0xE7, 0xB3, 0x51, 0x89, 0x63, 0xAB, 0x23, 0x23,
	0x02, 0x84, 0x18, 0x72, 0xAA, 0x61, 0x2F, 0x3B,
	0x51, 0xA8, 0xE5, 0x37, 0x49, 0xFB, 0xC9, 0xCA,
	0x0C, 0x18, 0x53, 0x2C, 0xFD, 0x45, 0xE3, 0x9A,
	0xE6, 0xF1, 0x5D, 0xB0, 0xB6, 0x1B, 0xB4, 0xBE,
	0x2A, 0x50, 0xEA, 0xE9, 0x0E, 0x9C, 0x4B, 0x5E,
	0x57, 0x24, 0xCC, 0xA1, 0xB7, 0x59, 0xB8, 0x87,
	0xFF, 0xE0, 0x7D, 0x74, 0x26, 0x48, 0xB9, 0xC5,
	0xF3, 0xD9, 0xA8, 0xC4, 0xB1, 0xD5, 0x91, 0x11,
	0x01, 0x42, 0x0C, 0x39, 0xD5, 0xB0, 0x97, 0x9D,
	0x28, 0xD4, 0xF2, 0x9B, 0xA4, 0xFD, 0x64, 0x65,
	0x06, 0x8C, 0x29, 0x96, 0xFE, 0xA2, 0x71, 0x4D,
	0xF3, 0xF8, 0x2E, 0x58, 0xDB, 0x0D, 0x5A, 0x5F,
	0x15, 0x28, 0xF5, 0x74, 0x07, 0xCE, 0x25, 0xAF,
	0x2B, 0x12, 0xE6, 0xD0, 0xDB, 0x2C, 0xDC, 0xC3,
	0x7F, 0xF0, 0x3E, 0x3A, 0x13, 0xA4, 0xDC, 0xE2,
	0xF9, 0x6C, 0x54, 0xE2, 0xD8, 0xEA, 0xC8, 0x88,
	0x00, 0x21, 0x86, 0x9C, 0x6A, 0xD8, 0xCB, 0x4E,
	0x14, 0x6A, 0xF9, 0x4D, 0xD2, 0x7E, 0xB2, 0x32,
	0x03, 0xC6, 0x14, 0x4B, 0x7F, 0xD1, 0xB8, 0xA6,
	0x79, 0x7C, 0x17, 0xAC, 0xED, 0x06, 0xAD, 0xAF,
	0x0A, 0x94, 0x7A, 0xBA, 0x03, 0xE7, 0x92, 0xD7,
	0x15, 0x09, 0x73, 0xE8, 0x6D, 0x16, 0xEE, 0xE1,
	0x3F, 0x78, 0x1F, 0x9D, 0x09, 0x52, 0x6E, 0xF1,
	0x7C, 0x36, 0x2A, 0x71, 0x6C, 0x75, 0x64, 0x44,
	0x80, 0x10, 0x43, 0x4E, 0x35, 0xEC, 0x65, 0x27,
	0x0A, 0xB5, 0xFC, 0x26, 0x69, 0x3F, 0x59, 0x99,
	0x01, 0x63, 0x8A, 0xA5, 0xBF, 0x68, 0x5C, 0xD3,
	0x3C, 0xBE, 0x0B, 0xD6, 0x76, 0x83, 0xD6, 0x57,
	0x05, 0x4A, 0x3D, 0xDD, 0x81, 0x73, 0xC9, 0xEB,
	0x8A, 0x84, 0x39, 0xF4, 0x36, 0x0B, 0xF7};

/* TODO: fill correct prbs15 */
static const uint8_t prbs15[255] = { 0x00, };

static uint8_t tx_req;
static uint8_t volatile tx_ack;

static void isr_tx(void *param)
{
	uint32_t l, i, s, t;

	/* Clear radio status and events */
	radio_status_reset();
	radio_tmr_status_reset();

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
	radio_gpio_pa_lna_disable();
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN */

	/* Exit if radio disabled */
	if (((tx_req - tx_ack) & 0x01) == 0U) {
		tx_ack = tx_req;

		return;
	}

	/* LE Test Packet Interval */
	l = radio_tmr_end_get() - radio_tmr_ready_get();
	i = ((l + 249 + (SCAN_INT_UNIT_US - 1)) / SCAN_INT_UNIT_US) *
		SCAN_INT_UNIT_US;
	t = radio_tmr_end_get() - l + i;
	t -= radio_tx_ready_delay_get(test_phy, test_phy_flags);

	/* Set timer capture in the future. */
	radio_tmr_sample();
	s = radio_tmr_sample_get();
	while (t < s) {
		t += SCAN_INT_UNIT_US;
	}

	/* Setup next Tx */
	radio_switch_complete_and_disable();
	radio_tmr_start_us(1, t);
	radio_tmr_aa_capture();
	radio_tmr_end_capture();

	/* TODO: check for probable stale timer capture being set */

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
	radio_gpio_pa_setup();
	radio_gpio_pa_lna_enable(t + radio_tx_ready_delay_get(test_phy,
							      test_phy_flags) -
				 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN */
}

static void isr_rx(void *param)
{
	uint8_t crc_ok = 0U;
	uint8_t trx_done;

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
	}

	/* Clear radio status and events */
	radio_status_reset();
	radio_tmr_status_reset();

	/* Exit if radio disabled */
	if (!trx_done) {
		return;
	}

	/* Setup next Rx */
	radio_switch_complete_and_rx(test_phy);

	/* Count Rx-ed packets */
	if (crc_ok) {
		test_num_rx++;
	}
}

static uint32_t init(uint8_t chan, uint8_t phy, void (*isr)(void *))
{
	int err;

	if (started) {
		return 1;
	}

	/* start coarse timer */
	cntr_start();

	/* Setup resources required by Radio */
	err = lll_clk_on_wait();

	/* Reset Radio h/w */
	radio_reset();
	radio_isr_set(isr, NULL);

	/* Store value needed in Tx/Rx ISR */
	if (phy < 0x04) {
		test_phy = BIT(phy - 1);
		test_phy_flags = 1U;
	} else {
		test_phy = BIT(2);
		test_phy_flags = 0U;
	}

	/* Setup Radio in Tx/Rx */
	/* NOTE: No whitening in test mode. */
	radio_phy_set(test_phy, test_phy_flags);
	radio_tmr_tifs_set(150);
	radio_tx_power_max_set();
	radio_freq_chan_set((chan << 1) + 2);
	radio_aa_set((uint8_t *)&test_sync_word);
	radio_crc_configure(0x65b, 0x555555);
	radio_pkt_configure(8, 255, (test_phy << 1));

	return 0;
}

uint32_t ll_test_tx(uint8_t chan, uint8_t len, uint8_t type, uint8_t phy)
{
	uint32_t start_us;
	uint8_t *payload;
	uint8_t *pdu;
	uint32_t err;

	if ((type > 0x07) || !phy || (phy > 0x04)) {
		return 1;
	}

	err = init(chan, phy, isr_tx);
	if (err) {
		return err;
	}

	tx_req++;

	pdu = radio_pkt_scratch_get();
	payload = &pdu[2];

	switch (type) {
	case 0x00:
		memcpy(payload, prbs9, len);
		break;

	case 0x01:
		memset(payload, 0x0f, len);
		break;

	case 0x02:
		memset(payload, 0x55, len);
		break;

	case 0x03:
		memcpy(payload, prbs15, len);
		break;

	case 0x04:
		memset(payload, 0xff, len);
		break;

	case 0x05:
		memset(payload, 0x00, len);
		break;

	case 0x06:
		memset(payload, 0xf0, len);
		break;

	case 0x07:
		memset(payload, 0xaa, len);
		break;
	}

	pdu[0] = type;
	pdu[1] = len;

	radio_pkt_tx_set(pdu);
	radio_switch_complete_and_disable();
	start_us = radio_tmr_start(1, cntr_cnt_get() + CNTR_MIN_DELTA, 0);
	radio_tmr_aa_capture();
	radio_tmr_end_capture();

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
	radio_gpio_pa_setup();
	radio_gpio_pa_lna_enable(start_us +
				 radio_tx_ready_delay_get(test_phy,
							  test_phy_flags) -
				 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#else /* !CONFIG_BT_CTLR_GPIO_PA_PIN */
	ARG_UNUSED(start_us);
#endif /* !CONFIG_BT_CTLR_GPIO_PA_PIN */

	started = true;

	return 0;
}

uint32_t ll_test_rx(uint8_t chan, uint8_t phy, uint8_t mod_idx)
{
	uint32_t err;

	if (!phy || (phy > 0x03)) {
		return 1;
	}

	err = init(chan, phy, isr_rx);
	if (err) {
		return err;
	}

	radio_pkt_rx_set(radio_pkt_scratch_get());
	radio_switch_complete_and_rx(test_phy);
	radio_tmr_start(0, cntr_cnt_get() + CNTR_MIN_DELTA, 0);

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_on();
#endif /* !CONFIG_BT_CTLR_GPIO_LNA_PIN */

	started = true;

	return 0;
}

uint32_t ll_test_end(uint16_t *num_rx)
{
	int err;
	uint8_t ack;

	if (!started) {
		return 1;
	}

	/* Return packets Rx-ed/Completed */
	*num_rx = test_num_rx;
	test_num_rx = 0U;

	/* Disable Radio, if in Rx test */
	ack = tx_ack;
	if (tx_req == ack) {
		radio_disable();
	} else {
		/* Wait for Tx to complete */
		tx_req = ack + 2;
		while (tx_req != tx_ack) {
			cpu_sleep();
		}
	}

	/* Stop packet timer */
	radio_tmr_stop();

	/* Release resources acquired for Radio */
	err = lll_clk_off();
	LL_ASSERT(!err || err == -EBUSY);

	/* Stop coarse timer */
	cntr_stop();

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_off();
#endif /* !CONFIG_BT_CTLR_GPIO_LNA_PIN */

	started = false;

	return 0;
}
