/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <soc.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/memq.h"

#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_chan.h"
#include "lll_sync_iso.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_sync_iso
#include "common/log.h"
#include "hal/debug.h"

static int init_reset(void);
static void prepare(void *param);
static void create_prepare_bh(void *param);
static void prepare_bh(void *param);
static int create_prepare_cb(struct lll_prepare_param *p);
static int prepare_cb(struct lll_prepare_param *p);
static int prepare_cb_common(struct lll_prepare_param *p);
static void isr_rx_estab(void *param);
static void isr_rx(void *param);

int lll_sync_iso_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_sync_iso_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void lll_sync_iso_create_prepare(void *param)
{
	prepare(param);
	create_prepare_bh(param);
}

void lll_sync_iso_prepare(void *param)
{
	prepare(param);
	prepare_bh(param);
}

static int init_reset(void)
{
	return 0;
}

static void prepare(void *param)
{
	struct lll_prepare_param *p;
	struct lll_sync_iso *lll;
	uint16_t elapsed;
	int err;

	err = lll_hfclock_on();
	LL_ASSERT(err >= 0);

	p = param;

	/* Instants elapsed */
	elapsed = p->lazy + 1;

	lll = p->param;

	/* Save the (latency + 1) for use in event */
	lll->latency_prepare += elapsed;

	/* Accumulate window widening */
	lll->window_widening_prepare_us += lll->window_widening_periodic_us *
					   elapsed;
	if (lll->window_widening_prepare_us > lll->window_widening_max_us) {
		lll->window_widening_prepare_us = lll->window_widening_max_us;
	}
}

static void create_prepare_bh(void *param)
{
	int err;

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_is_abort_cb, lll_abort_cb, create_prepare_cb, 0,
			  param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static void prepare_bh(void *param)
{
	int err;

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_is_abort_cb, lll_abort_cb, prepare_cb, 0, param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static int create_prepare_cb(struct lll_prepare_param *p)
{
	int err;

	err = prepare_cb_common(p);
	if (err) {
		DEBUG_RADIO_START_O(1);
		return 0;
	}

	radio_isr_set(isr_rx_estab, p->param);

	DEBUG_RADIO_START_O(1);
	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	int err;

	err = prepare_cb_common(p);
	if (err) {
		DEBUG_RADIO_START_O(1);
		return 0;
	}

	radio_isr_set(isr_rx, p->param);

	DEBUG_RADIO_START_O(1);
	return 0;
}

static int prepare_cb_common(struct lll_prepare_param *p)
{
	struct node_rx_pdu *node_rx;
	struct lll_sync_iso *lll;
	uint32_t ticks_at_event;
	uint32_t ticks_at_start;
	uint16_t event_counter;
	uint8_t access_addr[4];
	uint8_t data_chan_use;
	uint32_t remainder_us;
	uint8_t crc_init[3];
	struct ull_hdr *ull;
	uint32_t remainder;
	uint32_t hcto;
	uint8_t phy;
	uint8_t bis;

	DEBUG_RADIO_START_O(1);

	lll = p->param;

	/* Deduce the latency */
	lll->latency_event = lll->latency_prepare - 1;

	/* Calculate the current event counter value */
	event_counter = (lll->payload_count & 0xFFFF) + lll->latency_event;

	/* Update BIS packet counter to next value */
	lll->payload_count += (lll->latency_prepare * lll->bn);

	/* Reset accumulated latencies */
	lll->latency_prepare = 0;

	/* Current window widening */
	lll->window_widening_event_us += lll->window_widening_prepare_us;
	lll->window_widening_prepare_us = 0;
	if (lll->window_widening_event_us > lll->window_widening_max_us) {
		lll->window_widening_event_us =	lll->window_widening_max_us;
	}

	/* Calculate the radio channel to use */
	data_chan_use = lll_chan_sel_2(event_counter, lll->data_chan_id,
				       &lll->data_chan_map[0],
				       lll->data_chan_count);

	/* Calculate the Access Address for the BIS event */
	bis = 1U;
	util_bis_aa_le32(bis, lll->seed_access_addr, access_addr);

	/* Calculate the CRC init value for the BIS event,
	 * preset with the BaseCRCInit value from the BIGInfo data the most
	 * significant 2 octets and the BIS_Number for the specific BIS in the
	 * least significant octet.
	 */
	memcpy(&crc_init[1], lll->base_crc_init, sizeof(uint16_t));
	crc_init[0] = bis;

	/* Start setting up of Radio h/w */
	radio_reset();

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif

	phy = lll->phy;
	radio_phy_set(phy, 1);
	radio_pkt_configure(8, PDU_AC_PAYLOAD_SIZE_MAX, (phy << 1));
	radio_aa_set(access_addr);
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    (((uint32_t)crc_init[2] << 16) |
			     ((uint32_t)crc_init[1] << 8) |
			     ((uint32_t)crc_init[0])));

	lll_chan_set(data_chan_use);

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);
	radio_pkt_rx_set(node_rx->pdu);

	radio_switch_complete_and_disable();

	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(lll);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	remainder_us = radio_tmr_start(0, ticks_at_start, remainder);

	radio_tmr_aa_capture();

	hcto = remainder_us +
	       ((EVENT_JITTER_US + EVENT_TICKER_RES_MARGIN_US +
		 lll->window_widening_event_us) << 1) +
	       lll->window_size_event_us;
	hcto += radio_rx_ready_delay_get(lll->phy, 1);
	hcto += addr_us_get(lll->phy);
	hcto += radio_rx_chain_delay_get(lll->phy, 1);
	radio_tmr_hcto_configure(hcto);

	radio_tmr_end_capture();
	radio_rssi_measure();

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
	radio_gpio_pa_setup();

	radio_gpio_pa_lna_enable(remainder_us +
				 radio_tx_ready_delay_get(phy, 1) -
				 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#else /* !CONFIG_BT_CTLR_GPIO_PA_PIN */
	ARG_UNUSED(remainder_us);
#endif /* !CONFIG_BT_CTLR_GPIO_PA_PIN */

	if (0) {
#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	/* check if preempt to start has changed */
	} else if (lll_preempt_calc(ull, (TICKER_ID_SCAN_SYNC_ISO_BASE +
					  ull_sync_iso_lll_handle_get(lll)),
				    ticks_at_event)) {
		radio_isr_set(lll_isr_abort, lll);
		radio_disable();

		return -ECANCELED;
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	} else {
		uint32_t ret;

		ret = lll_prepare_done(lll);
		LL_ASSERT(!ret);
	}

	return 0;
}


static void isr_rx_estab(void *param)
{
	struct event_done_extra *e;
	uint8_t trx_done;
	uint8_t trx_cnt;
	uint8_t crc_ok;

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		trx_cnt = 1U;
	} else {
		crc_ok = 0U;
		trx_cnt = 0U;
	}

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();

	/* Calculate and place the drift information in done event */
	e = ull_event_done_extra_get();
	LL_ASSERT(e);

	e->type = EVENT_DONE_EXTRA_TYPE_SYNC_ISO_ESTAB;
	e->trx_cnt = trx_cnt;
	e->crc_valid = crc_ok;

	if (trx_cnt) {
		struct lll_sync_iso *lll;

		lll = param;
		e->drift.preamble_to_addr_us = addr_us_get(lll->phy);
		e->drift.start_to_address_actual_us =
			radio_tmr_aa_get() - radio_tmr_ready_get();
		e->drift.window_widening_event_us =
			lll->window_widening_event_us;

		/* Reset window widening, as anchor point sync-ed */
		lll->window_widening_event_us = 0U;
		lll->window_size_event_us = 0U;
	}

	lll_isr_cleanup(param);
}

static void isr_rx(void *param)
{
	struct event_done_extra *e;
	struct lll_sync_iso *lll;
	uint8_t rssi_ready;
	uint8_t trx_done;
	uint8_t trx_cnt;
	uint8_t crc_ok;

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		rssi_ready = radio_rssi_is_ready();
		trx_cnt = 1U;
	} else {
		crc_ok = 0U;
		rssi_ready = 0U;
		trx_cnt = 0U;
	}

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();

	lll = param;

	/* No Rx */
	if (!trx_done) {
		/* TODO: Combine the early exit with above if-then-else block
		 */
		goto isr_rx_done;
	}

	/* Check CRC and generate ISO Data PDU */
	if (crc_ok) {
		struct node_rx_pdu *node_rx;

		node_rx = ull_pdu_rx_alloc_peek(3);
		if (node_rx) {
			struct node_rx_ftr *ftr;

			ull_pdu_rx_alloc();

			node_rx->hdr.type = NODE_RX_TYPE_SYNC_ISO_PDU;

			ftr = &(node_rx->hdr.rx_ftr);
			ftr->param = lll;
			ftr->rssi = (rssi_ready) ? radio_rssi_get() :
						   BT_HCI_LE_RSSI_NOT_AVAILABLE;
			ftr->ticks_anchor = radio_tmr_start_get();
			ftr->radio_end_us = radio_tmr_end_get() -
					    radio_rx_chain_delay_get(lll->phy,
								     1);

			ull_rx_put(node_rx->hdr.link, node_rx);
#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
			ull_rx_sched();
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */
		}
	}

isr_rx_done:
	/* Calculate and place the drift information in done event */
	e = ull_event_done_extra_get();
	LL_ASSERT(e);

	e->type = EVENT_DONE_EXTRA_TYPE_SYNC_ISO;
	e->trx_cnt = trx_cnt;
	e->crc_valid = crc_ok;

	if (trx_cnt) {
		e->drift.preamble_to_addr_us = addr_us_get(lll->phy);
		e->drift.start_to_address_actual_us =
			radio_tmr_aa_get() - radio_tmr_ready_get();
		e->drift.window_widening_event_us =
			lll->window_widening_event_us;

		/* Reset window widening, as anchor point sync-ed */
		lll->window_widening_event_us = 0U;
		lll->window_size_event_us = 0U;
	}

	lll_isr_cleanup(param);
}
