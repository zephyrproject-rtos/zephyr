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

#include "hal/ccm.h"

#include "util/mem.h"
#include "util/memq.h"

#include "pdu.h"
#include "ll.h"
#include "ll_settings.h"

#include "lll.h"
#include "ll_feat.h"
#include "lll_conn.h"

#include "ull_tx_queue.h"

#include "ull_conn_types.h"
#include "ull_internal.h"
#include "ull_llcp.h"
#include "ull_llcp_features.h"
#include "ull_llcp_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_llcp_phy
#include "common/log.h"
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
	LP_PU_STATE_WAIT_INSTANT,
	LP_PU_STATE_WAIT_NTF,
};

/* LLCP Local Procedure PHY Update FSM events */
enum {
	/* Procedure run */
	LP_PU_EVT_RUN,

	/* Response recieved */
	LP_PU_EVT_PHY_RSP,

	/* Indication recieved */
	LP_PU_EVT_PHY_UPDATE_IND,

	/* Ack received */
	LP_PU_EVT_ACK,

	/* Reject response recieved */
	LP_PU_EVT_REJECT,

	/* Unknown response recieved */
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
	RP_PU_STATE_WAIT_INSTANT,
	RP_PU_STATE_WAIT_NTF,
};

/* LLCP Remote Procedure PHY Update FSM events */
enum {
	/* Procedure run */
	RP_PU_EVT_RUN,

	/* Request recieved */
	RP_PU_EVT_PHY_REQ,

	/* Ack received */
	RP_PU_EVT_ACK,

	/* Indication recieved */
	RP_PU_EVT_PHY_UPDATE_IND,
};

/* Hardcoded instant delta +6 */
#define PHY_UPDATE_INSTANT_DELTA  6

/* statics to signal through generic interface of lp_pu_complete */
static const uint8_t generate_ntf = 1U;
static const uint8_t dont_generate_ntf = 0U;
static uint8_t ntf;

/* PHY preference order*/
#define PHY_PREF_1 PHY_2M
#define PHY_PREF_2 PHY_1M
#define PHY_PREF_3 PHY_CODED

static inline uint8_t pu_select_phy(uint8_t phys)
{
	/* select only one phy, select prefered */
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
		ctx->data.pu.m_to_s_phy = ctx->data.pu.tx;
	} else {
		ctx->data.pu.m_to_s_phy = 0U;
	}
	if (ctx->data.pu.rx != conn->lll.phy_rx) {
		ctx->data.pu.s_to_m_phy = ctx->data.pu.rx;
	} else {
		ctx->data.pu.s_to_m_phy = 0U;
	}
}

static uint8_t pu_select_phy_timing_restrict(struct ll_conn *conn, uint8_t phy_tx)
{
	/* select the probable PHY with longest Tx time, which
	 * will be restricted to fit current
	 * connEffectiveMaxTxTime.
	 */
	/* Note - entry 0 in table is unused, so 0 on purpose */
	uint8_t phy_tx_time[8] = {0, PHY_1M, PHY_2M,
				  PHY_1M, PHY_CODED, PHY_CODED,
				  PHY_CODED, PHY_CODED};
	struct lll_conn *lll = &conn->lll;
	const uint8_t phys = phy_tx | lll->phy_tx;

	return phy_tx_time[phys];
}

static void pu_set_timing_restrict(struct ll_conn *conn, uint8_t phy_tx)
{
	struct lll_conn *lll = &conn->lll;

	lll->phy_tx_time = phy_tx;
}

static void pu_reset_timing_restrict(struct ll_conn *conn)
{
	pu_set_timing_restrict(conn, conn->lll.phy_tx);
}

static uint16_t pu_event_counter(struct ll_conn *conn)
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

static uint8_t pu_check_update_ind(struct ll_conn *conn, struct proc_ctx *ctx)
{
	/* Both tx and rx PHY unchanged */
	if (!((ctx->data.pu.m_to_s_phy | ctx->data.pu.s_to_m_phy) & 0x07)) {
		/* if no phy changes, quit procedure, and possibly signal host */
		ctx->data.pu.error = BT_HCI_ERR_SUCCESS;
		return 1;
	} else {
		/* if instant already passed, quit procedure with error */
		if (is_instant_reached_or_passed(ctx->data.pu.instant, pu_event_counter(conn))) {
			ctx->data.pu.error = BT_HCI_ERR_INSTANT_PASSED;
			return 1;
		}
	}
	return 0;
}

static uint8_t pu_apply_phy_update(struct ll_conn *conn, struct proc_ctx *ctx)
{
	if (ctx->data.pu.s_to_m_phy) {
		if (conn->lll.role == BT_HCI_ROLE_SLAVE) {
			conn->lll.phy_tx = ctx->data.pu.s_to_m_phy;
		} else {
			conn->lll.phy_rx = ctx->data.pu.s_to_m_phy;
		}
	}

	if (ctx->data.pu.m_to_s_phy) {
		if (conn->lll.role == BT_HCI_ROLE_SLAVE) {
			conn->lll.phy_rx = ctx->data.pu.m_to_s_phy;
		} else {
			conn->lll.phy_tx = ctx->data.pu.m_to_s_phy;
		}
	}

	return (ctx->data.pu.m_to_s_phy || ctx->data.pu.s_to_m_phy);
}

static inline void pu_set_preferred_phys(struct ll_conn *conn, struct proc_ctx *ctx)
{
	conn->phy_pref_rx = ctx->data.pu.rx;
	conn->phy_pref_tx = ctx->data.pu.tx;

	/* Note: Since 'flags' indicate local coded phy preference (S2 or S8) and
	   this is not negotiated with the peer, it is simply reconfigured in conn->lll when
	   the update is initiated, and takes effect whenever the coded phy is in use. */
	conn->lll.phy_flags = ctx->data.pu.flags;
}

static inline void pu_combine_phys(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t tx, uint8_t rx)
{
	/* Combine requested phys with locally preferred phys */
	ctx->data.pu.rx &= rx;
	ctx->data.pu.tx &= tx;
	/* If either tx or rx is 'no change' at this point we force both to no change to
	   comply with the spec
		Spec. BT5.2 Vol6, Part B, section 5.1.10:
		The remainder of this section shall apply irrespective of which device initiated
		the procedure.

		Irrespective of the above rules, the master may leave both directions
		unchanged. If the slave specified a single PHY in both the TX_PHYS and
		RX_PHYS fields and both fields are the same, the master shall either select
		the PHY specified by the slave for both directions or shall leave both directions
		unchanged.
	*/
	if (conn->lll.role == BT_HCI_ROLE_MASTER && (!ctx->data.pu.rx  || !ctx->data.pu.tx) ) {
		ctx->data.pu.tx = 0;
		ctx->data.pu.rx = 0;
	}
}

/*
 * LLCP Local Procedure PHY Update FSM
 */

static void lp_pu_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = tx_alloc();
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (opcode) {
	case PDU_DATA_LLCTRL_TYPE_PHY_REQ:
		pu_set_preferred_phys(conn, ctx);
		pdu_encode_phy_req(ctx, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
		pu_prep_update_ind(conn,ctx);
		pdu_encode_phy_update_ind(ctx, pdu);
		break;
	default:
		LL_ASSERT(0);
	}

	/* Always 'request' the ACK signal */
	ctx->tx_ack = tx;
	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	tx_enqueue(conn, tx);
}

static void pu_ntf(struct ll_conn *conn, struct proc_ctx *ctx)
{
	struct node_rx_pdu *ntf;
	struct node_rx_pu *pdu;

	/* Allocate ntf node */
	ntf = ntf_alloc();
	LL_ASSERT(ntf);

	ntf->hdr.type = NODE_RX_TYPE_PHY_UPDATE;
	ntf->hdr.handle = conn->lll.handle;
	pdu = (struct node_rx_pu *)ntf->pdu;

	pdu->status = ctx->data.pu.error;
	pdu->rx = conn->lll.phy_rx;
	pdu->tx = conn->lll.phy_tx;

	/* Enqueue notification towards LL */
	ll_rx_put(ntf->hdr.link, ntf);
	ll_rx_sched();
}

static void lp_pu_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	const uint8_t ntf = *(uint8_t *)param;
	// when complete reset timing restrictions - idempotent (so no problem if we need to wait for NTF buffer)
	pu_reset_timing_restrict(conn);

	if (ntf && !ntf_alloc_is_available()) {
		ctx->state = LP_PU_STATE_WAIT_NTF;
	} else {
		if (ntf) {
			pu_ntf(conn, ctx);
		}
		lr_complete(conn);
		ctx->state = LP_PU_STATE_IDLE;
	}
}

static void lp_pu_send_phy_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!tx_alloc_is_available() || rr_get_collision(conn)) {
		ctx->state = LP_PU_STATE_WAIT_TX_PHY_REQ;
	} else {
		rr_set_incompat(conn, INCOMPAT_RESOLVABLE);
		lp_pu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_PHY_REQ);
		tx_pause_data(conn);
		ctx->state = LP_PU_STATE_WAIT_TX_ACK_PHY_REQ;
	}
}

static void lp_pu_send_phy_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = LP_PU_STATE_WAIT_TX_PHY_UPDATE_IND;
	} else {
		ctx->data.pu.instant = pu_event_counter(conn) + PHY_UPDATE_INSTANT_DELTA;
		lp_pu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		ctx->state = LP_PU_STATE_WAIT_TX_ACK_PHY_UPDATE_IND;
	}
}

static void lp_pu_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case LP_PU_EVT_RUN:
		lp_pu_send_phy_req(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_pu_st_wait_tx_phy_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
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

static void lp_pu_st_wait_rx_phy_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_PU_EVT_RUN:
		if (conn->lll.role == BT_HCI_ROLE_SLAVE && rr_get_collision(conn)) {
			rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
			ctx->data.pu.error = BT_HCI_ERR_LL_PROC_COLLISION;
			lp_pu_complete(conn, ctx, evt, (void*)&generate_ntf);
		}
		break;
	case LP_PU_EVT_PHY_RSP:
		rr_set_incompat(conn, INCOMPAT_RESERVED);
		/* 'Prefer' the phys from the REQ */
		uint8_t tx_pref = ctx->data.pu.tx;
		uint8_t rx_pref = ctx->data.pu.rx;
		pdu_decode_phy_rsp(ctx, (struct pdu_data *)param);
		/* Pause data tx */
		tx_pause_data(conn);
		/* Combine with the 'Prefered' phys */
		pu_combine_phys(conn, ctx, tx_pref, rx_pref);
		lp_pu_send_phy_update_ind(conn, ctx, evt, param);
		break;
	case LP_PU_EVT_UNKNOWN:
		rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
		/* Unsupported in peer, so disable locally for this connection
		 * Peer does not accept PHY UPDATE, so disable non 1M phys on current connection
		 */
		feature_unmask_features(conn, LL_FEAT_BIT_PHY_2M | LL_FEAT_BIT_PHY_CODED);
		ctx->data.pu.error = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE;
		lp_pu_complete(conn, ctx, evt, (void*)&generate_ntf);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_pu_st_wait_tx_ack_phy_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_PU_EVT_ACK:
		switch (conn->lll.role) {
		case BT_HCI_ROLE_MASTER:
			ctx->state = LP_PU_STATE_WAIT_RX_PHY_RSP;
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_PHY_RSP;
			break;
		case BT_HCI_ROLE_SLAVE:
			/* If we act as peripheral apply timing restriction */
			pu_set_timing_restrict(conn, pu_select_phy_timing_restrict(conn, ctx->data.pu.tx));
			ctx->state = LP_PU_STATE_WAIT_RX_PHY_UPDATE_IND;
			ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND;
			break;
		default:
			/* Unknown role */
			LL_ASSERT(0);
		}
		tx_resume_data(conn);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_pu_st_wait_tx_phy_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
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

static void lp_pu_st_wait_tx_ack_phy_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_PU_EVT_ACK:
		LL_ASSERT(conn->lll.role == BT_HCI_ROLE_MASTER);
		if (ctx->data.pu.s_to_m_phy || ctx->data.pu.m_to_s_phy) {
			/* Either phys should change */
			if (ctx->data.pu.m_to_s_phy)
			{
				/* master to slave tx phy changes so, apply timing restriction */
				pu_set_timing_restrict(conn, ctx->data.pu.m_to_s_phy);
			}
			/* Now we should wait for instant */
			ctx->state = LP_PU_STATE_WAIT_INSTANT;
		} else {
			rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
			ctx->data.pu.error = BT_HCI_ERR_SUCCESS;
			ntf = ctx->data.pu.host_initiated;
			lp_pu_complete(conn, ctx, evt, (void*)&ntf);
		}
		tx_resume_data(conn);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_pu_st_wait_rx_phy_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_PU_EVT_PHY_UPDATE_IND:
		LL_ASSERT(conn->lll.role == BT_HCI_ROLE_SLAVE);
		pdu_decode_phy_update_ind(ctx, (struct pdu_data *)param);
		const uint8_t end_procedure = pu_check_update_ind(conn, ctx);
		if (!end_procedure)
		{
			if (ctx->data.pu.s_to_m_phy)
			{
				/* If slave to master phy changes apply tx timing restriction */
				pu_set_timing_restrict(conn, ctx->data.pu.s_to_m_phy);
			}
			ctx->state = LP_PU_STATE_WAIT_INSTANT;
		} else {
			rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
			ntf = ctx->data.pu.host_initiated;
			lp_pu_complete(conn, ctx, evt, (void*)&ntf);
		}
		break;
	case LP_PU_EVT_REJECT:
		rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
		ctx->data.pu.error = BT_HCI_ERR_LL_PROC_COLLISION;
		lp_pu_complete(conn, ctx, evt, (void*)&generate_ntf);
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_pu_check_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (is_instant_reached_or_passed(ctx->data.pu.instant, pu_event_counter(conn))) {
		const uint8_t phy_changed = pu_apply_phy_update(conn, ctx);
		rr_set_incompat(conn, INCOMPAT_NO_COLLISION);
		ctx->data.pu.error = BT_HCI_ERR_SUCCESS;
		ntf = phy_changed || ctx->data.pu.host_initiated;
		lp_pu_complete(conn, ctx, evt, (void*)&ntf);
	}
}

static void lp_pu_st_wait_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case LP_PU_EVT_RUN:
		lp_pu_check_instant(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void lp_pu_st_wait_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case LP_PU_EVT_RUN:
		lp_pu_complete(conn, ctx, evt, (void*)&generate_ntf);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

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
	case LP_PU_STATE_WAIT_RX_PHY_RSP:
		lp_pu_st_wait_rx_phy_rsp(conn, ctx, evt, param);
		break;
	case LP_PU_STATE_WAIT_TX_PHY_UPDATE_IND:
		lp_pu_st_wait_tx_phy_update_ind(conn, ctx, evt, param);
		break;
	case LP_PU_STATE_WAIT_TX_ACK_PHY_UPDATE_IND:
		lp_pu_st_wait_tx_ack_phy_update_ind(conn, ctx, evt, param);
		break;
	case LP_PU_STATE_WAIT_RX_PHY_UPDATE_IND:
		lp_pu_st_wait_rx_phy_update_ind(conn, ctx, evt, param);
		break;
	case LP_PU_STATE_WAIT_INSTANT:
		lp_pu_st_wait_instant(conn, ctx, evt, param);
		break;
	case LP_PU_STATE_WAIT_NTF:
		lp_pu_st_wait_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_lp_pu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *) rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_PHY_RSP:
		lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_PHY_RSP, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
		lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_PHY_UPDATE_IND, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_UNKNOWN, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_REJECT, pdu);
		break;
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_lp_pu_init_proc(struct proc_ctx *ctx)
{
	ctx->state = LP_PU_STATE_IDLE;
}

void ull_cp_priv_lp_pu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_RUN, param);
}

void ull_cp_priv_lp_pu_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	lp_pu_execute_fsm(conn, ctx, LP_PU_EVT_ACK, param);
}

/*
 * LLCP Remote Procedure PHY Update FSM
 */
static void rp_pu_tx(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data *pdu;

	/* Allocate tx node */
	tx = tx_alloc();
	LL_ASSERT(tx);

	pdu = (struct pdu_data *)tx->pdu;

	/* Encode LL Control PDU */
	switch (opcode) {
	case PDU_DATA_LLCTRL_TYPE_PHY_RSP:
		pdu_encode_phy_rsp(conn, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
		pu_prep_update_ind(conn,ctx);
		pdu_encode_phy_update_ind(ctx, pdu);
		break;
	default:
		LL_ASSERT(0);
	}

	ctx->tx_ack = tx;
	ctx->tx_opcode = pdu->llctrl.opcode;

	/* Enqueue LL Control PDU towards LLL */
	tx_enqueue(conn, tx);
}

static void rp_pu_complete(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	const uint8_t ntf = *(uint8_t *)param;
	/* when complete reset timing restrictions - idempotent (so no problem if we need to wait for NTF) */
	pu_reset_timing_restrict(conn);

	if (ntf && !ntf_alloc_is_available()) {
		ctx->state = RP_PU_STATE_WAIT_NTF;
	} else {
		if (ntf)
		{
			pu_ntf(conn, ctx);
		}
		rr_complete(conn);
		ctx->state = RP_PU_STATE_IDLE;
	}
}

static void rp_pu_send_phy_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = RP_PU_STATE_WAIT_TX_PHY_UPDATE_IND;
	} else {
		ctx->data.pu.instant = pu_event_counter(conn) + PHY_UPDATE_INSTANT_DELTA;
		rp_pu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_UNUSED;
		ctx->state = RP_PU_STATE_WAIT_TX_ACK_PHY_UPDATE_IND;
	}
}

static void rp_pu_send_phy_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (!tx_alloc_is_available()) {
		ctx->state = RP_PU_STATE_WAIT_TX_PHY_RSP;
	} else {
		rp_pu_tx(conn, ctx, PDU_DATA_LLCTRL_TYPE_PHY_RSP);
		ctx->rx_opcode = PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND;
		ctx->state = RP_PU_STATE_WAIT_TX_ACK_PHY_RSP;
	}
}

static void rp_pu_st_idle(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case RP_PU_EVT_RUN:
		ctx->state = RP_PU_STATE_WAIT_RX_PHY_REQ;
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_pu_st_wait_rx_phy_req(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	pdu_decode_phy_req(ctx, (struct pdu_data *)param);
	/* Combine with the 'Prefered' the phys in conn->phy_pref_?x */
	pu_combine_phys(conn,ctx, conn->phy_pref_tx, conn->phy_pref_rx);
	tx_pause_data(conn);
	switch (evt) {
	case RP_PU_EVT_PHY_REQ:
		switch (conn->lll.role) {
		case BT_HCI_ROLE_MASTER:
			rp_pu_send_phy_update_ind(conn, ctx, evt, param);
			break;
		case BT_HCI_ROLE_SLAVE:
			rp_pu_send_phy_rsp(conn, ctx, evt, param);
			break;
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

static void rp_pu_st_wait_tx_phy_rsp(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
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

static void rp_pu_st_wait_tx_ack_phy(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_PU_EVT_ACK:
		if (ctx->state == RP_PU_STATE_WAIT_TX_ACK_PHY_RSP) {
			LL_ASSERT(conn->lll.role == BT_HCI_ROLE_SLAVE);
			/* When we act as peripheral apply timing restriction */
			pu_set_timing_restrict(conn, pu_select_phy_timing_restrict(conn, ctx->data.pu.tx));
			/* RSP acked, now await update ind from master */
			ctx->state = RP_PU_STATE_WAIT_RX_PHY_UPDATE_IND;
		} else if (ctx->state == RP_PU_STATE_WAIT_TX_ACK_PHY_UPDATE_IND) {
			LL_ASSERT(conn->lll.role == BT_HCI_ROLE_MASTER);
			if (ctx->data.pu.m_to_s_phy || ctx->data.pu.s_to_m_phy) {
				/* UPDATE_IND acked, so lets await instant */
				if (ctx->data.pu.m_to_s_phy)
				{
					/* And if master to slave phys changes apply timining restrictions */
					pu_set_timing_restrict(conn, ctx->data.pu.m_to_s_phy);
				}
				ctx->state = RP_PU_STATE_WAIT_INSTANT;
			} else {
				rp_pu_complete(conn, ctx, evt, (void*)&dont_generate_ntf);
			}
		}
		tx_resume_data(conn);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_pu_st_wait_tx_phy_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
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

static void rp_pu_st_wait_rx_phy_update_ind(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_PU_EVT_PHY_UPDATE_IND:
		pdu_decode_phy_update_ind(ctx, (struct pdu_data *)param);
		const uint8_t end_procedure = pu_check_update_ind(conn, ctx);
		if (!end_procedure)
		{

			ctx->state = LP_PU_STATE_WAIT_INSTANT;
		} else {
			rp_pu_complete(conn, ctx, evt, (void*)&dont_generate_ntf);
		}
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_pu_check_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	if (is_instant_reached_or_passed(ctx->data.pu.instant, pu_event_counter(conn))) {
		ctx->data.pu.error = BT_HCI_ERR_SUCCESS;
		const uint8_t phy_changed = pu_apply_phy_update(conn, ctx);
		/* if PHY settings changed we should generate NTF */
		ntf = phy_changed;
		rp_pu_complete(conn, ctx, evt, (void*)&ntf);
	}
}

static void rp_pu_st_wait_instant(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	/* TODO */
	switch (evt) {
	case RP_PU_EVT_RUN:
		rp_pu_check_instant(conn, ctx, evt, param);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}

static void rp_pu_st_wait_ntf(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (evt) {
	case RP_PU_EVT_RUN:
		rp_pu_complete(conn, ctx, evt, (void*)&generate_ntf);
		break;
	default:
		/* Ignore other evts */
		break;
	}
}


static void rp_pu_execute_fsm(struct ll_conn *conn, struct proc_ctx *ctx, uint8_t evt, void *param)
{
	switch (ctx->state) {
	case RP_PU_STATE_IDLE:
		rp_pu_st_idle(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_RX_PHY_REQ:
		rp_pu_st_wait_rx_phy_req(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_TX_PHY_RSP:
		rp_pu_st_wait_tx_phy_rsp(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_TX_ACK_PHY_RSP:
		rp_pu_st_wait_tx_ack_phy(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_TX_PHY_UPDATE_IND:
		rp_pu_st_wait_tx_phy_update_ind(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_TX_ACK_PHY_UPDATE_IND:
		rp_pu_st_wait_tx_ack_phy(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_RX_PHY_UPDATE_IND:
		rp_pu_st_wait_rx_phy_update_ind(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_INSTANT:
		rp_pu_st_wait_instant(conn, ctx, evt, param);
		break;
	case RP_PU_STATE_WAIT_NTF:
		rp_pu_st_wait_ntf(conn, ctx, evt, param);
		break;
	default:
		/* Unknown state */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_rp_pu_rx(struct ll_conn *conn, struct proc_ctx *ctx, struct node_rx_pdu *rx)
{
	struct pdu_data *pdu = (struct pdu_data *) rx->pdu;

	switch (pdu->llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_PHY_REQ:
		rp_pu_execute_fsm(conn, ctx, RP_PU_EVT_PHY_REQ, pdu);
		break;
	case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
		rp_pu_execute_fsm(conn, ctx, RP_PU_EVT_PHY_UPDATE_IND, pdu);
		break;
	default:
		/* Unknown opcode */
		LL_ASSERT(0);
	}
}

void ull_cp_priv_rp_pu_init_proc(struct proc_ctx *ctx)
{
	ctx->state = RP_PU_STATE_IDLE;
}

void ull_cp_priv_rp_pu_run(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_pu_execute_fsm(conn, ctx, RP_PU_EVT_RUN, param);
}

void ull_cp_priv_rp_pu_tx_ack(struct ll_conn *conn, struct proc_ctx *ctx, void *param)
{
	rp_pu_execute_fsm(conn, ctx, RP_PU_EVT_ACK, param);
}

