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
#include "ll_settings.h"

#include "lll.h"
#include "ll_feat.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"
#include "ull_conn_iso_internal.h"

#include "ull_conn_types.h"
#include "ull_internal.h"
#include "ull_llcp.h"
#include "ull_llcp_features.h"
#include "ull_llcp_internal.h"
#include "ull_conn_internal.h"

#include <soc.h>
#include "hal/debug.h"

/* LLCP Local Procedure PHY Update FSM states */
enum {
	LP_PU_STATE_IDLE,
	LP_PU_STATE_WAIT_TX_PHY_REQ,
	LP_PU_STATE_WAIT_TX_ACK_PHY_REQ,
	LP_PU_STATE_WAIT_RX_PHY_RSP,
	LP_PU_STATE_WAIT_TX_PHY_UPDATE_IND,
	LP_PU_STATE_WAIT_TX_ACK_PHY_UPDATE_IND,
	LP_PU_STATE_WAIT_RX_PHY_UPDATE_IND,
	LP_PU_STATE_WAIT_NTF_AVAIL,
	LP_PU_STATE_WAIT_INSTANT,
	LP_PU_STATE_WAIT_INSTANT_ON_AIR,
};

/* LLCP Local Procedure PHY Update FSM events */
enum {
	/* Procedure run */
	LP_PU_EVT_RUN,

	/* Response received */
	LP_PU_EVT_PHY_RSP,

	/* Indication received */
	LP_PU_EVT_PHY_UPDATE_IND,

	/* Ack received */
	LP_PU_EVT_ACK,

	/* Ready to notify host */
	LP_PU_EVT_NTF,

	/* Reject response received */
	LP_PU_EVT_REJECT,

	/* Unknown response received */
	LP_PU_EVT_UNKNOWN,
};

/* LLCP Remote Procedure PHY Update FSM states */
enum {
	RP_PU_STATE_IDLE,
	RP_PU_STATE_WAIT_RX_PHY_REQ,
	RP_PU_STATE_WAIT_TX_PHY_RSP,
	RP_PU_STATE_WAIT_TX_ACK_PHY_RSP,
	RP_PU_STATE_WAIT_TX_PHY_UPDATE_IND,
	RP_PU_STATE_WAIT_TX_ACK_PHY_UPDATE_IND,
	RP_PU_STATE_WAIT_RX_PHY_UPDATE_IND,
	RP_PU_STATE_WAIT_NTF_AVAIL,
	RP_PU_STATE_WAIT_INSTANT,
	RP_PU_STATE_WAIT_INSTANT_ON_AIR,
};

/* LLCP Remote Procedure PHY Update FSM events */
enum {
	/* Procedure run */
	RP_PU_EVT_RUN,

	/* Request received */
	RP_PU_EVT_PHY_REQ,

	/* Ack received */
	RP_PU_EVT_ACK,

	/* Indication received */
	RP_PU_EVT_PHY_UPDATE_IND,

	/* Ready to notify host */
	RP_PU_EVT_NTF,
};

/* Hardcoded instant delta +6 */
#define PHY_UPDATE_INSTANT_DELTA 6

#if defined(CONFIG_BT_CENTRAL)
/* PHY preference order*/
#define PHY_PREF_1 PHY_2M
#define PHY_PREF_2 PHY_1M
#define PHY_PREF_3 PHY_CODED

static inline uint8_t pu_select_phy(uint8_t phys)
{
	/* select only one phy, select preferred */
	if (phys & PHY_PREF_1) {
		return PHY_PREF_1;
	} else if (phys & PHY_PREF_2) {
		return PHY_PREF_2;
	} else if (phys & PHY_PREF_3) {
		return PHY_PREF_3;
	} else {
		return 0U;
	}
}

static void pu_prep_update_ind(struct ll_conn *conn, struct proc_ctx *ctx)
{
	ctx->data.pu.tx = pu_select_phy(ctx->data.pu.tx);
	ctx->data.pu.rx = pu_select_phy(ctx->data.pu.rx);

	if (ctx->data.pu.tx != conn->lll.phy_tx) {
		ctx->data.pu.c_to_p_phy = ctx->data.pu.tx;
	} else {
		ctx->data.pu.c_to_p_phy = 0U;
	}
	if (ctx->data.pu.rx != conn->lll.phy_rx) {
		ctx->data.pu.p_to_c_phy = ctx->data.pu.rx;
	} else {
		ctx->data.pu.p_to_c_phy = 0U;
	}
}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
static uint8_t pu_select_phy_timing_restrict(struct ll_conn *conn, uint8_t phy_tx)
{
	/* select the probable PHY with longest Tx time, which
	 * will be restricted to fit current
	 * connEffectiveMaxTxTime.
	 */
	/* Note - entry 0 in table is unused, so 0 on purpose */
	uint8_t phy_tx_time[8] = { 0,	      PHY_1M,	 PHY_2M,    PHY_1M,
				   PHY_CODED, PHY_CODED, PHY_CODED, PHY_CODED };
	struct lll_conn *lll = &conn->lll;
	const uint8_t phys = phy_tx | lll->phy_tx;

	return phy_tx_time[phys];
}
#endif /* CONFIG_BT_PERIPHERAL */

static void pu_set_timing_restrict(struct ll_conn *conn, uint8_t phy_tx)
{
	struct lll_conn *lll = &conn->lll;

	lll->phy_tx_time = phy_tx;
}

static void pu_reset_timing_restrict(struct ll_conn *conn)
{
	pu_set_timing_restrict(conn, conn->lll.phy_tx);
}

#if defined(CONFIG_BT_PERIPHERAL)
static inline bool phy_valid(uint8_t phy)
{
	/* This is equivalent to:
	 * maximum one bit set, and no bit set is rfu's
	 */
	return (phy < 5 && phy != 3);
}

static uint8_t pu_check_update_ind(struct ll_conn *conn, struct proc_ctx *ctx)
{
	uint8_t ret = 0;

	/* Check if either phy selected is invalid */
	if (!phy_valid(ctx->data.pu.c_to_p_phy) || !phy_valid(ctx->data.pu.p_to_c_phy)) {
		/* more than one or any rfu bit selected in either phy */
		ctx->data.pu.error = BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		ret = 1;
	}

	/* Both tx and rx PHY unchanged */
	if (!((ctx->data.pu.c_to_p_phy | ctx->data.pu.p_to_c_phy) & 0x07)) {
		/* if no phy changes, quit procedure, and possibly signal host */
		ctx->data.pu.error = BT_HCI_ERR_SUCCESS;
		ret = 1;
	} else {
		/* if instant already passed, quit procedure with error */
		if (is_instant_reached_or_passed(ctx->data.pu.instant,
						 ull_conn_event_counter(conn))) {
			ctx->data.pu.error = BT_HCI_ERR_INSTANT_PASSED;
			ret = 1;
		}
	}
	return ret;
}
#endif /* CONFIG_BT_PERIPHERAL */

static uint8_t pu_apply_phy_update(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct lll_conn *lll = &conn->lll;
	uint8_t phy_bitmask = PHY_1M;
	const uint8_t old_tx = lll->phy_tx;
	const uint8_t old_rx = lll->phy_rx;

#if defined(CONFIG_BT_CTLR_PHY_2M)
	phy_bitmask |= PHY_2M;
#endif
#if defined(CONFIG_BT_CTLR_PHY_CODED)
	phy_bitmask |= PHY_CODED;
#endif
	const uint8_t p_to_c_phy = ctx->data.pu.p_to_c_phy & phy_bitmask;
	const uint8_t c_to_p_phy = ctx->data.pu.c_to_p_phy & phy_bitmask;

	if (0) {
#if defined(CONFIG_BT_PERIPHERAL)
	} else if (lll->role == BT_HCI_ROLE_PERIPHERAL) {
		if (p_to_c_phy) {
			lll->phy_tx = p_to_c_phy;
		}
		if (c_to_p_phy) {
			lll->phy_rx = c_to_p_phy;
		}
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CENTRAL)
	} else if (lll->role == BT_HCI_ROLE_CENTRAL) {
		if (p_to_c_phy) {
			lll->phy_rx = p_to_c_phy;
		}
		if (c_to_p_phy) {
			lll->phy_tx = c_to_p_phy;
		}
#endif /* CONFIG_BT_CENTRAL */
	}

	return ((old_tx != lll->phy_tx) || (old_rx != lll->phy_rx));
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static uint16_t pu_calc_eff_time(uint8_t max_octets, uint8_t phy, uint16_t default_time)
{
	uint16_t payload_time = PDU_DC_MAX_US(max_octets, phy);
	uint16_t eff_time;

	eff_time = MAX(PDU_DC_PAYLOAD_TIME_MIN, payload_time);
	eff_time = MIN(eff_time, default_time);
#if defined(CONFIG_BT_CTLR_PHY_CODED)
	eff_time = MAX(eff_time, PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN, phy));
#endif

	return eff_time;
}

static uint8_t pu_update_eff_times(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct lll_conn *lll = &conn->lll;
	uint16_t eff_tx_time = lll->dle.eff.max_tx_time;
	uint16_t eff_rx_time = lll->dle.eff.max_rx_time;
	uint16_t max_rx_time, max_tx_time;

	ull_dle_max_time_get(conn, &max_rx_time, &max_tx_time);

	if ((ctx->data.pu.p_to_c_phy && (lll->role == BT_HCI_ROLE_PERIPHERAL)) ||
	    (ctx->data.pu.c_to_p_phy && (lll->role == BT_HCI_ROLE_CENTRAL))) {
		eff_tx_time =
			pu_calc_eff_time(lll->dle.eff.max_tx_octets, lll->phy_tx, max_tx_time);
	}
	if ((ctx->data.pu.p_to_c_phy && (lll->role == BT_HCI_ROLE_CENTRAL)) ||
	    (ctx->data.pu.c_to_p_phy && (lll->role == BT_HCI_ROLE_PERIPHERAL))) {
		eff_rx_time =
			pu_calc_eff_time(lll->dle.eff.max_rx_octets, lll->phy_rx, max_rx_time);
	}

	if ((eff_tx_time > lll->dle.eff.max_tx_time) ||
	    (lll->dle.eff.max_tx_time > max_tx_time) ||
	    (eff_rx_time > lll->dle.eff.max_rx_time) ||
	    (lll->dle.eff.max_rx_time > max_rx_time)) {
		lll->dle.eff.max_tx_time = eff_tx_time;
		lll->dle.eff.max_rx_time = eff_rx_time;
#if defined(CONFIG_BT_CTLR_SLOT_RESERVATION_UPDATE)
		lll->evt_len_upd = 1U;
#endif /* CONFIG_BT_CTLR_SLOT_RESERVATION_UPDATE */
		return 1U;
	}

	return 0U;
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

static inline void pu_set_preferred_phys(struct ll_conn *conn, struct proc_ctx *ctx)
{
	conn->phy_pref_rx = ctx->data.pu.rx;
	conn->phy_pref_tx = ctx->data.pu.tx;

	/*
	 * Note: Since 'flags' indicate local coded phy preference (S2 or S8) and
	 * this is not negotiated with the peer, it is simply reconfigured in conn->lll when
	 * the update is initiated, and takes effect whenever the coded phy is in use.
	 */
	conn->lll.phy_flags = ctx->data.pu.flags;
}

static inline void pu_combine_phys(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t tx,
				   uint8_t rx)
{
	/* Combine requested phys with locally preferred phys */
	ctx->data.pu.rx &= rx;
	ctx->data.pu.tx &= tx;
	/* If either tx or rx is 'no change' at this point we force both to no change to
	 * comply with the spec
	 *	Spec. BT5.2 Vol6, Part B, section 5.1.10:
	 *	The remainder of this section shall apply irrespective of which device initiated
	 *	the procedure.
	 *
	 *	Irrespective of the above rules, the central may leave both directions
	 *	unchanged. If the periph specified a single PHY in both the TX_PHYS and
	 *	RX_PHYS fields and both fields are the same, the central shall either select
	 *	the PHY specified by the periph for both directions or shall leave both directions
	 *	unchanged.
	 */
	if (conn->lll.role == BT_HCI_ROLE_CENTRAL && (!ctx->data.pu.rx || !ctx->data.pu.tx)) {
		ctx->data.pu.tx = 0;
		ctx->data.pu.rx = 0;
	}
}

#if defined(CONFIG_BT_CENTRAL)
static void pu_prepare_instant(struct ll_conn *conn, struct proc_ctx *ctx)
{
	/* Set instance only in case there is actual PHY change. Otherwise the instant should be
	 * set to 0.
	 */
	if (ctx->data.pu.c_to_p_phy != 0 || ctx->data.pu.p_to_c_phy != 0) {
		ctx->data.pu.instant = ull_conn_event_counter(conn) + conn->lll.latency +
			PHY_UPDATE_INSTANT_DELTA;
	} else {
		ctx->data.pu.instant = 0;
	}
}
#endif /* CONFIG_BT_CENTRAL */

/*
 * LLCP Local Procedure PHY Update FSM
 */

static void lp_pu_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	LL_ASSERT(ctx->node_ref.tx);

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	if (!((ctx->tx_opcode == PDU_DATA_LLCTRL_TYPE_PHY_REQ) &&
	    (conn->lll.role == BT_HCI_ROLE_CENTRAL))) {
		if (!llcp_ntf_alloc_is_available()) {
			/* No NTF nodes avail, so we need to hold off TX */
			ctx->state = LP_PU_STATE_WAIT_NTF_AVAIL;
			return;
		}
		ctx->data.pu.ntf_dle_node = llcp_ntf_alloc();
		LL_ASSERT(ctx->data.pu.ntf_dle_node);
	}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

	tx = ctx->node_ref.tx;
	ctx->node_ref.tx = NULL;
	ctx->node_ref.tx_ack = tx;
	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (ctx->tx_opcode) {
	case PDU_DATA_LLCTRL_TYPE_PHY_REQ:
		pu_set_preferred_phys(conn, ctx);
		llcp_pdu_encode_phy_req(ctx, pdu);
		llcp_tx_pause_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_PHY_UPDATE);
		ctx->state = LP_PU_STATE_WAIT_TX_ACK_PHY_REQ;
		break;
#if defined(CONFIG_BT_CENTRAL)
	case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
		pu_prep_update_ind(conn, ctx);
		pu_prepare_instant(conn, ctx);
		llcp_pdu_encode_phy_update_ind(ctx, pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		ctx->state = LP_PU_STATE_WAIT_TX_ACK_PHY_UPDATE_IND;
		break;
#endif /* CONFIG_BT_CENTRAL */
	default:
		LL_ASSERT(0);
	}

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);

	/* Restart procedure response timeout timer */
	llcp_lr_prt_restart(conn);
}

static void pu_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct node_rx_pu *pdu;

	/* Piggy-back on stored RX node */
	ntf = ctx->node_ref.rx;
	ctx->node_ref.rx = NULL;
	LL_ASSERT(ntf);

	if (ctx->data.pu.ntf_pu) {
		LL_ASSERT(ntf->hdr.type == NODE_RX_TYPE_RETAIN);
		ntf->hdr.type = NODE_RX_TYPE_PHY_UPDATE;
		ntf->hdr.handle = conn->lll.handle;
		pdu = (struct node_rx_pu *)ntf->pdu;

		pdu->status = ctx->data.pu.error;
		pdu->rx = conn->lll.phy_rx;
		pdu->tx = conn->lll.phy_tx;
	} else {
		ntf->hdr.type = NODE_RX_TYPE_RELEASE;
	}

	/* Enqueue notification towards LL */
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	/* only 'put' as the 'sched' is handled when handling DLE ntf */
	ll_rx_put(ntf->hdr.link, ntf);
#else
	ll_rx_put_sched(ntf->hdr.link, ntf);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

	ctx->data.pu.ntf_pu = 0;
	ctx->node_ref.rx = NULL;
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static void pu_dle_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;

	/* Retrieve DLE ntf node */
	ntf = ctx->data.pu.ntf_dle_node;

	if (!ctx->data.pu.ntf_dle) {
		if (!ntf) {
			/* If no DLE ntf was pre-allocated there is nothing more to do */
			/* This will happen in case of a completion on UNKNOWN_RSP to PHY_REQ
			 * in Central case.
			 */
			return;
		}
		/* Signal to release pre-allocated node in case there is no DLE ntf */
		ntf->hdr.type = NODE_RX_TYPE_RELEASE;
	} else {
		LL_ASSERT(ntf);

		ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
		ntf->hdr.handle = conn->lll.handle;
		pdu = (struct pdu_data *)ntf->pdu;

		llcp_ntf_encode_length_change(conn, pdu);
	}

	/* Enqueue notification towards LL */
	ll_rx_put_sched(ntf->hdr.link, ntf);

	ctx->data.pu.ntf_dle = 0;
	ctx->data.pu.ntf_dle_node = NULL;
}
#endif

static void lp_pu_complete_finalize(struct ll_conn *conn, struct proc_ctx *ctx)
{
	llcp_lr_complete(conn);
	llcp_rr_set_paused_cmd(conn, PROC_NONE);
	ctx->state = LP_PU_STATE_IDLE;
}

static void lp_pu_tx_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	pu_ntf(conn, ctx);
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	pu_dle_ntf(conn, ctx);
#endif
	lp_pu_complete_finalize(conn, ctx);
}

static void lp_pu_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	pu_reset_timing_restrict(conn);

	/* Postpone procedure completion (and possible NTF generation) to actual 'air instant'
	 * Since LLCP STM is driven from LLL prepare this actually happens BEFORE instant
	 * and thus NTFs are generated and propagated up prior to actual instant on air.
	 * Instead postpone completion/NTF to the beginning of RX handling
	 */
	ctx->state = LP_PU_STATE_WAIT_INSTANT_ON_AIR;
}

static void lp_pu_send_phy_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (llcp_lr_ispaused(conn) || llcp_rr_get_collision(conn) ||
	    !llcp_tx_alloc_peek(conn, ctx) ||
	    (llcp_rr_get_paused_cmd(conn) == PROC_PHY_UPDATE)) {
		ctx->state = LP_PU_STATE_WAIT_TX_PHY_REQ;
	} else {
		llcp_rr_set_incompat(conn, INCOMPAT_RESOLVABLE);
		llcp_rr_set_paused_cmd(conn, PROC_CTE_REQ);
		ctx->tx_opcode = PDU_DATA_LLCTRL_TYPE_PHY_REQ;

		/* Allocate TX node */
		ctx->node_ref.tx = llcp_tx_alloc(conn, ctx);
		lp_pu_tx(conn, ctx, evt, param);
	}
}

#if defined(CONFIG_BT_CENTRAL)
static void lp_pu_send_phy_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				      void *param)
{
	if (llcp_lr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx)) {
		ctx->state = LP_PU_STATE_WAIT_TX_PHY_UPDATE_IND;
	} else {
		ctx->tx_opcode = PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND;

		/* Allocate TX node */
		ctx->node_ref.tx = llcp_tx_alloc(conn, ctx);
		lp_pu_tx(conn, ctx, evt, param);
	}
}
#endif /* CONFIG_BT_CENTRAL */

static void lp_pu_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_PU_EVT_RUN:
		lp_pu_send_phy_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_pu_st_wait_tx_phy_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				     void *param)
{
	switch (evt) {
	case LP_PU_EVT_RUN:
		lp_pu_send_phy_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

#if defined(CONFIG_BT_CENTRAL)
static void lp_pu_st_wait_rx_phy_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				     void *param)
{
	switch (evt) {
	case LP_PU_EVT_PHY_RSP:
		llcp_rr_set_incompat(conn, INCOMPAT_RESERVED);
		/* 'Prefer' the phys from the REQ */
		uint8_t tx_pref = ctx->data.pu.tx;
		uint8_t rx_pref = ctx->data.pu.rx;

		llcp_pdu_decode_phy_rsp(ctx, (struct pdu_data *)param);
		/* Pause data tx */
		llcp_tx_pause_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_PHY_UPDATE);
		/* Combine with the 'Preferred' phys */
		pu_combine_phys(conn, ctx, tx_pref, rx_pref);

		/* Mark RX node to NOT release */
		llcp_rx_node_retain(ctx);

		lp_pu_send_phy_update_ind(conn, ctx, evt, param);
		break;
	case LP_PU_EVT_UNKNOWN:
		llcp_rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
		/* Unsupported in peer, so disable locally for this connection
		 * Peer does not accept PHY UPDATE, so disable non 1M phys on current connection
		 */
		feature_unmask_features(conn, LL_FEAT_BIT_PHY_2M | LL_FEAT_BIT_PHY_CODED);

		/* Mark RX node to NOT release */
		llcp_rx_node_retain(ctx);

		ctx->data.pu.error = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
		ctx->data.pu.ntf_pu = 1;
		lp_pu_complete(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}
#endif /* CONFIG_BT_CENTRAL */

static void lp_pu_st_wait_tx_ack_phy_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					 void *param)
{
	switch (evt) {
	case LP_PU_EVT_ACK:
		switch (conn->lll.role) {
#if defined(CONFIG_BT_CENTRAL)
		case BT_HCI_ROLE_CENTRAL:
			ctx->state = LP_PU_STATE_WAIT_RX_PHY_RSP;
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_PHY_RSP;
			break;
#endif /* CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_PERIPHERAL)
		case BT_HCI_ROLE_PERIPHERAL:
			/* If we act as peripheral apply timing restriction */
			pu_set_timing_restrict(
				conn, pu_select_phy_timing_restrict(conn, ctx->data.pu.tx));
			ctx->state = LP_PU_STATE_WAIT_RX_PHY_UPDATE_IND;
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND;
			llcp_tx_resume_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_PHY_UPDATE);
			break;
#endif /* CONFIG_BT_PERIPHERAL */
		default:
			/* Unknown role */
			LL_ASSERT(0);
		}

		break;
	default:
		/* Ignore other evts */
		break;
	}
}

#if defined(CONFIG_BT_CENTRAL)
static void lp_pu_st_wait_tx_phy_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					    void *param)
{
	switch (evt) {
	case LP_PU_EVT_RUN:
		lp_pu_send_phy_update_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_pu_st_wait_tx_ack_phy_update_ind(struct ll_conn *conn, struct proc_ctx *ctx,
						uint8_t evt, void *param)
{
	switch (evt) {
	case LP_PU_EVT_ACK:
		LL_ASSERT(conn->lll.role == BT_HCI_ROLE_CENTRAL);
		if (ctx->data.pu.p_to_c_phy || ctx->data.pu.c_to_p_phy) {
			/* Either phys should change */
			if (ctx->data.pu.c_to_p_phy) {
				/* central to periph tx phy changes so, apply timing restriction */
				pu_set_timing_restrict(conn, ctx->data.pu.c_to_p_phy);
			}

			/* Since at least one phy will change,
			 * stop the procedure response timeout
			 */
			llcp_lr_prt_stop(conn);

			/* Now we should wait for instant */
			ctx->state = LP_PU_STATE_WAIT_INSTANT;
		} else {
			llcp_rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
			ctx->data.pu.error = BT_HCI_ERR_SUCCESS;
			ctx->data.pu.ntf_pu = ctx->data.pu.host_initiated;
			lp_pu_complete(conn, ctx, evt, param);
		}
		llcp_tx_resume_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_PHY_UPDATE);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
static void lp_pu_st_wait_rx_phy_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					    void *param)
{
	switch (evt) {
	case LP_PU_EVT_PHY_UPDATE_IND:
		LL_ASSERT(conn->lll.role == BT_HCI_ROLE_PERIPHERAL);
		llcp_pdu_decode_phy_update_ind(ctx, (struct pdu_data *)param);
		const uint8_t end_procedure = pu_check_update_ind(conn, ctx);

		/* Mark RX node to NOT release */
		llcp_rx_node_retain(ctx);

		if (!end_procedure) {
			if (ctx->data.pu.p_to_c_phy) {
				/* If periph to central phy changes apply tx timing restriction */
				pu_set_timing_restrict(conn, ctx->data.pu.p_to_c_phy);
			}

			/* Since at least one phy will change,
			 * stop the procedure response timeout
			 */
			llcp_lr_prt_stop(conn);

			ctx->state = LP_PU_STATE_WAIT_INSTANT;
		} else {
			llcp_rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
			if (ctx->data.pu.error != BT_HCI_ERR_SUCCESS) {
				/* Mark the connection for termination */
				conn->llcp_terminate.reason_final = ctx->data.pu.error;
			}
			ctx->data.pu.ntf_pu = ctx->data.pu.host_initiated;
			lp_pu_complete(conn, ctx, evt, param);
		}
		llcp_tx_resume_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_PHY_UPDATE);
		break;
	case LP_PU_EVT_REJECT:
		llcp_pdu_decode_reject_ext_ind(ctx, (struct pdu_data *)param);
		ctx->data.pu.error = ctx->reject_ext_ind.error_code;
		/* Fallthrough */
	case LP_PU_EVT_UNKNOWN:
		llcp_rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
		if (evt == LP_PU_EVT_UNKNOWN) {
			feature_unmask_features(conn, LL_FEAT_BIT_PHY_2M | LL_FEAT_BIT_PHY_CODED);
			ctx->data.pu.error = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
		}
		/* Mark RX node to NOT release */
		llcp_rx_node_retain(ctx);

		ctx->data.pu.ntf_pu = 1;
		lp_pu_complete(conn, ctx, evt, param);
		llcp_tx_resume_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_PHY_UPDATE);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}
#endif /* CONFIG_BT_PERIPHERAL */

static void lp_pu_check_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	if (is_instant_reached_or_passed(ctx->data.pu.instant, ull_conn_event_counter(conn))) {
		const uint8_t phy_changed = pu_apply_phy_update(conn, ctx);
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		if (phy_changed) {
			ctx->data.pu.ntf_dle = pu_update_eff_times(conn, ctx);
		}
#endif
		llcp_rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
		ctx->data.pu.error = BT_HCI_ERR_SUCCESS;
		ctx->data.pu.ntf_pu = (phy_changed || ctx->data.pu.host_initiated);
		lp_pu_complete(conn, ctx, evt, param);
	}
}

static void lp_pu_st_wait_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				  void *param)
{
	switch (evt) {
	case LP_PU_EVT_RUN:
		lp_pu_check_instant(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_pu_st_wait_instant_on_air(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					 void *param)
{
	switch (evt) {
	case LP_PU_EVT_NTF:
		lp_pu_tx_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static void lp_pu_st_wait_ntf_avail(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				    void *param)
{
	switch (evt) {
	case LP_PU_EVT_RUN:
		lp_pu_tx(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

static void lp_pu_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->state) {
	case LP_PU_STATE_IDLE:
		lp_pu_st_idle(conn, ctx, evt, param);
		break;
	case LP_PU_STATE_WAIT_TX_PHY_REQ:
		lp_pu_st_wait_tx_phy_req(conn, ctx, evt, param);
		break;
	case LP_PU_STATE_WAIT_TX_ACK_PHY_REQ:
		lp_pu_st_wait_tx_ack_phy_req(conn, ctx, evt, param);
		break;
#if defined(CONFIG_BT_CENTRAL)
	case LP_PU_STATE_WAIT_RX_PHY_RSP:
		lp_pu_st_wait_rx_phy_rsp(conn, ctx, evt, param);
		break;
	case LP_PU_STATE_WAIT_TX_PHY_UPDATE_IND:
		lp_pu_st_wait_tx_phy_update_ind(conn, ctx, evt, param);
		break;
	case LP_PU_STATE_WAIT_TX_ACK_PHY_UPDATE_IND:
		lp_pu_st_wait_tx_ack_phy_update_ind(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_PERIPHERAL)
	case LP_PU_STATE_WAIT_RX_PHY_UPDATE_IND:
		lp_pu_st_wait_rx_phy_update_ind(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_PERIPHERAL */
	case LP_PU_STATE_WAIT_INSTANT:
		lp_pu_st_wait_instant(conn, ctx, evt, param);
		break;
	case LP_PU_STATE_WAIT_INSTANT_ON_AIR:
		lp_pu_st_wait_instant_on_air(conn, ctx, evt, param);
		break;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case LP_PU_STATE_WAIT_NTF_AVAIL:
		lp_pu_st_wait_ntf_avail(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void llcp_lp_pu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *)rx->pdu;

	switch (pdu->llctrl.opcode) {
#if defined(CONFIG_BT_CENTRAL)
	case PDU_DATA_LLCTRL_TYPE_PHY_RSP:
		lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_PHY_RSP, pdu);
		break;
#endif /* CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_PERIPHERAL)
	case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
		lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_PHY_UPDATE_IND, pdu);
		break;
#endif /* CONFIG_BT_PERIPHERAL */
	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_UNKNOWN, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_REJECT, pdu);
		break;
	default:
		/* Invalid behaviour */
		/* Invalid PDU received so terminate connection */
		conn->llcp_terminate.reason_final = BT_HCI_ERR_LMP_PDU_NOT_ALLOWED;
		llcp_lr_complete(conn);
		ctx->state = LP_PU_STATE_IDLE;
		break;
	}
}

void llcp_lp_pu_init_proc(struct proc_ctx *ctx)
{
	ctx->state = LP_PU_STATE_IDLE;
}

void llcp_lp_pu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_RUN, param);
}

void llcp_lp_pu_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_ACK, param);
}

void llcp_lp_pu_tx_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_NTF, NULL);
}

bool llcp_lp_pu_awaiting_instant(struct proc_ctx *ctx)
{
	return (ctx->state == LP_PU_STATE_WAIT_INSTANT);
}

/*
 * LLCP Remote Procedure PHY Update FSM
 */
static void rp_pu_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	LL_ASSERT(ctx->node_ref.tx);

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	if (!llcp_ntf_alloc_is_available()) {
		/* No NTF nodes avail, so we need to hold off TX */
		ctx->state = RP_PU_STATE_WAIT_NTF_AVAIL;
		return;
	}

	ctx->data.pu.ntf_dle_node = llcp_ntf_alloc();
	LL_ASSERT(ctx->data.pu.ntf_dle_node);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

	tx = ctx->node_ref.tx;
	ctx->node_ref.tx = NULL;
	pdu = (struct pdu_data *)tx->pdu;
	ctx->node_ref.tx_ack = tx;

	/* Encode LL Control PDU */
	switch (ctx->tx_opcode) {
#if defined(CONFIG_BT_PERIPHERAL)
	case PDU_DATA_LLCTRL_TYPE_PHY_RSP:
		llcp_pdu_encode_phy_rsp(conn, pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND;
		ctx->state = RP_PU_STATE_WAIT_TX_ACK_PHY_RSP;
		break;
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CENTRAL)
	case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
		pu_prep_update_ind(conn, ctx);
		pu_prepare_instant(conn, ctx);
		llcp_pdu_encode_phy_update_ind(ctx, pdu);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		ctx->state = RP_PU_STATE_WAIT_TX_ACK_PHY_UPDATE_IND;
		break;
#endif /* CONFIG_BT_CENTRAL */
	default:
		LL_ASSERT(0);
	}

	/* Enqueue LL Control PDU towards LLL */
	llcp_tx_enqueue(conn, tx);

	/* Restart procedure response timeout timer */
	llcp_rr_prt_restart(conn);
}

static void rp_pu_complete_finalize(struct ll_conn *conn, struct proc_ctx *ctx)
{
	llcp_rr_complete(conn);
	llcp_rr_set_paused_cmd(conn, PROC_NONE);
	ctx->state = RP_PU_STATE_IDLE;
}

static void rp_pu_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	pu_reset_timing_restrict(conn);
	/* Postpone procedure completion (and possible NTF generation) to actual 'air instant'
	 * Since LLCP STM is driven from LLL prepare this actually happens BEFORE instant
	 * and thus NTFs are generated and propagated up prior to actual instant on air.
	 * Instead postpone completion/NTF to the beginning of RX handling
	 */
	ctx->state = RP_PU_STATE_WAIT_INSTANT_ON_AIR;
}

static void rp_pu_tx_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	pu_ntf(conn, ctx);
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	pu_dle_ntf(conn, ctx);
#endif
	rp_pu_complete_finalize(conn, ctx);
}

#if defined(CONFIG_BT_CENTRAL)
static void rp_pu_send_phy_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				      void *param)
{
	if (llcp_rr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx) ||
	    (llcp_rr_get_paused_cmd(conn) == PROC_PHY_UPDATE) ||
	    !ull_is_lll_tx_queue_empty(conn)) {
		ctx->state = RP_PU_STATE_WAIT_TX_PHY_UPDATE_IND;
	} else {
		llcp_rr_set_paused_cmd(conn, PROC_CTE_REQ);
		ctx->tx_opcode = PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND;
		ctx->node_ref.tx = llcp_tx_alloc(conn, ctx);
		rp_pu_tx(conn, ctx, evt, param);

	}
}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
static void rp_pu_send_phy_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (llcp_rr_ispaused(conn) || !llcp_tx_alloc_peek(conn, ctx) ||
	    (llcp_rr_get_paused_cmd(conn) == PROC_PHY_UPDATE)) {
		ctx->state = RP_PU_STATE_WAIT_TX_PHY_RSP;
	} else {
		llcp_rr_set_paused_cmd(conn, PROC_CTE_REQ);
		ctx->tx_opcode = PDU_DATA_LLCTRL_TYPE_PHY_RSP;
		ctx->node_ref.tx = llcp_tx_alloc(conn, ctx);
		rp_pu_tx(conn, ctx, evt, param);
	}
}
#endif /* CONFIG_BT_CENTRAL */

static void rp_pu_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_PU_EVT_RUN:
		ctx->state = RP_PU_STATE_WAIT_RX_PHY_REQ;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_pu_st_wait_rx_phy_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				     void *param)
{
	llcp_pdu_decode_phy_req(ctx, (struct pdu_data *)param);
	/* Combine with the 'Preferred' the phys in conn->phy_pref_?x */
	pu_combine_phys(conn, ctx, conn->phy_pref_tx, conn->phy_pref_rx);
	llcp_tx_pause_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_PHY_UPDATE);

	switch (evt) {
	case RP_PU_EVT_PHY_REQ:
		switch (conn->lll.role) {
#if defined(CONFIG_BT_CENTRAL)
		case BT_HCI_ROLE_CENTRAL:
			/* Mark RX node to NOT release */
			llcp_rx_node_retain(ctx);
			rp_pu_send_phy_update_ind(conn, ctx, evt, param);
			break;
#endif /* CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_PERIPHERAL)
		case BT_HCI_ROLE_PERIPHERAL:
			rp_pu_send_phy_rsp(conn, ctx, evt, param);
			break;
#endif /* CONFIG_BT_PERIPHERAL */
		default:
			/* Unknown role */
			LL_ASSERT(0);
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

#if defined(CONFIG_BT_PERIPHERAL)
static void rp_pu_st_wait_tx_phy_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				     void *param)
{
	switch (evt) {
	case RP_PU_EVT_RUN:
		rp_pu_send_phy_rsp(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}
#endif /* CONFIG_BT_PERIPHERAL */

static void rp_pu_st_wait_tx_ack_phy(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				     void *param)
{
	switch (evt) {
	case RP_PU_EVT_ACK:
		if (0) {
#if defined(CONFIG_BT_PERIPHERAL)
		} else if (ctx->state == RP_PU_STATE_WAIT_TX_ACK_PHY_RSP) {
			LL_ASSERT(conn->lll.role == BT_HCI_ROLE_PERIPHERAL);
			/* When we act as peripheral apply timing restriction */
			pu_set_timing_restrict(
				conn, pu_select_phy_timing_restrict(conn, ctx->data.pu.tx));
			/* RSP acked, now await update ind from central */
			ctx->state = RP_PU_STATE_WAIT_RX_PHY_UPDATE_IND;
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CENTRAL)
		} else if (ctx->state == RP_PU_STATE_WAIT_TX_ACK_PHY_UPDATE_IND) {
			LL_ASSERT(conn->lll.role == BT_HCI_ROLE_CENTRAL);
			if (ctx->data.pu.c_to_p_phy || ctx->data.pu.p_to_c_phy) {
				/* UPDATE_IND acked, so lets await instant */
				if (ctx->data.pu.c_to_p_phy) {
					/*
					 * And if central to periph phys changes
					 * apply timining restrictions
					 */
					pu_set_timing_restrict(conn, ctx->data.pu.c_to_p_phy);
				}
				ctx->state = RP_PU_STATE_WAIT_INSTANT;
			} else {
				rp_pu_complete(conn, ctx, evt, param);
			}
#endif /* CONFIG_BT_CENTRAL */
		} else {
			/* empty clause */
		}
		llcp_tx_resume_data(conn, LLCP_TX_QUEUE_PAUSE_DATA_PHY_UPDATE);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

#if defined(CONFIG_BT_CENTRAL)
static void rp_pu_st_wait_tx_phy_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					    void *param)
{
	switch (evt) {
	case RP_PU_EVT_RUN:
		rp_pu_send_phy_update_ind(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
static void rp_pu_st_wait_rx_phy_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					    void *param)
{
	switch (evt) {
	case RP_PU_EVT_PHY_UPDATE_IND:
		llcp_pdu_decode_phy_update_ind(ctx, (struct pdu_data *)param);
		const uint8_t end_procedure = pu_check_update_ind(conn, ctx);

		/* Mark RX node to NOT release */
		llcp_rx_node_retain(ctx);

		if (!end_procedure) {
			/* Since at least one phy will change,
			 * stop the procedure response timeout
			 */
			llcp_rr_prt_stop(conn);
			ctx->state = RP_PU_STATE_WAIT_INSTANT;
		} else {
			if (ctx->data.pu.error == BT_HCI_ERR_INSTANT_PASSED) {
				/* Mark the connection for termination */
				conn->llcp_terminate.reason_final = BT_HCI_ERR_INSTANT_PASSED;
			}
			rp_pu_complete(conn, ctx, evt, param);
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}
#endif /* CONFIG_BT_PERIPHERAL */

static void rp_pu_check_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				void *param)
{
	if (is_instant_reached_or_passed(ctx->data.pu.instant, ull_conn_event_counter(conn))) {
		ctx->data.pu.error = BT_HCI_ERR_SUCCESS;
		const uint8_t phy_changed = pu_apply_phy_update(conn, ctx);
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		if (phy_changed) {
			ctx->data.pu.ntf_dle = pu_update_eff_times(conn, ctx);
		}
#endif
		/* if PHY settings changed we should generate NTF */
		ctx->data.pu.ntf_pu = phy_changed;
		rp_pu_complete(conn, ctx, evt, param);
	}
}

static void rp_pu_st_wait_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				  void *param)
{
	switch (evt) {
	case RP_PU_EVT_RUN:
		rp_pu_check_instant(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_pu_st_wait_instant_on_air(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
					 void *param)
{
	switch (evt) {
	case RP_PU_EVT_NTF:
		rp_pu_tx_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static void rp_pu_st_wait_ntf_avail(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt,
				    void *param)
{
	switch (evt) {
	case RP_PU_EVT_RUN:
		rp_pu_tx(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

static void rp_pu_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->state) {
	case RP_PU_STATE_IDLE:
		rp_pu_st_idle(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_RX_PHY_REQ:
		rp_pu_st_wait_rx_phy_req(conn, ctx, evt, param);
		break;
#if defined(CONFIG_BT_PERIPHERAL)
	case RP_PU_STATE_WAIT_TX_PHY_RSP:
		rp_pu_st_wait_tx_phy_rsp(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_TX_ACK_PHY_RSP:
		rp_pu_st_wait_tx_ack_phy(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_RX_PHY_UPDATE_IND:
		rp_pu_st_wait_rx_phy_update_ind(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_PERIPHERAL */
#if defined(CONFIG_BT_CENTRAL)
	case RP_PU_STATE_WAIT_TX_PHY_UPDATE_IND:
		rp_pu_st_wait_tx_phy_update_ind(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_TX_ACK_PHY_UPDATE_IND:
		rp_pu_st_wait_tx_ack_phy(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_CENTRAL */
	case RP_PU_STATE_WAIT_INSTANT:
		rp_pu_st_wait_instant(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_INSTANT_ON_AIR:
		rp_pu_st_wait_instant_on_air(conn, ctx, evt, param);
		break;
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case RP_PU_STATE_WAIT_NTF_AVAIL:
		rp_pu_st_wait_ntf_avail(conn, ctx, evt, param);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void llcp_rp_pu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *)rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_PHY_REQ:
		rp_pu_execute_fsm(conn, ctx, RP_PU_EVT_PHY_REQ, pdu);
		break;
#if defined(CONFIG_BT_PERIPHERAL)
	case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
		rp_pu_execute_fsm(conn, ctx, RP_PU_EVT_PHY_UPDATE_IND, pdu);
		break;
#endif /* CONFIG_BT_PERIPHERAL */
	default:
		/* Invalid behaviour */
		/* Invalid PDU received so terminate connection */
		conn->llcp_terminate.reason_final = BT_HCI_ERR_LMP_PDU_NOT_ALLOWED;
		llcp_rr_complete(conn);
		ctx->state = RP_PU_STATE_IDLE;
		break;
	}
}

void llcp_rp_pu_init_proc(struct proc_ctx *ctx)
{
	ctx->state = RP_PU_STATE_IDLE;
}

void llcp_rp_pu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_pu_execute_fsm(conn, ctx, RP_PU_EVT_RUN, param);
}

void llcp_rp_pu_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_pu_execute_fsm(conn, ctx, RP_PU_EVT_ACK, param);
}

void llcp_rp_pu_tx_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	rp_pu_execute_fsm(conn, ctx, RP_PU_EVT_NTF, NULL);
}

bool llcp_rp_pu_awaiting_instant(struct proc_ctx *ctx)
{
	return (ctx->state == RP_PU_STATE_WAIT_INSTANT);
}
