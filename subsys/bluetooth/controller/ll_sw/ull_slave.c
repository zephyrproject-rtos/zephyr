/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <soc.h>
#include <bluetooth/hci.h>
#include <sys/byteorder.h>

#include "util/util.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "ticker/ticker.h"

#include "pdu.h"

#include "lll.h"
#include "lll_clock.h"
#include "lll/lll_vendor.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_conn.h"
#include "lll_slave.h"
#include "lll_filter.h"

#include "ull_adv_types.h"
#include "ull_conn_types.h"
#include "ull_filter.h"

#include "ull_internal.h"
#include "ull_adv_internal.h"
#include "ull_conn_internal.h"
#include "ull_slave_internal.h"

#include "ll.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_slave
#include "common/log.h"
#include "hal/debug.h"

static void ticker_op_stop_adv_cb(uint32_t status, void *param);
static void ticker_op_cb(uint32_t status, void *param);
static void ticker_update_latency_cancel_op_cb(uint32_t ticker_status,
					       void *params);

void ull_slave_setup(memq_link_t *link, struct node_rx_hdr *rx,
		     struct node_rx_ftr *ftr, struct lll_conn *lll)
{
	uint32_t conn_offset_us, conn_interval_us;
	uint8_t ticker_id_adv, ticker_id_conn;
	uint8_t peer_addr[BDADDR_SIZE];
	uint32_t ticks_slot_overhead;
	uint32_t ticks_slot_offset;
	struct pdu_adv *pdu_adv;
	struct ll_adv_set *adv;
	struct node_rx_cc *cc;
	struct ll_conn *conn;
	uint32_t ready_delay_us;
	uint32_t ticker_status;
	uint8_t peer_addr_type;
	uint16_t win_offset;
	uint16_t win_delay_us;
	uint16_t timeout;
	uint16_t interval;
	uint8_t chan_sel;

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
	if (lll->data_chan_count < 2) {
		return;
	}
	lll->data_chan_hop = pdu_adv->connect_ind.hop;
	if ((lll->data_chan_hop < 5) || (lll->data_chan_hop > 16)) {
		return;
	}

	((struct lll_adv *)ftr->param)->conn = NULL;

	interval = sys_le16_to_cpu(pdu_adv->connect_ind.interval);
	lll->interval = interval;
	lll->latency = sys_le16_to_cpu(pdu_adv->connect_ind.latency);

	win_offset = sys_le16_to_cpu(pdu_adv->connect_ind.win_offset);
	conn_interval_us = interval * CONN_INT_UNIT_US;

	if (0) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	} else if (adv->lll.aux) {
		if (adv->lll.phy_s & BIT(2)) {
			win_delay_us = WIN_DELAY_CODED;
		} else {
			win_delay_us = WIN_DELAY_UNCODED;
		}
#endif
	} else {
		win_delay_us = WIN_DELAY_LEGACY;
	}

	/* calculate the window widening */
	conn->slave.sca = pdu_adv->connect_ind.sca;
	lll->slave.window_widening_periodic_us =
		(((lll_clock_ppm_local_get() +
		   lll_clock_ppm_get(conn->slave.sca)) *
		  conn_interval_us) + (1000000 - 1)) / 1000000U;
	lll->slave.window_widening_max_us = (conn_interval_us >> 1) -
					    EVENT_IFS_US;
	lll->slave.window_size_event_us = pdu_adv->connect_ind.win_size *
		CONN_INT_UNIT_US;

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

#if defined(CONFIG_BT_CTLR_CONN_RANDOM_FORCE)
	memcpy((void *)&conn->slave.force, &lll->access_addr[0],
	       sizeof(conn->slave.force));
#endif /* CONFIG_BT_CTLR_CONN_RANDOM_FORCE */

	peer_addr_type = pdu_adv->tx_addr;
	memcpy(peer_addr, pdu_adv->connect_ind.init_addr, BDADDR_SIZE);

	if (0) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	} else if (adv->lll.aux) {
		chan_sel = 1U;
#endif
	} else {
		chan_sel = pdu_adv->chan_sel;
	}

	cc = (void *)pdu_adv;
	cc->status = 0U;
	cc->role = 1U;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	uint8_t rl_idx = ftr->rl_idx;

	if (ull_filter_lll_lrpa_used(adv->lll.rl_idx)) {
		memcpy(&cc->local_rpa[0], &pdu_adv->connect_ind.adv_addr[0],
		       BDADDR_SIZE);
	} else {
		memset(&cc->local_rpa[0], 0x0, BDADDR_SIZE);
	}

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
	cc->sca = conn->slave.sca;

	lll->handle = ll_conn_handle_get(conn);
	rx->handle = lll->handle;

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	lll->tx_pwr_lvl = RADIO_TXP_DEFAULT;
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

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
			uint16_t aa_ls = ((uint16_t)lll->access_addr[1] << 8) |
				      lll->access_addr[0];
			uint16_t aa_ms = ((uint16_t)lll->access_addr[3] << 8) |
				      lll->access_addr[2];

			lll->data_chan_sel = 1;
			lll->data_chan_id = aa_ms ^ aa_ls;

			cs->csa = 0x01;
		} else {
			cs->csa = 0x00;
		}
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	if (ll_adv_cmds_is_ext()) {
		uint8_t handle;

		/* Enqueue connection or CSA event */
		ll_rx_put(link, rx);

		/* use reserved link and node_rx to prepare
		 * advertising terminate event
		 */
		rx = adv->lll.node_rx_adv_term;
		link = rx->link;

		handle = ull_adv_handle_get(adv);
		LL_ASSERT(handle < BT_CTLR_ADV_SET);

		rx->type = NODE_RX_TYPE_EXT_ADV_TERMINATE;
		rx->handle = handle;
		rx->rx_ftr.param_adv_term.status = 0U;
		rx->rx_ftr.param_adv_term.conn_handle = lll->handle;
		rx->rx_ftr.param_adv_term.num_events = 0U;
	}
#endif

	ll_rx_put(link, rx);
	ll_rx_sched();

#if defined(CONFIG_BT_CTLR_PHY)
	ready_delay_us = lll_radio_rx_ready_delay_get(lll->phy_rx, 1);
#else
	ready_delay_us = lll_radio_rx_ready_delay_get(0, 0);
#endif

	/* TODO: active_to_start feature port */
	conn->evt.ticks_active_to_start = 0U;
	conn->evt.ticks_xtal_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	conn->evt.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	conn->evt.ticks_slot =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US +
				       ready_delay_us +
				       328 + EVENT_IFS_US + 328);

	ticks_slot_offset = MAX(conn->evt.ticks_active_to_start,
				conn->evt.ticks_xtal_to_start);

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = ticks_slot_offset;
	} else {
		ticks_slot_overhead = 0U;
	}

	conn_interval_us -= lll->slave.window_widening_periodic_us;

	conn_offset_us = ftr->radio_end_us;
	conn_offset_us += win_offset * CONN_INT_UNIT_US;
	conn_offset_us += win_delay_us;
	conn_offset_us -= EVENT_OVERHEAD_START_US;
	conn_offset_us -= EVENT_TICKER_RES_MARGIN_US;
	conn_offset_us -= EVENT_JITTER_US;
	conn_offset_us -= ready_delay_us;

#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	/* disable ticker job, in order to chain stop and start to avoid RTC
	 * being stopped if no tickers active.
	 */
	mayfly_enable(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 0);
#endif

#if defined(CONFIG_BT_CTLR_ADV_EXT) && (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
	struct lll_adv_aux *lll_aux = adv->lll.aux;

	if (lll_aux) {
		struct ll_adv_aux_set *aux;

		aux = (void *)HDR_LLL2EVT(lll_aux);

		ticker_id_adv = TICKER_ID_ADV_AUX_BASE +
				ull_adv_aux_handle_get(aux);
		ticker_status = ticker_stop(TICKER_INSTANCE_ID_CTLR,
					    TICKER_USER_ID_ULL_HIGH,
					    ticker_id_adv,
					    ticker_op_stop_adv_cb, aux);
		ticker_op_stop_adv_cb(ticker_status, aux);

		aux->is_started = 0U;
	}
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
				     TICKER_NULL_LAZY,
				     (conn->evt.ticks_slot +
				      ticks_slot_overhead),
				     ull_slave_ticker_cb, conn, ticker_op_cb,
				     (void *)__LINE__);
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));

#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	/* enable ticker job, irrespective of disabled in this function so
	 * first connection event can be scheduled as soon as possible.
	 */
	mayfly_enable(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 1);
#endif
}

void ull_slave_latency_cancel(struct ll_conn *conn, uint16_t handle)
{
	/* break peripheral latency */
	if (conn->lll.latency_event && !conn->slave.latency_cancel) {
		uint32_t ticker_status;

		conn->slave.latency_cancel = 1U;

		ticker_status =
			ticker_update(TICKER_INSTANCE_ID_CTLR,
				      TICKER_USER_ID_THREAD,
				      (TICKER_ID_CONN_BASE + handle),
				      0, 0, 0, 0, 1, 0,
				      ticker_update_latency_cancel_op_cb,
				      (void *)conn);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
	}
}

void ull_slave_ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
			 uint16_t lazy, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_slave_prepare};
	static struct lll_prepare_param p;
	struct ll_conn *conn;
	uint32_t err;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_S(1);

	conn = param;

	/* Check if stopping ticker (on disconnection, race with ticker expiry)
	 */
	if (unlikely(conn->lll.handle == 0xFFFF)) {
		DEBUG_RADIO_CLOSE_S(0);
		return;
	}

#if defined(CONFIG_BT_CTLR_CONN_META)
	conn->common.is_must_expire = (lazy == TICKER_LAZY_MUST_EXPIRE);
#endif
	/* If this is a must-expire callback, LLCP state machine does not need
	 * to know. Will be called with lazy > 0 when scheduled in air.
	 */
	if (!IS_ENABLED(CONFIG_BT_CTLR_CONN_META) ||
	    (lazy != TICKER_LAZY_MUST_EXPIRE)) {
		int ret;

		/* Handle any LL Control Procedures */
		ret = ull_conn_llcp(conn, ticks_at_expire, lazy);
		if (ret) {
			DEBUG_RADIO_CLOSE_S(0);
			return;
		}
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
uint8_t ll_start_enc_req_send(uint16_t handle, uint8_t error_code,
			    uint8_t const *const ltk)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (error_code) {
		if (conn->llcp_enc.refresh == 0U) {
			if ((conn->llcp_req == conn->llcp_ack) ||
			     (conn->llcp_type != LLCP_ENCRYPTION)) {
				return BT_HCI_ERR_CMD_DISALLOWED;
			}

			conn->llcp.encryption.error_code = error_code;
			conn->llcp.encryption.state = LLCP_ENC_STATE_INPROG;
		} else {
			if (conn->llcp_terminate.ack !=
			    conn->llcp_terminate.req) {
				return BT_HCI_ERR_CMD_DISALLOWED;
			}

			conn->llcp_terminate.reason_own = error_code;

			conn->llcp_terminate.req++;
		}
	} else {
		if ((conn->llcp_req == conn->llcp_ack) ||
		     (conn->llcp_type != LLCP_ENCRYPTION)) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		memcpy(&conn->llcp_enc.ltk[0], ltk,
		       sizeof(conn->llcp_enc.ltk));

		conn->llcp.encryption.error_code = 0U;
		conn->llcp.encryption.state = LLCP_ENC_STATE_INPROG;
	}

	return 0;
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

static void ticker_op_stop_adv_cb(uint32_t status, void *param)
{
	LL_ASSERT(status != TICKER_STATUS_FAILURE ||
		  param == ull_disable_mark_get());
}

static void ticker_op_cb(uint32_t status, void *param)
{
	ARG_UNUSED(param);

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);
}

static void ticker_update_latency_cancel_op_cb(uint32_t ticker_status,
					       void *params)
{
	struct ll_conn *conn = params;

	LL_ASSERT(ticker_status == TICKER_STATUS_SUCCESS);

	conn->slave.latency_cancel = 0U;
}
