/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>
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

#define INTVL_MIN 6U /* multiple of 1.25 ms (min 6, max 3200) */
#define INTVL_MAX 6U /* multiple of 1.25 ms (min 6, max 3200) */
#define LATENCY 1U
#define TIMEOUT 10U /* multiple of 10 ms (min 10, max 3200) */

#define PREFER_S8_CODING 1
#define PREFER_S2_CODING 0

static struct ll_conn conn;

struct pdu_data_llctrl_conn_update_ind conn_update_ind = { .win_size = 1U,
							   .win_offset = 0U,
							   .interval = INTVL_MAX,
							   .latency = LATENCY,
							   .timeout = TIMEOUT,
							   .instant = 6U };

struct pdu_data_llctrl_conn_param_req conn_param_req_B = {
	.interval_min = INTVL_MIN,
	.interval_max = INTVL_MAX,
	.latency = LATENCY + 1U, /* differentiate parameter */
	.timeout = TIMEOUT + 1U, /* differentiate parameter */
	.preferred_periodicity = 0U,
	.reference_conn_event_count = 0u,
	.offset0 = 0x0000U,
	.offset1 = 0xffffU,
	.offset2 = 0xffffU,
	.offset3 = 0xffffU,
	.offset4 = 0xffffU,
	.offset5 = 0xffffU
};

struct pdu_data_llctrl_conn_param_rsp conn_param_rsp = { .interval_min = INTVL_MIN,
							 .interval_max = INTVL_MAX,
							 .latency = LATENCY,
							 .timeout = TIMEOUT,
							 .preferred_periodicity = 0U,
							 .reference_conn_event_count = 0u,
							 .offset0 = 0x0000U,
							 .offset1 = 0xffffU,
							 .offset2 = 0xffffU,
							 .offset3 = 0xffffU,
							 .offset4 = 0xffffU,
							 .offset5 = 0xffffU };

struct pdu_data_llctrl_conn_param_req *req_B = &conn_param_req_B;

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
	err = ull_cp_phy_update(&conn, PHY_2M, PREFER_S8_CODING, PHY_2M, 1);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

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

	/* Check that data tx is paused */
	zassert_equal(conn.tx_q.pause_data, 1U, "Data tx is paused");

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
	struct pdu_data_llctrl_phy_upd_ind ind_2 = { .instant = 15,
						     .c_to_p_phy = PHY_2M,
						     .p_to_c_phy = 0 };
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
	err = ull_cp_phy_update(&conn, PHY_2M, PREFER_S8_CODING, PHY_2M, 1);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

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

	/*** ***/

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

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
	err = ull_cp_phy_update(&conn, PHY_2M, PREFER_S8_CODING, PHY_2M, 1);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

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

void test_phy_conn_update_central_loc_collision(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct pdu_data *pdu;
	uint16_t instant;

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ,
		.error_code = BT_HCI_ERR_DIFF_TRANS_COLLISION
	};
	struct pdu_data_llctrl_phy_req req = { .rx_phys = PHY_2M, .tx_phys = PHY_2M };
	struct pdu_data_llctrl_phy_req rsp = { .rx_phys = PHY_1M | PHY_2M,
					       .tx_phys = PHY_1M | PHY_2M };
	struct pdu_data_llctrl_phy_upd_ind ind = { .instant = 9,
						   .c_to_p_phy = PHY_2M,
						   .p_to_c_phy = PHY_2M };

	struct node_rx_pu pu = { .status = BT_HCI_ERR_SUCCESS };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Emulate valid feature exchange */
	conn.llcp.fex.valid = 1;

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* (A) Initiate a PHY update procedure */

	err = ull_cp_phy_update(&conn, PHY_2M, PREFER_S8_CODING, PHY_2M, 1);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* (A) Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_REQ, &conn, &tx, &req);
	lt_rx_q_is_empty(&conn);

	/* (B) Rx */
	lt_tx(LL_CONNECTION_PARAM_REQ, &conn, req_B);

	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* (B) Rx Queue should have a REJECT_EXT_IND PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn);

	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/**/

	/* Prepare */
	event_prepare(&conn);

	/* (B) Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* (A) Rx */
	lt_tx(LL_PHY_RSP, &conn, &rsp);

	/* Done */
	event_done(&conn);

	zassert_equal(conn.tx_q.pause_data, 1U, "Data tx is not paused");


	/* Prepare */
	event_prepare(&conn);

	/* (A) Tx Queue should have one LL Control PDU */
	lt_rx(LL_PHY_UPDATE_IND, &conn, &tx, &ind);
	lt_rx_q_is_empty(&conn);
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

		/* (A) Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* (A) There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* (A) Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* (A) There should be one host notification */
	ut_rx_node(NODE_PHY_UPDATE, &ntf, &pu);

	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);
	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", ctx_buffers_free());
}


void test_main(void)
{
	ztest_test_suite(
		collision,
		ztest_unit_test_setup_teardown(test_phy_update_central_loc_collision, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_phy_update_central_rem_collision, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_phy_update_periph_loc_collision, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_phy_conn_update_central_loc_collision, setup,
					       unit_test_noop));

	ztest_run_test_suite(collision);
}
