/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
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

#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_llcp_internal.h"
#include "ull_conn_internal.h"

#include <soc.h>
#include "hal/debug.h"

/* Hardcoded instant delta +6 */
#define CHMU_INSTANT_DELTA 6U

/* LLCP Local Procedure Channel Map Update FSM states */
enum {
	LP_CHMU_STATE_IDLE = LLCP_STATE_IDLE,
	LP_CHMU_STATE_WAIT_TX_CHAN_MAP_IND,
	LP_CHMU_STATE_WAIT_INSTANT,
};

/* LLCP Local Procedure Channel Map Update FSM events */
enum {
	/* Procedure run */
	LP_CHMU_EVT_RUN,
};

/* LLCP Remote Procedure Channel Map Update FSM states */
enum {
	RP_CHMU_STATE_IDLE = LLCP_STATE_IDLE,
	RP_CHMU_STATE_WAIT_RX_CHAN_MAP_IND,
	RP_CHMU_STATE_WAIT_INSTANT,
};

/* LLCP Remote Procedure Channel Map Update FSM events */
enum {
	/* Procedure run */
	RP_CHMU_EVT_RUN,

	/* Indication received */
	RP_CHMU_EVT_RX_CHAN_MAP_IND,
};

#if defined(CONFIG_BT_CENTRAL)
/*
 * LLCP Local Procedure Channel Map Update FSM
 */
static void lp_chmu_tx(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = llcp_tx_alloc(conn, ctx);
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	llcp_pdu_encode_chan_map_update_ind(ctx, pdu);

	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);
}

static void lp_chmu_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	ull_conn_chan_map_set(conn, ctx->data.chmu.chm);
	llcp_lr_complete(conn);
	ctx->state = LP_CHMU_STATE_IDLE;
}

static void lp_chmu_send_channel_map_update_ind(struct ll_conn *conn, struct proc_ctx *ctx,
						uint8_t evt, void *param)
{
	if (llcp_lr_ispaused(conn) || llcp_rr_get_collision(conn) ||
	    !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = LP_CHMU_STATE_WAIT_TX_CHAN_MAP_IND;
	} else {
		llcp_rr_set_incompat(conn, INCOMPAT_RESOLVABLE);

		ctx->data.chmu.instant = ull_conn_event_counter(conn) + CHMU_INSTANT_DELTA;

		lp_chmu_tx(conn, ctx);

		ctx->state = LP_CHMU_STATE_WAIT_INSTANT;
	}
}

static void lp_chmu_st_wait_tx_chan_map_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					    void *param)
{
	switch (evt) {
	case LP_CHMU_EVT_RUN:
		lp_chmu_send_channel_map_update_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_chmu_check_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				  void *param)
{
	uint16_t event_counter = ull_conn_event_counter(conn);

	if (is_instant_reached_or_passed(ctx->data.chmu.instant, event_counter)) {
		llcp_rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
		lp_chmu_complete(conn, ctx, evt, param);
	}
}

static void lp_chmu_st_wait_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				    void *param)
{
	switch (evt) {
	case LP_CHMU_EVT_RUN:
		lp_chmu_check_instant(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_chmu_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	switch (ctx->state) {
	case LP_CHMU_STATE_IDLE:
		/* Empty/fallthrough on purpose as idle state handling is equivalent to
		 * 'wait for tx state' - simply to attempt TX'ing chan map ind
		 */
	case LP_CHMU_STATE_WAIT_TX_CHAN_MAP_IND:
		lp_chmu_st_wait_tx_chan_map_ind(conn, ctx, evt, param);
		break;
	case LP_CHMU_STATE_WAIT_INSTANT:
		lp_chmu_st_wait_instant(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void llcp_lp_chmu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *)rx->pdu;

	switch (pdu->llctrl.opcode) {
	default:
		/* Invalid behaviour */
		/* Invalid PDU received so terminate connection */
		conn->llcp_terminate.reason_final = BT_HCI_ERR_LMP_PDU_NOT_ALLOWED;
		llcp_lr_complete(conn);
		ctx->state = LP_CHMU_STATE_IDLE;
		break;
	}
}

void llcp_lp_chmu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	lp_chmu_execute_fsm(conn, ctx, LP_CHMU_EVT_RUN, param);
}

bool llcp_lp_chmu_awaiting_instant(struct proc_ctx *ctx)
{
	return (ctx->state == LP_CHMU_STATE_WAIT_INSTANT);
}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
/*
 * LLCP Remote Procedure Channel Map Update FSM
 */
static void rp_chmu_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	ull_conn_chan_map_set(conn, ctx->data.chmu.chm);
	llcp_rr_complete(conn);
	ctx->state = RP_CHMU_STATE_IDLE;
}

static void rp_chmu_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CHMU_EVT_RUN:
		ctx->state = RP_CHMU_STATE_WAIT_RX_CHAN_MAP_IND;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_chmu_st_wait_rx_channel_map_update_ind(struct ll_conn *conn, struct proc_ctx *ctx,
						      uint8_t evt, void *param)
{
	switch (evt) {
	case RP_CHMU_EVT_RX_CHAN_MAP_IND:
		llcp_pdu_decode_chan_map_update_ind(ctx, param);
		if (is_instant_not_passed(ctx->data.chmu.instant,
					  ull_conn_event_counter(conn))) {

			ctx->state = RP_CHMU_STATE_WAIT_INSTANT;
		} else {
			conn->llcp_terminate.reason_final = BT_HCI_ERR_INSTANT_PASSED;
			llcp_rr_complete(conn);
			ctx->state = RP_CHMU_STATE_IDLE;
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_chmu_check_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				  void *param)
{
	uint16_t event_counter = ull_conn_event_counter(conn);

	if (((event_counter - ctx->data.chmu.instant) & 0xFFFF) <= 0x7FFF) {
		rp_chmu_complete(conn, ctx, evt, param);
	}
}

static void rp_chmu_st_wait_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				    void *param)
{
	switch (evt) {
	case RP_CHMU_EVT_RUN:
		rp_chmu_check_instant(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_chmu_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	switch (ctx->state) {
	case RP_CHMU_STATE_IDLE:
		rp_chmu_st_idle(conn, ctx, evt, param);
		break;
	case RP_CHMU_STATE_WAIT_RX_CHAN_MAP_IND:
		rp_chmu_st_wait_rx_channel_map_update_ind(conn, ctx, evt, param);
		break;
	case RP_CHMU_STATE_WAIT_INSTANT:
		rp_chmu_st_wait_instant(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void llcp_rp_chmu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *)rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND:
		rp_chmu_execute_fsm(conn, ctx, RP_CHMU_EVT_RX_CHAN_MAP_IND, pdu);
		break;
	default:
		/* Invalid behaviour */
		/* Invalid PDU received so terminate connection */
		conn->llcp_terminate.reason_final = BT_HCI_ERR_LMP_PDU_NOT_ALLOWED;
		llcp_rr_complete(conn);
		ctx->state = RP_CHMU_STATE_IDLE;
		break;
	}
}

void llcp_rp_chmu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_chmu_execute_fsm(conn, ctx, RP_CHMU_EVT_RUN, param);
}

bool llcp_rp_chmu_awaiting_instant(struct proc_ctx *ctx)
{
	return (ctx->state == RP_CHMU_STATE_WAIT_INSTANT);
}
#endif /* CONFIG_BT_PERIPHERAL */
