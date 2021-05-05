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

#include "ll_feat.h"

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

/* FIXME: Optimize by moving to a common place, as similar variable is used for
 *        connections too.
 */
static uint8_t trx_cnt;

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
	uint16_t data_chan_id;
	uint8_t data_chan_use;
	uint32_t remainder_us;
	uint8_t crc_init[3];
	struct ull_hdr *ull;
	uint32_t remainder;
	uint32_t hcto;
	uint8_t phy;

	DEBUG_RADIO_START_O(1);

	lll = p->param;

	/* Deduce the latency */
	lll->latency_event = lll->latency_prepare - 1;

	/* Calculate the current event counter value */
	event_counter = ((lll->payload_count / lll->bn) & 0xFFFF) +
			(lll->latency_event * lll->bn);

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

	/* Initialize to mandatory parameter values */
	lll->bis_curr = 1U;
	lll->ptc_curr = 0U;
	lll->irc_curr = 1U;
	lll->bn_curr = 1U;

	/* Initialize trx chain count */
	trx_cnt = 0U;

	/* Calculate the Access Address for the BIS event */
	util_bis_aa_le32(lll->bis_curr, lll->seed_access_addr, access_addr);
	data_chan_id = lll_chan_id(access_addr);

	/* Calculate the CRC init value for the BIS event,
	 * preset with the BaseCRCInit value from the BIGInfo data the most
	 * significant 2 octets and the BIS_Number for the specific BIS in the
	 * least significant octet.
	 */
	memcpy(&crc_init[1], lll->base_crc_init, sizeof(uint16_t));
	crc_init[0] = lll->bis_curr;

	/* Calculate the radio channel to use for ISO event */
	data_chan_use = lll_chan_iso_event(event_counter, data_chan_id,
					   &lll->data_chan_map[0],
					   lll->data_chan_count,
					   &lll->data_chan_prn_s,
					   &lll->data_chan_remap_idx);
	lll->ctrl_chan_use = data_chan_use;

	/* Start setting up of Radio h/w */
	radio_reset();

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif

	phy = lll->phy;
	radio_phy_set(phy, 1);
	radio_pkt_configure(8, lll->max_pdu, (phy << 1));
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

	radio_tmr_ready_save(remainder_us);
	radio_tmr_aa_save(0);
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

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_setup();

	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(lll->phy, 1) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

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
	uint8_t crc_ok;

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		trx_cnt++;
	} else {
		crc_ok = 0U;
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
	struct node_rx_pdu *node_rx;
	struct event_done_extra *e;
	struct lll_sync_iso *lll;
	uint8_t access_addr[4];
	uint16_t data_chan_id;
	uint8_t data_chan_use;
	uint8_t payload_index;
	uint8_t crc_init[3];
	uint8_t rssi_ready;
	uint8_t trx_done;
	uint8_t crc_ok;
	uint32_t hcto;
	uint8_t bis;
	uint8_t bn;

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		rssi_ready = radio_rssi_is_ready();
		trx_cnt++;
	} else {
		crc_ok = 0U;
		rssi_ready = 0U;
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
		payload_index = lll->payload_tail + (lll->bn_curr - 1) +
				(lll->ptc_curr * lll->pto);
		if (payload_index >= lll->payload_count_max) {
			payload_index -= lll->payload_count_max;
		}

		node_rx = ull_pdu_rx_alloc_peek(3);

		if (node_rx && !lll->payload[payload_index] &&
		    ((payload_index >= lll->payload_tail) ||
		     (payload_index < lll->payload_head))) {
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

			lll->payload[payload_index] = node_rx;
		}
	}

isr_rx_done:
	/* Save the AA captured for the first anchor point sync */
	if (!radio_tmr_aa_restore() && trx_cnt) {
		radio_tmr_aa_save(radio_tmr_aa_get());
		radio_tmr_ready_save(radio_tmr_ready_get());
	}

	uint8_t is_new_burst = 0U;
	uint8_t skipped = 0U;

isr_rx_find_subevent:
	/* FIXME: Sequential or Interleaved BIS subevents decision */
	/* Sequential Rx complete flow pseudo code */
	while (lll->bn_curr < lll->bn) {
		lll->bn_curr++;

		payload_index = lll->payload_tail + (lll->bn_curr - 1);
		if (payload_index >= lll->payload_count_max) {
			payload_index -= lll->payload_count_max;
		}

		if (!lll->payload[payload_index]) {
			bis = lll->bis_curr;

			/* Receive the (bn_curr)th Rx PDU of bis_curr */
			goto isr_rx_next_subevent;
		}

		skipped++;
	}

	if (lll->irc_curr < lll->irc) {
		if (!is_new_burst) {
			lll->bn_curr = 1U;
			lll->irc_curr++;

			payload_index = lll->payload_tail + (lll->bn_curr - 1);
			if (payload_index >= lll->payload_count_max) {
				payload_index -= lll->payload_count_max;
			}

			if (!lll->payload[payload_index]) {
				bis = lll->bis_curr;

				goto isr_rx_next_subevent;
			} else {
				skipped++;
				is_new_burst = 1U;

				/* Receive the missing (bn_curr)th Rx PDU of bis_curr */
				goto isr_rx_find_subevent;
			}
		} else {
			skipped += (lll->irc - lll->irc_curr) * lll->bn;
			lll->irc_curr = lll->irc;
		}
	}

	if (lll->ptc_curr < lll->ptc) {
		lll->ptc_curr++;
		/* Receive the (ptc_curr)th Rx PDU */

		bis = lll->bis_curr;

		goto isr_rx_next_subevent;
	}

	if (lll->bis_curr < lll->num_bis) {
		lll->bis_curr++;
		lll->ptc_curr = 0;
		lll->irc_curr = 1U;
		lll->bn_curr = 0;
		/* Receive the (bn_curr)th PDU of bis_curr */
		lll->bn_curr++;  /* post increment */

		bis = lll->bis_curr;

		goto isr_rx_next_subevent;
	}

	if (0) {
		/* Reeive the control PDU and close the BIG event
		 *  there after.
		 */

		goto isr_rx_next_subevent;
	}

	/* Enqueue PDUs to ULL */
	node_rx = NULL;
	bn = lll->bn;
	while (bn--) {
		if (lll->payload[lll->payload_tail]) {
			node_rx = lll->payload[lll->payload_tail];
			lll->payload[lll->payload_tail] = NULL;
			ull_rx_put(node_rx->hdr.link, node_rx);
		}

		payload_index = lll->payload_tail + 1U;
		if (payload_index >= lll->payload_count_max) {
			payload_index = 0U;
		}
		lll->payload_tail = payload_index;
	}

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
	if (node_rx) {
		ull_rx_sched();
	}
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

	/* Calculate and place the drift information in done event */
	e = ull_event_done_extra_get();
	LL_ASSERT(e);

	e->type = EVENT_DONE_EXTRA_TYPE_SYNC_ISO;
	e->trx_cnt = trx_cnt;
	e->crc_valid = crc_ok;

	if (trx_cnt) {
		e->drift.preamble_to_addr_us = addr_us_get(lll->phy);
		e->drift.start_to_address_actual_us =
			radio_tmr_aa_restore() - radio_tmr_ready_restore();
		e->drift.window_widening_event_us =
			lll->window_widening_event_us;

		/* Reset window widening, as anchor point sync-ed */
		lll->window_widening_event_us = 0U;
		lll->window_size_event_us = 0U;
	}

	lll_isr_cleanup(param);

	return;

isr_rx_next_subevent:
	/* Calculate the Access Address for the BIS event */
	util_bis_aa_le32(bis, lll->seed_access_addr, access_addr);
	data_chan_id = lll_chan_id(access_addr);

	/* Calculate the CRC init value for the BIS event,
	 * preset with the BaseCRCInit value from the BIGInfo data the most
	 * significant 2 octets and the BIS_Number for the specific BIS in the
	 * least significant octet.
	 */
	memcpy(&crc_init[1], lll->base_crc_init, sizeof(uint16_t));
	crc_init[0] = bis;

	radio_aa_set(access_addr);
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    (((uint32_t)crc_init[2] << 16) |
			     ((uint32_t)crc_init[1] << 8) |
			     ((uint32_t)crc_init[0])));

	/* Set the channel to use */
	if (bis) {
		do {
			/* Calculate the radio channel to use for ISO event */
			data_chan_use =	lll_chan_iso_subevent(data_chan_id,
						&lll->data_chan_map[0],
						lll->data_chan_count,
						&lll->data_chan_prn_s,
						&lll->data_chan_remap_idx);
		} while (skipped--);
	} else {
		data_chan_use = lll->ctrl_chan_use;
	}
	lll_chan_set(data_chan_use);

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);
	radio_pkt_rx_set(node_rx->pdu);

	radio_switch_complete_and_disable();

	hcto = (lll->sub_interval *
		(((lll->bis_curr - 1) * ((lll->bn * lll->irc) + lll->ptc)) +
		 (((lll->irc_curr - 1) * lll->bn) + (lll->bn_curr - 1) +
		  lll->ptc_curr) + lll->ctrl));

	if (trx_cnt) {
		hcto += radio_tmr_aa_restore();
		hcto -= radio_rx_chain_delay_get(lll->phy, 1);
		hcto -= addr_us_get(lll->phy);
		hcto -= radio_rx_ready_delay_get(lll->phy, 1);
		hcto -= 4;

		radio_tmr_start_us(0, hcto);

		hcto += 8;
	} else {
		hcto += radio_tmr_ready_restore();

		radio_tmr_start_us(0, hcto);

		hcto += ((EVENT_JITTER_US + EVENT_TICKER_RES_MARGIN_US +
			  lll->window_widening_event_us) << 1) +
			lll->window_size_event_us;
	}

	hcto += radio_rx_ready_delay_get(lll->phy, 1);
	hcto += addr_us_get(lll->phy);
	hcto += radio_rx_chain_delay_get(lll->phy, 1);

	radio_tmr_hcto_configure(hcto);
	radio_tmr_end_capture();

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_setup();

	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(lll->phy, 1) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */
}
