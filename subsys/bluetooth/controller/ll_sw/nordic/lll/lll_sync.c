/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <toolchain.h>
#include <sys/util.h>

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
#include "lll_sync.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_prof_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_sync
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *p);
static void isr_rx(void *param);

int lll_sync_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_sync_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void lll_sync_prepare(void *param)
{
	struct lll_prepare_param *p;
	struct lll_sync *lll;
	uint16_t elapsed;
	int err;

	/* Request to start HF Clock */
	err = lll_hfclock_on();
	LL_ASSERT(err >= 0);

	p = param;

	/* Instants elapsed */
	elapsed = p->lazy + 1;

	lll = p->param;

	/* Save the (skip + 1) for use in event */
	lll->skip_prepare += elapsed;

	/* Accumulate window widening */
	lll->window_widening_prepare_us += lll->window_widening_periodic_us *
					   elapsed;
	if (lll->window_widening_prepare_us > lll->window_widening_max_us) {
		lll->window_widening_prepare_us = lll->window_widening_max_us;
	}

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_is_abort_cb, lll_abort_cb, prepare_cb, 0, p);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static int init_reset(void)
{
	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	struct node_rx_pdu *node_rx;
	uint32_t ticks_at_event;
	uint32_t ticks_at_start;
	uint16_t event_counter;
	uint32_t remainder_us;
	uint8_t data_chan_use;
	struct lll_sync *lll;
	struct evt_hdr *evt;
	uint32_t remainder;
	uint32_t hcto;

	DEBUG_RADIO_START_O(1);

	lll = p->param;

	/* Deduce the skip */
	lll->skip_event = lll->skip_prepare - 1;

	/* Calculate the current event counter value */
	event_counter = lll->event_counter + lll->skip_event;

	/* Update event counter to next value */
	lll->event_counter = lll->event_counter + lll->skip_prepare;

	/* Reset accumulated latencies */
	lll->skip_prepare = 0;

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

	/* Start setting up Radio h/w */
	radio_reset();

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

	radio_phy_set(lll->phy, 1);
	radio_pkt_configure(8, PDU_AC_PAYLOAD_SIZE_MAX, (lll->phy << 1));

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	radio_pkt_rx_set(node_rx->pdu);

	radio_aa_set(lll->access_addr);
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    (((uint32_t)lll->crc_init[2] << 16) |
			     ((uint32_t)lll->crc_init[1] << 8) |
			     ((uint32_t)lll->crc_init[0])));

	lll_chan_set(data_chan_use);

	radio_isr_set(isr_rx, lll);

	radio_tmr_tifs_set(EVENT_IFS_US);

	radio_switch_complete_and_disable();

	ticks_at_event = p->ticks_at_expire;
	evt = HDR_LLL2EVT(lll);
	ticks_at_event += lll_evt_offset_get(evt);

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

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_setup();

	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(lll->phy_rx, 1) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	/* check if preempt to start has changed */
	if (lll_preempt_calc(evt, (TICKER_ID_SCAN_SYNC_BASE +
				   ull_sync_lll_handle_get(lll)),
			     ticks_at_event)) {
		radio_isr_set(lll_isr_abort, lll);
		radio_disable();
	} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	{
		uint32_t ret;

		ret = lll_prepare_done(lll);
		LL_ASSERT(!ret);
	}

	DEBUG_RADIO_START_O(1);
	return 0;
}

static void isr_rx(void *param)
{
	struct event_done_extra *e;
	struct lll_sync *lll;
	uint8_t rssi_ready;
	uint8_t trx_done;
	uint8_t trx_cnt;
	uint8_t crc_ok;

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		rssi_ready = radio_rssi_is_ready();
	} else {
		crc_ok = rssi_ready = 0U;
	}

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();

	lll = param;

	/* No Rx */
	trx_cnt = 0U;
	if (!trx_done) {
		/* TODO: Combine the early exit with above if-then-else block
		 */
		goto isr_rx_done;
	}

	/* Rx-ed */
	trx_cnt = 1U;

	/* Check CRC and generate Periodic Advertising Report */
	if (crc_ok) {
		struct node_rx_pdu *node_rx;

		node_rx = ull_pdu_rx_alloc_peek(3);
		if (node_rx) {
			struct node_rx_ftr *ftr;

			ull_pdu_rx_alloc();

			node_rx->hdr.type = NODE_RX_TYPE_SYNC_REPORT;

			ftr = &(node_rx->hdr.rx_ftr);
			ftr->param = lll;
			ftr->rssi = (rssi_ready) ? radio_rssi_get() :
						   BT_HCI_LE_RSSI_NOT_AVAILABLE;
			ftr->ticks_anchor = radio_tmr_start_get();
			ftr->radio_end_us = radio_tmr_end_get() -
					    radio_rx_chain_delay_get(lll->phy,
								     1);

			ull_rx_put(node_rx->hdr.link, node_rx);
			ull_rx_sched();
		}
	}

isr_rx_done:
	/* Calculate and place the drift information in done event */
	e = ull_event_done_extra_get();
	LL_ASSERT(e);

	e->type = EVENT_DONE_EXTRA_TYPE_SYNC;
	e->trx_cnt = trx_cnt;
	e->crc_valid = crc_ok;

	e->drift.preamble_to_addr_us = addr_us_get(lll->phy);

	e->drift.start_to_address_actual_us = radio_tmr_aa_get() -
					      radio_tmr_ready_get();
	e->drift.window_widening_event_us = lll->window_widening_event_us;

	/* Reset window widening, as anchor point sync-ed */
	lll->window_widening_event_us = 0U;
	lll->window_size_event_us = 0U;

	lll_isr_cleanup(param);
}
