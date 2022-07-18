/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>
#include "kconfig.h"

#define ULL_LLCP_UNITTEST

#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "pdu.h"
#include "ll.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"

#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_conn_internal.h"
#include "ull_llcp_internal.h"

#include "helper_pdu.h"
#include "helper_util.h"

#define PREFER_S8_CODING 1
#define PREFER_S2_CODING 0

#define HOST_INITIATED 1U

static struct ll_conn conn;

static void setup(void)
{
	test_setup(&conn);

	/* Emulate initial conn state */
	conn.phy_pref_rx = PHY_1M | PHY_2M | PHY_CODED;
	conn.phy_pref_tx = PHY_1M | PHY_2M | PHY_CODED;
	conn.lll.phy_flags = PREFER_S2_CODING;
	conn.lll.phy_tx_time = PHY_1M;
	conn.lll.phy_rx = PHY_1M;
	conn.lll.phy_tx = PHY_1M;

	/* Init DLE data */
	ull_conn_default_tx_octets_set(251);
	ull_conn_default_tx_time_set(2120);
	ull_dle_init(&conn, PHY_1M);
	/* Emulate different remote numbers to trigger update of eff */
	conn.lll.dle.remote.max_tx_octets = PDU_DC_PAYLOAD_SIZE_MIN * 3;
	conn.lll.dle.remote.max_rx_octets = PDU_DC_PAYLOAD_SIZE_MIN * 3;
	conn.lll.dle.remote.max_tx_time = PDU_DC_MAX_US(conn.lll.dle.remote.max_tx_octets,
							 PHY_1M);
	conn.lll.dle.remote.max_rx_time = PDU_DC_MAX_US(conn.lll.dle.remote.max_rx_octets,
							 PHY_1M);
	ull_dle_update_eff(&conn);
}

#define CHECK_PREF_PHY_STATE(_conn, _tx, _rx)                               \
	do {                                                                    \
		zassert_equal(_conn.phy_pref_rx, _rx,                              \
			      "Preferred RX PHY mismatch %d (actual) != %d (expected)", \
			      _conn.phy_pref_rx, _rx);                                 \
		zassert_equal(_conn.phy_pref_tx, _tx,                              \
			      "Preferred TX PHY mismatch %d (actual) != %d (expected)", \
			      _conn.phy_pref_tx, _tx);                                 \
	} while (0)

#define CHECK_CURRENT_PHY_STATE(_conn, _tx, _flags, _rx)                    \
	do {                                                                    \
		zassert_equal(_conn.lll.phy_rx, _rx,                               \
			      "Current RX PHY mismatch %d (actual) != %d (expected)",   \
			      _conn.lll.phy_rx, _rx);                                  \
		zassert_equal(_conn.lll.phy_tx, _tx,                               \
			      "Current TX PHY mismatch %d (actual) != %d (expected)",   \
			      _conn.lll.phy_tx, _tx);                                  \
		zassert_equal(_conn.lll.phy_rx, _rx,                               \
			      "Current Flags mismatch %d (actual) != %d (expected)",    \
			      _conn.lll.phy_flags, _flags);                            \
	} while (0)

static bool is_instant_reached(struct ll_conn *conn, uint16_t instant)
{
	return ((event_counter(conn) - instant) & 0xFFFF) <= 0x7FFF;
}

/* +-----+                +-------+              +-----+
 * | UT  |                | LL_A  |              | LT  |
 * +-----+                +-------+              +-----+
 *    |                       |                     |
 */
void test_phy_update_central_loc(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	struct pdu_data_llctrl_phy_req req = { .rx_phys = PHY_2M, .tx_phys = PHY_2M };
	struct pdu_data_llctrl_phy_req rsp = { .rx_phys = PHY_1M | PHY_2M,
					       .tx_phys = PHY_1M | PHY_2M };
	struct pdu_data_llctrl_phy_upd_ind ind = { .instant = 7,
						   .c_to_p_phy = PHY_2M,
						   .p_to_c_phy = PHY_2M };
	struct pdu_data_llctrl_length_rsp length_ntf = {
		3 * PDU_DC_PAYLOAD_SIZE_MIN, PDU_DC_MAX_US(3 * PDU_DC_PAYLOAD_SIZE_MIN, PHY_2M),
		3 * PDU_DC_PAYLOAD_SIZE_MIN, PDU_DC_MAX_US(3 * PDU_DC_PAYLOAD_SIZE_MIN, PHY_2M)
	};
	uint16_t instant;

	struct node_rx_pu pu = { .status = BT_HCI_ERR_SUCCESS };

	/* 'Trigger' DLE ntf on PHY update, as this forces change to eff tx/rx times */
	conn.lll.dle.eff.max_rx_time = 0;

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an PHY Update Procedure */
	err = ull_cp_phy_update(&conn, PHY_2M, PREFER_S8_CODING, PHY_2M, HOST_INITIATED);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_REQ, &conn, &tx, &req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_PHY_RSP, &conn, &rsp);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Check that data tx was paused */
	zassert_equal(conn.tx_q.pause_data, 1U, "Data tx is not paused");

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_UPDATE_IND, &conn, &tx, &ind);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Check that data tx is no longer paused */
	zassert_equal(conn.tx_q.pause_data, 0U, "Data tx is paused");

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.phy_upd_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		CHECK_CURRENT_PHY_STATE(conn, PHY_1M, PREFER_S8_CODING, PHY_1M);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be two host notifications, one pu and one dle */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_pdu(LL_LENGTH_RSP, &ntf, &length_ntf);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	CHECK_CURRENT_PHY_STATE(conn, PHY_2M, PREFER_S8_CODING, PHY_2M);
	CHECK_PREF_PHY_STATE(conn, PHY_2M, PHY_2M);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", ctx_buffers_free());
}

void test_phy_update_central_loc_invalid(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct pdu_data_llctrl_phy_req req = { .rx_phys = PHY_2M, .tx_phys = PHY_2M };

	struct pdu_data_llctrl_reject_ind reject_ind = { };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an PHY Update Procedure */
	err = ull_cp_phy_update(&conn, PHY_2M, PREFER_S8_CODING, PHY_2M, HOST_INITIATED);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_REQ, &conn, &tx, &req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_REJECT_IND, &conn, &reject_ind);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Termination 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_LMP_PDU_NOT_ALLOWED,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	/* There should be nohost notifications */
	ut_rx_q_is_empty();

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", ctx_buffers_free());
}

void test_phy_update_central_loc_unsupp_feat(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data_llctrl_phy_req req = { .rx_phys = PHY_2M, .tx_phys = PHY_2M };

	struct pdu_data_llctrl_unknown_rsp unknown_rsp = { .type = PDU_DATA_LLCTRL_TYPE_PHY_REQ };

	struct node_rx_pu pu = { .status = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an PHY Update Procedure */
	err = ull_cp_phy_update(&conn, PHY_2M, PREFER_S8_CODING, PHY_2M, HOST_INITIATED);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_REQ, &conn, &tx, &req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_UNKNOWN_RSP, &conn, &unknown_rsp);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* There should be one host notification */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", ctx_buffers_free());
}

void test_phy_update_central_rem(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	struct pdu_data_llctrl_phy_req req = { .rx_phys = PHY_1M, .tx_phys = PHY_2M };
	struct pdu_data_llctrl_phy_upd_ind ind = { .instant = 7,
						   .c_to_p_phy = 0,
						   .p_to_c_phy = PHY_2M };
	uint16_t instant;

	struct node_rx_pu pu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_PHY_REQ, &conn, &req);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Check that data tx was paused */
	zassert_equal(conn.tx_q.pause_data, 1U, "Data tx is not paused");

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_UPDATE_IND, &conn, &tx, &ind);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Check that data tx is no longer paused */
	zassert_equal(conn.tx_q.pause_data, 0U, "Data tx is paused");

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.phy_upd_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
	CHECK_CURRENT_PHY_STATE(conn, PHY_1M, PREFER_S8_CODING, PHY_2M);
	CHECK_PREF_PHY_STATE(conn, PHY_1M | PHY_2M | PHY_CODED, PHY_1M | PHY_2M | PHY_CODED);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", ctx_buffers_free());
}

void test_phy_update_periph_loc(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data_llctrl_phy_req req = { .rx_phys = PHY_2M, .tx_phys = PHY_2M };
	uint16_t instant;

	struct node_rx_pu pu = { .status = BT_HCI_ERR_SUCCESS };

	struct pdu_data_llctrl_phy_upd_ind phy_update_ind = { .c_to_p_phy = PHY_2M,
							      .p_to_c_phy = PHY_2M };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an PHY Update Procedure */
	err = ull_cp_phy_update(&conn, PHY_2M, PREFER_S8_CODING, PHY_2M, HOST_INITIATED);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_REQ, &conn, &tx, &req);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	phy_update_ind.instant = instant = event_counter(&conn) + 6;
	lt_tx(LL_PHY_UPDATE_IND, &conn, &phy_update_ind);

	/* Done */
	event_done(&conn);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
	CHECK_CURRENT_PHY_STATE(conn, PHY_2M, PREFER_S8_CODING, PHY_2M);
	CHECK_PREF_PHY_STATE(conn, PHY_2M, PHY_2M);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", ctx_buffers_free());
}

void test_phy_update_periph_rem(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data_llctrl_phy_req req = { .rx_phys = PHY_1M, .tx_phys = PHY_2M };
	struct pdu_data_llctrl_phy_req rsp = { .rx_phys = PHY_1M | PHY_2M | PHY_CODED,
					       .tx_phys = PHY_1M | PHY_2M | PHY_CODED };
	struct pdu_data_llctrl_phy_upd_ind ind = { .instant = 7,
						   .c_to_p_phy = 0,
						   .p_to_c_phy = PHY_2M };
	uint16_t instant;

	struct node_rx_pu pu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_PHY_REQ, &conn, &req);

	/* Done */
	event_done(&conn);

	/* We received a REQ, so data tx should be paused */
	zassert_equal(conn.tx_q.pause_data, 1U, "Data tx is not paused");

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_RSP, &conn, &tx, &rsp);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	ind.instant = instant = event_counter(&conn) + 6;
	lt_tx(LL_PHY_UPDATE_IND, &conn, &ind);

	/* We are sending RSP, so data tx should be paused until after tx ack */
	zassert_equal(conn.tx_q.pause_data, 1U, "Data tx is not paused");

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Check that data tx is no longer paused */
	zassert_equal(conn.tx_q.pause_data, 0U, "Data tx is paused");

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	CHECK_CURRENT_PHY_STATE(conn, PHY_2M, PREFER_S8_CODING, PHY_1M);
	CHECK_PREF_PHY_STATE(conn, PHY_1M | PHY_2M | PHY_CODED, PHY_1M | PHY_2M | PHY_CODED);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", ctx_buffers_free());
}

void test_phy_update_periph_rem_invalid(void)
{
	struct node_tx *tx;
	struct pdu_data_llctrl_phy_req req = { .rx_phys = PHY_1M, .tx_phys = PHY_2M };
	struct pdu_data_llctrl_phy_req rsp = { .rx_phys = PHY_1M | PHY_2M | PHY_CODED,
					       .tx_phys = PHY_1M | PHY_2M | PHY_CODED };
	struct pdu_data_llctrl_reject_ind reject_ind = { };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_PHY_REQ, &conn, &req);

	/* Done */
	event_done(&conn);

	/* We received a REQ, so data tx should be paused */
	zassert_equal(conn.tx_q.pause_data, 1U, "Data tx is not paused");

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_RSP, &conn, &tx, &rsp);
	lt_rx_q_is_empty(&conn);

	/* Inject invalid PDU */
	lt_tx(LL_REJECT_IND, &conn, &reject_ind);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Termination 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_LMP_PDU_NOT_ALLOWED,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", ctx_buffers_free());
}

void test_phy_update_central_loc_collision(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	struct pdu_data_llctrl_phy_req req = { .rx_phys = PHY_2M, .tx_phys = PHY_2M };
	struct pdu_data_llctrl_phy_req rsp = { .rx_phys = PHY_1M | PHY_2M,
					       .tx_phys = PHY_1M | PHY_2M };
	struct pdu_data_llctrl_phy_upd_ind ind = { .instant = 9,
						   .c_to_p_phy = PHY_2M,
						   .p_to_c_phy = PHY_2M };
	uint16_t instant;

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_PHY_REQ,
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};

	struct node_rx_pu pu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Emulate valid feature exchange */
	conn.llcp.fex.valid = 1;

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an PHY Update Procedure */
	err = ull_cp_phy_update(&conn, PHY_2M, PREFER_S8_CODING, PHY_2M, HOST_INITIATED);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/*** ***/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_REQ, &conn, &tx, &req);
	lt_rx_q_is_empty(&conn);

	/* Rx - emulate colliding PHY_REQ from peer */
	lt_tx(LL_PHY_REQ, &conn, &req);

	/* Check that data tx is paused */
	zassert_equal(conn.tx_q.pause_data, 1U, "Data tx is not paused");

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Check that data tx is not paused */
	zassert_equal(conn.tx_q.pause_data, 1U, "Data tx is not paused");

	/* Done */
	event_done(&conn);

	/* Check that data tx is not paused */
	zassert_equal(conn.tx_q.pause_data, 1U, "Data tx is not paused");

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/*** ***/

	/* Prepare */
	event_prepare(&conn);
	test_print_conn(&conn);
	/* Tx Queue should have one LL Control PDU */
	printf("Tx REJECT\n");
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	printf("Done again\n");
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/*** ***/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	printf("Empty\n");
	lt_rx_q_is_empty(&conn);

	/* Rx */
	printf("Tx again\n");
	lt_tx(LL_PHY_RSP, &conn, &rsp);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Check that data tx is paused */
	zassert_equal(conn.tx_q.pause_data, 1U, "Data tx is not paused");

	/*** ***/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	printf("And again\n");
	lt_rx(LL_PHY_UPDATE_IND, &conn, &tx, &ind);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Check that data tx is not paused */
	zassert_equal(conn.tx_q.pause_data, 0U, "Data tx is paused");

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.phy_upd_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/*** ***/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", ctx_buffers_free());
}

void test_phy_update_central_rem_collision(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	struct pdu_data_llctrl_phy_req req_peripheral = { .rx_phys = PHY_1M, .tx_phys = PHY_2M };
	struct pdu_data_llctrl_phy_req req_central = { .rx_phys = PHY_2M, .tx_phys = PHY_2M };
	struct pdu_data_llctrl_phy_req rsp = { .rx_phys = PHY_1M | PHY_2M,
					       .tx_phys = PHY_1M | PHY_2M };
	struct pdu_data_llctrl_phy_upd_ind ind_1 = { .instant = 7,
						     .c_to_p_phy = 0,
						     .p_to_c_phy = PHY_2M };
	struct pdu_data_llctrl_phy_upd_ind ind_2 = {
		.instant = 15, .c_to_p_phy = PHY_2M, .p_to_c_phy = 0};
	uint16_t instant;

	struct node_rx_pu pu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/*** ***/

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_PHY_REQ, &conn, &req_peripheral);

	/* Done */
	event_done(&conn);

	/*** ***/

	/* Initiate an PHY Update Procedure */
	err = ull_cp_phy_update(&conn, PHY_2M, PREFER_S8_CODING, PHY_2M, HOST_INITIATED);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/*** ***/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_UPDATE_IND, &conn, &tx, &ind_1);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.phy_upd_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Execute connection event that is an instant. It is required to send notifications to
	 * Host that complete already started PHY update procedure.
	 */

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Start execution of a paused local PHY update procedure. It is delayed by one connection
	 * event due to completion of remote PHY update at end of the "at instant" conneciton event.
	 */

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_REQ, &conn, &tx, &req_central);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_PHY_RSP, &conn, &rsp);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* There should be one host notification */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_UPDATE_IND, &conn, &tx, &ind_2);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.phy_upd_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", ctx_buffers_free());
}

void test_phy_update_periph_loc_collision(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data_llctrl_phy_req req_central = { .rx_phys = PHY_1M, .tx_phys = PHY_2M };
	struct pdu_data_llctrl_phy_req req_peripheral = { .rx_phys = PHY_2M, .tx_phys = PHY_2M };
	struct pdu_data_llctrl_phy_req rsp = { .rx_phys = PHY_2M, .tx_phys = PHY_2M };
	struct pdu_data_llctrl_phy_upd_ind ind = { .instant = 7,
						   .c_to_p_phy = PHY_2M,
						   .p_to_c_phy = PHY_1M };
	uint16_t instant;

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_PHY_REQ,
		.error_code = BT_HCI_ERR_LL_PROC_COLLISION
	};

	struct node_rx_pu pu = { 0 };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/*** ***/

	/* Initiate an PHY Update Procedure */
	err = ull_cp_phy_update(&conn, PHY_2M, PREFER_S8_CODING, PHY_2M, HOST_INITIATED);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_REQ, &conn, &tx, &req_peripheral);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_PHY_REQ, &conn, &req_central);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_RSP, &conn, &tx, &rsp);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* Done */
	event_done(&conn);

	/* There should be one host notification */
	pu.status = BT_HCI_ERR_LL_PROC_COLLISION;
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	ind.instant = instant = event_counter(&conn) + 6;
	lt_tx(LL_PHY_UPDATE_IND, &conn, &ind);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be one host notification */
	pu.status = BT_HCI_ERR_SUCCESS;
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", ctx_buffers_free());
}

void test_phy_update_central_loc_no_act_change(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	struct pdu_data_llctrl_phy_req req = { .rx_phys = PHY_1M, .tx_phys = PHY_1M };
	struct pdu_data_llctrl_phy_req rsp = { .rx_phys = PHY_1M | PHY_2M,
					       .tx_phys = PHY_1M | PHY_2M };
	struct pdu_data_llctrl_phy_upd_ind ind = { .instant = 0, .c_to_p_phy = 0, .p_to_c_phy = 0 };
	uint16_t instant;

	struct node_rx_pu pu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an PHY Update Procedure */
	err = ull_cp_phy_update(&conn, PHY_1M, PREFER_S8_CODING, PHY_1M, HOST_INITIATED);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_REQ, &conn, &tx, &req);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_PHY_RSP, &conn, &rsp);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Check that data tx was paused */
	zassert_equal(conn.tx_q.pause_data, 1U, "Data tx is not paused");

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_UPDATE_IND, &conn, &tx, &ind);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Check that data tx is no longer paused */
	zassert_equal(conn.tx_q.pause_data, 0U, "Data tx is paused");

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.phy_upd_ind.instant);

	/* Check if instant is zero, due to no actual PHY change */
	zassert_equal(instant, 0, "Unexpected instant %d", instant);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* There should be one host notifications, due to host initiated PHY upd */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	CHECK_CURRENT_PHY_STATE(conn, PHY_1M, PREFER_S8_CODING, PHY_1M);
	CHECK_PREF_PHY_STATE(conn, PHY_1M, PHY_1M);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(), "Free CTX buffers %d",
		      ctx_buffers_free());
}

void test_phy_update_central_rem_no_actual_change(void)
{
	struct node_tx *tx;
	struct pdu_data *pdu;
	struct pdu_data_llctrl_phy_req req = { .rx_phys = PHY_1M, .tx_phys = PHY_1M };
	struct pdu_data_llctrl_phy_upd_ind ind = { .instant = 0, .c_to_p_phy = 0, .p_to_c_phy = 0 };
	uint16_t instant;

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_PHY_REQ, &conn, &req);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Check that data tx was paused */
	zassert_equal(conn.tx_q.pause_data, 1U, "Data tx is not paused");

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_UPDATE_IND, &conn, &tx, &ind);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Check that data tx is no longer paused */
	zassert_equal(conn.tx_q.pause_data, 0U, "Data tx is paused");

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.phy_upd_ind.instant);

	/* Check if instant is zero, due to no actual PHY change */
	zassert_equal(instant, 0, "Unexpected instant %d", instant);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* There is not actual PHY change, so there shouldn't be host notification */
	ut_rx_q_is_empty();

	CHECK_CURRENT_PHY_STATE(conn, PHY_1M, PREFER_S8_CODING, PHY_1M);
	CHECK_PREF_PHY_STATE(conn, PHY_1M | PHY_2M | PHY_CODED, PHY_1M | PHY_2M | PHY_CODED);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(), "Free CTX buffers %d",
		      ctx_buffers_free());
}

void test_phy_update_periph_loc_no_actual_change(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data_llctrl_phy_req req = { .rx_phys = PHY_1M, .tx_phys = PHY_1M };
	struct node_rx_pu pu = { .status = BT_HCI_ERR_SUCCESS };
	struct pdu_data_llctrl_phy_upd_ind phy_update_ind = { .instant = 0,
							      .c_to_p_phy = 0,
							      .p_to_c_phy = 0 };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an PHY Update Procedure */
	err = ull_cp_phy_update(&conn, PHY_1M, PREFER_S8_CODING, PHY_1M, HOST_INITIATED);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_REQ, &conn, &tx, &req);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_PHY_UPDATE_IND, &conn, &phy_update_ind);

	/* Done */
	event_done(&conn);

	/* There should be one notification due to Host initiated PHY UPD */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	CHECK_CURRENT_PHY_STATE(conn, PHY_1M, PREFER_S8_CODING, PHY_1M);
	CHECK_PREF_PHY_STATE(conn, PHY_1M, PHY_1M);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(), "Free CTX buffers %d",
		      ctx_buffers_free());
}

void test_phy_update_periph_rem_no_actual_change(void)
{
	struct node_tx *tx;
	struct pdu_data_llctrl_phy_req req = { .rx_phys = PHY_1M, .tx_phys = PHY_1M };
	struct pdu_data_llctrl_phy_req rsp = { .rx_phys = PHY_1M | PHY_2M | PHY_CODED,
					       .tx_phys = PHY_1M | PHY_2M | PHY_CODED };
	struct pdu_data_llctrl_phy_upd_ind ind = { .instant = 0, .c_to_p_phy = 0, .p_to_c_phy = 0 };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_PHY_REQ, &conn, &req);

	/* Done */
	event_done(&conn);

	/* We received a REQ, so data tx should be paused */
	zassert_equal(conn.tx_q.pause_data, 1U, "Data tx is not paused");

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_RSP, &conn, &tx, &rsp);
	lt_rx_q_is_empty(&conn);

	/* Rx */
	lt_tx(LL_PHY_UPDATE_IND, &conn, &ind);

	/* We are sending RSP, so data tx should be paused until after tx ack */
	zassert_equal(conn.tx_q.pause_data, 1U, "Data tx is not paused");

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Check that data tx is no longer paused */
	zassert_equal(conn.tx_q.pause_data, 0U, "Data tx is paused");

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* There should be no host notification */
	ut_rx_q_is_empty();

	CHECK_CURRENT_PHY_STATE(conn, PHY_1M, PREFER_S8_CODING, PHY_1M);
	CHECK_PREF_PHY_STATE(conn, PHY_1M | PHY_2M | PHY_CODED, PHY_1M | PHY_2M | PHY_CODED);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(), "Free CTX buffers %d",
		      ctx_buffers_free());
}

void test_main(void)
{
	ztest_test_suite(
		phy,
		ztest_unit_test_setup_teardown(test_phy_update_central_loc_invalid, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_phy_update_central_loc, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_phy_update_central_loc_unsupp_feat, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_phy_update_central_rem, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_phy_update_periph_loc, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_phy_update_periph_rem, setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_phy_update_periph_rem_invalid, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_phy_update_central_loc_collision, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_phy_update_central_rem_collision, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_phy_update_periph_loc_collision, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_phy_update_central_loc_no_act_change, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_phy_update_central_rem_no_actual_change, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_phy_update_periph_rem_no_actual_change, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_phy_update_periph_loc_no_actual_change, setup,
					       unit_test_noop));

	ztest_run_test_suite(phy);
}
