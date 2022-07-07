/*
 * Copyright (c) 2022 Demant
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
 *    |        | LL_UNKNOWN_RSP    |
 *    |        |------------------>|
 *    |        |                   |
 */
static void lt_tx_pdu_and_rx_unknown_rsp(enum helper_pdu_opcode opcode)
{
	struct node_tx *tx;
	struct pdu_data pdu;
	/* PDU contents does not matter when testing for invalid PDU opcodes */
	uint8_t data[LLCTRL_PDU_SIZE] = { 0 };

	/* Encode a PDU for the opcode */
	encode_pdu(opcode, &pdu, &data);

	/* Setup the LL_UNKNOWN_RSP expected for the PDU */
	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = pdu.llctrl.opcode
	};

	/* Connect */
	ull_cp_state_set(&test_conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&test_conn);

	/* Rx */
	lt_tx(opcode, &test_conn, &pdu.llctrl.unknown_rsp);

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

static void lt_tx_undef_opcode_and_rx_unknown_rsp(uint8_t opcode)
{
	struct node_tx *tx;
	struct pdu_data pdu;
	/* PDU contents does not matter when testing for invalid PDU opcodes */
	uint8_t data[LLCTRL_PDU_SIZE] = { 0 };

	/* Undefined opcodes cannot be encoded, so encode a LL_UNKNOWN_RSP as
	 * a placeholder and override the opcode
	 */
	encode_pdu(LL_UNKNOWN_RSP, &pdu, &data);
	pdu.llctrl.opcode = opcode;

	/* Setup the LL_UNKNOWN_RSP expected for the PDU */
	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = pdu.llctrl.opcode
	};

	/* Connect */
	ull_cp_state_set(&test_conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&test_conn);

	/* Rx */
	lt_tx_no_encode(&pdu, &test_conn, &pdu.llctrl.unknown_rsp);

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

void test_invalid_per_rem(void)
{
	/* Role */
	test_set_role(&test_conn, BT_HCI_ROLE_PERIPHERAL);

	/* Test opcodes that cannot initiate a remote procedure */
	lt_tx_pdu_and_rx_unknown_rsp(LL_ENC_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_START_ENC_REQ);
	lt_tx_pdu_and_rx_unknown_rsp(LL_START_ENC_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_UNKNOWN_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_FEATURE_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_PAUSE_ENC_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_REJECT_IND);
	lt_tx_pdu_and_rx_unknown_rsp(LL_CONNECTION_PARAM_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_REJECT_EXT_IND);
	lt_tx_pdu_and_rx_unknown_rsp(LL_LE_PING_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_LENGTH_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_PHY_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_PHY_UPDATE_IND);
	lt_tx_pdu_and_rx_unknown_rsp(LL_CTE_RSP);

	/* Test opcodes that can initiate a remote procedure but use the wrong
	 * role
	 */
	lt_tx_pdu_and_rx_unknown_rsp(LL_PERIPH_FEAT_XCHG);
	lt_tx_pdu_and_rx_unknown_rsp(LL_MIN_USED_CHANS_IND);
}

void test_invalid_cen_rem(void)
{
	/* Role */
	test_set_role(&test_conn, BT_HCI_ROLE_CENTRAL);

	/* Test opcodes that cannot initiate a remote procedure */
	lt_tx_pdu_and_rx_unknown_rsp(LL_ENC_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_START_ENC_REQ);
	lt_tx_pdu_and_rx_unknown_rsp(LL_START_ENC_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_UNKNOWN_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_FEATURE_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_PAUSE_ENC_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_REJECT_IND);
	lt_tx_pdu_and_rx_unknown_rsp(LL_CONNECTION_PARAM_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_REJECT_EXT_IND);
	lt_tx_pdu_and_rx_unknown_rsp(LL_LE_PING_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_LENGTH_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_PHY_RSP);
	lt_tx_pdu_and_rx_unknown_rsp(LL_PHY_UPDATE_IND);
	lt_tx_pdu_and_rx_unknown_rsp(LL_CTE_RSP);

	/* Test opcodes that can initiate a remote procedure but use the wrong
	 * role
	 */
	lt_tx_pdu_and_rx_unknown_rsp(LL_CONNECTION_UPDATE_IND);
	lt_tx_pdu_and_rx_unknown_rsp(LL_CHAN_MAP_UPDATE_IND);
	lt_tx_pdu_and_rx_unknown_rsp(LL_ENC_REQ);
	lt_tx_pdu_and_rx_unknown_rsp(LL_FEATURE_REQ);
	lt_tx_pdu_and_rx_unknown_rsp(LL_PAUSE_ENC_REQ);
}

void test_undefined_per_rem(void)
{
	/* Role */
	test_set_role(&test_conn, BT_HCI_ROLE_PERIPHERAL);

	/* Test undefined opcodes that cannot initiate a remote procedure */

	/* BLUETOOTH CORE SPECIFICATION Version 5.3 | Vol 6, Part B, Table 2.18:
	 * LL Control PDU opcodes
	 */
	for (uint16_t opcode = 0x30; opcode < 0x100; opcode++) {
		lt_tx_undef_opcode_and_rx_unknown_rsp(opcode);
	}
}

void test_undefined_cen_rem(void)
{
	/* Role */
	test_set_role(&test_conn, BT_HCI_ROLE_CENTRAL);

	/* Test undefined opcodes that cannot initiate a remote procedure */

	/* BLUETOOTH CORE SPECIFICATION Version 5.3 | Vol 6, Part B, Table 2.18:
	 * LL Control PDU opcodes
	 */
	for (uint16_t opcode = 0x30; opcode < 0x100; opcode++) {
		lt_tx_undef_opcode_and_rx_unknown_rsp(opcode);
	}
}

#ifdef CONFIG_BT_CTLR_LE_ENC
void test_no_enc_per_rem(void)
{
	/* Skip test;
	 * LE Encryption support is available
	 */
	ztest_test_skip();
}
#else
void test_no_enc_per_rem(void)
{
	/* Role */
	test_set_role(&test_conn, BT_HCI_ROLE_PERIPHERAL);

	lt_tx_pdu_and_rx_unknown_rsp(LL_ENC_REQ);
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

void test_no_enc_cen_rem(void)
{
	/* Role */
	test_set_role(&test_conn, BT_HCI_ROLE_CENTRAL);

	lt_tx_pdu_and_rx_unknown_rsp(LL_ENC_REQ);
}

#if defined(CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG)
void test_no_per_feat_exch_per_rem(void)
{
	/* Skip test;
	 * Peripheral-initiated Features Exchange support is available
	 */
	ztest_test_skip();
}
#else
void test_no_per_feat_exch_per_rem(void)
{
	/* Role */
	test_set_role(&test_conn, BT_HCI_ROLE_PERIPHERAL);

	lt_tx_pdu_and_rx_unknown_rsp(LL_PERIPH_FEAT_XCHG);
}
#endif /* CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG */

#if defined(CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG)
void test_no_per_feat_exch_cen_rem(void)
{
	/* Skip test;
	 * Peripheral-initiated Features Exchange support is available
	 */
	ztest_test_skip();
}
#else
void test_no_per_feat_exch_cen_rem(void)
{
	/* Role */
	test_set_role(&test_conn, BT_HCI_ROLE_CENTRAL);

	lt_tx_pdu_and_rx_unknown_rsp(LL_PERIPH_FEAT_XCHG);
}
#endif /* CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG */


#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
void test_no_cpr_per_rem(void)
{
	/* Skip test;
	 * Connection Parameters Request procedure support is available
	 */
	ztest_test_skip();
}
#else
void test_no_cpr_per_rem(void)
{
	/* Role */
	test_set_role(&test_conn, BT_HCI_ROLE_PERIPHERAL);

	lt_tx_pdu_and_rx_unknown_rsp(LL_CONNECTION_PARAM_REQ);
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
void test_no_cpr_cen_rem(void)
{
	/* Skip test;
	 * Connection Parameters Request procedure support is available
	 */
	ztest_test_skip();
}
#else
void test_no_cpr_cen_rem(void)
{
	/* Role */
	test_set_role(&test_conn, BT_HCI_ROLE_CENTRAL);

	lt_tx_pdu_and_rx_unknown_rsp(LL_CONNECTION_PARAM_REQ);
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */


#if defined(CONFIG_BT_CTLR_PHY)
void test_no_phy_per_rem(void)
{
	/* Skip test;
	 * LE 2M PHY support is available
	 */
	ztest_test_skip();
}
#else
void test_no_phy_per_rem(void)
{
	/* Role */
	test_set_role(&test_conn, BT_HCI_ROLE_PERIPHERAL);

	lt_tx_pdu_and_rx_unknown_rsp(LL_PHY_REQ);
}
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_PHY)
void test_no_phy_cen_rem(void)
{
	/* Skip test;
	 * LE 2M PHY support is available
	 */
	ztest_test_skip();
}
#else
void test_no_phy_cen_rem(void)
{
	/* Role */
	test_set_role(&test_conn, BT_HCI_ROLE_CENTRAL);

	lt_tx_pdu_and_rx_unknown_rsp(LL_PHY_REQ);
}
#endif /* CONFIG_BT_CTLR_PHY */



void test_main(void)
{
	ztest_test_suite(invalid,
			 ztest_unit_test_setup_teardown(test_invalid_per_rem, setup,
							unit_test_noop),
			 ztest_unit_test_setup_teardown(test_invalid_cen_rem, setup,
							unit_test_noop));

	ztest_test_suite(undefined,
			 ztest_unit_test_setup_teardown(test_undefined_per_rem, setup,
							unit_test_noop),
			 ztest_unit_test_setup_teardown(test_undefined_cen_rem, setup,
							unit_test_noop));

	ztest_test_suite(unsupported,
			 ztest_unit_test_setup_teardown(test_no_enc_per_rem, setup,
							unit_test_noop),
			 ztest_unit_test_setup_teardown(test_no_enc_cen_rem, setup,
							unit_test_noop),
			 ztest_unit_test_setup_teardown(test_no_per_feat_exch_per_rem, setup,
							unit_test_noop),
			 ztest_unit_test_setup_teardown(test_no_per_feat_exch_cen_rem, setup,
							unit_test_noop),
			 ztest_unit_test_setup_teardown(test_no_cpr_per_rem, setup,
							unit_test_noop),
			 ztest_unit_test_setup_teardown(test_no_cpr_cen_rem, setup,
							unit_test_noop),
			 ztest_unit_test_setup_teardown(test_no_phy_per_rem, setup,
							unit_test_noop),
			 ztest_unit_test_setup_teardown(test_no_phy_cen_rem, setup,
							unit_test_noop)
			 );

	ztest_run_test_suite(invalid);
	ztest_run_test_suite(undefined);
	ztest_run_test_suite(unsupported);
}
