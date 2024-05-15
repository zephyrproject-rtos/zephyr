/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci_types.h>

#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "ll.h"
#include "ll_feat.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"

#include "lll_conn_iso.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"
#include "ull_conn_iso_internal.h"

#include "ull_conn_internal.h"
#include "ull_conn_types.h"

#if defined(CONFIG_BT_CTLR_USER_EXT)
#include "ull_vendor.h"
#endif /* CONFIG_BT_CTLR_USER_EXT */

#include "ull_internal.h"
#include "ull_llcp.h"
#include "ull_llcp_features.h"
#include "ull_llcp_internal.h"

#include <soc.h>
#include "hal/debug.h"

/* Hardcoded instant delta +6 */
#define CONN_UPDATE_INSTANT_DELTA	6U

/* CPR parameter ranges */
#define CONN_UPDATE_TIMEOUT_100MS	10U
#define CONN_UPDATE_TIMEOUT_32SEC	3200U
#define CONN_UPDATE_LATENCY_MAX		499U
#define CONN_UPDATE_CONN_INTV_4SEC	3200U

/*
 * TODO - Known, missing items (missing implementation):
 *
 * If DLE procedure supported:
 *  and current PHY is Coded PHY:
 *  ... (5.3.6.B.5.1.1) the new connection interval shall be at least connIntervalCodedMin us.
 *  ... (5.3.6.B.5.1.7.4) packet tx time restrictions should be in effect
 *
 * Inter-connection mutual exclusion on CPR
 *
 * LL/CON/MAS/BV-34-C [Accepting Connection Parameter Request w. event masked]
 */

/* LLCP Local Procedure Connection Update FSM states */
enum {
	LP_CU_STATE_IDLE,
	LP_CU_STATE_WAIT_TX_CONN_PARAM_REQ,
	LP_CU_STATE_WAIT_RX_CONN_PARAM_RSP,
	LP_CU_STATE_WAIT_TX_CONN_UPDATE_IND,
	LP_CU_STATE_WAIT_RX_CONN_UPDATE_IND,
	LP_CU_STATE_WAIT_TX_REJECT_EXT_IND,
	LP_CU_STATE_WAIT_INSTANT,
	LP_CU_STATE_WAIT_NTF_AVAIL,
};

/* LLCP Local Procedure Connection Update FSM events */
enum {
	/* Procedure run */
	LP_CU_EVT_RUN,

	/* Response received */
	LP_CU_EVT_CONN_PARAM_RSP,

	/* Indication received */
	LP_CU_EVT_CONN_UPDATE_IND,

	/* Reject response received */
	LP_CU_EVT_REJECT,

	/* Unknown response received */
	LP_CU_EVT_UNKNOWN,
};

/* LLCP Remote Procedure Connection Update FSM states */
enum {
	RP_CU_STATE_IDLE,
	RP_CU_STATE_WAIT_RX_CONN_PARAM_REQ,
	RP_CU_STATE_WAIT_CONN_PARAM_REQ_AVAILABLE,
	RP_CU_STATE_WAIT_NTF_CONN_PARAM_REQ,
	RP_CU_STATE_WAIT_CONN_PARAM_REQ_REPLY,
	RP_CU_STATE_WAIT_CONN_PARAM_REQ_REPLY_CONTINUE,
	RP_CU_STATE_WAIT_TX_REJECT_EXT_IND,
	RP_CU_STATE_WAIT_USER_REPLY,
	RP_CU_STATE_WAIT_TX_CONN_PARAM_RSP,
	RP_CU_STATE_WAIT_TX_CONN_UPDATE_IND,
	RP_CU_STATE_WAIT_RX_CONN_UPDATE_IND,
	RP_CU_STATE_WAIT_INSTANT,
	RP_CU_STATE_WAIT_NTF_AVAIL,
	RP_CU_STATE_WAIT_TX_UNKNOWN_RSP
};

/* LLCP Remote Procedure Connection Update FSM events */
enum {
	/* Procedure run */
	RP_CU_EVT_RUN,

	/* Request received */
	RP_CU_EVT_CONN_PARAM_REQ,

	/* Indication received */
	RP_CU_EVT_CONN_UPDATE_IND,

	/* CONN_PARAM_REQ reply */
	RP_CU_EVT_CONN_PARAM_REQ_REPLY,

	/* CONN_PARAM_REQ negative reply */
	RP_CU_EVT_CONN_PARAM_REQ_NEG_REPLY,

	/* CONN_PARAM_REQ Ancjor Point Move reply */
	RP_CU_EVT_CONN_PARAM_REQ_USER_REPLY,
};

/*
 * LLCP Local Procedure Connection Update FSM
 */

static bool cu_have_params_changed(struct ll_conn *conn, uint16_t interval, uint16_t latency,
				   uint16_t timeout)
{
	struct lll_conn *lll = &conn->lll;

	if ((interval != lll->interval) || (latency != lll->latency) ||
	    (timeout != conn->supervision_timeout)) {
		return true;
	}
	return false;
}

static void cu_update_conn_parameters(struct ll_conn *conn, struct proc_ctx *ctx)
{
	ctx->data.cu.params_changed = cu_have_params_changed(
		conn, ctx->data.cu.interval_max, ctx->data.cu.latency, ctx->data.cu.timeout);

	ull_conn_update_parameters(conn, (ctx->proc == PROC_CONN_UPDATE), ctx->data.cu.win_size,
				   ctx->data.cu.win_offset_us, ctx->data.cu.interval_max,
				   ctx->data.cu.latency, ctx->data.cu.timeout,
				   ctx->data.cu.instant);
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static bool cu_check_conn_parameters(struct ll_conn *conn, struct proc_ctx *ctx)
{
	const uint16_t interval_min = ctx->data.cu.interval_min;
	const uint16_t interval_max = ctx->data.cu.interval_max; /* unit conn events (ie 1.25ms) */
	const uint16_t timeout = ctx->data.cu.timeout; /* unit 10ms */
	const uint16_t latency = ctx->data.cu.latency;
	const uint16_t preferred_periodicity = ctx->data.cu.preferred_periodicity;

	/* Invalid parameters */
	const bool invalid = ((interval_min < CONN_INTERVAL_MIN(conn)) ||
	    (interval_max > CONN_UPDATE_CONN_INTV_4SEC) ||
	    (interval_min > interval_max) ||
	    (latency > CONN_UPDATE_LATENCY_MAX) ||
	    (timeout < CONN_UPDATE_TIMEOUT_100MS) || (timeout > CONN_UPDATE_TIMEOUT_32SEC) ||
	    ((timeout * 4U) <= /* *4U re. conn events is equivalent to *2U re. ms */
	     ((latency + 1) * interval_max)) ||
	    (preferred_periodicity > interval_max));

	return !invalid;
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

static void cu_prepare_update_ind(struct ll_conn *conn, struct proc_ctx *ctx)
{
	ctx->data.cu.win_size = 1U;
	ctx->data.cu.win_offset_us = 0U;

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	/* Handle preferred periodicity */
	const uint8_t preferred_periodicity = ctx->data.cu.preferred_periodicity;

	if (preferred_periodicity) {
		const uint16_t interval_max = (ctx->data.cu.interval_max / preferred_periodicity) *
			preferred_periodicity;
		if (interval_max >= ctx->data.cu.interval_min) {
			/* In case of there is no underflowing interval_min use 'preferred max' */
			ctx->data.cu.interval_max = interval_max;
		}
	}

#if !defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
	/* Use valid offset0 in range [0..interval]. An offset of
	 * 0xffff means not valid. Disregard other preferred offsets.
	 */
	/* Handle win_offset/'anchor point move' */
	if (ctx->data.cu.offsets[0] <= ctx->data.cu.interval_max) {
		ctx->data.cu.win_offset_us = ctx->data.cu.offsets[0] * CONN_INT_UNIT_US;
	}
#endif /* !CONFIG_BT_CTLR_SCHED_ADVANCED */
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

	ctx->data.cu.instant = ull_conn_event_counter(conn) + conn->lll.latency +
			       CONN_UPDATE_INSTANT_DELTA;
}

static bool cu_should_notify_host(struct proc_ctx *ctx)
{
	return (((ctx->proc == PROC_CONN_PARAM_REQ) && (ctx->data.cu.error != 0U)) ||
		(ctx->data.cu.params_changed != 0U));
}

static void cu_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct node_rx_cu *pdu;
	uint8_t piggy_back;

	/* Allocate ntf node */
	ntf = ctx->node_ref.rx;
	ctx->node_ref.rx = NULL;
	LL_ASSERT(ntf);

	piggy_back = (ntf->hdr.type != NODE_RX_TYPE_RETAIN);

	ntf->hdr.type = NODE_RX_TYPE_CONN_UPDATE;
	ntf->hdr.handle = conn->lll.handle;
	pdu = (struct node_rx_cu *)ntf->pdu;

	pdu->status = ctx->data.cu.error;
	if (!ctx->data.cu.error) {
		pdu->interval = ctx->data.cu.interval_max;
		pdu->latency = ctx->data.cu.latency;
		pdu->timeout = ctx->data.cu.timeout;
	} else {
		pdu->interval = conn->lll.interval;
		pdu->latency = conn->lll.latency;
		pdu->timeout = conn->supervision_timeout;
	}

	if (!piggy_back) {
		/* Enqueue notification towards LL, unless piggy-backing,
		 * in which case this is done on the rx return path
		 */
		ll_rx_put_sched(ntf->hdr.link, ntf);
	}
}

#if defined(CONFIG_BT_CENTRAL) || defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static void lp_cu_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Get pre-allocated tx node */
	tx = ctx->node_ref.tx;
	ctx->node_ref.tx = NULL;

	if (!tx) {
		/* Allocate tx node if non pre-alloc'ed */
		tx = llcp_tx_alloc(conn, ctx);
		LL_ASSERT(tx);
	}

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (opcode) {
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ:
		llcp_pdu_encode_conn_param_req(ctx, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		llcp_pdu_encode_reject_ext_ind(pdu, ctx->data.cu.rejected_opcode,
					       ctx->data.cu.error);
		break;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
#if defined(CONFIG_BT_CENTRAL)
	case PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND:
		llcp_pdu_encode_conn_update_ind(ctx, pdu);
		break;
#endif /* CONFIG_BT_CENTRAL */
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
		break;
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	if (ctx->proc == PROC_CONN_PARAM_REQ) {
		/* Restart procedure response timeout timer */
		llcp_lr_prt_restart(conn);
	}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
}
#endif /* CONFIG_BT_CENTRAL || CONFIG_BT_CTLR_CONN_PARAM_REQ */

static void lp_cu_complete(struct ll_conn *conn, struct proc_ctx *ctx)
{
	llcp_lr_complete(conn);
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	if (ctx->proc == PROC_CONN_PARAM_REQ &&
	    !(conn->lll.role && ull_cp_remote_cpr_pending(conn))) {
		/* For a peripheral without a remote initiated CPR */
		cpr_active_check_and_reset(conn);
	}
#endif /* defined(CONFIG_BT_CTLR_CONN_PARAM_REQ) */
	ctx->state = LP_CU_STATE_IDLE;
}

static void lp_cu_ntf_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	cu_ntf(conn, ctx);
	lp_cu_complete(conn, ctx);
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static void lp_cu_send_reject_ext_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				      void *param)
{
	if (llcp_lr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = LP_CU_STATE_WAIT_TX_REJECT_EXT_IND;
	} else {
		llcp_rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
		lp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND);
		lp_cu_complete(conn, ctx);
	}
}

static void lp_cu_send_conn_param_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				      void *param)
{
	if (cpr_active_is_set(conn) || llcp_lr_ispaused(conn) ||
	     llcp_rr_get_collision(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = LP_CU_STATE_WAIT_TX_CONN_PARAM_REQ;
	} else {
		uint16_t event_counter = ull_conn_event_counter(conn);

		llcp_rr_set_incompat(conn, INCOMPAT_RESOLVABLE);

		ctx->data.cu.reference_conn_event_count = event_counter;
		ctx->data.cu.preferred_periodicity = 0U;

		/* Mark CPR as active */
		cpr_active_set(conn);

		lp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ);

		switch (conn->lll.role) {
#if defined(CONFIG_BT_CENTRAL)
		case BT_HCI_ROLE_CENTRAL:
			ctx->state = LP_CU_STATE_WAIT_RX_CONN_PARAM_RSP;
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP;
			break;
#endif /* CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_PERIPHERAL)
		case BT_HCI_ROLE_PERIPHERAL:
			ctx->state = LP_CU_STATE_WAIT_RX_CONN_UPDATE_IND;
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND;
			break;
#endif /* CONFIG_BT_PERIPHERAL */
		default:
			/* Unknown role */
			LL_ASSERT(0);
			break;
		}
	}
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CENTRAL)
static void lp_cu_send_conn_update_ind_finalize(struct ll_conn *conn, struct proc_ctx *ctx,
						uint8_t evt, void *param)
{
	if (ctx->node_ref.rx == NULL) {
		/* If we get here without RX node we know one is avail to be allocated,
		 * so pre-alloc NTF node
		 */
		ctx->node_ref.rx = llcp_ntf_alloc();
	}

	/* Signal put/sched on NTF - ie non-RX node piggy */
	ctx->node_ref.rx->hdr.type = NODE_RX_TYPE_RETAIN;

	cu_prepare_update_ind(conn, ctx);
	lp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND);
	ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
	ctx->state = LP_CU_STATE_WAIT_INSTANT;
}

static void lp_cu_send_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				       void *param)
{
	if (llcp_lr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = LP_CU_STATE_WAIT_TX_CONN_UPDATE_IND;
	} else {
		/* ensure alloc of TX node, before possibly waiting for NTF node */
		ctx->node_ref.tx = llcp_tx_alloc(conn, ctx);
		if (ctx->node_ref.rx == NULL && !llcp_ntf_alloc_is_available()) {
			/* No RX node piggy, and no NTF avail, so go wait for one, before TX'ing */
			ctx->state = LP_CU_STATE_WAIT_NTF_AVAIL;
		} else {
			lp_cu_send_conn_update_ind_finalize(conn, ctx, evt, param);
		}
	}
}

static void lp_cu_st_wait_ntf_avail(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				    void *param)
{
	switch (evt) {
	case LP_CU_EVT_RUN:
		if (llcp_ntf_alloc_is_available()) {
			lp_cu_send_conn_update_ind_finalize(conn, ctx, evt, param);
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}
#endif /* CONFIG_BT_CENTRAL */

static void lp_cu_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_CU_EVT_RUN:
		switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		case PROC_CONN_PARAM_REQ:
			lp_cu_send_conn_param_req(conn, ctx, evt, param);
			break;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
#if defined(CONFIG_BT_CENTRAL)
		case PROC_CONN_UPDATE:
			/* Ensure the non-piggy-back'ing is signaled */
			ctx->node_ref.rx = NULL;
			lp_cu_send_conn_update_ind(conn, ctx, evt, param);
			break;
#endif /* CONFIG_BT_CENTRAL */
		default:
			/* Unknown procedure */
			LL_ASSERT(0);
			break;
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static void lp_cu_st_wait_tx_reject_ext_ind(struct ll_conn *conn, struct proc_ctx *ctx,
					       uint8_t evt, void *param)
{
	switch (evt) {
	case LP_CU_EVT_RUN:
		lp_cu_send_reject_ext_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}


static void lp_cu_st_wait_tx_conn_param_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					    void *param)
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
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CENTRAL)
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static void lp_cu_st_wait_rx_conn_param_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					    void *param)
{
	struct pdu_data *pdu = (struct pdu_data *)param;

	switch (evt) {
	case LP_CU_EVT_CONN_PARAM_RSP:
		llcp_pdu_decode_conn_param_rsp(ctx, param);
		llcp_rr_set_incompat(conn, INCOMPAT_RESERVED);
		/* Perform Param check and possibly reject (LL_REJECT_EXT_IND) */
		if (!cu_check_conn_parameters(conn, ctx)) {
			ctx->data.cu.rejected_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP;
			ctx->data.cu.error = BT_HCI_ERR_INVALID_LL_PARAM;
			lp_cu_send_reject_ext_ind(conn, ctx, evt, param);
			break;
		}
		/* Keep RX node to use for NTF */
		llcp_rx_node_retain(ctx);
		lp_cu_send_conn_update_ind(conn, ctx, evt, param);
		break;
	case LP_CU_EVT_UNKNOWN:
		llcp_rr_set_incompat(conn, INCOMPAT_RESERVED);
		/* Unsupported in peer, so disable locally for this connection */
		feature_unmask_features(conn, LL_FEAT_BIT_CONN_PARAM_REQ);
		/* Keep RX node to use for NTF */
		llcp_rx_node_retain(ctx);
		lp_cu_send_conn_update_ind(conn, ctx, evt, param);
		break;
	case LP_CU_EVT_REJECT:
		if (pdu->llctrl.reject_ext_ind.error_code == BT_HCI_ERR_UNSUPP_REMOTE_FEATURE) {
			/* Remote legacy Host */
			llcp_rr_set_incompat(conn, INCOMPAT_RESERVED);
			/* Unsupported in peer, so disable locally for this connection */
			feature_unmask_features(conn, LL_FEAT_BIT_CONN_PARAM_REQ);
			/* Keep RX node to use for NTF */
			llcp_rx_node_retain(ctx);
			lp_cu_send_conn_update_ind(conn, ctx, evt, param);
		} else {
			llcp_rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
			ctx->data.cu.error = pdu->llctrl.reject_ext_ind.error_code;
			lp_cu_ntf_complete(conn, ctx, evt, param);
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

static void lp_cu_st_wait_tx_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx,
					     uint8_t evt, void *param)
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
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
static void lp_cu_st_wait_rx_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx,
					     uint8_t evt, void *param)
{
	struct pdu_data *pdu = (struct pdu_data *)param;

	switch (evt) {
	case LP_CU_EVT_CONN_UPDATE_IND:
		llcp_pdu_decode_conn_update_ind(ctx, param);
		/* Keep RX node to use for NTF */
		llcp_rx_node_retain(ctx);
		ctx->state = LP_CU_STATE_WAIT_INSTANT;
		break;
	case LP_CU_EVT_UNKNOWN:
		/* Unsupported in peer, so disable locally for this connection */
		feature_unmask_features(conn, LL_FEAT_BIT_CONN_PARAM_REQ);
		ctx->data.cu.error = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
		lp_cu_ntf_complete(conn, ctx, evt, param);
		break;
	case LP_CU_EVT_REJECT:
		ctx->data.cu.error = pdu->llctrl.reject_ext_ind.error_code;
		lp_cu_ntf_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}
#endif /* CONFIG_BT_PERIPHERAL */

static void lp_cu_check_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	uint16_t event_counter = ull_conn_event_counter(conn);

	if (is_instant_reached_or_passed(ctx->data.cu.instant, event_counter)) {
		bool notify;

		/* Procedure is complete when the instant has passed, and the
		 * new connection event parameters have been applied.
		 */
		llcp_rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
		cu_update_conn_parameters(conn, ctx);

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		if (ctx->proc == PROC_CONN_PARAM_REQ) {
			/* Stop procedure response timeout timer */
			llcp_lr_prt_stop(conn);
		}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

		notify = cu_should_notify_host(ctx);
		if (notify) {
			ctx->data.cu.error = BT_HCI_ERR_SUCCESS;
			lp_cu_ntf_complete(conn, ctx, evt, param);
		} else {
			/* Release RX node kept for NTF */
			llcp_rx_node_release(ctx);
			ctx->node_ref.rx = NULL;

			lp_cu_complete(conn, ctx);
		}
	}
}

static void lp_cu_st_wait_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				  void *param)
{
	switch (evt) {
	case LP_CU_EVT_RUN:
		lp_cu_check_instant(conn, ctx, evt, param);
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
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	case LP_CU_STATE_WAIT_TX_CONN_PARAM_REQ:
		lp_cu_st_wait_tx_conn_param_req(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
#if defined(CONFIG_BT_CENTRAL)
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	case LP_CU_STATE_WAIT_RX_CONN_PARAM_RSP:
		lp_cu_st_wait_rx_conn_param_rsp(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
	case LP_CU_STATE_WAIT_TX_CONN_UPDATE_IND:
		lp_cu_st_wait_tx_conn_update_ind(conn, ctx, evt, param);
		break;
	case LP_CU_STATE_WAIT_NTF_AVAIL:
		lp_cu_st_wait_ntf_avail(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_PERIPHERAL)
	case LP_CU_STATE_WAIT_RX_CONN_UPDATE_IND:
		lp_cu_st_wait_rx_conn_update_ind(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	case LP_CU_STATE_WAIT_TX_REJECT_EXT_IND:
		lp_cu_st_wait_tx_reject_ext_ind(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
	case LP_CU_STATE_WAIT_INSTANT:
		lp_cu_st_wait_instant(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
		break;
	}
}

void llcp_lp_cu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *)rx->pdu;

	switch (pdu->llctrl.opcode) {
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP:
		lp_cu_execute_fsm(conn, ctx, LP_CU_EVT_CONN_PARAM_RSP, pdu);
		break;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
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
		/* Invalid behaviour */
		/* Invalid PDU received so terminate connection */
		conn->llcp_terminate.reason_final = BT_HCI_ERR_LMP_PDU_NOT_ALLOWED;
		lp_cu_complete(conn, ctx);
		break;
	}
}

void llcp_lp_cu_init_proc(struct proc_ctx *ctx)
{
	ctx->state = LP_CU_STATE_IDLE;
}

void llcp_lp_cu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	lp_cu_execute_fsm(conn, ctx, LP_CU_EVT_RUN, param);
}

/*
 * LLCP Remote Procedure Connection Update FSM
 */

static void rp_cu_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Get pre-allocated tx node */
	tx = ctx->node_ref.tx;
	ctx->node_ref.tx = NULL;

	if (!tx) {
		/* Allocate tx node if non pre-alloc'ed */
		tx = llcp_tx_alloc(conn, ctx);
		LL_ASSERT(tx);
	}

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (opcode) {
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP:
		llcp_pdu_encode_conn_param_rsp(ctx, pdu);
		break;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
	case PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND:
		llcp_pdu_encode_conn_update_ind(ctx, pdu);
		break;
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		llcp_pdu_encode_reject_ext_ind(pdu, ctx->data.cu.rejected_opcode,
					       ctx->data.cu.error);
		break;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		llcp_pdu_encode_unknown_rsp(ctx, pdu);
		break;
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
		break;
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	if (ctx->proc == PROC_CONN_PARAM_REQ) {
		/* Restart procedure response timeout timer */
		llcp_rr_prt_restart(conn);
	}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static void rp_cu_conn_param_req_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	uint8_t piggy_back;


	/* Allocate ntf node */
	ntf = ctx->node_ref.rx;
	ctx->node_ref.rx = NULL;
	LL_ASSERT(ntf);

	piggy_back = (ntf->hdr.type != NODE_RX_TYPE_RETAIN);

	ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	ntf->hdr.handle = conn->lll.handle;
	pdu = (struct pdu_data *)ntf->pdu;

	llcp_pdu_encode_conn_param_req(ctx, pdu);

	if (!piggy_back) {
		/* Enqueue notification towards LL, unless piggy-backing,
		 * in which case this is done on the rx return path
		 */
		ll_rx_put_sched(ntf->hdr.link, ntf);
	}
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

static void rp_cu_complete(struct ll_conn *conn, struct proc_ctx *ctx)
{
	llcp_rr_complete(conn);
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	if (ctx->proc == PROC_CONN_PARAM_REQ) {
		cpr_active_check_and_reset(conn);
	}
#endif /* defined(CONFIG_BT_CTLR_CONN_PARAM_REQ) */
	ctx->state = RP_CU_STATE_IDLE;
}

static void rp_cu_send_conn_update_ind_finalize(struct ll_conn *conn, struct proc_ctx *ctx,
						uint8_t evt, void *param)
{
	/* Central role path, should not get here with !=NULL rx-node reference */
	LL_ASSERT(ctx->node_ref.rx == NULL);
	/* We pre-alloc NTF node */
	ctx->node_ref.rx = llcp_ntf_alloc();

	/* Signal put/sched on NTF - ie non-RX node piggy */
	ctx->node_ref.rx->hdr.type = NODE_RX_TYPE_RETAIN;

	cu_prepare_update_ind(conn, ctx);
	rp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND);
	ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
	ctx->state = RP_CU_STATE_WAIT_INSTANT;
}

static void rp_cu_send_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				       void *param)
{
	if (llcp_rr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = RP_CU_STATE_WAIT_TX_CONN_UPDATE_IND;
	} else {
		/* ensure alloc of TX node, before possibly waiting for NTF node */
		ctx->node_ref.tx = llcp_tx_alloc(conn, ctx);
		if (!llcp_ntf_alloc_is_available()) {
			/* No RX node piggy, and no NTF avail, so go wait for one, before TX'ing */
			ctx->state = RP_CU_STATE_WAIT_NTF_AVAIL;
		} else {
			rp_cu_send_conn_update_ind_finalize(conn, ctx, evt, param);
		}
	}
}

static void rp_cu_st_wait_ntf_avail(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				    void *param)
{
	switch (evt) {
	case RP_CU_EVT_RUN:
		if (llcp_ntf_alloc_is_available()) {
			/* If NTF node is now avail, so pick it up and continue */
			rp_cu_send_conn_update_ind_finalize(conn, ctx, evt, param);
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static void rp_cu_send_reject_ext_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				      void *param)
{
	if (llcp_rr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = RP_CU_STATE_WAIT_TX_REJECT_EXT_IND;
	} else {
		rp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND);
		rp_cu_complete(conn, ctx);
	}
}

static void rp_cu_send_conn_param_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				      void *param)
{
	if (llcp_rr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = RP_CU_STATE_WAIT_TX_CONN_PARAM_RSP;
	} else {
		rp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND;
		ctx->state = RP_CU_STATE_WAIT_RX_CONN_UPDATE_IND;
	}
}

static void rp_cu_send_conn_param_req_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					  void *param)
{
	if (!llcp_ntf_alloc_is_available()) {
		ctx->state = RP_CU_STATE_WAIT_NTF_CONN_PARAM_REQ;
	} else {
		rp_cu_conn_param_req_ntf(conn, ctx);
		ctx->state = RP_CU_STATE_WAIT_CONN_PARAM_REQ_REPLY;
	}
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

static void rp_cu_send_unknown_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				   void *param)
{
	if (llcp_rr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = RP_CU_STATE_WAIT_TX_UNKNOWN_RSP;
	} else {
		rp_cu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP);
		rp_cu_complete(conn, ctx);
	}
}

static void rp_cu_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_RUN:
		switch (ctx->proc) {
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		case PROC_CONN_PARAM_REQ:
			ctx->state = RP_CU_STATE_WAIT_RX_CONN_PARAM_REQ;
			break;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
		case PROC_CONN_UPDATE:
			ctx->state = RP_CU_STATE_WAIT_RX_CONN_UPDATE_IND;
			break;
		default:
			/* Unknown procedure */
			LL_ASSERT(0);
			break;
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static void rp_cu_st_wait_conn_param_req_available(struct ll_conn *conn, struct proc_ctx *ctx,
						   uint8_t evt, void *param)
{
	/* Check if CPR is already active on other connection.
	 * If so check if possible to send reject right away
	 * otherwise stay in wait state in case CPR becomes
	 * available before we can send send reject
	 */
	switch (evt) {
	case RP_CU_EVT_CONN_PARAM_REQ:
	case RP_CU_EVT_RUN:
		if (cpr_active_is_set(conn)) {
			ctx->state = RP_CU_STATE_WAIT_CONN_PARAM_REQ_AVAILABLE;

			if (!llcp_rr_ispaused(conn) && llcp_tx_alloc_peek(conn, ctx)) {
				/* We're good to reject immediately */
				ctx->data.cu.rejected_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ;
				ctx->data.cu.error = BT_HCI_ERR_UNSUPP_LL_PARAM_VAL;
				rp_cu_send_reject_ext_ind(conn, ctx, evt, param);

				/* Possibly retained rx node to be released as we won't need it */
				llcp_rx_node_release(ctx);
				ctx->node_ref.rx = NULL;

				break;
			}
			/* In case we have to defer NTF */
			llcp_rx_node_retain(ctx);
		} else {
			cpr_active_set(conn);
			const bool params_changed =
				cu_have_params_changed(conn, ctx->data.cu.interval_max,
						       ctx->data.cu.latency, ctx->data.cu.timeout);

			/* notify Host if conn parameters changed, else respond */
			if (params_changed) {
				rp_cu_conn_param_req_ntf(conn, ctx);
				ctx->state = RP_CU_STATE_WAIT_CONN_PARAM_REQ_REPLY;
			} else {
				/* Possibly retained rx node to be released as we won't need it */
				llcp_rx_node_release(ctx);
				ctx->node_ref.rx = NULL;
#if defined(CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE)
				/* Handle APM as a vendor specific user extension */
				if (conn->lll.role == BT_HCI_ROLE_PERIPHERAL &&
				    DEFER_APM_CHECK(conn, ctx->data.cu.offsets,
						    &ctx->data.cu.error)) {
					/* Wait for user response */
					ctx->state = RP_CU_STATE_WAIT_USER_REPLY;
					break;
				}
#endif /* CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE */
				ctx->state = RP_CU_STATE_WAIT_CONN_PARAM_REQ_REPLY_CONTINUE;
			}
		}
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_st_wait_rx_conn_param_req(struct ll_conn *conn, struct proc_ctx *ctx,
					    uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_CONN_PARAM_REQ:
		llcp_pdu_decode_conn_param_req(ctx, param);

		/* Perform Param check and reject if invalid (LL_REJECT_EXT_IND) */
		if (!cu_check_conn_parameters(conn, ctx)) {
			ctx->data.cu.rejected_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ;
			ctx->data.cu.error = BT_HCI_ERR_INVALID_LL_PARAM;
			rp_cu_send_reject_ext_ind(conn, ctx, evt, param);
			break;
		}

		rp_cu_st_wait_conn_param_req_available(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_state_wait_ntf_conn_param_req(struct ll_conn *conn, struct proc_ctx *ctx,
						uint8_t evt, void *param)
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

static void rp_cu_state_wait_conn_param_req_reply(struct ll_conn *conn, struct proc_ctx *ctx,
						  uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_CONN_PARAM_REQ_REPLY:
		/* Continue procedure in next prepare run */
		ctx->state = RP_CU_STATE_WAIT_CONN_PARAM_REQ_REPLY_CONTINUE;
		break;
	case RP_CU_EVT_CONN_PARAM_REQ_NEG_REPLY:
		/* Send reject in next prepare run */
		ctx->data.cu.rejected_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ;
		ctx->state = RP_CU_STATE_WAIT_TX_REJECT_EXT_IND;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_state_wait_conn_param_req_reply_continue(struct ll_conn *conn,
							   struct proc_ctx *ctx, uint8_t evt,
							   void *param)
{
	switch (evt) {
	case RP_CU_EVT_RUN:
		if (conn->lll.role == BT_HCI_ROLE_CENTRAL) {
			/* Ensure that node_ref does not indicate RX node for piggyback */
			ctx->node_ref.rx = NULL;
			rp_cu_send_conn_update_ind(conn, ctx, evt, param);
		} else if (conn->lll.role == BT_HCI_ROLE_PERIPHERAL) {
			if (!ctx->data.cu.error) {
				rp_cu_send_conn_param_rsp(conn, ctx, evt, param);
			} else {
				ctx->data.cu.rejected_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ;
				rp_cu_send_reject_ext_ind(conn, ctx, evt, param);

			}
		} else {
			/* Unknown role */
			LL_ASSERT(0);
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_state_wait_tx_reject_ext_ind(struct ll_conn *conn, struct proc_ctx *ctx,
					       uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_RUN:
		rp_cu_send_reject_ext_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_st_wait_tx_conn_param_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					    void *param)
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

#if defined(CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE)
static void rp_cu_st_wait_user_response(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					    void *param)
{
	switch (evt) {
	case RP_CU_EVT_CONN_PARAM_REQ_USER_REPLY:
		/* Continue procedure in next prepare run */
		ctx->state = RP_CU_STATE_WAIT_CONN_PARAM_REQ_REPLY_CONTINUE;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}
#endif /* CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE */
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

static void rp_cu_st_wait_tx_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx,
					     uint8_t evt, void *param)
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

static void rp_cu_check_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	uint16_t event_counter = ull_conn_event_counter(conn);

	if (is_instant_reached_or_passed(ctx->data.cu.instant, event_counter)) {
		bool notify;

		/* Procedure is complete when the instant has passed, and the
		 * new connection event parameters have been applied.
		 */
		cu_update_conn_parameters(conn, ctx);

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		if (ctx->proc == PROC_CONN_PARAM_REQ) {
			/* Stop procedure response timeout timer */
			llcp_rr_prt_stop(conn);
		}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

		notify = cu_should_notify_host(ctx);
		if (notify) {
			ctx->data.cu.error = BT_HCI_ERR_SUCCESS;
			cu_ntf(conn, ctx);
		} else {
			/* Release RX node kept for NTF */
			llcp_rx_node_release(ctx);
			ctx->node_ref.rx = NULL;
		}
		rp_cu_complete(conn, ctx);
	}
}

static void rp_cu_st_wait_rx_conn_update_ind(struct ll_conn *conn, struct proc_ctx *ctx,
					     uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CU_EVT_CONN_UPDATE_IND:
		switch (conn->lll.role) {
		case BT_HCI_ROLE_CENTRAL:
			ctx->unknown_response.type = PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND;
			rp_cu_send_unknown_rsp(conn, ctx, evt, param);
			break;
		case BT_HCI_ROLE_PERIPHERAL:
			llcp_pdu_decode_conn_update_ind(ctx, param);

			if (is_instant_not_passed(ctx->data.cu.instant,
						  ull_conn_event_counter(conn))) {

				llcp_rx_node_retain(ctx);

				ctx->state = RP_CU_STATE_WAIT_INSTANT;
				/* In case we only just received it in time */
				rp_cu_check_instant(conn, ctx, evt, param);
			} else {
				conn->llcp_terminate.reason_final = BT_HCI_ERR_INSTANT_PASSED;
				llcp_rr_complete(conn);
				ctx->state = RP_CU_STATE_IDLE;
			}
			break;
		default:
			/* Unknown role */
			LL_ASSERT(0);
		}
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cu_st_wait_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				  void *param)
{
	switch (evt) {
	case RP_CU_EVT_RUN:
		rp_cu_check_instant(conn, ctx, evt, param);
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
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	case RP_CU_STATE_WAIT_RX_CONN_PARAM_REQ:
		rp_cu_st_wait_rx_conn_param_req(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_CONN_PARAM_REQ_AVAILABLE:
		rp_cu_st_wait_conn_param_req_available(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_NTF_CONN_PARAM_REQ:
		rp_cu_state_wait_ntf_conn_param_req(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_CONN_PARAM_REQ_REPLY:
		rp_cu_state_wait_conn_param_req_reply(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_CONN_PARAM_REQ_REPLY_CONTINUE:
		rp_cu_state_wait_conn_param_req_reply_continue(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_TX_REJECT_EXT_IND:
		rp_cu_state_wait_tx_reject_ext_ind(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_TX_CONN_PARAM_RSP:
		rp_cu_st_wait_tx_conn_param_rsp(conn, ctx, evt, param);
		break;
#if defined(CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE)
	case RP_CU_STATE_WAIT_USER_REPLY:
		rp_cu_st_wait_user_response(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE */
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
	case RP_CU_STATE_WAIT_TX_CONN_UPDATE_IND:
		rp_cu_st_wait_tx_conn_update_ind(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_RX_CONN_UPDATE_IND:
		rp_cu_st_wait_rx_conn_update_ind(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_INSTANT:
		rp_cu_st_wait_instant(conn, ctx, evt, param);
		break;
	case RP_CU_STATE_WAIT_NTF_AVAIL:
		rp_cu_st_wait_ntf_avail(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
		break;
	}
}

void llcp_rp_cu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *)rx->pdu;

	switch (pdu->llctrl.opcode) {
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ:
		rp_cu_execute_fsm(conn, ctx, RP_CU_EVT_CONN_PARAM_REQ, pdu);
		break;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
	case PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND:
		rp_cu_execute_fsm(conn, ctx, RP_CU_EVT_CONN_UPDATE_IND, pdu);
		break;
	default:
		/* Invalid behaviour */
		/* Invalid PDU received so terminate connection */
		conn->llcp_terminate.reason_final = BT_HCI_ERR_LMP_PDU_NOT_ALLOWED;
		rp_cu_complete(conn, ctx);
		break;
	}
}

void llcp_rp_cu_init_proc(struct proc_ctx *ctx)
{
	ctx->state = RP_CU_STATE_IDLE;
}

void llcp_rp_cu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_cu_execute_fsm(conn, ctx, RP_CU_EVT_RUN, param);
}

bool llcp_rp_cu_awaiting_instant(struct proc_ctx *ctx)
{
	return (ctx->state == RP_CU_STATE_WAIT_INSTANT);
}

bool llcp_lp_cu_awaiting_instant(struct proc_ctx *ctx)
{
	return (ctx->state == LP_CU_STATE_WAIT_INSTANT);
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
void llcp_rp_conn_param_req_reply(struct ll_conn *conn, struct proc_ctx *ctx)
{
	rp_cu_execute_fsm(conn, ctx, RP_CU_EVT_CONN_PARAM_REQ_REPLY, NULL);
}

void llcp_rp_conn_param_req_neg_reply(struct ll_conn *conn, struct proc_ctx *ctx)
{
	rp_cu_execute_fsm(conn, ctx, RP_CU_EVT_CONN_PARAM_REQ_NEG_REPLY, NULL);
}

#if defined(CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE)
bool llcp_rp_conn_param_req_apm_awaiting_reply(struct proc_ctx *ctx)
{
	return (ctx->state == RP_CU_STATE_WAIT_USER_REPLY);
}

void llcp_rp_conn_param_req_apm_reply(struct ll_conn *conn, struct proc_ctx *ctx)
{
	rp_cu_execute_fsm(conn, ctx, RP_CU_EVT_CONN_PARAM_REQ_USER_REPLY, NULL);
}
#endif /* CONFIG_BT_CTLR_USER_CPR_ANCHOR_POINT_MOVE */
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
