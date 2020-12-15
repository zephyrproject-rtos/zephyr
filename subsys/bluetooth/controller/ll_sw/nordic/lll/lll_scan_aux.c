/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <sys/byteorder.h>
#include <sys/util.h>

#include <bluetooth/hci.h>

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/memq.h"

#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_filter.h"
#include "lll_scan.h"
#include "lll_scan_aux.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_prof_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_scan_aux
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *p);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static void isr_rx(void *param);
static int isr_rx_pdu(struct lll_scan_aux *lll, uint8_t rssi_ready);
static void isr_done(void *param);

static uint16_t trx_cnt; /* TODO: move to a union in lll.c, common to all roles
			  */

int lll_scan_aux_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_scan_aux_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void lll_scan_aux_prepare(void *param)
{
	int err;

	err = lll_hfclock_on();
	LL_ASSERT(err >= 0);

	err = lll_prepare(lll_is_abort_cb, abort_cb, prepare_cb, 0, param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static int init_reset(void)
{
	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	struct node_rx_pdu *node_rx;
	struct lll_scan_aux *lll;
	uint32_t ticks_at_event;
	uint32_t ticks_at_start;
	struct evt_hdr *evt;
	uint32_t remainder_us;
	uint32_t remainder;
	uint32_t hcto;
	uint32_t aa;

	DEBUG_RADIO_START_O(1);

	/* Start setting up Radio h/w */
	radio_reset();

	/* Reset Tx/rx count */
	trx_cnt = 0U;

	lll = p->param;

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif

	radio_phy_set(lll->phy, 1);
	radio_pkt_configure(8, PDU_AC_PAYLOAD_SIZE_MAX, (lll->phy << 1));

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	radio_pkt_rx_set(node_rx->pdu);

	aa = sys_cpu_to_le32(PDU_AC_ACCESS_ADDR);
	radio_aa_set((uint8_t *)&aa);
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    0x555555);

	lll_chan_set(lll->chan);

	radio_isr_set(isr_rx, lll);

	/* setup tIFS switching */
	radio_tmr_tifs_set(EVENT_IFS_US);
	radio_switch_complete_and_tx(lll->phy, 0, lll->phy, 1);

	/* TODO: privacy */

	/* Calculate event timings, coarse and fine */
	ticks_at_event = p->ticks_at_expire;
	evt = HDR_LLL2EVT(lll);
	ticks_at_event += lll_evt_offset_get(evt);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	remainder_us = radio_tmr_start(0, ticks_at_start, remainder);

	hcto = remainder_us + lll->window_size_us;
	hcto += radio_rx_ready_delay_get(lll->phy, 1);
	hcto += addr_us_get(lll->phy);
	hcto += radio_rx_chain_delay_get(lll->phy, 1);
	radio_tmr_hcto_configure(hcto);

	/* capture end of Rx-ed PDU, extended scan to schedule auxiliary
	 * channel chaining or to create periodic sync.
	 */
	radio_tmr_end_capture();

	/* scanner always measures RSSI */
	radio_rssi_measure();

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_setup();

	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(lll->phy, 1) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	/* check if preempt to start has changed */
	if (lll_preempt_calc(evt, (TICKER_ID_SCAN_AUX_BASE +
				   ull_scan_aux_lll_handle_get(lll)),
			     ticks_at_event)) {
		radio_isr_set(isr_done, lll);
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

static void abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
	int err;

	/* NOTE: This is not a prepare being cancelled */
	if (!prepare_param) {
		/* Perform event abort here.
		 * After event has been cleanly aborted, clean up resources
		 * and dispatch event done.
		 */
		radio_isr_set(isr_done, param);
		radio_disable();
		return;
	}

	/* NOTE: Else clean the top half preparations of the aborted event
	 * currently in preparation pipeline.
	 */
	err = lll_hfclock_off();
	LL_ASSERT(err >= 0);

	lll_done(param);
}

static void isr_rx(void *param)
{
	struct lll_scan_aux *lll;
	uint8_t rssi_ready;
	uint8_t trx_done;
	uint8_t crc_ok;

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
	}

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
	if (!trx_done) {
		/* TODO: Combine the early exit with above if-then-else block
		 */
		goto isr_rx_do_close;
	}

	if (crc_ok) {
		int err;

		err = isr_rx_pdu(lll, rssi_ready);
		if (!err) {
			if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
				lll_prof_send();
			}

			return;
		}
	}

isr_rx_do_close:
	radio_isr_set(isr_done, lll);
	radio_disable();
}

static int isr_rx_pdu(struct lll_scan_aux *lll, uint8_t rssi_ready)
{
	struct node_rx_pdu *node_rx;
	struct node_rx_ftr *ftr;
	struct pdu_adv *pdu;

	node_rx = ull_pdu_rx_alloc_peek(3);
	if (!node_rx) {
		return -ENOBUFS;
	}

	pdu = (void *)node_rx->pdu;
	if ((pdu->type != PDU_ADV_TYPE_EXT_IND) || !pdu->len) {
		return -EINVAL;
	}

	ull_pdu_rx_alloc();

	trx_cnt++;

	node_rx->hdr.type = NODE_RX_TYPE_EXT_AUX_REPORT;

	ftr = &(node_rx->hdr.rx_ftr);
	ftr->param = lll;
	ftr->ticks_anchor = radio_tmr_start_get();
	ftr->radio_end_us = radio_tmr_end_get() -
			    radio_rx_chain_delay_get(lll->phy, 1);

	ftr->rssi = (rssi_ready) ? radio_rssi_get() :
				   BT_HCI_LE_RSSI_NOT_AVAILABLE;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* TODO: Use correct rl_idx value when privacy support is added */
	ftr->rl_idx = FILTER_IDX_NONE;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	ull_rx_put(node_rx->hdr.link, node_rx);
	ull_rx_sched();

	return -ECANCELED;
}

static void isr_done(void *param)
{
	struct event_done_extra *e;

	lll_isr_status_reset();

	if (!trx_cnt) {
		e = ull_event_done_extra_get();
		LL_ASSERT(e);

		e->type = EVENT_DONE_EXTRA_TYPE_SCAN_AUX;
	}

	lll_isr_cleanup(param);
}
