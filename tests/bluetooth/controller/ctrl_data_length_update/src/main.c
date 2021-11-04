/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <sys/byteorder.h>
#include <ztest.h>

#define ULL_LLCP_UNITTEST

#include <bluetooth/hci.h>
#include <sys/byteorder.h>
#include <sys/slist.h>
#include <sys/util.h>
#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"

#include "pdu.h"
#include "ll.h"
#include "ll_feat.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll_df_types.h"
#include "lll_conn.h"

#include "ull_tx_queue.h"
#include "ull_internal.h"
#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_conn_internal.h"
#include "ull_llcp_internal.h"

#include "helper_pdu.h"
#include "helper_util.h"
#include "helper_features.h"

struct ll_conn conn;

static void setup(void)
{
	test_setup(&conn);
}

/*C
 * Locally triggered Data Length Update procedure
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    | Start                      |                              |
 *    | Data Length Update Proc.   |                              |
 *    |--------------------------->|                              |
 *    |                            |  (251,2120,211,1800)         |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |----------------------------->|
 *    |                            |     (201,1720,251,2120)      |
 *    |                            |    LL_DATA_LENGTH_UPDATE_RSP |
 *    |                            |<-----------------------------|
 *    | (251,2120,201,1720)        |                              |
 *    | Data Length Update Proc.   |                              |
 *    |                   Complete |                              |
 *    |<---------------------------|                              |
 *    |                            |                              |
 */

void test_data_length_update_mas_loc(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_length_req local_length_req = { 251, 2120, 211, 1800 };
	struct pdu_data_llctrl_length_rsp remote_length_rsp = { 201, 1720, 251, 2120 };
	struct pdu_data_llctrl_length_rsp length_ntf = { 251, 2120, 201, 1720 };

	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(251);
	ull_conn_default_tx_time_set(2120);
	ull_dle_init(&conn, PHY_1M);

	/* Steal all ntf buffers, so as to check that the wait_ntf mechanism works */
	while (ll_pdu_rx_alloc_peek(1)) {
		ntf = ll_pdu_rx_alloc();
		/* Make sure we use a correct type or the release won't work */
		ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	}

	/* Initiate a Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 211, 1800);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_REQ, &conn, &tx, &local_length_req);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Rx */
	lt_tx(LL_LENGTH_RSP, &conn, &remote_length_rsp);

	event_done(&conn);

	ut_rx_q_is_empty();

	/* Release Ntf, so next cycle will generate NTF and complete procedure */
	ull_cp_release_ntf(ntf);

	event_prepare(&conn);
	event_done(&conn);

	/* There should be one host notification */
	ut_rx_pdu(LL_LENGTH_RSP, &ntf, &length_ntf);
	ut_rx_q_is_empty();
	zassert_equal(conn.lll.event_counter, 2, "Wrong event-count %d\n",
				  conn.lll.event_counter);
}

/*
 * Locally triggered Data Length Update procedure - with no update to eff and thus no ntf
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    | Start                      |                              |
 *    | Data Length Update Proc.   |                              |
 *    |--------------------------->|                              |
 *    |                            |  (251,2120,211,1800)         |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |----------------------------->|
 *    |                            |     (27,328,27,328)          |
 *    |                            |    LL_DATA_LENGTH_UPDATE_RSP |
 *    |                            |<-----------------------------|
 *    |                            |                              |
 */
void test_data_length_update_mas_loc_no_eff_change(void)
{
	uint8_t err;
	struct node_tx *tx;

	struct pdu_data_llctrl_length_req local_length_req = { 251, 2120, 211, 1800 };
	struct pdu_data_llctrl_length_rsp remote_length_rsp = { 27, 328, 27, 328 };

	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(251);
	ull_conn_default_tx_time_set(2120);
	ull_dle_init(&conn, PHY_1M);

	/* Initiate a Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 211, 1800);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_REQ, &conn, &tx, &local_length_req);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Rx */
	lt_tx(LL_LENGTH_RSP, &conn, &remote_length_rsp);

	event_done(&conn);

	/* There should be no host notification */
	ut_rx_q_is_empty();
	zassert_equal(conn.lll.event_counter, 1, "Wrong event-count %d\n",
				  conn.lll.event_counter);
}
/*
 * Locally triggered Data Length Update procedure -
 * - first updating effective DLE and then without update to eff and thus no ntf
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    | Start                      |                              |
 *    | Data Length Update Proc.   |                              |
 *    |--------------------------->|                              |
 *    |                            |  (251,2120,221,1800)         |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |----------------------------->|
 *    |                            |     (101,920,251,2120)       |
 *    |                            |    LL_DATA_LENGTH_UPDATE_RSP |
 *    |                            |<-----------------------------|
 *    | (251,2120,101,920)         |                              |
 *    | Data Length Update Proc.   |                              |
 *    |                   Complete |                              |
 *    |<---------------------------|                              |
 *    |                            |                              |
 *    | Start                      |                              |
 *    | Data Length Update Proc.   |                              |
 *    |--------------------------->|                              |
 *    |                            |  (251,2120,211,1800)         |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |----------------------------->|
 *    |                            |     (101, 920,251,2120)      |
 *    |                            |    LL_DATA_LENGTH_UPDATE_RSP |
 *    |                            |<-----------------------------|
 *    |                            |                              |
 */

void test_data_length_update_mas_loc_no_eff_change2(void)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_length_req local_length_req = { 251, 2120, 211, 1800 };
	struct pdu_data_llctrl_length_rsp remote_length_rsp = { 101, 920, 251, 2120 };
	struct pdu_data_llctrl_length_rsp length_ntf = { 251, 2120, 101, 920 };
	struct pdu_data_llctrl_length_req local_length_req2 = { 251, 2120, 211, 1800 };
	struct pdu_data_llctrl_length_rsp remote_length_rsp2 = { 101, 920, 251, 2120 };

	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(251);
	ull_conn_default_tx_time_set(2120);
	ull_dle_init(&conn, PHY_1M);

	/* Initiate a Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 211, 1800);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_REQ, &conn, &tx, &local_length_req);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Rx */
	lt_tx(LL_LENGTH_RSP, &conn, &remote_length_rsp);

	event_done(&conn);

	/* There should be one host notification */
	ut_rx_pdu(LL_LENGTH_RSP, &ntf, &length_ntf);
	ut_rx_q_is_empty();
	zassert_equal(conn.lll.event_counter, 1, "Wrong event-count %d\n",
				  conn.lll.event_counter);

	/* Now lets generate another DLU, but one that should not result in
	 * change to effective numbers, thus not generate NTF
	 */
	err = ull_cp_data_length_update(&conn, 211, 1800);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_REQ, &conn, &tx, &local_length_req2);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Rx */
	lt_tx(LL_LENGTH_RSP, &conn, &remote_length_rsp2);

	event_done(&conn);

	/* There should be no host notification */
	ut_rx_q_is_empty();
	zassert_equal(conn.lll.event_counter, 2, "Wrong event-count %d\n",
				  conn.lll.event_counter);
}

void test_data_length_update_sla_loc(void)
{
	uint64_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_length_req local_length_req = { 251, 2120, 211, 1800 };
	struct pdu_data_llctrl_length_rsp remote_length_rsp = { 211, 1800, 251, 2120 };
	struct pdu_data_llctrl_length_rsp length_ntf = { 251, 2120, 211, 1800 };

	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);
	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(251);
	ull_conn_default_tx_time_set(2120);
	ull_dle_init(&conn, PHY_1M);

	/* Initiate a Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 211, 1800);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	event_prepare(&conn);
	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_REQ, &conn, &tx, &local_length_req);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	/* Rx */
	lt_tx(LL_LENGTH_RSP, &conn, &remote_length_rsp);

	event_done(&conn);

	/* There should be one host notification */
	ut_rx_pdu(LL_LENGTH_RSP, &ntf, &length_ntf);
	ut_rx_q_is_empty();
	zassert_equal(conn.lll.event_counter, 1, "Wrong event-count %d\n",
				  conn.lll.event_counter);
}

/*
 * Remotely triggered Data Length Update procedure
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    |                            |  (27, 328, 251, 2120)        |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |<-----------------------------|
 *    |                            |   (251, 2120, 211, 1800)     |
 *    |                            |    LL_DATA_LENGTH_UPDATE_RSP |
 *    |                            |----------------------------->|
 *    |  (251,2120,27,328)         |                              |
 *    | Data Length Changed        |                              |
 *    |<---------------------------|                              |
 *    |                            |                              |
 */

void test_data_length_update_mas_rem(void)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_length_req remote_length_req = { 27, 328, 251, 2120 };
	struct pdu_data_llctrl_length_rsp local_length_rsp = { 251, 2120, 211, 1800 };
	struct pdu_data_llctrl_length_rsp length_ntf = { 251, 2120, 27, 328 };

	struct node_rx_pdu *ntf;

	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(211);
	ull_conn_default_tx_time_set(1800);
	ull_dle_init(&conn, PHY_1M);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_tx(LL_LENGTH_REQ, &conn, &remote_length_req);

	event_done(&conn);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_RSP, &conn, &tx, &local_length_rsp);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	event_done(&conn);

	ut_rx_pdu(LL_LENGTH_RSP, &ntf, &length_ntf);
	ut_rx_q_is_empty();
}

/*
 * Remotely triggered Data Length Update procedure
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    |                            | (27, 328, 201, 1720)         |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |<-----------------------------|
 *    |                            |                              |
 *    |                            |     (251, 2120, 211, 1800)   |
 *    |                            |    LL_DATA_LENGTH_UPDATE_RSP |
 *    |                            |----------------------------->|
 *    |  (201,1720,27,328)         |                              |
 *    | Data Length Changed        |                              |
 *    |<---------------------------|                              |
 *    |                            |                              |
 */

void test_data_length_update_sla_rem(void)
{
	struct node_tx *tx;

	struct pdu_data_llctrl_length_req remote_length_req = { 27, 328, 201, 1720 };
	struct pdu_data_llctrl_length_rsp local_length_rsp = { 251, 2120, 211, 1800 };
	struct pdu_data_llctrl_length_rsp length_ntf = { 201, 1720, 27, 328 };
	struct node_rx_pdu *ntf;

	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(211);
	ull_conn_default_tx_time_set(1800);
	ull_dle_init(&conn, PHY_1M);

	/* Steal all ntf buffers, so as to check that the wait_ntf mechanism works */
	while (ll_pdu_rx_alloc_peek(1)) {
		ntf = ll_pdu_rx_alloc();
		/* Make sure we use a correct type or the release won't work */
		ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	}

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_tx(LL_LENGTH_REQ, &conn, &remote_length_req);

	event_done(&conn);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_RSP, &conn, &tx, &local_length_rsp);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	event_done(&conn);
	ut_rx_q_is_empty();

	/* Release Ntf, so next cycle will generate NTF and complete procedure */
	ull_cp_release_ntf(ntf);

	event_prepare(&conn);
	event_done(&conn);

	ut_rx_pdu(LL_LENGTH_RSP, &ntf, &length_ntf);
	ut_rx_q_is_empty();
}

/*
 * Remotely triggered Data Length Update procedure with local request piggy back
 *
 * +-----+                     +-------+                       +-----+
 * | UT  |                     | LL_A  |                       | LT  |
 * +-----+                     +-------+                       +-----+
 *    |                            |                              |
 *    |                            | (27, 328, 211, 1800)         |
 *    |                            | LL_DATA_LENGTH_UPDATE_REQ    |
 *    |                            |<-----------------------------|
 *    | Start                      |                              |
 *    | Data Length Update Proc.   |                              |
 *    |--------------------------->|                              |
 *    |                            |                              |
 *    |                            |   (251, 2120, 211, 1800)     |
 *    |                            |  LL_DATA_LENGTH_UPDATE_RSP   |
 *    |                            |----------------------------->|
 *    |  (211,1800,27,328)         |                              |
 *    | Data Length Changed        |                              |
 *    |<---------------------------|                              |
 *    |                            |                              |
 */

void test_data_length_update_sla_rem_and_loc(void)
{
	uint64_t err;
	struct node_tx *tx;
	struct proc_ctx *ctx = NULL;

	struct pdu_data_llctrl_length_req remote_length_req = { 27, 328, 211, 1800 };
	struct pdu_data_llctrl_length_rsp local_length_rsp = { 251, 2120, 211, 1800 };
	struct pdu_data_llctrl_length_rsp length_ntf = { 211, 1800, 27, 328 };
	struct node_rx_pdu *ntf;

	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	/* Init DLE data */
	ull_conn_default_tx_octets_set(211);
	ull_conn_default_tx_time_set(1800);
	ull_dle_init(&conn, PHY_1M);

	/* Allocate dummy procedure used to steal all buffers */
	ctx = llcp_create_local_procedure(PROC_VERSION_EXCHANGE);

	/* Steal all tx buffers */
	while (llcp_tx_alloc_peek(&conn, ctx)) {
		tx = llcp_tx_alloc(&conn, ctx);
		zassert_not_null(tx, NULL);
	}

	/* Dummy remove, as above loop might queue up ctx */
	llcp_tx_alloc_unpeek(ctx);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_tx(LL_LENGTH_REQ, &conn, &remote_length_req);

	event_done(&conn);

	event_prepare(&conn);

	/* Tx Queue should have no LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Initiate a Data Length Update Procedure */
	err = ull_cp_data_length_update(&conn, 211, 1800);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	event_done(&conn);

	ull_cp_release_tx(&conn, tx);

	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_LENGTH_RSP, &conn, &tx, &local_length_rsp);
	lt_rx_q_is_empty(&conn);

	/* TX Ack */
	event_tx_ack(&conn, tx);

	event_done(&conn);

	ut_rx_pdu(LL_LENGTH_RSP, &ntf, &length_ntf);
	ut_rx_q_is_empty();
}

void test_data_length_update_dle_max_time_get(void)
{
	uint16_t max_time = 0xffff;
	uint16_t max_octets = 211;

#ifdef CONFIG_BT_CTLR_PHY
	max_time = 2120;
#endif
	conn.llcp.fex.valid = 0;

	ull_dle_local_tx_update(&conn, max_octets, max_time);

#ifdef CONFIG_BT_CTLR_PHY
#ifdef CONFIG_BT_CTLR_PHY_CODED
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 2120, "max_tx_time mismatch.\n");
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 2120, "max_tx_time mismatch.\n");
#endif
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 1800, "max_tx_time mismatch.\n");
#endif

	/* Emulate complete feat exch without CODED */
	conn.llcp.fex.valid = 1;
	conn.llcp.fex.features_used = 0;
	ull_dle_local_tx_update(&conn, max_octets, max_time);

#ifdef CONFIG_BT_CTLR_PHY
#ifdef CONFIG_BT_CTLR_PHY_CODED
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 2120, "max_tx_time mismatch.\n");
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 2120, "max_tx_time mismatch.\n");
#endif
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 1800, "max_tx_time mismatch.\n");
#endif

	/* Check the case of CODED PHY support */
	conn.llcp.fex.features_used = LL_FEAT_BIT_PHY_CODED;
	ull_dle_local_tx_update(&conn, max_octets, max_time);

#ifdef CONFIG_BT_CTLR_PHY
#ifdef CONFIG_BT_CTLR_PHY_CODED
	zassert_equal(conn.lll.dle.local.max_rx_time, 17040, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 2120, "max_tx_time mismatch.\n");
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 2120, "max_tx_time mismatch.\n");
#endif
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 1800, "max_tx_time mismatch.\n");
#endif

	/* Finally check that MAX on max_tx_time works */
	max_time = 20000;
	ull_dle_local_tx_update(&conn, max_octets, max_time);

#ifdef CONFIG_BT_CTLR_PHY
#ifdef CONFIG_BT_CTLR_PHY_CODED
	zassert_equal(conn.lll.dle.local.max_rx_time, 17040, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 17040, "max_tx_time mismatch.\n");
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 2120, "max_tx_time mismatch.\n");
#endif
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 1800, "max_tx_time mismatch.\n");
#endif

	/* Check that MIN works */
	max_time = 20;
	max_octets = 2;
	ull_dle_local_tx_update(&conn, max_octets, max_time);

#ifdef CONFIG_BT_CTLR_PHY
#ifdef CONFIG_BT_CTLR_PHY_CODED
	zassert_equal(conn.lll.dle.local.max_rx_time, 17040, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 328, "max_tx_time mismatch.\n");
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 328, "max_tx_time mismatch.\n");
#endif
#else
	zassert_equal(conn.lll.dle.local.max_rx_time, 2120, "max_rx_time mismatch.\n");
	zassert_equal(conn.lll.dle.local.max_tx_time, 328, "max_tx_time mismatch.\n");
#endif
}

void test_main(void)
{
	ztest_test_suite(
		data_length_update_master,
		ztest_unit_test_setup_teardown(test_data_length_update_mas_loc, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_data_length_update_mas_loc_no_eff_change, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_data_length_update_mas_loc_no_eff_change2,
					       setup, unit_test_noop),
		ztest_unit_test_setup_teardown(test_data_length_update_mas_rem, setup,
					       unit_test_noop));

	ztest_test_suite(data_length_update_slave,
			 ztest_unit_test_setup_teardown(test_data_length_update_sla_loc, setup,
							unit_test_noop),
			 ztest_unit_test_setup_teardown(test_data_length_update_sla_rem, setup,
							unit_test_noop),
			 ztest_unit_test_setup_teardown(test_data_length_update_sla_rem_and_loc,
							setup, unit_test_noop)
						    );

	ztest_test_suite(data_length_update_util,
			 ztest_unit_test_setup_teardown(test_data_length_update_dle_max_time_get,
							setup, unit_test_noop));

	ztest_run_test_suite(data_length_update_master);
	ztest_run_test_suite(data_length_update_slave);
	ztest_run_test_suite(data_length_update_util);
}
