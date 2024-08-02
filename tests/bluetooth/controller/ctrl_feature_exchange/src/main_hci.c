/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

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
#include "ull_llcp_internal.h"
#include "ull_conn_internal.h"

#include "ll_feat.h"

#include "helper_pdu.h"
#include "helper_util.h"
#include "helper_features.h"

static struct ll_conn *conn_from_pool;

static void hci_setup(void *data)
{
	ull_conn_init();

	conn_from_pool = ll_conn_acquire();
	zassert_not_null(conn_from_pool, "Could not allocate connection memory", NULL);

	test_setup(conn_from_pool);
}

/*
 * +-----+                     +-------+            +-----+
 * | UT  |                     | LL_A  |            | LT  |
 * +-----+                     +-------+            +-----+
 *    |                            |                   |
 *    | Start                      |                   |
 *    | Feature Exchange Proc.     |                   |
 *    |--------------------------->|                   |
 *    |                            |                   |
 *    |                            | LL_FEATURE_REQ    |
 *    |                            |------------------>|
 *    |                            |                   |
 *    |                            |    LL_FEATURE_RSP |
 *    |                            |<------------------|
 *    |                            |                   |
 *    |     Feature Exchange Proc. |                   |
 *    |                   Complete |                   |
 *    |<---------------------------|                   |
 *    |                            |                   |
 */
ZTEST(hci_fex, test_hci_feat_exchange_central_loc)
{
	uint64_t err;
	uint64_t set_featureset[] = {
		DEFAULT_FEATURE,
		DEFAULT_FEATURE };
	uint64_t rsp_featureset[] = { ((LL_FEAT_BIT_MASK_VALID & FEAT_FILTER_OCTET0) |
				       DEFAULT_FEATURE) &
					      LL_FEAT_BIT_MASK_VALID,
				      0x0 };
	int feat_to_test = ARRAY_SIZE(set_featureset);

	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	struct pdu_data_llctrl_feature_req local_feature_req;
	struct pdu_data_llctrl_feature_rsp remote_feature_rsp;
	int feat_counter;
	uint16_t conn_handle;

	for (feat_counter = 0; feat_counter < feat_to_test; feat_counter++) {
		conn_handle = ll_conn_handle_get(conn_from_pool);

		sys_put_le64(set_featureset[feat_counter], local_feature_req.features);
		sys_put_le64(rsp_featureset[feat_counter], remote_feature_rsp.features);

		test_set_role(conn_from_pool, BT_HCI_ROLE_CENTRAL);
		/* Connect */
		ull_cp_state_set(conn_from_pool, ULL_CP_CONNECTED);

		/* Initiate a Feature Exchange Procedure via HCI */
		err = ll_feature_req_send(conn_handle);

		zassert_equal(err, BT_HCI_ERR_SUCCESS, "Error: %d", err);

		event_prepare(conn_from_pool);

		/* Tx Queue should have one LL Control PDU */
		lt_rx(LL_FEATURE_REQ, conn_from_pool, &tx, &local_feature_req);
		lt_rx_q_is_empty(conn_from_pool);

		/* Rx */
		lt_tx(LL_FEATURE_RSP, conn_from_pool, &remote_feature_rsp);

		event_done(conn_from_pool);
		/* There should be one host notification */

		ut_rx_pdu(LL_FEATURE_RSP, &ntf, &remote_feature_rsp);

		ut_rx_q_is_empty();

		zassert_equal(conn_from_pool->lll.event_counter, feat_counter + 1,
			      "Wrong event count %d\n", conn_from_pool->lll.event_counter);

		ull_cp_release_tx(conn_from_pool, tx);
		release_ntf(ntf);

		ll_conn_release(conn_from_pool);
	}
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

ZTEST(hci_fex, test_hci_feat_exchange_wrong_handle)
{
	uint16_t conn_handle;
	uint64_t err;
	int ctx_counter;
	struct proc_ctx *ctx;

	conn_handle = ll_conn_handle_get(conn_from_pool);

	err = ll_feature_req_send(conn_handle + 1);

	zassert_equal(err, BT_HCI_ERR_UNKNOWN_CONN_ID, "Wrong reply for wrong handle\n");

	ctx_counter = 0;
	do {
		ctx = llcp_create_local_procedure(PROC_FEATURE_EXCHANGE);
		ctx_counter++;
	} while (ctx != NULL);

	zassert_equal(ctx_counter, CONFIG_BT_CTLR_LLCP_LOCAL_PROC_CTX_BUF_NUM + 1,
				   "Error in setup of test\n");

	err = ll_feature_req_send(conn_handle);
	zassert_equal(err, BT_HCI_ERR_CMD_DISALLOWED, "Wrong reply for wrong handle\n");

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt() - (ctx_counter - 1),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

ZTEST_SUITE(hci_fex, NULL, NULL, hci_setup, NULL, NULL);
