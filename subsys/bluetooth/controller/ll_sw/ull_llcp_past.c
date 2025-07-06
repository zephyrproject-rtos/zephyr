/*
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/sys/slist.h>

#include <zephyr/bluetooth/hci_types.h>

#include "hal/ccm.h"
#include "hal/debug.h"

#include "util/util.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll/lll_df_types.h"
#include "lll_filter.h"
#include "lll_scan.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_scan_types.h"
#include "ull_sync_types.h"
#include "ull_iso_types.h"
#include "ull_conn_types.h"
#include "ull_conn_iso_types.h"

#include "ll_settings.h"
#include "ll_feat.h"

#include "ull_llcp.h"
#include "ull_llcp_features.h"

#include "ull_internal.h"
#include "ull_sync_internal.h"
#include "ull_conn_internal.h"
#include "ull_llcp_internal.h"

#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER)
/* LLCP Remote Procedure FSM states */
enum {
	RP_PAST_STATE_IDLE,
	RP_PAST_STATE_WAIT_RX,
	RP_PAST_STATE_WAIT_NEXT_EVT,
#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
	RP_PAST_STATE_WAIT_RESOLVE_INTERFACE_AVAIL,
	RP_PAST_STATE_WAIT_RESOLVE_COMPLETE,
#endif /* CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY */
};

/* LLCP Remote Procedure PAST (receiver) FSM events */
enum {
	/* Procedure run */
	RP_PAST_EVT_RUN,

	/* IND received */
	RP_PAST_EVT_RX,

	/* RPA resolve completed */
	RP_PAST_EVT_RESOLVED,
};

#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
/* Active connection for RPA resolve */
static struct ll_conn *rp_past_resolve_conn;
#endif /* CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY */

static uint8_t rp_check_phy(struct ll_conn *conn, struct proc_ctx *ctx,
			    struct pdu_data *pdu)
{
	if (!phy_valid(pdu->llctrl.periodic_sync_ind.phy)) {
		/* zero, more than one or any rfu bit selected in either phy */
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

#if defined(CONFIG_BT_CTLR_PHY)
	const uint8_t phy = pdu->llctrl.periodic_sync_ind.phy;

	if (((phy & PHY_2M) && !IS_ENABLED(CONFIG_BT_CTLR_PHY_2M)) ||
	    ((phy & PHY_CODED) && !IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED))) {
		/* Unsupported phy selected */
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}
#endif /* CONFIG_BT_CTLR_PHY */

	return BT_HCI_ERR_SUCCESS;
}

static void rp_past_complete(struct ll_conn *conn, struct proc_ctx *ctx)
{
	llcp_rr_complete(conn);
	ctx->state = RP_PAST_STATE_IDLE;
}

static void rp_past_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_PAST_EVT_RUN:
		ctx->state = RP_PAST_STATE_WAIT_RX;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
static void rp_past_resolve_cb(void *param)
{
	uint8_t rl_idx = (uint8_t)(uint32_t)param;
	struct ll_conn *conn = rp_past_resolve_conn;
	struct proc_ctx *ctx;
	uint8_t id_addr_type;

	/* Release resolving interface */
	rp_past_resolve_conn = NULL;

	ctx = llcp_rr_peek(conn);
	if (!ctx) {
		/* No context - possibly due to connection termination or
		 * other cause of procedure completion.
		 */
		return;
	}

	if (ctx->state != RP_PAST_STATE_WAIT_RESOLVE_COMPLETE) {
		/* Wrong state - possibly due to connection termination or
		 * other cause of procedure completion.
		 */
		return;
	}

	/* If resolve failed then just continue in next event using the RPA */
	if (rl_idx != FILTER_IDX_NONE) {
		const bt_addr_t *id_addr;

		id_addr = ull_filter_lll_id_addr_get(rl_idx, &id_addr_type);
		if (id_addr) {
			memcpy(&ctx->data.periodic_sync.adv_addr, &id_addr->val, sizeof(bt_addr_t));

			ctx->data.periodic_sync.addr_type = id_addr_type;
			ctx->data.periodic_sync.addr_resolved = 1U;
		}
	}

	/* Let sync creation continue in next event */
	ctx->state = RP_PAST_STATE_WAIT_NEXT_EVT;
}

static bool rp_past_addr_resolve(struct ll_conn *conn, struct proc_ctx *ctx)
{
	bt_addr_t adv_addr;

	if (rp_past_resolve_conn) {
		/* Resolve interface busy */
		return false;
	}

	(void)memcpy(&adv_addr.val, ctx->data.periodic_sync.adv_addr, sizeof(bt_addr_t));
	rp_past_resolve_conn = conn;

	if (ull_filter_deferred_resolve(&adv_addr, rp_past_resolve_cb)) {
		/* Resolve initiated - wait for callback */
		return true;
	}

	/* Resolve interface busy (in ull_filter) */
	rp_past_resolve_conn = NULL;
	return false;
}

static void rp_past_st_wait_resolve_if(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				       void *param)
{
	switch (evt) {
	case RP_PAST_EVT_RUN:
		if (rp_past_addr_resolve(conn, ctx)) {
			ctx->state = RP_PAST_STATE_WAIT_RESOLVE_COMPLETE;
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}
#endif /* CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY */

static void rp_past_st_wait_rx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
			       void *param)
{
	struct pdu_data *pdu = (struct pdu_data *)param;

	switch (evt) {
	case RP_PAST_EVT_RX:
		llcp_pdu_decode_periodic_sync_ind(ctx, pdu);

		/* Check PHY */
		if (rp_check_phy(conn, ctx, pdu) != BT_HCI_ERR_SUCCESS) {
			/* Invalid PHY - ignore and silently complete */
			rp_past_complete(conn, ctx);
			return;
		}

		ctx->data.periodic_sync.addr_resolved = 0U;

		if (ctx->data.periodic_sync.addr_type == BT_ADDR_LE_RANDOM) {
			/* TODO: For efficiency, check if address resolve is needed */
#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
			if (ull_filter_lll_rl_enabled()) {
				if (rp_past_addr_resolve(conn, ctx)) {
					ctx->state = RP_PAST_STATE_WAIT_RESOLVE_COMPLETE;
					return;
				}
				ctx->state = RP_PAST_STATE_WAIT_RESOLVE_INTERFACE_AVAIL;
				return;
			}
#else
			/* TODO: Not implemented - use RPA */
#endif /* CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY */
		}

		if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
			/* If sync_conn_event_count is this connection event,
			 * we have to wait for drift correction for this event to be applied -
			 * continue processing in the next conn event
			 */
			if (conn->lll.role == BT_HCI_ROLE_PERIPHERAL &&
			ctx->data.periodic_sync.sync_conn_event_count ==
			ull_conn_event_counter(conn)) {
				ctx->state = RP_PAST_STATE_WAIT_NEXT_EVT;
				return;
			}
		}

		/* Hand over to ULL */
		ull_sync_transfer_received(conn,
					   ctx->data.periodic_sync.id,
					   &ctx->data.periodic_sync.sync_info,
					   ctx->data.periodic_sync.conn_event_count,
					   ctx->data.periodic_sync.last_pa_event_counter,
					   ctx->data.periodic_sync.sid,
					   ctx->data.periodic_sync.addr_type,
					   ctx->data.periodic_sync.sca,
					   ctx->data.periodic_sync.phy,
					   ctx->data.periodic_sync.adv_addr,
					   ctx->data.periodic_sync.sync_conn_event_count,
					   ctx->data.periodic_sync.addr_resolved);
		rp_past_complete(conn, ctx);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_past_st_wait_next_evt(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				     void *param)
{
	switch (evt) {
	case RP_PAST_EVT_RUN:
#if defined(CONFIG_BT_PERIPHERAL)
		/* If sync_conn_event_count is this connection event,
		 * we have to wait for drift correction for this event to be applied -
		 * continue processing in the next conn event
		 */
		if (conn->lll.role == BT_HCI_ROLE_PERIPHERAL &&
		    ctx->data.periodic_sync.sync_conn_event_count == ull_conn_event_counter(conn)) {
			return;
		}
#endif /* CONFIG_BT_PERIPHERAL */

		/* Hand over to ULL */
		ull_sync_transfer_received(conn,
					   ctx->data.periodic_sync.id,
					   &ctx->data.periodic_sync.sync_info,
					   ctx->data.periodic_sync.conn_event_count,
					   ctx->data.periodic_sync.last_pa_event_counter,
					   ctx->data.periodic_sync.sid,
					   ctx->data.periodic_sync.addr_type,
					   ctx->data.periodic_sync.sca,
					   ctx->data.periodic_sync.phy,
					   ctx->data.periodic_sync.adv_addr,
					   ctx->data.periodic_sync.sync_conn_event_count,
					   ctx->data.periodic_sync.addr_resolved);
		rp_past_complete(conn, ctx);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_past_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	switch (ctx->state) {
	case RP_PAST_STATE_IDLE:
		rp_past_st_idle(conn, ctx, evt, param);
		break;
	case RP_PAST_STATE_WAIT_RX:
		rp_past_st_wait_rx(conn, ctx, evt, param);
		break;
	case RP_PAST_STATE_WAIT_NEXT_EVT:
		rp_past_st_wait_next_evt(conn, ctx, evt, param);
		break;
#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
	case RP_PAST_STATE_WAIT_RESOLVE_INTERFACE_AVAIL:
		rp_past_st_wait_resolve_if(conn, ctx, evt, param);
		break;
	case RP_PAST_STATE_WAIT_RESOLVE_COMPLETE:
		/* Waiting for callback - do nothing */
		break;
#endif /* CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY */
	default:
		/* Unknown state */
		LL_ASSERT(0);
		break;
	}
}

void llcp_rp_past_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	rp_past_execute_fsm(conn, ctx, RP_PAST_EVT_RX, rx->pdu);
}

void llcp_rp_past_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_past_execute_fsm(conn, ctx, RP_PAST_EVT_RUN, param);
}

#endif /* CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER */

#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER)
/* LLCP Local Procedure FSM states */
enum {
	LP_PAST_STATE_IDLE,
	LP_PAST_STATE_WAIT_TX_REQ,
	LP_PAST_STATE_WAIT_OFFSET_CALC,
	LP_PAST_STATE_WAIT_TX_ACK,
	LP_PAST_STATE_WAIT_EVT_DONE,
};

/* LLCP Local Procedure PAST (sender) FSM events */
enum {
	/* Procedure run */
	LP_PAST_EVT_RUN,

	/* Offset calculation reply received */
	LP_PAST_EVT_OFFSET_CALC_REPLY,

	/* RX received in connection event */
	LP_PAST_EVT_RX_RECEIVED,

	/* RX not received in connection event */
	LP_PAST_EVT_NO_RX_RECEIVED,

	/* Ack received */
	LP_PAST_EVT_ACK,
};

static void lp_past_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param);

static void lp_past_complete(struct ll_conn *conn, struct proc_ctx *ctx)
{
	llcp_lr_complete(conn);
	ctx->state = LP_PAST_STATE_IDLE;
}

static void lp_past_tx(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		if (conn->lll.role == BT_HCI_ROLE_PERIPHERAL) {
			uint32_t offset_us;

			/* Correct offset can be calculated now that we know the event start time */
			offset_us = ctx->data.periodic_sync.offset_us -
				ctx->data.periodic_sync.conn_start_to_actual_us;

			llcp_pdu_fill_sync_info_offset(&ctx->data.periodic_sync.sync_info,
						       offset_us);
		}
	}

	/* Allocate tx node */
	tx = llcp_tx_alloc(conn, ctx);
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	llcp_pdu_encode_periodic_sync_ind(ctx, pdu);

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);

	/* Wait for TX Ack */
	ctx->state = LP_PAST_STATE_WAIT_TX_ACK;
	ctx->node_ref.tx_ack = tx;
}

void llcp_lp_past_offset_calc_reply(struct ll_conn *conn, struct proc_ctx *ctx)
{
	lp_past_execute_fsm(conn, ctx, LP_PAST_EVT_OFFSET_CALC_REPLY, NULL);
}

static void lp_past_offset_calc_req(struct ll_conn *conn, struct proc_ctx *ctx,
				    uint8_t evt, void *param)
{
	if (llcp_lr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = LP_PAST_STATE_WAIT_TX_REQ;
	} else {
		/* Call ULL and wait for reply */
		ctx->state = LP_PAST_STATE_WAIT_OFFSET_CALC;
		ull_conn_past_sender_offset_request(conn);
	}
}

static void lp_past_st_wait_tx_req(struct ll_conn *conn, struct proc_ctx *ctx,
				   uint8_t evt, void *param)
{
	switch (evt) {
	case LP_PAST_EVT_RUN:
		lp_past_offset_calc_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_past_st_wait_offset_calc(struct ll_conn *conn, struct proc_ctx *ctx,
					uint8_t evt, void *param)
{
	switch (evt) {
	case LP_PAST_EVT_OFFSET_CALC_REPLY:
		if (ull_ref_get(&conn->ull)) {
			/* Connection event still ongoing, wait for done */
			ctx->state = LP_PAST_STATE_WAIT_EVT_DONE;
		} else if (ctx->data.periodic_sync.conn_evt_trx) {
			/* Connection event done with successful rx from peer */
			lp_past_tx(conn, ctx);
		} else {
			/* Reset state and try again in next connection event */
			ctx->state = LP_PAST_STATE_WAIT_TX_REQ;
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

void llcp_lp_past_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, struct node_tx *tx)
{
	lp_past_execute_fsm(conn, ctx, LP_PAST_EVT_ACK, tx->pdu);
}

void llcp_lp_past_conn_evt_done(struct ll_conn *conn, struct proc_ctx *ctx)
{
	if (ctx->data.periodic_sync.conn_evt_trx != 0) {
		lp_past_execute_fsm(conn, ctx, LP_PAST_EVT_RX_RECEIVED, NULL);
	} else {
		lp_past_execute_fsm(conn, ctx, LP_PAST_EVT_NO_RX_RECEIVED, NULL);
	}
}

static void lp_past_state_wait_evt_done(struct ll_conn *conn, struct proc_ctx *ctx,
					uint8_t evt, void *param)
{
	/* Offset calculation has to be done in a connection event where a packet
	 * was received from the peer.
	 * From Core Spec v5.4, Vol 6, Part B, Section 2.4.2.27:
	 * syncConnEventCount shall be set to the connection event counter for the
	 * connection event that the sending device used in determining the contents
	 * of this PDU. This shall be a connection event where the sending device
	 * received a packet from the device it will send the LL_PERIODIC_SYNC_-
	 * IND PDU to and, if the sending device is the Peripheral on the piconet con-
	 * taining those two devices, it used the received packet to synchronize its
	 * anchor point
	 */
	switch (evt) {
	case LP_PAST_EVT_RX_RECEIVED:
		lp_past_tx(conn, ctx);
		break;
	case LP_PAST_EVT_NO_RX_RECEIVED:
		/* Try again in next connection event */
		ctx->state = LP_PAST_STATE_WAIT_TX_REQ;
		break;
	}
}

static void lp_past_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_PAST_EVT_RUN:
		/* In case feature exchange completed after past was enqueued
		 * peer PAST receiver support should be confirmed
		 */
		if (feature_peer_periodic_sync_recv(conn)) {
			lp_past_offset_calc_req(conn, ctx, evt, param);
		} else {
			/* Peer doesn't support PAST Receiver; HCI gives us no way to
			 * indicate this to the host so just silently complete the procedure
			 */
			lp_past_complete(conn, ctx);
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_past_st_wait_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				   void *param)
{
	switch (evt) {
	case LP_PAST_EVT_ACK:
		/* Received Ack - All done now */
		lp_past_complete(conn, ctx);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_past_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	switch (ctx->state) {
	case LP_PAST_STATE_IDLE:
		lp_past_st_idle(conn, ctx, evt, param);
		break;
	case LP_PAST_STATE_WAIT_TX_REQ:
		lp_past_st_wait_tx_req(conn, ctx, evt, param);
		break;
	case LP_PAST_STATE_WAIT_OFFSET_CALC:
		lp_past_st_wait_offset_calc(conn, ctx, evt, param);
		break;
	case LP_PAST_STATE_WAIT_TX_ACK:
		lp_past_st_wait_tx_ack(conn, ctx, evt, param);
		break;
	case LP_PAST_STATE_WAIT_EVT_DONE:
		lp_past_state_wait_evt_done(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
		break;
	}
}

void llcp_lp_past_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	lp_past_execute_fsm(conn, ctx, LP_PAST_EVT_RUN, param);
}
#endif /* CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER */
