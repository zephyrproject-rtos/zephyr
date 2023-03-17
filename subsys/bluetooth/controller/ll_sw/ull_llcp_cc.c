/*
 * Copyright (c) 2022 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include "hal/ecb.h"
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
#include "ull_conn_types.h"
#include "ull_conn_iso_types.h"
#include "ull_internal.h"
#include "ull_llcp.h"
#include "ull_llcp_internal.h"
#include "ull_llcp_features.h"
#include "ull_conn_internal.h"

#include "ull_iso_internal.h"
#include "ull_conn_iso_internal.h"
#include "ull_peripheral_iso_internal.h"
#include "ull_central_iso_internal.h"

#include <soc.h>
#include "hal/debug.h"

static void cc_ntf_established(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_conn_iso_estab *pdu;
	struct node_rx_pdu *ntf;
	uint8_t piggy_back;

	/* Allocate ntf node */
	ntf = ctx->node_ref.rx;
	LL_ASSERT(ntf);
	ctx->node_ref.rx = NULL;

	piggy_back = (ntf->hdr.type != NODE_RX_TYPE_RETAIN);

	ntf->hdr.type = NODE_RX_TYPE_CIS_ESTABLISHED;
	ntf->hdr.handle = conn->lll.handle;
	ntf->hdr.rx_ftr.param = ll_conn_iso_stream_get(ctx->data.cis_create.cis_handle);

	pdu = (struct node_rx_conn_iso_estab *)ntf->pdu;

	pdu->cis_handle = ctx->data.cis_create.cis_handle;
	pdu->status = ctx->data.cis_create.error;

	if (!piggy_back) {
		/* Enqueue notification towards LL */
		ll_rx_put_sched(ntf->hdr.link, ntf);
	}
}

#if defined(CONFIG_BT_PERIPHERAL)
/* LLCP Remote Procedure FSM states */
enum {
	/* Establish Procedure */
	RP_CC_STATE_IDLE,
	RP_CC_STATE_WAIT_RX_CIS_REQ,
	RP_CC_STATE_WAIT_REPLY,
	RP_CC_STATE_WAIT_TX_CIS_RSP,
	RP_CC_STATE_WAIT_TX_REJECT_IND,
	RP_CC_STATE_WAIT_RX_CIS_IND,
	RP_CC_STATE_WAIT_INSTANT,
	RP_CC_STATE_WAIT_CIS_ESTABLISHED,
	RP_CC_STATE_WAIT_NTF_AVAIL,
};

/* LLCP Remote Procedure FSM events */
enum {
	/* Procedure prepared */
	RP_CC_EVT_RUN,

	/* Request received */
	RP_CC_EVT_CIS_REQ,

	/* Response received */
	RP_CC_EVT_CIS_RSP,

	/* Indication received */
	RP_CC_EVT_CIS_IND,

	/* Create request accept reply */
	RP_CC_EVT_CIS_REQ_ACCEPT,

	/* Create request decline reply */
	RP_CC_EVT_CIS_REQ_REJECT,

	/* Reject response received */
	RP_CC_EVT_REJECT,

	/* Established */
	RP_CC_EVT_CIS_ESTABLISHED,

	/* Unknown response received */
	RP_CC_EVT_UNKNOWN,
};

static void rp_cc_check_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param);

/*
 * LLCP Remote Procedure FSM
 */

static void llcp_rp_cc_tx_rsp(struct ll_conn *conn, struct proc_ctx *ctx)
{
	uint16_t delay_conn_events;
	uint16_t conn_event_count;
	struct pdu_data *pdu;
	struct node_tx *tx;

	/* Allocate tx node */
	tx = llcp_tx_alloc(conn, ctx);
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;
	conn_event_count = ctx->data.cis_create.conn_event_count;

	/* Postpone if instant is in this or next connection event. This would handle obsolete value
	 * due to retransmission, as well as incorrect behavior by central.
	 * We need at least 2 connection events to get ready. First for receiving the indication,
	 * the second for setting up the CIS.
	 */
	ctx->data.cis_create.conn_event_count = MAX(ctx->data.cis_create.conn_event_count,
						    ull_conn_event_counter(conn) + 2U);

	delay_conn_events = ctx->data.cis_create.conn_event_count - conn_event_count;

	/* If instant is postponed, calculate the offset to add to CIS_Offset_Min and
	 * CIS_Offset_Max.
	 *
	 * BT Core v5.3, Vol 6, Part B, section 5.1.15:
	 * Two windows are equivalent if they have the same width and the difference between their
	 * start times is an integer multiple of ISO_Interval for the CIS.
	 *
	 * The offset shall compensate for the relation between ISO- and connection interval. The
	 * offset translates to what is additionally needed to move the window by an integer number
	 * of ISO intervals. I.e.:
	 *   offset = (delayed * CONN_interval) MOD ISO_interval
	 */
	if (delay_conn_events) {
		uint32_t conn_interval_us  = conn->lll.interval * CONN_INT_UNIT_US;
		uint32_t iso_interval_us   = ctx->data.cis_create.iso_interval * ISO_INT_UNIT_US;
		uint32_t offset_us = (delay_conn_events * conn_interval_us) % iso_interval_us;

		ctx->data.cis_create.cis_offset_min += offset_us;
		ctx->data.cis_create.cis_offset_max += offset_us;
	}

	llcp_pdu_encode_cis_rsp(ctx, pdu);
	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);
}

static void llcp_rp_cc_tx_reject(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = ctx->node_ref.tx;
	LL_ASSERT(tx);
	ctx->node_ref.tx = NULL;

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	llcp_pdu_encode_reject_ext_ind(pdu, opcode, ctx->data.cis_create.error);
	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);
}

static void rp_cc_ntf_create(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct node_rx_conn_iso_req *pdu;

	ntf = ctx->node_ref.rx;
	ctx->node_ref.rx = NULL;
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_CIS_REQUEST;
	ntf->hdr.handle = conn->lll.handle;
	pdu = (struct node_rx_conn_iso_req *)ntf->pdu;

	pdu->cig_id = ctx->data.cis_create.cig_id;
	pdu->cis_id = ctx->data.cis_create.cis_id;
	pdu->cis_handle = ctx->data.cis_create.cis_handle;

	ctx->data.cis_create.host_request_to = 0U;
}

static void rp_cc_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	cc_ntf_established(conn, ctx);
	llcp_rr_complete(conn);
	ctx->state = RP_CC_STATE_IDLE;
}

static void rp_cc_send_cis_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
			       void *param)
{
	if (llcp_rr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = RP_CC_STATE_WAIT_TX_CIS_RSP;
	} else {
		llcp_rp_cc_tx_rsp(conn, ctx);

		/* Wait for the LL_CIS_IND */
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_CIS_IND;
		ctx->state = RP_CC_STATE_WAIT_RX_CIS_IND;
	}
}

static void rp_cc_send_reject_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				  void *param)
{
	if (llcp_rr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = RP_CC_STATE_WAIT_TX_REJECT_IND;
	} else {
		/* Allocate TX node to use, store in case we need to wait for NTF node */
		ctx->node_ref.tx = llcp_tx_alloc(conn, ctx);
		if (ctx->data.cis_create.error == BT_HCI_ERR_CONN_ACCEPT_TIMEOUT) {
			/* We complete with error, so we must generate NTF, thus we must make sure
			 * we have a node to use for NTF before TX'ing
			 */
			if (!llcp_ntf_alloc_is_available()) {
				ctx->state = RP_CC_STATE_WAIT_NTF_AVAIL;
				return;
			}
			ctx->node_ref.rx = llcp_ntf_alloc();

			/* Mark node as RETAIN to trigger put/sched */
			ctx->node_ref.rx->hdr.type = NODE_RX_TYPE_RETAIN;
		}

		llcp_rp_cc_tx_reject(conn, ctx, PDU_DATA_LLCTRL_TYPE_CIS_REQ);

		if (ctx->data.cis_create.error == BT_HCI_ERR_CONN_ACCEPT_TIMEOUT) {
			/* We reject due to an accept timeout, so we should generate NTF */
			rp_cc_complete(conn, ctx, evt, param);
		} else {
			/* Otherwise we quietly complete the procedure */
			llcp_rr_complete(conn);
			ctx->state = RP_CC_STATE_IDLE;
		}
	}
}

static void rp_cc_state_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
			     void *param)
{
	switch (evt) {
	case RP_CC_EVT_RUN:
		ctx->state = RP_CC_STATE_WAIT_RX_CIS_REQ;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static inline bool phy_valid(uint8_t phy)
{
	/* This is equivalent to:
	 * exactly one bit set, and no bit set is rfu's
	 */
	return (phy == PHY_1M || phy == PHY_2M || phy == PHY_CODED);
}

static uint8_t rp_cc_check_phy(struct ll_conn *conn, struct proc_ctx *ctx,
					    struct pdu_data *pdu)
{
	if (!phy_valid(pdu->llctrl.cis_req.c_phy) ||
	    !phy_valid(pdu->llctrl.cis_req.p_phy)) {
		/* zero, more than one or any rfu bit selected in either phy */
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

#if defined(CONFIG_BT_CTLR_PHY)
	const uint8_t phys = pdu->llctrl.cis_req.p_phy | pdu->llctrl.cis_req.c_phy;

	if (((phys & PHY_2M) && !feature_phy_2m(conn)) ||
	    ((phys & PHY_CODED) && !feature_phy_coded(conn))) {
		/* Unsupported phy selected */
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}
#endif /* CONFIG_BT_CTLR_PHY */

	return BT_HCI_ERR_SUCCESS;
}

static void rp_cc_state_wait_rx_cis_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					void *param)
{
	struct pdu_data *pdu = (struct pdu_data *)param;

	switch (evt) {
	case RP_CC_EVT_CIS_REQ:
		/* Handle CIS request */
		llcp_pdu_decode_cis_req(ctx, pdu);

		/* Check PHY */
		ctx->data.cis_create.error = rp_cc_check_phy(conn, ctx, pdu);

		if (ctx->data.cis_create.error == BT_HCI_ERR_SUCCESS) {
			ctx->data.cis_create.error =
				ull_peripheral_iso_acquire(conn, &pdu->llctrl.cis_req,
							   &ctx->data.cis_create.cis_handle);
		}

		if (ctx->data.cis_create.error == BT_HCI_ERR_SUCCESS) {
			/* Now controller accepts, so go ask the host to accept or decline */
			rp_cc_ntf_create(conn, ctx);
			ctx->state = RP_CC_STATE_WAIT_REPLY;
		} else {
			/* Now controller rejects, right out */
			rp_cc_send_reject_ind(conn, ctx, evt, param);
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cc_state_wait_tx_cis_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					void *param)
{
	switch (evt) {
	case RP_CC_EVT_RUN:
		rp_cc_send_cis_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cc_state_wait_tx_reject_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					 void *param)
{
	switch (evt) {
	case RP_CC_EVT_RUN:
		rp_cc_send_reject_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cc_state_wait_rx_cis_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					void *param)
{
	struct pdu_data *pdu = (struct pdu_data *)param;

	switch (evt) {
	case RP_CC_EVT_CIS_IND:
		llcp_pdu_decode_cis_ind(ctx, pdu);
		if (!ull_peripheral_iso_setup(&pdu->llctrl.cis_ind, ctx->data.cis_create.cig_id,
					      ctx->data.cis_create.cis_handle,
					      &ctx->data.cis_create.conn_event_count)) {

			/* CIS has been setup, go wait for 'instant' before starting */
			ctx->state = RP_CC_STATE_WAIT_INSTANT;

			/* Mark node as RETAIN to keep until we need for NTF */
			llcp_rx_node_retain(ctx);

			/* Check if this connection event is where we need to start the CIS */
			rp_cc_check_instant(conn, ctx, evt, param);
			break;
		}
		/* If we get to here the CIG_ID referred in req/acquire has become void/invalid */
		/* This cannot happen unless the universe has started to deflate */
		LL_ASSERT(0);
	case RP_CC_EVT_REJECT:
		/* Handle CIS creation rejection */
		break;

	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_cc_state_wait_ntf_avail(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				 void *param)
{
	switch (evt) {
	case RP_CC_EVT_RUN:
		if (llcp_ntf_alloc_is_available()) {
			ctx->node_ref.rx = llcp_ntf_alloc();
			/* Mark node as RETAIN to trigger put/sched */
			ctx->node_ref.rx->hdr.type = NODE_RX_TYPE_RETAIN;

			/* Now we're good to TX reject and complete procedure*/
			llcp_rp_cc_tx_reject(conn, ctx, PDU_DATA_LLCTRL_TYPE_CIS_REQ);
			rp_cc_complete(conn, ctx, evt, param);
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}


static void rp_cc_check_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	uint16_t start_event_count;
	uint16_t event_counter;

	event_counter = ull_conn_event_counter(conn);
	start_event_count = ctx->data.cis_create.conn_event_count;

	if (is_instant_reached_or_passed(start_event_count, event_counter)) {
		uint16_t instant_latency = (event_counter - start_event_count) & 0xffff;

		/* Start CIS */
		ull_conn_iso_start(conn, conn->llcp.prep.ticks_at_expire,
				   ctx->data.cis_create.cis_handle,
				   instant_latency);

		/* Now we can wait for CIS to become established */
		ctx->state = RP_CC_STATE_WAIT_CIS_ESTABLISHED;
	}
}

static void rp_cc_state_wait_reply(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				   void *param)
{

	switch (evt) {
	case RP_CC_EVT_CIS_REQ_ACCEPT:
		/* Continue procedure in next prepare run */
		ctx->state = RP_CC_STATE_WAIT_TX_CIS_RSP;
		break;
	case RP_CC_EVT_RUN:
		/* Update 'time' and check for timeout on the Reply */
		ctx->data.cis_create.host_request_to += (conn->lll.interval * CONN_INT_UNIT_US);
		if (ctx->data.cis_create.host_request_to < conn->connect_accept_to) {
			break;
		}
		/* Reject 'reason/error' */
		ctx->data.cis_create.error = BT_HCI_ERR_CONN_ACCEPT_TIMEOUT;
		/* If timeout is hit, fall through and reject */
	case RP_CC_EVT_CIS_REQ_REJECT:
		/* CIS Request is rejected, so clean up CIG/CIS acquisitions */
		ull_peripheral_iso_release(ctx->data.cis_create.cis_handle);
		/* Continue procedure in next prepare run */
		ctx->state = RP_CC_STATE_WAIT_TX_REJECT_IND;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}


static void rp_cc_state_wait_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				     void *param)
{
	switch (evt) {
	case RP_CC_EVT_RUN:
		rp_cc_check_instant(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}


static void rp_cc_state_wait_cis_established(struct ll_conn *conn, struct proc_ctx *ctx,
					     uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CC_EVT_CIS_ESTABLISHED:
		rp_cc_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}


static void rp_cc_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->state) {
	/* Create Procedure */
	case RP_CC_STATE_IDLE:
		rp_cc_state_idle(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_RX_CIS_REQ:
		rp_cc_state_wait_rx_cis_req(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_TX_REJECT_IND:
		rp_cc_state_wait_tx_reject_ind(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_TX_CIS_RSP:
		rp_cc_state_wait_tx_cis_rsp(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_REPLY:
		rp_cc_state_wait_reply(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_RX_CIS_IND:
		rp_cc_state_wait_rx_cis_ind(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_INSTANT:
		rp_cc_state_wait_instant(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_CIS_ESTABLISHED:
		rp_cc_state_wait_cis_established(conn, ctx, evt, param);
		break;
	case RP_CC_STATE_WAIT_NTF_AVAIL:
		rp_cc_state_wait_ntf_avail(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void llcp_rp_cc_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *)rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_CIS_REQ:
		rp_cc_execute_fsm(conn, ctx, RP_CC_EVT_CIS_REQ, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_CIS_IND:
		rp_cc_execute_fsm(conn, ctx, RP_CC_EVT_CIS_IND, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		rp_cc_execute_fsm(conn, ctx, RP_CC_EVT_REJECT, pdu);
		break;
	default:
		/* Invalid behaviour */
		/* Invalid PDU received so terminate connection */
		conn->llcp_terminate.reason_final = BT_HCI_ERR_LMP_PDU_NOT_ALLOWED;
		llcp_rr_complete(conn);
		ctx->state = RP_CC_STATE_IDLE;
		break;
	}
}

void llcp_rp_cc_init_proc(struct proc_ctx *ctx)
{
	switch (ctx->proc) {
	case PROC_CIS_CREATE:
		ctx->state = RP_CC_STATE_IDLE;
		break;
	default:
		LL_ASSERT(0);
	}
}

bool llcp_rp_cc_awaiting_reply(struct proc_ctx *ctx)
{
	return (ctx->state == RP_CC_STATE_WAIT_REPLY);
}

bool llcp_rp_cc_awaiting_established(struct proc_ctx *ctx)
{
	return (ctx->state == RP_CC_STATE_WAIT_CIS_ESTABLISHED);
}

void llcp_rp_cc_accept(struct ll_conn *conn, struct proc_ctx *ctx)
{
	rp_cc_execute_fsm(conn, ctx, RP_CC_EVT_CIS_REQ_ACCEPT, NULL);
}

void llcp_rp_cc_reject(struct ll_conn *conn, struct proc_ctx *ctx)
{
	rp_cc_execute_fsm(conn, ctx, RP_CC_EVT_CIS_REQ_REJECT, NULL);
}

void llcp_rp_cc_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_cc_execute_fsm(conn, ctx, RP_CC_EVT_RUN, param);
}

bool llcp_rp_cc_awaiting_instant(struct proc_ctx *ctx)
{
	return (ctx->state == RP_CC_STATE_WAIT_INSTANT);
}

void llcp_rp_cc_established(struct ll_conn *conn, struct proc_ctx *ctx)
{
	rp_cc_execute_fsm(conn, ctx, RP_CC_EVT_CIS_ESTABLISHED, NULL);
}
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
static void lp_cc_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param);

/* LLCP Local Procedure FSM states */
enum {
	LP_CC_STATE_IDLE,
	LP_CC_STATE_WAIT_TX_CIS_REQ,
	LP_CC_STATE_WAIT_RX_CIS_RSP,
	LP_CC_STATE_WAIT_TX_CIS_IND,
	LP_CC_STATE_WAIT_INSTANT,
	LP_CC_STATE_WAIT_ESTABLISHED,
};

/* LLCP Local Procedure CIS Creation FSM events */
enum {
	/* Procedure run */
	LP_CC_EVT_RUN,

	/* Response received */
	LP_CC_EVT_CIS_RSP,

	/* Reject response received */
	LP_CC_EVT_REJECT,

	/* CIS established */
	LP_CC_EVT_ESTABLISHED,

	/* Unknown response received */
	LP_CC_EVT_UNKNOWN,
};

static void lp_cc_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = llcp_tx_alloc(conn, ctx);
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (opcode) {
	case PDU_DATA_LLCTRL_TYPE_CIS_REQ:
		llcp_pdu_encode_cis_req(ctx, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_CIS_IND:
		llcp_pdu_encode_cis_ind(ctx, pdu);
		break;
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
		break;
	}

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);
}

void llcp_lp_cc_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *)rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_CIS_RSP:
		lp_cc_execute_fsm(conn, ctx, LP_CC_EVT_CIS_RSP, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		lp_cc_execute_fsm(conn, ctx, LP_CC_EVT_REJECT, pdu);
		break;
	default:
		/* Invalid behaviour */
		/* Invalid PDU received so terminate connection */
		conn->llcp_terminate.reason_final = BT_HCI_ERR_LMP_PDU_NOT_ALLOWED;
		llcp_lr_complete(conn);
		ctx->state = LP_CC_STATE_IDLE;
		break;
	}
}

static void lp_cc_send_cis_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
			       void *param)
{
	if (llcp_lr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = LP_CC_STATE_WAIT_TX_CIS_REQ;
	} else {
		/* Update conn_event_count */
		ctx->data.cis_create.conn_event_count =
			ull_central_iso_cis_offset_get(ctx->data.cis_create.cis_handle, NULL, NULL);

		lp_cc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_CIS_REQ);

		ctx->state = LP_CC_STATE_WAIT_RX_CIS_RSP;
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_CIS_RSP;
	}
}

static void lp_cc_st_wait_tx_cis_req(struct ll_conn *conn, struct proc_ctx *ctx,
				     uint8_t evt, void *param)
{
	switch (evt) {
	case LP_CC_EVT_RUN:
		lp_cc_send_cis_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cc_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_CC_EVT_RUN:
		switch (ctx->proc) {
		case PROC_CIS_CREATE:
			lp_cc_send_cis_req(conn, ctx, evt, param);
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

static void cc_prepare_cis_ind(struct ll_conn *conn, struct proc_ctx *ctx)
{
	uint8_t err;

	/* Setup central parameters based on CIS_RSP */
	err = ull_central_iso_setup(ctx->data.cis_create.cis_handle,
				    &ctx->data.cis_create.cig_sync_delay,
				    &ctx->data.cis_create.cis_sync_delay,
				    &ctx->data.cis_create.cis_offset_min,
				    &ctx->data.cis_create.cis_offset_max,
				    &ctx->data.cis_create.conn_event_count,
				    ctx->data.cis_create.aa);
	LL_ASSERT(!err);

	ctx->state = LP_CC_STATE_WAIT_INSTANT;
	ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
}

static void lp_cc_send_cis_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
			       void *param)
{
	if (llcp_lr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = LP_CC_STATE_WAIT_TX_CIS_IND;
	} else {
		cc_prepare_cis_ind(conn, ctx);
		lp_cc_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_CIS_IND);
	}
}

static void lp_cc_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	cc_ntf_established(conn, ctx);
	llcp_lr_complete(conn);
	ctx->state = LP_CC_STATE_IDLE;
}

static void lp_cc_st_wait_rx_cis_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				     void *param)
{
	struct pdu_data *pdu = (struct pdu_data *)param;

	switch (evt) {
	case LP_CC_EVT_CIS_RSP:
		/* TODO: Reject response if outside offset range? */
		llcp_pdu_decode_cis_rsp(ctx, param);

		/* Mark RX node to NOT release */
		llcp_rx_node_retain(ctx);

		lp_cc_send_cis_ind(conn, ctx, evt, param);
		break;
	case LP_CC_EVT_UNKNOWN:
		/* Unsupported in peer, so disable locally for this connection */
		feature_unmask_features(conn, LL_FEAT_BIT_CIS_PERIPHERAL);
		ctx->data.cis_create.error = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
		lp_cc_complete(conn, ctx, evt, param);
		break;
	case LP_CC_EVT_REJECT:
		if (pdu->llctrl.reject_ext_ind.error_code == BT_HCI_ERR_UNSUPP_REMOTE_FEATURE) {
			/* Unsupported in peer, so disable locally for this connection */
			feature_unmask_features(conn, LL_FEAT_BIT_CIS_PERIPHERAL);
		}
		ctx->data.cis_create.error = pdu->llctrl.reject_ext_ind.error_code;
		lp_cc_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cc_st_wait_tx_cis_ind(struct ll_conn *conn, struct proc_ctx *ctx,
				     uint8_t evt, void *param)
{
	switch (evt) {
	case LP_CC_EVT_RUN:
		lp_cc_send_cis_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cc_check_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	uint16_t start_event_count;
	uint16_t instant_latency;
	uint16_t event_counter;

	event_counter = ull_conn_event_counter(conn);
	start_event_count = ctx->data.cis_create.conn_event_count;

	instant_latency = (event_counter - start_event_count) & 0xffff;
	if (instant_latency <= 0x7fff) {
		/* Start CIS */
		ull_conn_iso_start(conn, conn->llcp.prep.ticks_at_expire,
				   ctx->data.cis_create.cis_handle,
				   instant_latency);

		/* Now we can wait for CIS to become established */
		ctx->state = LP_CC_STATE_WAIT_ESTABLISHED;
	}
}

static void lp_cc_st_wait_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				  void *param)
{
	switch (evt) {
	case LP_CC_EVT_RUN:
		lp_cc_check_instant(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cc_st_wait_established(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				      void *param)
{
	switch (evt) {
	case LP_CC_EVT_ESTABLISHED:
		/* CIS was established, so let's go ahead and complete procedure */
		lp_cc_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_cc_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->state) {
	case LP_CC_STATE_IDLE:
		lp_cc_st_idle(conn, ctx, evt, param);
		break;
	case LP_CC_STATE_WAIT_TX_CIS_REQ:
		lp_cc_st_wait_tx_cis_req(conn, ctx, evt, param);
		break;
	case LP_CC_STATE_WAIT_RX_CIS_RSP:
		lp_cc_st_wait_rx_cis_rsp(conn, ctx, evt, param);
		break;
	case LP_CC_STATE_WAIT_TX_CIS_IND:
		lp_cc_st_wait_tx_cis_ind(conn, ctx, evt, param);
		break;
	case LP_CC_STATE_WAIT_INSTANT:
		lp_cc_st_wait_instant(conn, ctx, evt, param);
		break;
	case LP_CC_STATE_WAIT_ESTABLISHED:
		lp_cc_st_wait_established(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
		break;
	}
}

void llcp_lp_cc_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	lp_cc_execute_fsm(conn, ctx, LP_CC_EVT_RUN, param);
}

bool llcp_lp_cc_is_active(struct proc_ctx *ctx)
{
	return ctx->state != LP_CC_STATE_IDLE;
}

bool llcp_lp_cc_awaiting_established(struct proc_ctx *ctx)
{
	return (ctx->state == LP_CC_STATE_WAIT_ESTABLISHED);
}

void llcp_lp_cc_established(struct ll_conn *conn, struct proc_ctx *ctx)
{
	lp_cc_execute_fsm(conn, ctx, LP_CC_EVT_ESTABLISHED, NULL);
}
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */
