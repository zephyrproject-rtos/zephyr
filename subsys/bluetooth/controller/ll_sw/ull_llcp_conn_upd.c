/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

#include <bluetooth/hci.h>
#include <sys/byteorder.h>
#include <sys/slist.h>
#include <sys/util.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/ticker.h"

#include "util/mem.h"
#include "util/memq.h"
#include "util/mfifo.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"
#include "ll.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll_conn.h"
#include "lll_clock.h"

#include "ull_tx_queue.h"
#include "ull_conn_llcp_internal.h"
#include "ull_conn_types.h"
#include "ull_internal.h"
#include "ull_llcp.h"
#include "ull_llcp_internal.h"

#include "ull_slave_internal.h"
#include "ull_master_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_llcp_conn_upd
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

/* LLCP Local Procedure Connection Update FSM states */
enum {
	LP_CU_STATE_IDLE,
	LP_CU_STATE_WAIT_TX_CONN_PARAM_REQ,
	LP_CU_STATE_WAIT_RX_CONN_PARAM_RSP,
	LP_CU_STATE_WAIT_TX_CONN_UPDATE_IND,
	LP_CU_STATE_WAIT_RX_CONN_UPDATE_IND,
	LP_CU_STATE_WAIT_INSTANT,
	LP_CU_STATE_WAIT_NTF,
};

/* LLCP Local Procedure Connection Update FSM events */
enum {
	/* Procedure run */
	LP_CU_EVT_RUN,

	/* Response recieved */
	LP_CU_EVT_CONN_PARAM_RSP,

	/* Indication recieved */
	LP_CU_EVT_CONN_UPDATE_IND,

	/* Reject response recieved */
	LP_CU_EVT_REJECT,

	/* Unknown response recieved */
	LP_CU_EVT_UNKNOWN,
};

/* LLCP Remote Procedure Connection Update FSM states */
enum {
	RP_CU_STATE_IDLE,
	RP_CU_STATE_WAIT_RX_CONN_PARAM_REQ,
	RP_CU_STATE_WAIT_NTF_CONN_PARAM_REQ,
	RP_CU_STATE_WAIT_CONN_PARAM_REQ_REPLY,
	RP_CU_STATE_WAIT_TX_REJECT_IND,
	RP_CU_STATE_WAIT_TX_CONN_PARAM_RSP,
	RP_CU_STATE_WAIT_TX_CONN_UPDATE_IND,
	RP_CU_STATE_WAIT_RX_CONN_UPDATE_IND,
	RP_CU_STATE_WAIT_INSTANT,
	RP_CU_STATE_WAIT_NTF,
};

/* LLCP Remote Procedure Connection Update FSM events */
enum {
	/* Procedure run */
	RP_CU_EVT_RUN,

	/* Request recieved */
	RP_CU_EVT_CONN_PARAM_REQ,

	/* Indication recieved */
	RP_CU_EVT_CONN_UPDATE_IND,

	/* CONN_PARAM_REQ reply */
	RP_CU_EVT_CONN_PARAM_REQ_REPLY,

	/* CONN_PARAM_REQ negative reply */
	RP_CU_EVT_CONN_PARAM_REQ_NEG_REPLY,
};

static void ticker_op_stop_cb(uint32_t status, void *param);
static void ticker_start_conn_op_cb(uint32_t status, void *param);
static void tx_lll_flush(void *param);

/*
 * LLCP Local Procedure Connection Update FSM
 */

static uint16_t lp_event_counter(struct ll_conn *conn)
{
	struct lll_conn *lll;
	uint16_t event_counter;

	/* TODO(thoh): Lazy hardcoded */
	uint16_t lazy = 0;

	/**/
	lll = &conn->lll;

	/* Calculate current event counter */
	event_counter = lll->event_counter + lll->latency_prepare + lazy;

	return event_counter;
}

static void lp_cu_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = tx_alloc();
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (opcode) {
	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ:
		pdu_encode_conn_param_req(ctx, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND:
		pdu_encode_conn_update_ind(ctx, pdu);
		break;
	default:
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	tx_enqueue(conn, tx);
}

static void lp_cu_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct node_rx_cu *pdu;

	/* Allocate ntf node */
	ntf = ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_CONN_UPDATE;
	ntf->hdr.handle = conn->lll.handle;
	pdu = (struct node_rx_cu *)ntf->pdu;

	pdu->status = ctx->data.cu.error;

	/* Enqueue notification towards LL */
	ll_rx_put(ntf->hdr.link, ntf);
	ll_rx_sched();
}

static void cu_apply_params(struct ll_conn *conn, struct proc_ctx *ctx)		//TODO(tosk): Split into smaller functions!
{
	struct lll_conn *lll;
	uint32_t ticks_win_offset = 0;
	uint32_t ticks_slot_overhead;
	uint16_t conn_interval_old;
	uint16_t conn_interval_new;
	uint32_t conn_interval_us;
	uint8_t ticker_id_conn;
	uint32_t ticker_status;
	uint32_t periodic_us;
	uint16_t latency;
	uint16_t instant_latency;
	uint16_t event_counter;

	lll = &conn->lll;

	/* Calculate current event counter */
	event_counter = lp_event_counter(conn);

	instant_latency = (event_counter - conn->llcp.conn_upd.instant) &
			  0xFFFF;

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	/* Stop procedure timeout */
	conn->procedure_expire = 0U;						//TODO(tosk): check when this applies!
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

	/* check if paramaters are changed (and set flag if so) */
	if ((ctx->data.cu.interval != lll->interval) ||
	    (ctx->data.cu.latency != lll->latency) ||
	    (RADIO_CONN_EVENTS(ctx->data.cu.timeout * 10000U,
			       lll->interval * CONN_INT_UNIT_US) !=
	     conn->supervision_reload)) {
		ctx->data.cu.params_changed = 1U;
	}

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
	/* restore to normal prepare */
	if (conn->ull.ticks_prepare_to_start & XON_BITMASK) {
		uint32_t ticks_prepare_to_start =
			MAX(conn->ull.ticks_active_to_start,
			    conn->ull.ticks_preempt_to_start);

		conn->ull.ticks_prepare_to_start &= ~XON_BITMASK;
		conn->llcp.conn_upd.ticks_at_expire -= 				//TODO(tosk): place of 'ticks_at_expire'?
			(conn->ull.ticks_prepare_to_start -
				    ticks_prepare_to_start);
	}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

	/* compensate for instant_latency due to laziness */
	conn_interval_old = instant_latency * lll->interval;
	latency = conn_interval_old / ctx->data.cu.interval;
	conn_interval_new = latency * ctx->data.cu.interval;
	if (conn_interval_new > conn_interval_old) {
		conn->llcp.conn_upd.ticks_at_expire += HAL_TICKER_US_TO_TICKS(
			(conn_interval_new - conn_interval_old) *
			CONN_INT_UNIT_US);
	} else {
		conn->llcp.conn_upd.ticks_at_expire -= HAL_TICKER_US_TO_TICKS(
			(conn_interval_old - conn_interval_new) *
			CONN_INT_UNIT_US);
	}
	lll->latency_prepare += conn->llcp.conn_upd.lazy;			//TODO(tosk): place of 'lazy'?
	lll->latency_prepare -= (instant_latency - latency);

	/* calculate the offset */
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead =
			MAX(conn->ull.ticks_active_to_start,
			    conn->ull.ticks_prepare_to_start);
	} else {
		ticks_slot_overhead = 0U;
	}

	/* calculate the window widening and interval */
	conn_interval_us = ctx->data.cu.interval * CONN_INT_UNIT_US;
	periodic_us = conn_interval_us;

	switch (lll->role) {
#if defined(CONFIG_BT_PERIPHERAL)
	case BT_HCI_ROLE_SLAVE:
		lll->slave.window_widening_prepare_us -=
			lll->slave.window_widening_periodic_us *
			instant_latency;

		lll->slave.window_widening_periodic_us =
			(((lll_clock_ppm_local_get() +
			   lll_clock_ppm_get(conn->slave.sca)) *
			  conn_interval_us) + (1000000 - 1)) / 1000000U;
		lll->slave.window_widening_max_us =
			(conn_interval_us >> 1) - EVENT_IFS_US;
		lll->slave.window_size_prepare_us =
			ctx->data.cu.win_size * CONN_INT_UNIT_US;

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		conn->slave.ticks_to_offset = 0U;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

		lll->slave.window_widening_prepare_us +=
			lll->slave.window_widening_periodic_us *
			latency;
		if (lll->slave.window_widening_prepare_us >
		    lll->slave.window_widening_max_us) {
			lll->slave.window_widening_prepare_us =
				lll->slave.window_widening_max_us;
		}

		conn->llcp.conn_upd.ticks_at_expire -= HAL_TICKER_US_TO_TICKS(
			lll->slave.window_widening_periodic_us *
			latency);
		ticks_win_offset = HAL_TICKER_US_TO_TICKS(
			(ctx->data.cu.win_offset_us /
			CONN_INT_UNIT_US) * CONN_INT_UNIT_US);
		periodic_us -= lll->slave.window_widening_periodic_us;
		break;
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CENTRAL)
	case BT_HCI_ROLE_MASTER:
		ticks_win_offset = HAL_TICKER_US_TO_TICKS(
			ctx->data.cu.win_offset_us);

		/* Workaround: Due to the missing remainder param in
		 * ticker_start function for first interval; add a
		 * tick so as to use the ceiled value.
		 */
		ticks_win_offset += 1U;
		break;
#endif /*CONFIG_BT_CENTRAL */
	default:
		LL_ASSERT(0);
		break;
	}

	lll->interval = ctx->data.cu.interval;
	lll->latency = ctx->data.cu.latency;

	conn->supervision_reload =
		RADIO_CONN_EVENTS((ctx->data.cu.timeout * 10U * 1000U),
				  conn_interval_us);
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

	conn->supervision_expire = 0U; 						//TODO(tosk): Check this (ConnUpd/ParamReq)!

#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	/* disable ticker job, in order to chain stop and start
	 * to avoid RTC being stopped if no tickers active.
	 */
	uint32_t mayfly_was_enabled =
		mayfly_is_enabled(TICKER_USER_ID_ULL_HIGH,
				  TICKER_USER_ID_ULL_LOW);
	mayfly_enable(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW,
		      0);
#endif /* CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO */

	/* start slave/master with new timings */
	ticker_id_conn = TICKER_ID_CONN_BASE + ll_conn_handle_get(conn);
	ticker_status =	ticker_stop(TICKER_INSTANCE_ID_CTLR,
				    TICKER_USER_ID_ULL_HIGH,
				    ticker_id_conn,
				    ticker_op_stop_cb,
				    (void *)conn);
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));
	ticker_status =
		ticker_start(TICKER_INSTANCE_ID_CTLR,
			     TICKER_USER_ID_ULL_HIGH,
			     ticker_id_conn,
			     conn->llcp.conn_upd.ticks_at_expire,
			     ticks_win_offset,
			     HAL_TICKER_US_TO_TICKS(periodic_us),
			     HAL_TICKER_REMAINDER(periodic_us),
#if defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
			     TICKER_NULL_LAZY,
#else /* !CONFIG_BT_TICKER_COMPATIBILITY_MODE */
			     TICKER_LAZY_MUST_EXPIRE_KEEP,
#endif /* CONFIG_BT_TICKER_COMPATIBILITY_MODE */
			     (ticks_slot_overhead +
			      conn->ull.ticks_slot),
#if defined(CONFIG_BT_PERIPHERAL) && defined(CONFIG_BT_CENTRAL)
			     (lll->role == BT_HCI_ROLE_SLAVE) ?
			     		ull_slave_ticker_cb :
					ull_master_ticker_cb,
#elif defined(CONFIG_BT_PERIPHERAL)
			     ull_slave_ticker_cb,
#else
			     ull_master_ticker_cb,
#endif /* CONFIG_BT_PERIPHERAL && CONFIG_BT_CENTRAL */
			     conn, ticker_start_conn_op_cb,
			     (void *)conn);
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));

#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	/* enable ticker job, if disabled in this function */
	if (mayfly_was_enabled) {
		mayfly_enable(TICKER_USER_ID_ULL_HIGH,
			      TICKER_USER_ID_ULL_LOW, 1);
	}
#endif /* CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO */
}

static void lp_cu_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	/*
	 * ConnUpd: Notify Host if any of the three conn. params have changed.
	 * If no conn params are changed, the Host would not be notified.

	 * ParamReq: Notify Host with error.
	 */
	if ((ctx->data.cu.params_changed != 0U) || (ctx->data.cu.error != 0U)) {//TODO(tosk): is this OK?
		if (!ntf_alloc_is_available()) {
			ctx->state = LP_CU_STATE_WAIT_NTF;
		} else {
			lp_cu_ntf(conn, ctx);
			lr_complete(conn);
			ctx->state = LP_CU_STATE_IDLE;
		}
	} else {
		lr_complete(conn);
		ctx->state = LP_CU_STATE_IDLE;
	}
}

static void lp_cu_send_conn_param_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!tx_alloc_is_available() || rr_get_collision(conn)) {
		ctx->state = LP_CU_STATE_WAIT_TX_CONN_PARAM_REQ;
	} else {
		rr_set_incompat(conn, INCOMPAT_RESOLVABLE);

		lp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ);

		switch (conn->lll.role) {
		case BT_HCI_ROLE_MASTER:
			ctx->state = LP_CU_STATE_WAIT_RX_CONN_PARAM_RSP;
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP;
			break;
		case BT_HCI_ROLE_SLAVE:
			ctx->state = LP_CU_STATE_WAIT_RX_CONN_UPDATE_IND;
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND;
			break;
		default:
			/* Unknown role */
			LL_ASSERT(0);
		}
	}
}

static void lp_cu_send_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = LP_CU_STATE_WAIT_TX_CONN_UPDATE_IND;
	} else {
		/* TODO(thoh): Hardcoded instant delta +6 */
		ctx->data.cu.instant = lp_event_counter(conn) + 6;
		lp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND);
		ctx->rx_opcode = 0xFF; /* TODO(thoh): Hmm */
		ctx->state = LP_CU_STATE_WAIT_INSTANT;
	}
}

static void lp_cu_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_CU_EVT_RUN:
		switch(ctx->state) {
		case LP_CU_STATE_WAIT_TX_CONN_PARAM_REQ:
			lp_cu_send_conn_param_req(conn, ctx, evt, param);
			break;
		case LP_CU_STATE_WAIT_TX_CONN_UPDATE_IND:
			lp_cu_send_conn_update_ind(conn, ctx, evt, param);
			break;
		default:
			/* Unknown state */
			LL_ASSERT(0);
		}
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cu_st_wait_tx_conn_param_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_CU_EVT_RUN:
		lp_cu_send_conn_param_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cu_st_wait_rx_conn_param_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	struct pdu_data *pdu = (struct pdu_data *) param;

	switch (evt) {
	case LP_CU_EVT_CONN_PARAM_RSP:
		rr_set_incompat(conn, INCOMPAT_RESERVED);
		lp_cu_send_conn_update_ind(conn, ctx, evt, param);
		break;
	case LP_CU_EVT_UNKNOWN:
		rr_set_incompat(conn, INCOMPAT_RESERVED);
		lp_cu_send_conn_update_ind(conn, ctx, evt, param);
		break;
	case LP_CU_EVT_REJECT:
		rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
		ctx->data.cu.error = pdu->llctrl.reject_ext_ind.error_code;
		lp_cu_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cu_st_wait_tx_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_CU_EVT_RUN:
		lp_cu_send_conn_update_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cu_st_wait_rx_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	struct pdu_data *pdu = (struct pdu_data *) param;

	switch (evt) {
	case LP_CU_EVT_CONN_UPDATE_IND:
		pdu_decode_conn_update_ind(ctx, param);
		ctx->state = LP_CU_STATE_WAIT_INSTANT;
		break;
	case LP_CU_EVT_UNKNOWN:
		ctx->data.cu.error = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
		lp_cu_complete(conn, ctx, evt, param);
		break;
	case LP_CU_EVT_REJECT:
		ctx->data.cu.error = pdu->llctrl.reject_ext_ind.error_code;
		lp_cu_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cu_check_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	uint16_t event_counter = lp_event_counter(conn);
	if (is_instant_reached_or_passed(ctx->data.cu.instant, event_counter)) {
		/* Procedure is complete when the instant has passed, and the
		 * new connection event parameters have been applied.
		 */
		cu_apply_params(conn, ctx);
		rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
		ctx->data.cu.error = BT_HCI_ERR_SUCCESS;
		lp_cu_complete(conn, ctx, evt, param);
	}
}

static void lp_cu_st_wait_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case LP_CU_EVT_RUN:
		lp_cu_check_instant(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cu_st_wait_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_CU_EVT_RUN:
		lp_cu_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cu_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->state) {
	case LP_CU_STATE_IDLE:
		lp_cu_st_idle(conn, ctx, evt, param);
		break;
	case LP_CU_STATE_WAIT_TX_CONN_PARAM_REQ:
		lp_cu_st_wait_tx_conn_param_req(conn, ctx, evt, param);
		break;
	case LP_CU_STATE_WAIT_RX_CONN_PARAM_RSP:
		lp_cu_st_wait_rx_conn_param_rsp(conn, ctx, evt, param);
		break;
	case LP_CU_STATE_WAIT_TX_CONN_UPDATE_IND:
		lp_cu_st_wait_tx_conn_update_ind(conn, ctx, evt, param);
		break;
	case LP_CU_STATE_WAIT_RX_CONN_UPDATE_IND:
		lp_cu_st_wait_rx_conn_update_ind(conn, ctx, evt, param);
		break;
	case LP_CU_STATE_WAIT_INSTANT:
		lp_cu_st_wait_instant(conn, ctx, evt, param);
		break;
	case LP_CU_STATE_WAIT_NTF:
		lp_cu_st_wait_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_lp_cu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *) rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP:
		lp_cu_execute_fsm(conn, ctx, LP_CU_EVT_CONN_PARAM_RSP, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND:
		lp_cu_execute_fsm(conn, ctx, LP_CU_EVT_CONN_UPDATE_IND, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		lp_cu_execute_fsm(conn, ctx, LP_CU_EVT_UNKNOWN, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		lp_cu_execute_fsm(conn, ctx, LP_CU_EVT_REJECT, pdu);
		break;
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_lp_cu_init_proc(struct proc_ctx *ctx)
{
	//TODO(tosk): The state selection here will bypass the idle state! Check if OK!
	switch(ctx->proc) {
	case PROC_CONN_UPDATE:
		ctx->state = LP_CU_STATE_WAIT_TX_CONN_UPDATE_IND;
		break;
	case PROC_CONN_PARAM_REQ:
		ctx->state = LP_CU_STATE_WAIT_TX_CONN_PARAM_REQ;
		break;
	default:
		/* Unknown procedure */
		LL_ASSERT(0);

	}
}

void ull_cp_priv_lp_cu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	lp_cu_execute_fsm(conn, ctx, LP_CU_EVT_RUN, param);
}

/*
 * LLCP Remote Procedure Connection Update FSM
 */

static uint16_t rp_event_counter(struct ll_conn *conn)
{
	struct lll_conn *lll;
	uint16_t event_counter;

	/* TODO(thoh): Lazy hardcoded */
	uint16_t lazy = 0;

	/**/
	lll = &conn->lll;

	/* Calculate current event counter */
	event_counter = lll->event_counter + lll->latency_prepare + lazy;

	return event_counter;
}

static void rp_cu_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = tx_alloc();
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (opcode) {
	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP:
		ctx->data.conn_param.interval_min = conn->llcp.conn_param.interval_min;
		ctx->data.conn_param.interval_max = conn->llcp.conn_param.interval_max;
		ctx->data.conn_param.latency = conn->llcp.conn_param.latency;
		ctx->data.conn_param.timeout = conn->llcp.conn_param.timeout;
		pdu_encode_conn_param_rsp(ctx, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND:
		ctx->data.cu.win_size = 1U;
		ctx->data.cu.win_offset_us = 0U;
		ctx->data.cu.interval = conn->llcp.cu.interval;
		ctx->data.cu.latency = conn->llcp.cu.latency;
		ctx->data.cu.timeout = conn->llcp.cu.timeout;
		pdu_encode_conn_update_ind(ctx, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
		/* TODO(thoh): Select between LL_REJECT_IND and LL_REJECT_EXT_IND */
		pdu_encode_reject_ext_ind(pdu, PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ, BT_HCI_ERR_UNACCEPT_CONN_PARAM);
		break;
	default:
		LL_ASSERT(0);
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	tx_enqueue(conn, tx);
}

static void rp_cu_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct node_rx_cu *pdu;

	/* Allocate ntf node */
	ntf = ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_CONN_UPDATE;
	ntf->hdr.handle = conn->lll.handle;
	pdu = (struct node_rx_cu *)ntf->pdu;

	pdu->status = ctx->data.cu.error;

	/* Enqueue notification towards LL */
	ll_rx_put(ntf->hdr.link, ntf);
	ll_rx_sched();
}

static void rp_cu_conn_param_req_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;

	/* Allocate ntf node */
	ntf = ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	ntf->hdr.handle = conn->lll.handle;
	pdu = (struct pdu_data *) ntf->pdu;

	pdu_encode_conn_param_req(ctx, pdu);

	/* Enqueue notification towards LL */
	ll_rx_put(ntf->hdr.link, ntf);
	ll_rx_sched();
}

static void rp_cu_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	/*
	 * ConnUpd: Notify Host if any of the three conn. params have changed.
	 * If no conn params are changed, the Host would not be notified.
	 *
	 * ParamReq: Notify Host with error.
	 */
	if ((ctx->data.cu.params_changed != 0U) || (ctx->data.cu.error != 0U)) {//TODO(tosk): is this OK?
		if (!ntf_alloc_is_available()) {
			ctx->state = RP_CU_STATE_WAIT_NTF;
		} else {
			rp_cu_ntf(conn, ctx);
			rr_complete(conn);
			ctx->state = RP_CU_STATE_IDLE;
		}
	} else {
		rr_complete(conn);
		ctx->state = RP_CU_STATE_IDLE;
	}
}

static void rp_cu_send_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = RP_CU_STATE_WAIT_TX_CONN_UPDATE_IND;
	} else {
		/* TODO(thoh): Hardcoded instant delta +6 */
		ctx->data.cu.instant = rp_event_counter(conn) + 6;
		rp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND);
		ctx->rx_opcode = 0xFF; /* TODO(thoh): Hmm */
		ctx->state = RP_CU_STATE_WAIT_INSTANT;
	}
}

static void rp_cu_send_reject_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = RP_CU_STATE_WAIT_TX_REJECT_IND;
	} else {
		rp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_REJECT_IND);
		rr_complete(conn);
		ctx->state = RP_CU_STATE_IDLE;
	}
}

static void rp_cu_send_conn_param_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = RP_CU_STATE_WAIT_TX_CONN_PARAM_RSP;
	} else {
		rp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND;
		ctx->state = RP_CU_STATE_WAIT_RX_CONN_UPDATE_IND;
	}
}

static void rp_cu_send_conn_param_req_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!ntf_alloc_is_available()) {
		ctx->state = RP_CU_STATE_WAIT_NTF_CONN_PARAM_REQ;
	} else {
		rp_cu_conn_param_req_ntf(conn, ctx);
		ctx->state = RP_CU_STATE_WAIT_CONN_PARAM_REQ_REPLY;
	}
}

static void rp_cu_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_RUN:
		switch(ctx->proc) {
		case PROC_CONN_PARAM_REQ:
			ctx->state = RP_CU_STATE_WAIT_RX_CONN_PARAM_REQ;
			break;
		case PROC_CONN_UPDATE:
			ctx->state = RP_CU_STATE_WAIT_RX_CONN_UPDATE_IND;
			break;
		default:
			/* Unknown proceduce */
			LL_ASSERT(0);
		}
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_st_wait_rx_conn_param_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_CONN_PARAM_REQ:
		rp_cu_send_conn_param_req_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_state_wait_ntf_conn_param_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_RUN:
		rp_cu_send_conn_param_req_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_state_wait_conn_param_req_reply(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_CONN_PARAM_REQ_REPLY:
		if (conn->lll.role == BT_HCI_ROLE_MASTER) {
			rp_cu_send_conn_update_ind(conn, ctx, evt, param);
		} else if (conn->lll.role == BT_HCI_ROLE_SLAVE) {
			rp_cu_send_conn_param_rsp(conn, ctx, evt, param);	//TODO: Check feature???
		} else {
			LL_ASSERT(0);
		}
		break;
	case RP_CU_EVT_CONN_PARAM_REQ_NEG_REPLY:
		rp_cu_send_reject_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_state_wait_tx_reject_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_RUN:
		rp_cu_send_reject_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_st_wait_tx_conn_param_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_RUN:
		rp_cu_send_conn_param_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_st_wait_tx_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_RUN:
		rp_cu_send_conn_update_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_st_wait_rx_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_CONN_UPDATE_IND:
		pdu_decode_conn_update_ind(ctx, param);
		ctx->state = RP_CU_STATE_WAIT_INSTANT;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_check_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	uint16_t event_counter = rp_event_counter(conn);

	if (is_instant_reached_or_passed(ctx->data.cu.instant, event_counter)) {
		/* Procedure is complete when the instant has passed, and the
		 * new connection event parameters have been applied.
		 */
		cu_apply_params(conn, ctx);

		ctx->data.cu.error = BT_HCI_ERR_SUCCESS;
		rp_cu_complete(conn, ctx, evt, param);
	}
}

static void rp_cu_st_wait_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case RP_CU_EVT_RUN:
		rp_cu_check_instant(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_st_wait_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_RUN:
		rp_cu_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->state) {
	case RP_CU_STATE_IDLE:
		rp_cu_st_idle(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_RX_CONN_PARAM_REQ:
		rp_cu_st_wait_rx_conn_param_req(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_NTF_CONN_PARAM_REQ:
		rp_cu_state_wait_ntf_conn_param_req(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_CONN_PARAM_REQ_REPLY:
		rp_cu_state_wait_conn_param_req_reply(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_TX_REJECT_IND:
		rp_cu_state_wait_tx_reject_ind(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_TX_CONN_PARAM_RSP:
		rp_cu_st_wait_tx_conn_param_rsp(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_TX_CONN_UPDATE_IND:
		rp_cu_st_wait_tx_conn_update_ind(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_RX_CONN_UPDATE_IND:
		rp_cu_st_wait_rx_conn_update_ind(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_INSTANT:
		rp_cu_st_wait_instant(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_NTF:
		rp_cu_st_wait_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_rp_cu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *) rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ:
		rp_cu_execute_fsm(conn, ctx, RP_CU_EVT_CONN_PARAM_REQ, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND:
		rp_cu_execute_fsm(conn, ctx, RP_CU_EVT_CONN_UPDATE_IND, pdu);
		break;
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_rp_cu_init_proc(struct proc_ctx *ctx)
{
	ctx->state = RP_CU_STATE_IDLE;
}

void ull_cp_priv_rp_cu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_cu_execute_fsm(conn, ctx, RP_CU_EVT_RUN, param);
}

void ull_cp_priv_rp_conn_param_req_reply(struct ll_conn *conn, struct proc_ctx *ctx)
{
	rp_cu_execute_fsm(conn, ctx, RP_CU_EVT_CONN_PARAM_REQ_REPLY, NULL);
}

void ull_cp_priv_rp_conn_param_req_neg_reply(struct ll_conn *conn, struct proc_ctx *ctx)
{
	rp_cu_execute_fsm(conn, ctx, RP_CU_EVT_CONN_PARAM_REQ_NEG_REPLY, NULL);
}

static void tx_lll_flush(void *param)
{
	struct ll_conn *conn = (void *)HDR_LLL2ULL(param);
	uint16_t handle = ll_conn_handle_get(conn);
	struct lll_conn *lll = param;
	struct node_rx_pdu *rx;
	struct node_tx *tx;
	memq_link_t *link;

	lll_conn_flush(handle, lll);

	link = memq_dequeue(lll->memq_tx.tail, &lll->memq_tx.head,
			    (void **)&tx);
	while (link) {
		struct lll_tx *lll_tx;
		uint8_t idx;

		idx = ull_conn_mfifo_get_ack((void **)&lll_tx);
		LL_ASSERT(lll_tx);

		lll_tx->handle = 0xFFFF;
		lll_tx->node = tx;

		/* TX node UPSTREAM, i.e. Tx node ack path */
		link->next = tx->next; /* Indicates ctrl pool or data pool */
		tx->next = link;

		ull_conn_mfifo_enqueue_ack(idx);

		link = memq_dequeue(lll->memq_tx.tail, &lll->memq_tx.head,
				    (void **)&tx);
	}

	/* Get the terminate structure reserved in the connection context.
	 * The terminate reason and connection handle should already be
	 * populated before this mayfly function was scheduled.
	 */
	rx = (void *)&conn->terminate.node_rx;
	LL_ASSERT(rx->hdr.link);
	link = rx->hdr.link;
	rx->hdr.link = NULL;

	/* Enqueue the terminate towards ULL context */
	ull_rx_put(link, rx);
	ull_rx_sched();
}

static void ticker_start_conn_op_cb(uint32_t status, void *param)
{
	void *p;

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	p = ull_update_unmark(param);
	LL_ASSERT(p == param);
}

static void ticker_op_stop_cb(uint32_t status, void *param)
{
	uint32_t retval;
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, tx_lll_flush};

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	mfy.param = param;

	/* Flush pending tx PDUs in LLL (using a mayfly) */
	retval = mayfly_enqueue(TICKER_USER_ID_ULL_LOW, TICKER_USER_ID_LLL, 0,
				&mfy);
	LL_ASSERT(!retval);
}
