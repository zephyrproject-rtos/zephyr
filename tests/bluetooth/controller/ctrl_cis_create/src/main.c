/*
 * Copyright (c) 2022 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include "kconfig.h"

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
#include "lll_conn_iso.h"
#include "lll_conn.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_types.h"
#include "ull_conn_iso_types.h"
#include "ull_llcp.h"
#include "ull_conn_internal.h"
#include "ull_llcp_internal.h"

#include "helper_pdu.h"
#include "helper_util.h"

struct ll_conn conn;

static void setup(void)
{
	test_setup(&conn);
}

static bool is_instant_reached(struct ll_conn *conn, uint16_t instant)
{
	return ((event_counter(conn) - instant) & 0xFFFF) <= 0x7FFF;
}

static struct pdu_data_llctrl_cis_req remote_cis_req = {
	.cig_id           =   0x01,
	.cis_id           =   0x02,
	.c_phy            =   0x01,
	.p_phy            =   0x01,
	.c_max_sdu_packed =   { 0, 160},
	.p_max_sdu        =   { 0, 160},
	.c_max_pdu        =   160,
	.p_max_pdu        =   160,
	.nse              =   2,
	.p_bn             =   1,
	.c_bn             =   1,
	.c_ft             =   1,
	.p_ft             =   1,
	.iso_interval     =   6,
	.conn_event_count =   12,
	.c_sdu_interval   =   { 0, 0, 0},
	.p_sdu_interval   =   { 0, 0, 0},
	.sub_interval     =   { 0, 0, 0},
	.cis_offset_min   =   { 0, 0, 0},
	.cis_offset_max   =   { 0, 0, 0}
};

static struct pdu_data_llctrl_cis_ind remote_cis_ind = {
	.aa = { 0, 0, 0, 0},
	.cig_sync_delay = { 0, 0, 0},
	.cis_offset = { 0, 0, 0},
	.cis_sync_delay = { 0, 0, 0},
	.conn_event_count = 12
};

#define ERROR_CODE 0x17
/*
 * Central-initiated CIS Create procedure.
 * Central requests CIS, peripheral Host rejects.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_S  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CIS_REQ              |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |      LE CIS Request       |                           |
 *    |<--------------------------|                           |
 *    | LE CIS Request            |                           |
 *    | Accept                    |                           |
 *    |-------------------------->|                           |
 *    |                           |                           |
 *    |                           | LL_CIS_RSP                |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |   LL_CIS_IND              |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE CIS ESTABLISHED   |                           |
 *    |<--------------------------|                           |
 */
static void test_cc_create_periph_rem_host_accept(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct node_rx_conn_iso_req cis_req = {
		.cig_id = 0x01,
		.cis_handle = 0x00,
		.cis_id = 0x02
	};
	struct pdu_data_llctrl_cis_rsp local_cis_rsp = {
		.cis_offset_max = { 0, 0, 0},
		.cis_offset_min = { 0, 0, 0},
		.conn_event_count = 12
	};
	struct node_rx_conn_iso_estab cis_estab = {
		.cis_handle = 0x00,
		.status = 0x00
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CIS_REQ, &conn, &remote_cis_req);

	/* Done */
	event_done(&conn);

	/* There should be excactly one host notification */
	ut_rx_node(NODE_CIS_REQUEST, &ntf, &cis_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Accept request */
	ull_cp_cc_accept(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CIS_RSP, &conn, &tx, &local_cis_rsp);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Rx */
	lt_tx(LL_CIS_IND, &conn, &remote_cis_ind);

	/* Done */
	event_done(&conn);

	/* */
	while (!is_instant_reached(&conn, remote_cis_ind.conn_event_count)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Emulate CIS becoming established */
	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should NOT be a host notification */
	ut_rx_q_is_empty();

	/* Prepare */
	event_prepare(&conn);

	/* There should be excactly one host notification */
	ut_rx_node(NODE_CIS_ESTABLISHED, &ntf, &cis_estab);
	ut_rx_q_is_empty();

	/* Done */
	event_done(&conn);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", ctx_buffers_free());
}

/*
 * Central-initiated CIS Create procedure.
 * Central requests CIS, peripheral Host rejects.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_S  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CIS_REQ              |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |      LE CIS Request       |                           |
 *    |<--------------------------|                           |
 *    | LE CIS Request            |                           |
 *    | Decline                   |                           |
 *    |-------------------------->|                           |
 *    |                           |                           |
 *    |                           | LL_REJECT_EXT_IND         |
 *    |                           |-------------------------->|
 *    |                           |                           |
 */
static void test_cc_create_periph_rem_host_reject(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct node_rx_conn_iso_req cis_req = {
		.cig_id = 0x01,
		.cis_handle = 0x00,
		.cis_id = 0x02
	};
	struct pdu_data_llctrl_reject_ext_ind local_reject = {
		.error_code = ERROR_CODE,
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CIS_REQ
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CIS_REQ, &conn, &remote_cis_req);

	/* Done */
	event_done(&conn);

	/* There should be excactly one host notification */
	ut_rx_node(NODE_CIS_REQUEST, &ntf, &cis_req);
	ut_rx_q_is_empty();

	/* Decline request */
	ull_cp_cc_reject(&conn, ERROR_CODE);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &local_reject);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", ctx_buffers_free());
}

/*
 * Central-initiated CIS Create procedure.
 * Central requests CIS, ask peripheral host peripheral Host fails to reply
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_S  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CIS_REQ              |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |      LE CIS Request       |                           |
 *    |<--------------------------|                           |
 *    |                           |                           |
 *
 *
 *			Let time pass ......
 *
 *
 *    |                           |                           |
 *    |                           | LL_REJECT_EXT_IND (to)    |
 *    |                           |-------------------------->|
 *    |                           |                           |
 */
static void test_cc_create_periph_rem_host_accept_to(void)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct node_rx_conn_iso_req cis_req = {
		.cig_id = 0x01,
		.cis_handle = 0x00,
		.cis_id = 0x02
	};
	struct pdu_data_llctrl_reject_ext_ind local_reject = {
		.error_code = BT_HCI_ERR_CONN_ACCEPT_TIMEOUT,
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CIS_REQ
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CIS_REQ, &conn, &remote_cis_req);

	/* Done */
	event_done(&conn);

	/* There should be excactly one host notification */
	ut_rx_node(NODE_CIS_REQUEST, &ntf, &cis_req);
	ut_rx_q_is_empty();

	/* Emulate that time passes real fast re. timeout */
	conn.connect_accept_to = 0;

	/* Prepare */
	event_prepare(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should now have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &local_reject);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", ctx_buffers_free());
}

/*
 * Central-initiated CIS Create procedure.
 * Central requests CIS w. invalid PHY param, peripheral Host rejects.
 *
 * +-----+          +-------+                       +-----+
 * | UT  |          | LL_S  |                       | LT  |
 * +-----+          +-------+                       +-----+
 *    |                 |                               |
 *    |                 |   LL_CIS_REQ  (w. invald PHY) |
 *    |                 |<------------------------------|
 *    |                 |                               |
 *    |                 |                               |
 *    |                 |                               |
 *    |                 |                               |
 *    |                 |                               |
 *    |                 |                               |
 *    |                 | LL_REJECT_EXT_IND             |
 *    |                 |------------------------------>|
 *    |                 |                               |
 */
static void test_cc_create_periph_rem_invalid_phy(void)
{
	static struct pdu_data_llctrl_cis_req remote_cis_req_invalid_phy = {
		.cig_id           =   0x01,
		.cis_id           =   0x02,
		.c_phy            =   0x03,
		.p_phy            =   0x01,
		.c_max_sdu_packed =   { 0, 160},
		.p_max_sdu        =   { 0, 160},
		.c_max_pdu        =   160,
		.p_max_pdu        =   160,
		.nse              =   2,
		.p_bn             =   1,
		.c_bn             =   1,
		.c_ft             =   1,
		.p_ft             =   1,
		.iso_interval     =   6,
		.conn_event_count =   12,
		.c_sdu_interval   =   { 0, 0, 0},
		.p_sdu_interval   =   { 0, 0, 0},
		.sub_interval     =   { 0, 0, 0},
		.cis_offset_min   =   { 0, 0, 0},
		.cis_offset_max   =   { 0, 0, 0}
	};
	struct node_tx *tx;
	struct pdu_data_llctrl_reject_ext_ind local_reject = {
		.error_code = BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL,
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CIS_REQ
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CIS_REQ, &conn, &remote_cis_req_invalid_phy);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &local_reject);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	zassert_equal(ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", ctx_buffers_free());
}

void test_main(void)
{
	ztest_test_suite(
		cis_create,
		ztest_unit_test_setup_teardown(test_cc_create_periph_rem_host_accept, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_cc_create_periph_rem_host_reject, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_cc_create_periph_rem_host_accept_to, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_cc_create_periph_rem_invalid_phy, setup,
					       unit_test_noop));

	ztest_run_test_suite(cis_create);
}
