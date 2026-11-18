/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * LE Connection Subrating, Bluetooth Core Specification v5.3+:
 *   - Vol 6, Part B, Section 4.5.1   (subrated connection events)
 *   - Vol 6, Part B, Section 5.1.19  (Connection Subrate Update procedure, Central)
 *   - Vol 6, Part B, Section 5.1.20  (Connection Subrate Request procedure, Peripheral)
 *   - Vol 6, Part B, Section 2.4.2.36/37 (LL_SUBRATE_REQ / LL_SUBRATE_IND)
 *
 * The two procedures are role-asymmetric:
 *   - Only the Central sends LL_SUBRATE_IND (5.1.19). It enters subrate transition
 *     mode and the new parameters take effect once the IND is acknowledged.
 *   - Only the Peripheral sends LL_SUBRATE_REQ (5.1.20). The Central either runs
 *     5.1.19 in response, or rejects with LL_REJECT_EXT_IND.
 *   - The Peripheral applies an LL_SUBRATE_IND immediately on receipt and shall accept.
 *
 * Subrated connection event skipping is done in ull_conn_done() using conn->subrate.
 * It is suspended while a control procedure is active, hence the Central is present
 * on every connection event in subrate transition mode, a superset of the old and the
 * new subrated events.
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

#include "ull_internal.h"
#include "ull_llcp.h"
#include "ull_llcp_features.h"
#include "ull_llcp_internal.h"

#include <soc.h>
#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_SUBRATING)

/* Parameter ranges, Core Spec Vol 4, Part E, Section 7.8.124 */
#define SUBRATE_FACTOR_MIN          0x0001U
#define SUBRATE_FACTOR_MAX          0x01F4U /* 500 */
#define SUBRATE_MAX_LATENCY_MAX     0x01F3U /* 499 */
#define SUBRATE_CONT_NUM_MAX        0x01F3U /* 499 */
#define SUBRATE_TIMEOUT_MIN         0x000AU /* 100 ms */
#define SUBRATE_TIMEOUT_MAX         0x0C80U /* 32 s */

/* subrate_factor * (max_latency + 1) shall be <= 500 (Vol 6, Part B, 4.5.1) */
#define SUBRATE_FACTOR_LATENCY_PRODUCT_MAX 500U

/* Offset, in (underlying) connection events, used by the Central to place the
 * new connSubrateBaseEvent in the near future. Mirrors CONN_UPDATE_INSTANT_DELTA.
 */
#define SUBRATE_BASE_EVENT_DELTA    6U

/* LLCP Local Procedure Subrate FSM states */
enum {
	LP_SUBRATE_STATE_IDLE = LLCP_STATE_IDLE,
	/* Peripheral-initiated request (5.1.20) */
	LP_SUBRATE_STATE_WAIT_TX_SUBRATE_REQ,
	LP_SUBRATE_STATE_WAIT_RX_SUBRATE_IND,
	/* Central-initiated update (5.1.19) */
	LP_SUBRATE_STATE_WAIT_TX_SUBRATE_IND,
	LP_SUBRATE_STATE_WAIT_TX_ACK_SUBRATE_IND,
	LP_SUBRATE_STATE_WAIT_NTF_AVAIL,
};

/* LLCP Local Procedure Subrate FSM events */
enum {
	/* Procedure run */
	LP_SUBRATE_EVT_RUN,
	/* Indication received */
	LP_SUBRATE_EVT_SUBRATE_IND,
	/* Reject response received */
	LP_SUBRATE_EVT_REJECT,
	/* Unknown response received */
	LP_SUBRATE_EVT_UNKNOWN,
	/* Tx Ack received */
	LP_SUBRATE_EVT_ACK,
};

/* LLCP Remote Procedure Subrate FSM states */
enum {
	RP_SUBRATE_STATE_IDLE = LLCP_STATE_IDLE,
	/* Central handling a peripheral request (5.1.20 -> 5.1.19) */
	RP_SUBRATE_STATE_WAIT_RX_SUBRATE_REQ,
	RP_SUBRATE_STATE_WAIT_TX_SUBRATE_IND,
	RP_SUBRATE_STATE_WAIT_TX_ACK_SUBRATE_IND,
	RP_SUBRATE_STATE_WAIT_TX_REJECT_EXT_IND,
	/* Peripheral handling a central indication (5.1.19) */
	RP_SUBRATE_STATE_WAIT_RX_SUBRATE_IND,
	RP_SUBRATE_STATE_WAIT_NTF_AVAIL,
};

/* LLCP Remote Procedure Subrate FSM events */
enum {
	/* Procedure run */
	RP_SUBRATE_EVT_RUN,
	/* Request received */
	RP_SUBRATE_EVT_SUBRATE_REQ,
	/* Indication received */
	RP_SUBRATE_EVT_SUBRATE_IND,
	/* Tx Ack received */
	RP_SUBRATE_EVT_ACK,
};

/*
 * Shared helpers
 */

/* Compute the new connSubrateBaseEvent (Central only). The base event is placed a
 * few connection events into the future so both devices converge on the same phase.
 * TODO: apply the S[15:14] = E[15:14] wrap-handling rule (5.1.19).
 */
static uint16_t subrate_base_event_calc(struct ll_conn *conn)
{
	return ull_conn_event_counter(conn) + SUBRATE_BASE_EVENT_DELTA;
}

/* Validate a received LL_SUBRATE_REQ against the Host-provided acceptable
 * parameters and, if acceptable, fill in the negotiated values to be sent in
 * the LL_SUBRATE_IND. Core Spec Vol 6, Part B, Section 5.1.20.
 * Returns true if the request is acceptable.
 */
__maybe_unused static bool subrate_req_negotiate(struct ll_conn *conn, struct proc_ctx *ctx)
{
	const uint16_t req_min = ctx->data.subrate.subrate_factor_min;
	const uint16_t req_max = ctx->data.subrate.subrate_factor_max;
	const uint16_t req_latency = ctx->data.subrate.max_latency;
	const uint16_t req_cont = ctx->data.subrate.continuation_number;
	const uint16_t req_timeout = ctx->data.subrate.timeout;

	const uint16_t acc_min = conn->subrate.acc_factor_min;
	const uint16_t acc_max = conn->subrate.acc_factor_max;
	const uint16_t acc_latency = conn->subrate.acc_max_latency;
	const uint16_t acc_timeout = conn->subrate.acc_supervision_timeout;
	const uint16_t acc_cont = conn->subrate.acc_continuation_number;

	uint16_t factor;
	uint16_t latency;
	uint16_t cont;
	uint16_t timeout;

	/* Acceptance criteria (5.1.20) */
	if ((req_latency > acc_latency) || (req_timeout > acc_timeout) ||
	    (req_max < acc_min) || (req_min > acc_max)) {
		return false;
	}

	/* Supervision timeout must exceed 2 x connInterval x SubrateFactorMin x
	 * (Max_Latency + 1). connInterval is in 1.25 ms units and the timeout in
	 * 10 ms units, so the comparison is scaled by 4 (see ull_llcp_conn_upd.c).
	 */
	if (((uint32_t)req_timeout * 4U) <=
	    ((uint32_t)conn->lll.interval * req_min * (req_latency + 1U))) {
		return false;
	}

	/* Negotiated values within the overlap of acceptable and requested ranges */
	factor = MIN(acc_max, req_max);
	factor = MAX(factor, MAX(acc_min, req_min));
	if ((factor < SUBRATE_FACTOR_MIN) || (factor > SUBRATE_FACTOR_MAX)) {
		return false;
	}

	latency = MIN(req_latency, acc_latency);
	/* factor * (latency + 1) shall be <= 500 */
	if (factor * (latency + 1U) > SUBRATE_FACTOR_LATENCY_PRODUCT_MAX) {
		latency = (SUBRATE_FACTOR_LATENCY_PRODUCT_MAX / factor) - 1U;
	}

	cont = MIN(MAX(acc_cont, req_cont), (uint16_t)(factor - 1U));
	timeout = MIN(req_timeout, acc_timeout);

	ctx->data.subrate.subrate_factor = factor;
	ctx->data.subrate.latency = latency;
	ctx->data.subrate.continuation_number = cont;
	ctx->data.subrate.timeout = timeout;
	ctx->data.subrate.subrate_base_event = subrate_base_event_calc(conn);

	return true;
}

/* Choose the parameters for a Central-initiated update (5.1.19), driven by the
 * Host's HCI_LE_Subrate_Request values already stored in ctx. The Host has
 * validated ranges and the latency/timeout relationship; pick the largest
 * acceptable subrate factor for maximum power saving.
 */
__maybe_unused static void subrate_ind_params_calc(struct ll_conn *conn, struct proc_ctx *ctx)
{
	uint16_t factor = ctx->data.subrate.subrate_factor_max;
	uint16_t latency = ctx->data.subrate.max_latency;
	uint16_t cont = ctx->data.subrate.continuation_number;

	if (factor < SUBRATE_FACTOR_MIN) {
		factor = SUBRATE_FACTOR_MIN;
	}

	if (factor * (latency + 1U) > SUBRATE_FACTOR_LATENCY_PRODUCT_MAX) {
		latency = (SUBRATE_FACTOR_LATENCY_PRODUCT_MAX / factor) - 1U;
	}

	cont = MIN(cont, (uint16_t)(factor - 1U));

	ctx->data.subrate.subrate_factor = factor;
	ctx->data.subrate.latency = latency;
	ctx->data.subrate.continuation_number = cont;
	/* timeout already in ctx from the Host request */
	ctx->data.subrate.subrate_base_event = subrate_base_event_calc(conn);
}

/* Store the negotiated subrate parameters on the connection. The connection event
 * scheduling in ull_conn_done() uses conn->subrate to skip to subrated events
 * (5.1.19/5.1.20 + 4.5.1).
 */
static void subrate_apply(struct ll_conn *conn, struct proc_ctx *ctx)
{
	uint16_t factor = ctx->data.subrate.subrate_factor;
	uint16_t base_event = ctx->data.subrate.subrate_base_event;

	/* Normalise connSubrateBaseEvent to the most recent subrated event at or
	 * before the current event counter, preserving (base mod factor). Any
	 * value congruent mod factor selects the same set of subrated events, and
	 * keeping it close to "now" simplifies the scheduler's next-event math.
	 */
	if (factor > 1U) {
		int16_t event_diff = (int16_t)(ull_conn_event_counter(conn) - base_event);
		int16_t subrate_events_diff = event_diff / (int16_t)factor;

		base_event += (uint16_t)(factor * subrate_events_diff);
	}

	conn->subrate.factor = factor;
	conn->subrate.base_event = base_event;
	conn->subrate.continuation_number = ctx->data.subrate.continuation_number;
	conn->subrate.peripheral_latency = ctx->data.subrate.latency;
	conn->subrate.cont_num_left = 0U;
	conn->supervision_timeout = ctx->data.subrate.timeout;
}

static void subrate_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct node_rx_subrate_change *sr;

	ntf = ctx->node_ref.rx;
	ctx->node_ref.rx = NULL;
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_SUBRATE_CHANGE;
	ntf->hdr.handle = conn->lll.handle;

	sr = (struct node_rx_subrate_change *)ntf->pdu;
	sr->status = ctx->data.subrate.error;
	sr->subrate_factor = conn->subrate.factor;
	sr->peripheral_latency = conn->subrate.peripheral_latency;
	sr->continuation_number = conn->subrate.continuation_number;
	sr->supervision_timeout = conn->supervision_timeout;

	ll_rx_put_sched(ntf->hdr.link, ntf);
}

/*
 * LLCP Local Procedure Subrate FSM
 */

static void lp_subrate_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	tx = llcp_tx_alloc(conn, ctx);
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	switch (opcode) {
#if defined(CONFIG_BT_PERIPHERAL)
	case PDU_DATA_LLCTRL_TYPE_SUBRATE_REQ:
		llcp_pdu_encode_subrate_req(ctx, pdu);
		break;
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CENTRAL)
	case PDU_DATA_LLCTRL_TYPE_SUBRATE_IND:
		llcp_pdu_encode_subrate_ind(ctx, pdu);
		/* New parameters take effect once this PDU is acknowledged */
		ctx->node_ref.tx_ack = tx;
		break;
#endif /* CONFIG_BT_CENTRAL */
	default:
		LL_ASSERT(0);
		break;
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	llcp_tx_enqueue(conn, tx);

	/* Restart procedure response timeout timer */
	llcp_lr_prt_restart(conn);
}

static void lp_subrate_complete(struct ll_conn *conn, struct proc_ctx *ctx)
{
	llcp_lr_prt_stop(conn);
	llcp_lr_complete(conn);
	ctx->state = LP_SUBRATE_STATE_IDLE;
}

static void lp_subrate_ntf_complete(struct ll_conn *conn, struct proc_ctx *ctx)
{
	subrate_ntf(conn, ctx);
	lp_subrate_complete(conn, ctx);
}

#if defined(CONFIG_BT_PERIPHERAL)
static void lp_subrate_send_subrate_req(struct ll_conn *conn, struct proc_ctx *ctx)
{
	if (llcp_lr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = LP_SUBRATE_STATE_WAIT_TX_SUBRATE_REQ;
	} else {
		lp_subrate_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_SUBRATE_REQ);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_SUBRATE_IND;
		ctx->state = LP_SUBRATE_STATE_WAIT_RX_SUBRATE_IND;
	}
}
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
static void lp_subrate_send_subrate_ind(struct ll_conn *conn, struct proc_ctx *ctx)
{
	if (llcp_lr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = LP_SUBRATE_STATE_WAIT_TX_SUBRATE_IND;
	} else {
		subrate_ind_params_calc(conn, ctx);
		lp_subrate_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_SUBRATE_IND);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		ctx->state = LP_SUBRATE_STATE_WAIT_TX_ACK_SUBRATE_IND;
	}
}
#endif /* CONFIG_BT_CENTRAL */

static void lp_subrate_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
			       void *param)
{
	switch (evt) {
	case LP_SUBRATE_EVT_RUN:
		switch (conn->lll.role) {
#if defined(CONFIG_BT_CENTRAL)
		case BT_HCI_ROLE_CENTRAL:
			lp_subrate_send_subrate_ind(conn, ctx);
			break;
#endif /* CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_PERIPHERAL)
		case BT_HCI_ROLE_PERIPHERAL:
			lp_subrate_send_subrate_req(conn, ctx);
			break;
#endif /* CONFIG_BT_PERIPHERAL */
		default:
			LL_ASSERT(0);
			break;
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

#if defined(CONFIG_BT_PERIPHERAL)
static void lp_subrate_st_wait_tx_subrate_req(struct ll_conn *conn, struct proc_ctx *ctx,
					      uint8_t evt, void *param)
{
	switch (evt) {
	case LP_SUBRATE_EVT_RUN:
		lp_subrate_send_subrate_req(conn, ctx);
		break;
	default:
		break;
	}
}

static void lp_subrate_st_wait_rx_subrate_ind(struct ll_conn *conn, struct proc_ctx *ctx,
					      uint8_t evt, void *param)
{
	struct pdu_data *pdu = (struct pdu_data *)param;

	switch (evt) {
	case LP_SUBRATE_EVT_SUBRATE_IND:
		llcp_pdu_decode_subrate_ind(ctx, pdu);
		subrate_apply(conn, ctx);
		ctx->data.subrate.error = BT_HCI_ERR_SUCCESS;
		break;
	case LP_SUBRATE_EVT_UNKNOWN:
		/* Peer does not support the feature; disable locally */
		feature_unmask_features(conn, LL_FEAT_BIT_CONN_SUBRATING);
		ctx->data.subrate.error = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
		break;
	case LP_SUBRATE_EVT_REJECT:
		ctx->data.subrate.error = pdu->llctrl.reject_ext_ind.error_code;
		break;
	default:
		return;
	}

	/* Emit the Subrate Change notification on a freshly allocated node (the
	 * incoming PDU node is released by the framework).
	 */
	if (llcp_ntf_alloc_is_available()) {
		ctx->node_ref.rx = llcp_ntf_alloc();
		lp_subrate_ntf_complete(conn, ctx);
	} else {
		ctx->state = LP_SUBRATE_STATE_WAIT_NTF_AVAIL;
	}
}
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
static void lp_subrate_st_wait_tx_subrate_ind(struct ll_conn *conn, struct proc_ctx *ctx,
					      uint8_t evt, void *param)
{
	switch (evt) {
	case LP_SUBRATE_EVT_RUN:
		lp_subrate_send_subrate_ind(conn, ctx);
		break;
	default:
		break;
	}
}

static void lp_subrate_st_wait_tx_ack_subrate_ind(struct ll_conn *conn, struct proc_ctx *ctx,
						  uint8_t evt, void *param)
{
	switch (evt) {
	case LP_SUBRATE_EVT_ACK:
		/* IND acknowledged: leave subrate transition mode and apply. Until the
		 * ack, the active procedure keeps the Central present on every event.
		 */
		subrate_apply(conn, ctx);
		ctx->data.subrate.error = BT_HCI_ERR_SUCCESS;
		if (llcp_ntf_alloc_is_available()) {
			ctx->node_ref.rx = llcp_ntf_alloc();
			lp_subrate_ntf_complete(conn, ctx);
		} else {
			ctx->state = LP_SUBRATE_STATE_WAIT_NTF_AVAIL;
		}
		break;
	default:
		break;
	}
}

static void lp_subrate_st_wait_ntf_avail(struct ll_conn *conn, struct proc_ctx *ctx,
					 uint8_t evt, void *param)
{
	switch (evt) {
	case LP_SUBRATE_EVT_RUN:
		if (llcp_ntf_alloc_is_available()) {
			ctx->node_ref.rx = llcp_ntf_alloc();
			lp_subrate_ntf_complete(conn, ctx);
		}
		break;
	default:
		break;
	}
}
#endif /* CONFIG_BT_CENTRAL */

static void lp_subrate_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				   void *param)
{
	switch (ctx->state) {
	case LP_SUBRATE_STATE_IDLE:
		lp_subrate_st_idle(conn, ctx, evt, param);
		break;
#if defined(CONFIG_BT_PERIPHERAL)
	case LP_SUBRATE_STATE_WAIT_TX_SUBRATE_REQ:
		lp_subrate_st_wait_tx_subrate_req(conn, ctx, evt, param);
		break;
	case LP_SUBRATE_STATE_WAIT_RX_SUBRATE_IND:
		lp_subrate_st_wait_rx_subrate_ind(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CENTRAL)
	case LP_SUBRATE_STATE_WAIT_TX_SUBRATE_IND:
		lp_subrate_st_wait_tx_subrate_ind(conn, ctx, evt, param);
		break;
	case LP_SUBRATE_STATE_WAIT_TX_ACK_SUBRATE_IND:
		lp_subrate_st_wait_tx_ack_subrate_ind(conn, ctx, evt, param);
		break;
	case LP_SUBRATE_STATE_WAIT_NTF_AVAIL:
		lp_subrate_st_wait_ntf_avail(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_CENTRAL */
	default:
		LL_ASSERT(0);
		break;
	}
}

void llcp_lp_subrate_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *)rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_SUBRATE_IND:
		lp_subrate_execute_fsm(conn, ctx, LP_SUBRATE_EVT_SUBRATE_IND, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		lp_subrate_execute_fsm(conn, ctx, LP_SUBRATE_EVT_UNKNOWN, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		lp_subrate_execute_fsm(conn, ctx, LP_SUBRATE_EVT_REJECT, pdu);
		break;
	default:
		/* Invalid PDU received, terminate connection */
		conn->llcp_terminate.reason_final = BT_HCI_ERR_LMP_PDU_NOT_ALLOWED;
		lp_subrate_complete(conn, ctx);
		break;
	}
}

void llcp_lp_subrate_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, struct node_tx *tx)
{
	lp_subrate_execute_fsm(conn, ctx, LP_SUBRATE_EVT_ACK, tx);
}

void llcp_lp_subrate_init_proc(struct proc_ctx *ctx)
{
	ctx->state = LP_SUBRATE_STATE_IDLE;
}

void llcp_lp_subrate_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	lp_subrate_execute_fsm(conn, ctx, LP_SUBRATE_EVT_RUN, param);
}

/*
 * LLCP Remote Procedure Subrate FSM
 */

__maybe_unused static void rp_subrate_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	tx = llcp_tx_alloc(conn, ctx);
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	switch (opcode) {
#if defined(CONFIG_BT_CENTRAL)
	case PDU_DATA_LLCTRL_TYPE_SUBRATE_IND:
		llcp_pdu_encode_subrate_ind(ctx, pdu);
		/* New parameters take effect once this PDU is acknowledged */
		ctx->node_ref.tx_ack = tx;
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		llcp_pdu_encode_reject_ext_ind(pdu, PDU_DATA_LLCTRL_TYPE_SUBRATE_REQ,
					       ctx->data.subrate.error);
		break;
#endif /* CONFIG_BT_CENTRAL */
	default:
		LL_ASSERT(0);
		break;
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	llcp_tx_enqueue(conn, tx);

	llcp_rr_prt_restart(conn);
}

static void rp_subrate_complete(struct ll_conn *conn, struct proc_ctx *ctx)
{
	llcp_rr_prt_stop(conn);
	llcp_rr_complete(conn);
	ctx->state = RP_SUBRATE_STATE_IDLE;
}

static void rp_subrate_ntf_complete(struct ll_conn *conn, struct proc_ctx *ctx)
{
	subrate_ntf(conn, ctx);
	rp_subrate_complete(conn, ctx);
}

#if defined(CONFIG_BT_CENTRAL)
static void rp_subrate_send_reject_ext_ind(struct ll_conn *conn, struct proc_ctx *ctx)
{
	if (llcp_rr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = RP_SUBRATE_STATE_WAIT_TX_REJECT_EXT_IND;
	} else {
		rp_subrate_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND);
		rp_subrate_complete(conn, ctx);
	}
}

static void rp_subrate_send_subrate_ind(struct ll_conn *conn, struct proc_ctx *ctx)
{
	if (llcp_rr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = RP_SUBRATE_STATE_WAIT_TX_SUBRATE_IND;
	} else {
		rp_subrate_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_SUBRATE_IND);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		ctx->state = RP_SUBRATE_STATE_WAIT_TX_ACK_SUBRATE_IND;
	}
}

static void rp_subrate_st_wait_rx_subrate_req(struct ll_conn *conn, struct proc_ctx *ctx,
					      uint8_t evt, void *param)
{
	switch (evt) {
	case RP_SUBRATE_EVT_SUBRATE_REQ:
		llcp_pdu_decode_subrate_req(ctx, (struct pdu_data *)param);
		if ((ll_feat_get() & BIT64(BT_LE_FEAT_BIT_CONN_SUBRATING_HOST_SUPP)) == 0U) {
			/* Core Spec 5.1.20: the Central shall reject a Peripheral
			 * request if its own Host has not set the Connection Subrating
			 * (Host Support) feature bit.
			 */
			ctx->data.subrate.error = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
			rp_subrate_send_reject_ext_ind(conn, ctx);
		} else if (subrate_req_negotiate(conn, ctx)) {
			rp_subrate_send_subrate_ind(conn, ctx);
		} else {
			ctx->data.subrate.error = BT_HCI_ERR_UNSUPP_LL_PARAM_VAL;
			rp_subrate_send_reject_ext_ind(conn, ctx);
		}
		break;
	default:
		break;
	}
}

static void rp_subrate_st_wait_tx_subrate_ind(struct ll_conn *conn, struct proc_ctx *ctx,
					      uint8_t evt, void *param)
{
	switch (evt) {
	case RP_SUBRATE_EVT_RUN:
		rp_subrate_send_subrate_ind(conn, ctx);
		break;
	default:
		break;
	}
}

static void rp_subrate_st_wait_tx_reject_ext_ind(struct ll_conn *conn, struct proc_ctx *ctx,
						 uint8_t evt, void *param)
{
	switch (evt) {
	case RP_SUBRATE_EVT_RUN:
		rp_subrate_send_reject_ext_ind(conn, ctx);
		break;
	default:
		break;
	}
}

static void rp_subrate_st_wait_tx_ack_subrate_ind(struct ll_conn *conn, struct proc_ctx *ctx,
						  uint8_t evt, void *param)
{
	switch (evt) {
	case RP_SUBRATE_EVT_ACK:
		subrate_apply(conn, ctx);
		ctx->data.subrate.error = BT_HCI_ERR_SUCCESS;
		if (llcp_ntf_alloc_is_available()) {
			ctx->node_ref.rx = llcp_ntf_alloc();
			rp_subrate_ntf_complete(conn, ctx);
		} else {
			ctx->state = RP_SUBRATE_STATE_WAIT_NTF_AVAIL;
		}
		break;
	default:
		break;
	}
}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
static void rp_subrate_st_wait_rx_subrate_ind(struct ll_conn *conn, struct proc_ctx *ctx,
					      uint8_t evt, void *param)
{
	switch (evt) {
	case RP_SUBRATE_EVT_SUBRATE_IND:
		/* Peripheral shall accept and apply immediately on receipt (5.1.19) */
		llcp_pdu_decode_subrate_ind(ctx, (struct pdu_data *)param);
		subrate_apply(conn, ctx);
		ctx->data.subrate.error = BT_HCI_ERR_SUCCESS;
		if (llcp_ntf_alloc_is_available()) {
			ctx->node_ref.rx = llcp_ntf_alloc();
			rp_subrate_ntf_complete(conn, ctx);
		} else {
			ctx->state = RP_SUBRATE_STATE_WAIT_NTF_AVAIL;
		}
		break;
	default:
		break;
	}
}
#endif /* CONFIG_BT_PERIPHERAL */

static void rp_subrate_st_wait_ntf_avail(struct ll_conn *conn, struct proc_ctx *ctx,
					 uint8_t evt, void *param)
{
	switch (evt) {
	case RP_SUBRATE_EVT_RUN:
		if (llcp_ntf_alloc_is_available()) {
			ctx->node_ref.rx = llcp_ntf_alloc();
			rp_subrate_ntf_complete(conn, ctx);
		}
		break;
	default:
		break;
	}
}

static void rp_subrate_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
			       void *param)
{
	switch (evt) {
	case RP_SUBRATE_EVT_RUN:
		switch (conn->lll.role) {
#if defined(CONFIG_BT_CENTRAL)
		case BT_HCI_ROLE_CENTRAL:
			ctx->state = RP_SUBRATE_STATE_WAIT_RX_SUBRATE_REQ;
			break;
#endif /* CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_PERIPHERAL)
		case BT_HCI_ROLE_PERIPHERAL:
			ctx->state = RP_SUBRATE_STATE_WAIT_RX_SUBRATE_IND;
			break;
#endif /* CONFIG_BT_PERIPHERAL */
		default:
			LL_ASSERT(0);
			break;
		}
		break;
	default:
		break;
	}
}

static void rp_subrate_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				   void *param)
{
	switch (ctx->state) {
	case RP_SUBRATE_STATE_IDLE:
		rp_subrate_st_idle(conn, ctx, evt, param);
		break;
#if defined(CONFIG_BT_CENTRAL)
	case RP_SUBRATE_STATE_WAIT_RX_SUBRATE_REQ:
		rp_subrate_st_wait_rx_subrate_req(conn, ctx, evt, param);
		break;
	case RP_SUBRATE_STATE_WAIT_TX_SUBRATE_IND:
		rp_subrate_st_wait_tx_subrate_ind(conn, ctx, evt, param);
		break;
	case RP_SUBRATE_STATE_WAIT_TX_ACK_SUBRATE_IND:
		rp_subrate_st_wait_tx_ack_subrate_ind(conn, ctx, evt, param);
		break;
	case RP_SUBRATE_STATE_WAIT_TX_REJECT_EXT_IND:
		rp_subrate_st_wait_tx_reject_ext_ind(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_PERIPHERAL)
	case RP_SUBRATE_STATE_WAIT_RX_SUBRATE_IND:
		rp_subrate_st_wait_rx_subrate_ind(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_PERIPHERAL */
	case RP_SUBRATE_STATE_WAIT_NTF_AVAIL:
		rp_subrate_st_wait_ntf_avail(conn, ctx, evt, param);
		break;
	default:
		LL_ASSERT(0);
		break;
	}
}

void llcp_rp_subrate_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *)rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_SUBRATE_REQ:
		rp_subrate_execute_fsm(conn, ctx, RP_SUBRATE_EVT_SUBRATE_REQ, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_SUBRATE_IND:
		rp_subrate_execute_fsm(conn, ctx, RP_SUBRATE_EVT_SUBRATE_IND, pdu);
		break;
	default:
		/* Invalid PDU received, terminate connection */
		conn->llcp_terminate.reason_final = BT_HCI_ERR_LMP_PDU_NOT_ALLOWED;
		rp_subrate_complete(conn, ctx);
		break;
	}
}

void llcp_rp_subrate_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, struct node_tx *tx)
{
	rp_subrate_execute_fsm(conn, ctx, RP_SUBRATE_EVT_ACK, tx);
}

void llcp_rp_subrate_init_proc(struct proc_ctx *ctx)
{
	ctx->state = RP_SUBRATE_STATE_IDLE;
}

void llcp_rp_subrate_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_subrate_execute_fsm(conn, ctx, RP_SUBRATE_EVT_RUN, param);
}

#endif /* CONFIG_BT_CTLR_SUBRATING */
