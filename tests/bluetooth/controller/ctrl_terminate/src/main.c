/*
 * Copyright (c) 2020 Demant
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
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

#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_conn_internal.h"
#include "ull_llcp_internal.h"

#include "helper_pdu.h"
#include "helper_util.h"

static struct ll_conn conn;

static void term_setup(void *data)
{
	test_setup(&conn);
}

static void test_terminate_rem(uint8_t role)
{
	struct pdu_data_llctrl_terminate_ind remote_terminate_ind = {
		.error_code = 0x05,
	};

	/* Role */
	test_set_role(&conn, role);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_TERMINATE_IND, &conn, &remote_terminate_ind);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Done */
	event_done(&conn);

	/* There should be no host notification */
	ut_rx_q_is_empty();


	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

ZTEST(term_central, test_terminate_central_rem)
{
	test_terminate_rem(BT_HCI_ROLE_CENTRAL);
}

ZTEST(term_periph, test_terminate_periph_rem)
{
	test_terminate_rem(BT_HCI_ROLE_PERIPHERAL);
}

static void test_terminate_loc(uint8_t role)
{
	uint8_t err;
	struct node_tx *tx;

	struct pdu_data_llctrl_terminate_ind local_terminate_ind = {
		.error_code = 0x06,
	};

	/* Role */
	test_set_role(&conn, role);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Initiate an LE Ping Procedure */
	err = ull_cp_terminate(&conn, 0x06);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_TERMINATE_IND, &conn, &tx, &local_terminate_ind);
	lt_rx_q_is_empty(&conn);

	/* RX Ack */
	event_tx_ack(&conn, tx);

	/* Done */
	event_done(&conn);

	/* Release tx node */
	ull_cp_release_tx(&conn, tx);

	/* There should be no host notification */
	ut_rx_q_is_empty();

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

ZTEST(term_central, test_terminate_central_loc)
{
	test_terminate_loc(BT_HCI_ROLE_CENTRAL);
}

ZTEST(term_periph, test_terminate_periph_loc)
{
	test_terminate_loc(BT_HCI_ROLE_PERIPHERAL);
}

ZTEST_SUITE(term_central, NULL, NULL, term_setup, NULL, NULL);
ZTEST_SUITE(term_periph, NULL, NULL, term_setup, NULL, NULL);
