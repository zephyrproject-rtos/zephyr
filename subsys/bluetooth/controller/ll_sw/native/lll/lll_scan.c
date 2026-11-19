/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/hci_types.h>

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/dbuf.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_df_types.h"
#include "lll_scan.h"
#include "lll_filter.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_radio.h"
#include "lll_addr.h"

#include "hal/debug.h"

#define ADV_CHAN_MAX 3U

static void isr_rx(const struct lll_radio_evt *e, void *param);

static struct {
	struct nbr_pkt_cfg cfg;
	const struct lll_filter *filter;
	struct pdu_adv pdu_tx;
} evt;

int lll_scan_init(void)
{
	return 0;
}

int lll_scan_reset(void)
{
	return 0;
}

static void ticker_stop_cb(uint32_t ticks_at_expire, uint32_t ticks_drift, uint32_t remainder,
			   uint16_t lazy, uint8_t force, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_disable};
	uint32_t ret;

	ARGS_UNUSED(ticks_at_expire, ticks_drift, remainder, lazy, force);

	mfy.param = param;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL, 0U, &mfy);
	LL_ASSERT_ERR(ret == 0U);
}

static void ticker_op_start_cb(uint32_t status, void *param)
{
	ARG_UNUSED(param);

	LL_ASSERT_ERR(status == TICKER_STATUS_SUCCESS);
}

static void isr_done_cleanup(const struct lll_radio_evt *e, void *param)
{
	struct lll_scan *lll;
	bool is_resume;

	ARG_UNUSED(e);

	/* Nothing to clean up when the event has been reported done already */
	if (lll_is_done(param, &is_resume)) {
		return;
	}

	lll = param;
	lll->chan++;
	if (lll->chan == ADV_CHAN_MAX) {
		lll->chan = 0U;
	}

	/* Scanner stop can expire while here in this ISR.
	 * Deferred attempt to stop can fail as it would have
	 * expired, hence ignore failure.
	 */
	(void)ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_LLL, TICKER_ID_SCAN_STOP, NULL,
			  NULL);

	if (IS_ENABLED(CONFIG_BT_CTLR_SCAN_INDICATION)) {
		struct node_rx_pdu *node_rx;

		/* Check if there are enough free node rx available:
		 * 1. For generating this scan indication
		 * 2. Keep one available free for reception of ACL connection
		 *    Rx data
		 * 3. Keep one available free for reception on ACL connection
		 *    to NACK the PDU
		 */
		node_rx = ull_pdu_rx_alloc_peek(3);
		if (node_rx != NULL) {
			(void)ull_pdu_rx_alloc();

			node_rx->hdr.type = NODE_RX_TYPE_SCAN_INDICATION;

			ull_rx_put_sched(node_rx->hdr.link, node_rx);
		}
	}

	lll_isr_cleanup(param);
}

static int isr_rx_scan_report(const struct lll_radio_evt *e, bool dir_report)
{
	struct node_rx_pdu *node_rx;

	node_rx = ull_pdu_rx_alloc_peek(3);
	if (node_rx == NULL) {
		return -ENOBUFS;
	}
	(void)ull_pdu_rx_alloc();

	/* The advertising PDU or scan response is in the node rx */
	node_rx->hdr.handle = LLL_HANDLE_INVALID;
	node_rx->hdr.type = NODE_RX_TYPE_REPORT;

	node_rx->rx_ftr.rssi = lll_rssi_get(e->rssi);

#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
	node_rx->rx_ftr.direct = dir_report;
#else /* !CONFIG_BT_CTLR_EXT_SCAN_FP */
	ARG_UNUSED(dir_report);
#endif /* !CONFIG_BT_CTLR_EXT_SCAN_FP */

	ull_rx_put_sched(node_rx->hdr.link, node_rx);

	return 0;
}

/* The Rx goes on until a PDU is received or the scan window closes */
static void rx(struct lll_scan *lll, uint32_t start)
{
	struct node_rx_pdu *node_rx;

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT_DBG(node_rx != NULL);

	evt.cfg.freq_mhz = lll_radio_freq_get(37U + lll->chan);

	lll_radio_rx(&evt.cfg, start, 0U, node_rx->pdu, isr_rx, lll);
}

static void rx_restart(struct lll_scan *lll)
{
	lll->state = 0U;

	rx(lll, lll_radio_now() + 1U);
}

/* Only the scan response of the advertiser the SCAN_REQ was sent to */
static bool scan_rsp_check(const struct lll_scan *lll, const struct pdu_adv *pdu)
{
	return (pdu->type == PDU_ADV_TYPE_SCAN_RSP) &&
	       (pdu->len >= offsetof(struct pdu_adv_scan_rsp, data)) &&
	       (pdu->len <= sizeof(struct pdu_adv_scan_rsp)) && (lll->state != 0U) &&
	       (evt.pdu_tx.rx_addr == pdu->tx_addr) &&
	       (memcmp(&evt.pdu_tx.scan_req.adv_addr[0], &pdu->scan_rsp.addr[0], BDADDR_SIZE) == 0);
}

static void isr_tx(const struct lll_radio_evt *e, void *param)
{
	struct lll_scan *lll = param;
	struct node_rx_pdu *node_rx;

	if (e->status != NBR_STATUS_OK) {
		rx_restart(lll);

		return;
	}

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT_DBG(node_rx != NULL);

	lll_radio_rx(&evt.cfg, lll_radio_tifs_rx_start(e->ts_end, EVENT_IFS_US),
		     lll_radio_tifs_rx_window(PHY_1M), node_rx->pdu, isr_rx, lll);
}

static bool adv_ind_len_check(const struct pdu_adv *pdu)
{
	return (pdu->len >= offsetof(struct pdu_adv_adv_ind, data)) &&
	       (pdu->len <= sizeof(struct pdu_adv_adv_ind));
}

static bool scan_req_pdu_check(const struct lll_scan *lll, const struct pdu_adv *pdu)
{
	return ((pdu->type == PDU_ADV_TYPE_ADV_IND) || (pdu->type == PDU_ADV_TYPE_SCAN_IND)) &&
	       adv_ind_len_check(pdu) && (lll->type != 0U) && (lll->state == 0U);
}

static int isr_rx_scan_req(struct lll_scan *lll, const struct lll_radio_evt *e,
			   const struct pdu_adv *pdu_adv_rx)
{
	struct pdu_adv *pdu_tx;
	int err;

	err = isr_rx_scan_report(e, false);
	if (err != 0) {
		return err;
	}

	pdu_tx = &evt.pdu_tx;
	pdu_tx->type = PDU_ADV_TYPE_SCAN_REQ;
	pdu_tx->rfu = 0U;
	pdu_tx->chan_sel = 0U;
	pdu_tx->tx_addr = lll->init_addr_type;
	pdu_tx->rx_addr = pdu_adv_rx->tx_addr;
	pdu_tx->len = sizeof(struct pdu_adv_scan_req);
	(void)memcpy(&pdu_tx->scan_req.scan_addr[0], &lll->init_addr[0], BDADDR_SIZE);
	(void)memcpy(&pdu_tx->scan_req.adv_addr[0], &pdu_adv_rx->adv_ind.addr[0], BDADDR_SIZE);

	lll->state = 1U;

	lll_radio_tx(&evt.cfg, e->ts_end + EVENT_IFS_US, pdu_tx, isr_tx, lll);

	return 0;
}

static bool report_pdu_check(const struct lll_scan *lll, const struct pdu_adv *pdu,
			     bool *dir_report)
{
	if ((pdu->type == PDU_ADV_TYPE_ADV_IND) || (pdu->type == PDU_ADV_TYPE_NONCONN_IND) ||
	    (pdu->type == PDU_ADV_TYPE_SCAN_IND)) {
		return adv_ind_len_check(pdu);
	}

	if (pdu->type == PDU_ADV_TYPE_DIRECT_IND) {
		return (pdu->len == sizeof(struct pdu_adv_direct_ind)) &&
		       lll_scan_tgta_check(lll, false, pdu->rx_addr, pdu->direct_ind.tgt_addr,
					   FILTER_IDX_NONE, dir_report);
	}

	return scan_rsp_check(lll, pdu);
}

static int isr_rx_pdu(struct lll_scan *lll, const struct lll_radio_evt *e,
		      struct pdu_adv *pdu_adv_rx)
{
	bool dir_report = false;
	int err;

	if (scan_req_pdu_check(lll, pdu_adv_rx)) {
		return isr_rx_scan_req(lll, e, pdu_adv_rx);
	}

	if (!report_pdu_check(lll, pdu_adv_rx, &dir_report)) {
		return -EINVAL;
	}

	err = isr_rx_scan_report(e, dir_report);
	if (err != 0) {
		return err;
	}

	return -ECANCELED;
}

static void isr_rx(const struct lll_radio_evt *e, void *param)
{
	struct lll_scan *lll = param;
	int err = -EINVAL;

	/* Not when there is no PDU, or one with errors */
	if (e->status == NBR_STATUS_OK) {
		struct node_rx_pdu *node_rx;
		struct lll_addr_match match;
		struct pdu_adv *pdu;

		node_rx = ull_pdu_rx_alloc_peek(1);
		LL_ASSERT_DBG(node_rx != NULL);

		pdu = (void *)node_rx->pdu;
		lll_addr_match(pdu, evt.filter, &match);

		if (lll_scan_isr_rx_check(lll, 0U, match.devmatch_ok, FILTER_IDX_NONE)) {
			err = isr_rx_pdu(lll, e, pdu);
			if (err == 0) {
				return;
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) && (err == -ECANCELED)) {
		isr_done_cleanup(e, lll);
	} else {
		rx_restart(lll);
	}
}

static int common_prepare_cb(struct lll_prepare_param *p, bool is_resume)
{
	struct lll_scan *lll = p->param;
	uint32_t ticks_ref;
	uint32_t overhead;
	uint32_t start_us;
	int err;

	DEBUG_RADIO_START_O(1);

	overhead = lll_preempt_calc(p);
	if (overhead != 0U) {
		LL_ASSERT_OVERHEAD(overhead);

		lll_event_abort(lll);

		return -ECANCELED;
	}

	lll->state = 0U;

	evt.cfg.aa = PDU_AC_ACCESS_ADDR;
	evt.cfg.crc_init = PDU_AC_CRC_IV;
	evt.cfg.phy = NBR_PHY_1M;
	evt.cfg.max_len = PDU_AC_LEG_PAYLOAD_SIZE_MAX;
#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	evt.cfg.tx_power = lll->tx_pwr_lvl;
#else /* !CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */
	evt.cfg.tx_power = RADIO_TXP_DEFAULT;
#endif /* !CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

	evt.filter = NULL;
	if (IS_ENABLED(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST) &&
	    ((lll->filter_policy & SCAN_FP_FILTER) != 0U)) {
		evt.filter = ull_filter_lll_get(true);
	}

	/* A scanner reports no times to the ULL, so the reference is not used */
	start_us = lll_event_start_get(p, &ticks_ref);
	rx(lll, start_us);

	if (!is_resume && (lll->ticks_window != 0U)) {
		uint32_t ret;

		ret = ticker_start(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_LLL, TICKER_ID_SCAN_STOP,
				   p->ticks_at_expire +
					   HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US),
				   lll->ticks_window, TICKER_NULL_PERIOD, TICKER_NULL_REMAINDER,
				   TICKER_NULL_LAZY, TICKER_NULL_SLOT, ticker_stop_cb, lll,
				   ticker_op_start_cb, NULL);
		LL_ASSERT_ERR((ret == TICKER_STATUS_SUCCESS) || (ret == TICKER_STATUS_BUSY));
	}

	err = lll_prepare_done(lll);
	LL_ASSERT_ERR(err == 0);

	DEBUG_RADIO_START_O(1);

	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	return common_prepare_cb(p, false);
}

static int resume_prepare_cb(struct lll_prepare_param *p)
{
	lll_resume_param_set(p);

	return common_prepare_cb(p, true);
}

static void isr_window(const struct lll_radio_evt *e, void *param)
{
	struct lll_scan *lll = param;

	ARG_UNUSED(e);

	lll->chan++;
	if (lll->chan == ADV_CHAN_MAX) {
		lll->chan = 0U;
	}

	rx_restart(lll);
}

static int is_abort_cb(void *next, void *curr, lll_prepare_cb_t *resume_cb)
{
	struct lll_scan *lll = curr;

	if (next != curr) {
		/* Put back to resume state for continuous scanning */
		if (lll->ticks_window == 0U) {
			int err;

			*resume_cb = resume_prepare_cb;

			/* Keep the HF clock on until the resume */
			err = lll_hfclock_on();
			LL_ASSERT_ERR(err >= 0);

			return -EAGAIN;
		}

		return -ECANCELED;
	}

	/* Continuous scanning preempts itself. Once the scan is flagged to stop,
	 * cancel, so that the done event that the scan disable waits for comes.
	 */
	if (lll->is_stop != 0U) {
		return -ECANCELED;
	}

	/* Switch scan window to next radio channel */
	lll_radio_stop(isr_window, lll);

	return 0;
}

static void abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
	int err;

	/* An event in progress rather than one in the prepare pipeline */
	if (prepare_param == NULL) {
		/* The event is done once the radio has been stopped */
		lll_radio_stop(isr_done_cleanup, param);

		return;
	}

	err = lll_hfclock_off();
	LL_ASSERT_ERR(err >= 0);

	lll_done(param);
}

void lll_scan_prepare(void *param)
{
	int err;

	err = lll_hfclock_on();
	LL_ASSERT_ERR(err >= 0);

	err = lll_prepare(is_abort_cb, abort_cb, prepare_cb, 0, param);
	LL_ASSERT_ERR((err == 0) || (err == -EINPROGRESS));
}
