/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>

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
#include "ll_feat.h"

#include "lll.h"
#include "lll_df_types.h"
#include "lll_conn.h"
#include "ull_tx_queue.h"

#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_llcp_internal.h"

#include "helper_pdu.h"
#include "helper_util.h"

struct ll_conn test_conn;

static void setup(void)
{
	test_setup(&test_conn);
}

#define LLCTRL_PDU_SIZE (offsetof(struct pdu_data, llctrl) + sizeof(struct pdu_data_llctrl))

/* +-----+ +-------+            +-----+
 * | UT  | | LL_A  |            | LT  |
 * +-----+ +-------+            +-----+
 *    |        |                   |
 *    |        |             <PDU> |
 *    |        |<------------------|
 *    |        |                   |
 */

static void lt_tx_invalid_pdu_size(enum helper_pdu_opcode opcode, int adj_size)
{
	struct pdu_data_llctrl_unknown_rsp unknown_rsp;
	struct pdu_data pdu;
	struct node_tx *tx;
	/* PDU contents does not matter when testing for invalid PDU size */
	uint8_t data[LLCTRL_PDU_SIZE] = { 0 };

	/* Encode a PDU for the opcode */
	encode_pdu(opcode, &pdu, &data);

	/* Setup the LL_UNKNOWN_RSP expected for the PDU */
	if (opcode == LL_ZERO) {
		/* we use 0xff in response if length was 0 */
		unknown_rsp.type = PDU_DATA_LLCTRL_TYPE_UNUSED;
	} else {
		unknown_rsp.type = pdu.llctrl.opcode;
	}

	/* adjust PDU len */
	pdu.len += adj_size;

	/* Connect */
	ull_cp_state_set(&test_conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&test_conn);

	/* Rx */
	lt_tx_no_encode(&pdu, &test_conn, NULL);

	/* Done */
	event_done(&test_conn);

	/* Prepare */
	event_prepare(&test_conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_UNKNOWN_RSP, &test_conn, &tx, &unknown_rsp);
	lt_rx_q_is_empty(&test_conn);

	/* Done */
	event_done(&test_conn);

	/* Release Tx */
	ull_cp_release_tx(&test_conn, tx);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", ctx_buffers_free());
}

void test_invalid_pdu_ignore_rx(void)
{
	/* Role */
	test_set_role(&test_conn, BT_HCI_ROLE_PERIPHERAL);

	/* Test too small PDUs */
	lt_tx_invalid_pdu_size(LL_ZERO, 0); /* 0 length PDU */
	lt_tx_invalid_pdu_size(LL_VERSION_IND, -1);
/*	lt_tx_invalid_pdu_size(LL_LE_PING_REQ, -1); */
/*	lt_tx_invalid_pdu_size(LL_LE_PING_RSP, -1); */
	lt_tx_invalid_pdu_size(LL_FEATURE_REQ, -1);
	lt_tx_invalid_pdu_size(LL_PERIPH_FEAT_XCHG, -1);
	lt_tx_invalid_pdu_size(LL_FEATURE_RSP, -1);
	lt_tx_invalid_pdu_size(LL_MIN_USED_CHANS_IND, -1);
	lt_tx_invalid_pdu_size(LL_REJECT_IND, -1);
	lt_tx_invalid_pdu_size(LL_REJECT_EXT_IND, -1);
	lt_tx_invalid_pdu_size(LL_ENC_REQ, -1);
	lt_tx_invalid_pdu_size(LL_ENC_RSP, -1);
/*	lt_tx_invalid_pdu_size(LL_START_ENC_REQ, -1); 0 length */
/*	lt_tx_invalid_pdu_size(LL_START_ENC_RSP, -1); 0 length */
/*	lt_tx_invalid_pdu_size(LL_PAUSE_ENC_REQ, -1); 0 length */
/*	lt_tx_invalid_pdu_size(LL_PAUSE_ENC_RSP, -1); 0 length */
	lt_tx_invalid_pdu_size(LL_PHY_REQ, -1);
	lt_tx_invalid_pdu_size(LL_PHY_RSP, -1);
	lt_tx_invalid_pdu_size(LL_PHY_UPDATE_IND, -1);
	lt_tx_invalid_pdu_size(LL_UNKNOWN_RSP, -1);
	lt_tx_invalid_pdu_size(LL_CONNECTION_UPDATE_IND, -1);
	lt_tx_invalid_pdu_size(LL_CONNECTION_PARAM_REQ, -1);
	lt_tx_invalid_pdu_size(LL_CONNECTION_PARAM_RSP, -1);
	lt_tx_invalid_pdu_size(LL_TERMINATE_IND, -1);
	lt_tx_invalid_pdu_size(LL_CHAN_MAP_UPDATE_IND, -1);
	lt_tx_invalid_pdu_size(LL_LENGTH_REQ, -1);
	lt_tx_invalid_pdu_size(LL_LENGTH_RSP, -1);
	lt_tx_invalid_pdu_size(LL_CTE_REQ, -1);
/*	lt_tx_invalid_pdu_size(LL_CTE_RSP, -1); 0 length */

	/* Test too big PDUs */
	lt_tx_invalid_pdu_size(LL_VERSION_IND, 1);
	lt_tx_invalid_pdu_size(LL_LE_PING_REQ, 1);
	lt_tx_invalid_pdu_size(LL_LE_PING_RSP, 1);
	lt_tx_invalid_pdu_size(LL_FEATURE_REQ, 1);
	lt_tx_invalid_pdu_size(LL_PERIPH_FEAT_XCHG, 1);
	lt_tx_invalid_pdu_size(LL_FEATURE_RSP, 1);
	lt_tx_invalid_pdu_size(LL_MIN_USED_CHANS_IND, 1);
	lt_tx_invalid_pdu_size(LL_REJECT_IND, 1);
	lt_tx_invalid_pdu_size(LL_REJECT_EXT_IND, 1);
	lt_tx_invalid_pdu_size(LL_ENC_REQ, 1);
	lt_tx_invalid_pdu_size(LL_ENC_RSP, 1);
	lt_tx_invalid_pdu_size(LL_START_ENC_REQ, 1);
	lt_tx_invalid_pdu_size(LL_START_ENC_RSP, 1);
	lt_tx_invalid_pdu_size(LL_PAUSE_ENC_REQ, 1);
	lt_tx_invalid_pdu_size(LL_PAUSE_ENC_RSP, 1);
	lt_tx_invalid_pdu_size(LL_PHY_REQ, 1);
	lt_tx_invalid_pdu_size(LL_PHY_RSP, 1);
	lt_tx_invalid_pdu_size(LL_PHY_UPDATE_IND, 1);
	lt_tx_invalid_pdu_size(LL_UNKNOWN_RSP, 1);
	lt_tx_invalid_pdu_size(LL_CONNECTION_UPDATE_IND, 1);
	lt_tx_invalid_pdu_size(LL_CONNECTION_PARAM_REQ, 1);
	lt_tx_invalid_pdu_size(LL_CONNECTION_PARAM_RSP, 1);
	lt_tx_invalid_pdu_size(LL_TERMINATE_IND, 1);
	lt_tx_invalid_pdu_size(LL_CHAN_MAP_UPDATE_IND, 1);
	lt_tx_invalid_pdu_size(LL_LENGTH_REQ, 1);
	lt_tx_invalid_pdu_size(LL_LENGTH_RSP, 1);
	lt_tx_invalid_pdu_size(LL_CTE_REQ, 1);
	lt_tx_invalid_pdu_size(LL_CTE_RSP, 1);
}

void test_main(void)
{
	ztest_test_suite(invalid,
			 ztest_unit_test_setup_teardown(test_invalid_pdu_ignore_rx, setup,
							unit_test_noop));

	ztest_run_test_suite(invalid);
}
