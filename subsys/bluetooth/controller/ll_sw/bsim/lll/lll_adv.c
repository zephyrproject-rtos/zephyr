/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/hci_types.h>

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/dbuf.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_clock.h"
#include "lll_adv_types.h"
#include "lll_adv.h"
#include "lll_adv_pdu.h"
#include "lll_df_types.h"
#include "lll_filter.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_radio.h"
#include "lll_addr.h"

#include "hal/debug.h"

/* The gap before the PDU on the next channel, the ramp up time of a fast
 * ramping radio.
 */
#define ADV_CHAN_SWITCH_US 40U

static void isr_tx(const struct bsr_evt *e, void *param);

static struct {
	struct bsr_pkt_cfg cfg;
	const struct lll_filter *filter;
	uint32_t ticks_ref;
} evt;

int lll_adv_init(void)
{
	return lll_adv_pdu_init_reset();
}

int lll_adv_reset(void)
{
	return lll_adv_pdu_init_reset();
}

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
static int scan_req_report(struct lll_adv *lll, const struct bsr_evt *e)
{
	struct node_rx_pdu *node_rx;

	node_rx = ull_pdu_rx_alloc_peek(3);
	if (node_rx == NULL) {
		return -ENOBUFS;
	}
	ull_pdu_rx_alloc();

	/* The SCAN_REQ is in the node rx PDU */
	node_rx->hdr.type = NODE_RX_TYPE_SCAN_REQ;
	node_rx->hdr.handle = ull_adv_lll_handle_get(lll);

	node_rx->rx_ftr.rssi = lll_rssi_get(e->rssi);

	ull_rx_put_sched(node_rx->hdr.link, node_rx);

	return 0;
}
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

static void chan_tx(struct lll_adv *lll, uint32_t at)
{
	struct pdu_adv *pdu;
	uint8_t chan;
	uint8_t upd;

	chan = find_lsb_set(lll->chan_map_curr);
	LL_ASSERT_DBG(chan != 0U);

	lll->chan_map_curr &= (lll->chan_map_curr - 1U);

	evt.cfg.chan = 36U + chan;

	upd = 0U;
	pdu = lll_adv_data_latest_get(lll, &upd);
	LL_ASSERT_DBG(pdu != NULL);

	if (pdu->type != PDU_ADV_TYPE_NONCONN_IND) {
		struct pdu_adv *scan_pdu;

		/* Also picks up an update of the scan response */
		scan_pdu = lll_adv_scan_rsp_latest_get(lll, &upd);
		LL_ASSERT_DBG(scan_pdu != NULL);
		ARG_UNUSED(scan_pdu);
	}

	lll_radio_tx(&evt.cfg, at, pdu, isr_tx, lll);
}

static void isr_done(const struct bsr_evt *e, void *param)
{
	struct lll_adv *lll = param;

	ARG_UNUSED(e);

	if (lll->chan_map_curr != 0U) {
		chan_tx(lll, lll_radio_now() + ADV_CHAN_SWITCH_US);

		return;
	}

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
	struct node_rx_pdu *node_rx = ull_pdu_rx_alloc_peek(3);

	if (node_rx != NULL) {
		ull_pdu_rx_alloc();

		node_rx->hdr.type = NODE_RX_TYPE_ADV_INDICATION;

		ull_rx_put_sched(node_rx->hdr.link, node_rx);
	}
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */

#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	struct event_done_extra *extra;

	extra = ull_done_extra_type_set(EVENT_DONE_EXTRA_TYPE_ADV);
	LL_ASSERT_ERR(extra != NULL);
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */

	lll_isr_cleanup(lll);
}

static int isr_rx_pdu(struct lll_adv *lll, const struct bsr_evt *e, struct pdu_adv *pdu_rx,
		      const struct lll_addr_match *match)
{
	struct pdu_adv *pdu_adv;
	uint8_t *tgt_addr;
	uint8_t tx_addr;
	uint8_t rx_addr;
	uint8_t rl_idx;
	uint8_t *addr;

	rl_idx = FILTER_IDX_NONE;

	pdu_adv = lll_adv_data_curr_get(lll);

	addr = pdu_adv->adv_ind.addr;
	tx_addr = pdu_adv->tx_addr;
	rx_addr = pdu_adv->rx_addr;

	if (pdu_adv->type == PDU_ADV_TYPE_DIRECT_IND) {
		tgt_addr = pdu_adv->direct_ind.tgt_addr;
	} else {
		tgt_addr = NULL;
	}

	if ((pdu_rx->type == PDU_ADV_TYPE_SCAN_REQ) &&
	    (pdu_rx->len == sizeof(struct pdu_adv_scan_req)) && (tgt_addr == NULL) &&
	    lll_adv_scan_req_check(lll, pdu_rx, tx_addr, addr, rx_addr, tgt_addr,
				   match->devmatch_ok, &rl_idx)) {
#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
		int err;

		/* Without a report, the scan response is not transmitted */
		err = scan_req_report(lll, e);
		if (err != 0) {
			return err;
		}
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

		lll_radio_tx(&evt.cfg, e->ts_end + EVENT_IFS_US, lll_adv_scan_rsp_curr_get(lll),
			     isr_done, lll);

		return 0;
	}

	return -EINVAL;
}

static void isr_rx(const struct bsr_evt *e, void *param)
{
	struct lll_adv *lll = param;

	if (e->status == BSR_STATUS_OK) {
		struct node_rx_pdu *node_rx;
		struct lll_addr_match match;
		struct pdu_adv *pdu_rx;
		int err;

		node_rx = ull_pdu_rx_alloc_peek(1);
		LL_ASSERT_DBG(node_rx != NULL);

		pdu_rx = (void *)node_rx->pdu;
		lll_addr_match(pdu_rx, evt.filter, &match);

		err = isr_rx_pdu(lll, e, pdu_rx, &match);
		if (err == 0) {
			return;
		}
	}

	isr_done(e, lll);
}

static void isr_tx(const struct bsr_evt *e, void *param)
{
	struct node_rx_pdu *node_rx;
	struct lll_adv *lll = param;
	struct pdu_adv *pdu;

	pdu = lll_adv_data_curr_get(lll);
	if ((e->status != BSR_STATUS_OK) || (pdu->type == PDU_ADV_TYPE_NONCONN_IND)) {
		isr_done(e, lll);
		return;
	}

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT_DBG(node_rx != NULL);

	lll_radio_rx(&evt.cfg, lll_radio_tifs_rx_start(e->ts_end, EVENT_IFS_US),
		     lll_radio_tifs_rx_window(PHY_1M), node_rx->pdu, isr_rx, lll);
}

static int prepare_cb(struct lll_prepare_param *p)
{
	struct lll_adv *lll = p->param;
	uint32_t overhead;
	uint32_t start_us;
	int err;

	DEBUG_RADIO_START_A(1);

	overhead = lll_preempt_calc(p);
	if (overhead != 0U) {
		LL_ASSERT_OVERHEAD(overhead);

		lll_event_abort(lll);

		return -ECANCELED;
	}

	evt.cfg.aa = PDU_AC_ACCESS_ADDR;
	evt.cfg.crc_init = PDU_AC_CRC_IV;
	evt.cfg.phy = BSR_PHY_1M;
	evt.cfg.max_len = PDU_AC_LEG_PAYLOAD_SIZE_MAX;
#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	evt.cfg.tx_power = lll->tx_pwr_lvl;
#else /* !CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */
	evt.cfg.tx_power = RADIO_TXP_DEFAULT;
#endif /* !CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

	evt.filter = NULL;
	if (IS_ENABLED(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST) && (lll->filter_policy != 0U)) {
		evt.filter = ull_filter_lll_get(true);
	}

	start_us = lll_event_start_get(p, &evt.ticks_ref);

	lll->chan_map_curr = lll->chan_map;
	chan_tx(lll, start_us);

	err = lll_prepare_done(lll);
	LL_ASSERT_ERR(err == 0);

	DEBUG_RADIO_START_A(1);

	return 0;
}

static int is_abort_cb(void *next, void *curr, lll_prepare_cb_t *resume_cb)
{
	/* Cutting an advertising event short only delays the advertising, so
	 * it always gives way and is not resumed.
	 */
	return -ECANCELED;
}

void lll_adv_prepare(void *param)
{
	int err;

	err = lll_hfclock_on();
	LL_ASSERT_ERR(err >= 0);

	err = lll_prepare(is_abort_cb, lll_abort_cb, prepare_cb, 0, param);
	LL_ASSERT_ERR((err == 0) || (err == -EINPROGRESS));
}
