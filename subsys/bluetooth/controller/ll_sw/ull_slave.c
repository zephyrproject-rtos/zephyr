/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <bluetooth/hci.h>
#include <sys/byteorder.h>

#include "util/util.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "hal/ticker.h"
#include "hal/ccm.h"

#include "ticker/ticker.h"

#include "pdu.h"
#include "ll.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_adv.h"
#include "lll_conn.h"
#include "lll_slave.h"
#include "lll_filter.h"
#include "lll_tim_internal.h"

#include "ull_adv_types.h"
#include "ull_conn_types.h"
#include "ull_filter.h"

#include "ull_internal.h"
#include "ull_adv_internal.h"
#include "ull_conn_internal.h"
#include "ull_slave_internal.h"

#define LOG_MODULE_NAME bt_ctlr_llsw_ull_slave
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static void ticker_op_stop_adv_cb(u32_t status, void *param);
static void ticker_op_cb(u32_t status, void *param);

void ull_slave_setup(memq_link_t *link, struct node_rx_hdr *rx,
		     struct node_rx_ftr *ftr, struct lll_conn *lll)
{
	u32_t conn_offset_us, conn_interval_us;
	u8_t ticker_id_adv, ticker_id_conn;
	u8_t peer_addr[BDADDR_SIZE];
	u32_t ticks_slot_overhead;
	u32_t mayfly_was_enabled;
	u32_t ticks_slot_offset;
	struct pdu_adv *pdu_adv;
	struct ll_adv_set *adv;
	struct node_rx_cc *cc;
	struct ll_conn *conn;
	u32_t ticker_status;
	u8_t peer_addr_type;
	u16_t win_offset;
	u16_t timeout;
	u16_t interval;
	u8_t chan_sel;

	((struct lll_adv *)ftr->param)->conn = NULL;

	adv = ((struct lll_adv *)ftr->param)->hdr.parent;
	conn = lll->hdr.parent;

	/* Populate the slave context */
	pdu_adv = (void *)((struct node_rx_pdu *)rx)->pdu;
	memcpy(&lll->crc_init[0], &pdu_adv->connect_ind.crc_init[0], 3);
	memcpy(&lll->access_addr[0], &pdu_adv->connect_ind.access_addr[0], 4);
	memcpy(&lll->data_chan_map[0], &pdu_adv->connect_ind.chan_map[0],
	       sizeof(lll->data_chan_map));
	lll->data_chan_count = util_ones_count_get(&lll->data_chan_map[0],
			       sizeof(lll->data_chan_map));
	lll->data_chan_hop = pdu_adv->connect_ind.hop;
	interval = sys_le16_to_cpu(pdu_adv->connect_ind.interval);
	lll->interval = interval;
	lll->latency = sys_le16_to_cpu(pdu_adv->connect_ind.latency);

	win_offset = sys_le16_to_cpu(pdu_adv->connect_ind.win_offset);
	conn_interval_us = interval * 1250U;

	/* calculate the window widening */
	lll->slave.sca = pdu_adv->connect_ind.sca;
	lll->slave.window_widening_periodic_us =
		(((lll_conn_ppm_local_get() +
		   lll_conn_ppm_get(lll->slave.sca)) *
		  conn_interval_us) + (1000000 - 1)) / 1000000U;
	lll->slave.window_widening_max_us = (conn_interval_us >> 1) -
					    EVENT_IFS_US;
	lll->slave.window_size_event_us = pdu_adv->connect_ind.win_size * 1250U;

	/* procedure timeouts */
	timeout = sys_le16_to_cpu(pdu_adv->connect_ind.timeout);
	conn->supervision_reload =
		RADIO_CONN_EVENTS((timeout * 10U * 1000U), conn_interval_us);
	conn->procedure_reload =
		RADIO_CONN_EVENTS((40 * 1000 * 1000), conn_interval_us);

#if defined(CONFIG_BT_CTLR_LE_PING)
	/* APTO in no. of connection events */
	conn->apto_reload = RADIO_CONN_EVENTS((30 * 1000 * 1000),
					      conn_interval_us);
	/* Dispatch LE Ping PDU 6 connection events (that peer would
	 * listen to) before 30s timeout
	 * TODO: "peer listens to" is greater than 30s due to latency
	 */
	conn->appto_reload = (conn->apto_reload > (lll->latency + 6)) ?
			     (conn->apto_reload - (lll->latency + 6)) :
			     conn->apto_reload;
#endif /* CONFIG_BT_CTLR_LE_PING */

	memcpy((void *)&conn->slave.force, &lll->access_addr[0],
	       sizeof(conn->slave.force));

#if defined(CONFIG_BT_CTLR_PRIVACY)
	u8_t own_addr_type = pdu_adv->rx_addr;
	u8_t own_addr[BDADDR_SIZE];
	u8_t rl_idx = ftr->rl_idx;

	memcpy(own_addr, &pdu_adv->connect_ind.adv_addr[0], BDADDR_SIZE);
#endif

	peer_addr_type = pdu_adv->tx_addr;
	memcpy(peer_addr, pdu_adv->connect_ind.init_addr, BDADDR_SIZE);

	chan_sel = pdu_adv->chan_sel;

	cc = (void *)pdu_adv;
	cc->status = 0U;
	cc->role = 1U;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	cc->own_addr_type = own_addr_type;
	memcpy(&cc->own_addr[0], &own_addr[0], BDADDR_SIZE);

	if (rl_idx != FILTER_IDX_NONE) {
		/* TODO: store rl_idx instead if safe */
		/* Store identity address */
		ll_rl_id_addr_get(rl_idx, &cc->peer_addr_type,
				  &cc->peer_addr[0]);
		/* Mark it as identity address from RPA (0x02, 0x03) */
		cc->peer_addr_type += 2;

		/* Store peer RPA */
		memcpy(&cc->peer_rpa[0], &peer_addr[0], BDADDR_SIZE);
	} else {
		memset(&cc->peer_rpa[0], 0x0, BDADDR_SIZE);
#else
	if (1) {
#endif /* CONFIG_BT_CTLR_PRIVACY */
		cc->peer_addr_type = peer_addr_type;
		memcpy(cc->peer_addr, peer_addr, BDADDR_SIZE);
	}

	cc->interval = lll->interval;
	cc->latency = lll->latency;
	cc->timeout = timeout;
	cc->sca = lll->slave.sca;

	lll->handle = ll_conn_handle_get(conn);
	rx->handle = lll->handle;

	/* Use Channel Selection Algorithm #2 if peer too supports it */
	if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
		struct node_rx_pdu *rx_csa;
		struct node_rx_cs *cs;

		/* pick the rx node instance stored within the connection
		 * rx node.
		 */
		rx_csa = (void *)ftr->extra;

		/* Enqueue the connection event */
		ll_rx_put(link, rx);

		/* use the rx node for CSA event */
		rx = (void *)rx_csa;
		link = rx->link;

		rx->handle = lll->handle;
		rx->type = NODE_RX_TYPE_CHAN_SEL_ALGO;

		cs = (void *)rx_csa->pdu;

		if (chan_sel) {
			u16_t aa_ls = ((u16_t)lll->access_addr[1] << 8) |
				      lll->access_addr[0];
			u16_t aa_ms = ((u16_t)lll->access_addr[3] << 8) |
				      lll->access_addr[2];

			lll->data_chan_sel = 1;
			lll->data_chan_id = aa_ms ^ aa_ls;

			cs->csa = 0x01;
		} else {
			cs->csa = 0x00;
		}
	}

	ll_rx_put(link, rx);
	ll_rx_sched();

	/* TODO: active_to_start feature port */
	conn->evt.ticks_active_to_start = 0U;
	conn->evt.ticks_xtal_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	conn->evt.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	conn->evt.ticks_slot =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US +
				       ftr->us_radio_rdy + 328 + EVENT_IFS_US +
				       328);

	ticks_slot_offset = MAX(conn->evt.ticks_active_to_start,
				conn->evt.ticks_xtal_to_start);

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = ticks_slot_offset;
	} else {
		ticks_slot_overhead = 0U;
	}

	conn_interval_us -= lll->slave.window_widening_periodic_us;

	conn_offset_us = ftr->us_radio_end;
	conn_offset_us += ((u64_t)win_offset + 1) * 1250U;
	conn_offset_us -= EVENT_OVERHEAD_START_US;
	conn_offset_us -= EVENT_JITTER_US << 1;
	conn_offset_us -= EVENT_JITTER_US;
	conn_offset_us -= ftr->us_radio_rdy;

#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	/* disable ticker job, in order to chain stop and start to avoid RTC
	 * being stopped if no tickers active.
	 */
	mayfly_was_enabled = mayfly_is_enabled(TICKER_USER_ID_ULL_HIGH,
					       TICKER_USER_ID_ULL_LOW);
	mayfly_enable(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 0);
#endif

	/* Stop Advertiser */
	ticker_id_adv = TICKER_ID_ADV_BASE + ull_adv_handle_get(adv);
	ticker_status = ticker_stop(TICKER_INSTANCE_ID_CTLR,
				    TICKER_USER_ID_ULL_HIGH,
				    ticker_id_adv, ticker_op_stop_adv_cb, adv);
	ticker_op_stop_adv_cb(ticker_status, adv);

	/* Stop Direct Adv Stop */
	if (adv->lll.is_hdcd) {
		/* Advertiser stop can expire while here in this ISR.
		 * Deferred attempt to stop can fail as it would have
		 * expired, hence ignore failure.
		 */
		ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
			    TICKER_ID_ADV_STOP, NULL, NULL);
	}

	/* Start Slave */
	ticker_id_conn = TICKER_ID_CONN_BASE + ll_conn_handle_get(conn);
	ticker_status = ticker_start(TICKER_INSTANCE_ID_CTLR,
				     TICKER_USER_ID_ULL_HIGH,
				     ticker_id_conn,
				     ftr->ticks_anchor - ticks_slot_offset,
				     HAL_TICKER_US_TO_TICKS(conn_offset_us),
				     HAL_TICKER_US_TO_TICKS(conn_interval_us),
				     HAL_TICKER_REMAINDER(conn_interval_us),
#if defined(CONFIG_BT_CTLR_CONN_META)
				     TICKER_LAZY_MUST_EXPIRE,
#else
				     TICKER_NULL_LAZY,
#endif /* CONFIG_BT_CTLR_CONN_META */
				     (conn->evt.ticks_slot +
				      ticks_slot_overhead),
				     ull_slave_ticker_cb, conn, ticker_op_cb,
				     (void *)__LINE__);
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));

#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	/* enable ticker job, if disabled in this function */
	if (mayfly_was_enabled) {
		mayfly_enable(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW,
			      1);
	}
#else
	ARG_UNUSED(mayfly_was_enabled);
#endif
}

/**
 * @brief Extract timing from completed event
 *
 * @param node_rx_event_done[in] Done event containing fresh timing information
 * @param ticks_drift_plus[out]  Positive part of drift uncertainty window
 * @param ticks_drift_minus[out] Negative part of drift uncertainty window
 */
void ull_slave_done(struct node_rx_event_done *done, u32_t *ticks_drift_plus,
		    u32_t *ticks_drift_minus)
{
	u32_t start_to_address_expected_us;
	u32_t start_to_address_actual_us;
	u32_t window_widening_event_us;
	u32_t preamble_to_addr_us;

	start_to_address_actual_us =
		done->extra.slave.start_to_address_actual_us;
	window_widening_event_us =
		done->extra.slave.window_widening_event_us;
	preamble_to_addr_us =
		done->extra.slave.preamble_to_addr_us;

	start_to_address_expected_us = EVENT_JITTER_US +
				       (EVENT_JITTER_US << 1) +
				       window_widening_event_us +
				       preamble_to_addr_us;

	if (start_to_address_actual_us <= start_to_address_expected_us) {
		*ticks_drift_plus =
			HAL_TICKER_US_TO_TICKS(window_widening_event_us);
		*ticks_drift_minus =
			HAL_TICKER_US_TO_TICKS((start_to_address_expected_us -
					       start_to_address_actual_us));
	} else {
		*ticks_drift_plus =
			HAL_TICKER_US_TO_TICKS(start_to_address_actual_us);
		*ticks_drift_minus =
			HAL_TICKER_US_TO_TICKS(EVENT_JITTER_US +
					       (EVENT_JITTER_US << 1) +
					       preamble_to_addr_us);
	}
}

void ull_slave_ticker_cb(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
			 void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_slave_prepare};
	static struct lll_prepare_param p;
	struct ll_conn *conn = param;
	u32_t err;
	u8_t ref;
	int ret;

	DEBUG_RADIO_PREPARE_S(1);

	/* Handle any LL Control Procedures */
	ret = ull_conn_llcp(conn, ticks_at_expire, lazy);
	if (ret) {
		return;
	}

	/* Increment prepare reference count */
	ref = ull_ref_inc(&conn->ull);
	LL_ASSERT(ref);

	/* Append timing parameters */
	p.ticks_at_expire = ticks_at_expire;
	p.remainder = remainder;
	p.lazy = lazy;
	p.param = &conn->lll;
	mfy.param = &p;

	/* Kick LLL prepare */
	err = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL,
			     0, &mfy);
	LL_ASSERT(!err);

	/* De-mux remaining tx nodes from FIFO */
	ull_conn_tx_demux(UINT8_MAX);

	/* Enqueue towards LLL */
	ull_conn_tx_lll_enqueue(conn, UINT8_MAX);

	DEBUG_RADIO_PREPARE_S(1);
}

#if defined(CONFIG_BT_CTLR_LE_ENC)
u8_t ll_start_enc_req_send(u16_t handle, u8_t error_code,
			    u8_t const *const ltk)
{
	struct ll_conn *conn;
	u8_t ret;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (error_code) {
		if (conn->llcp_enc.refresh == 0U) {
			ret = ull_conn_llcp_req(conn);
			if (ret) {
				return ret;
			}

			conn->llcp.encryption.error_code = error_code;
			conn->llcp.encryption.initiate = 0U;

			conn->llcp_type = LLCP_ENCRYPTION;
			conn->llcp_req++;
		} else {
			if (conn->llcp_terminate.ack !=
			    conn->llcp_terminate.req) {
				return BT_HCI_ERR_CMD_DISALLOWED;
			}

			conn->llcp_terminate.reason_own = error_code;

			conn->llcp_terminate.req++;
		}
	} else {
		ret = ull_conn_llcp_req(conn);
		if (ret) {
			return ret;
		}

		memcpy(&conn->llcp_enc.ltk[0], ltk, sizeof(conn->llcp_enc.ltk));

		conn->llcp.encryption.error_code = 0U;
		conn->llcp.encryption.initiate = 0U;

		conn->llcp_type = LLCP_ENCRYPTION;
		conn->llcp_req++;
	}

	return 0;
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

static void ticker_op_stop_adv_cb(u32_t status, void *param)
{
	LL_ASSERT(status != TICKER_STATUS_FAILURE ||
		  param == ull_disable_mark_get());
}

static void ticker_op_cb(u32_t status, void *param)
{
	ARG_UNUSED(param);

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);
}
