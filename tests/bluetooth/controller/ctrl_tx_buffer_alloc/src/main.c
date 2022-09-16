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
#include "lll/lll_df_types.h"
#include "lll_conn.h"

#include "ull_tx_queue.h"

#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_conn_internal.h"
#include "ull_llcp_internal.h"

#include "helper_pdu.h"
#include "helper_util.h"


static struct ll_conn conn[CONFIG_BT_CTLR_LLCP_CONN];

static void setup(void)
{
	ull_conn_init();
	test_setup(&conn[0]);
}
void test_tx_buffer_alloc(void)
{
	struct proc_ctx *ctxs[CONFIG_BT_CTLR_LLCP_CONN];
	struct node_tx *tx[CONFIG_BT_CTLR_LLCP_COMMON_TX_CTRL_BUF_NUM +
		CONFIG_BT_CTLR_LLCP_CONN * CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM + 3];
	uint16_t tx_alloc_idx = 0;
	int i;

	for (int ctx_idx = 0; ctx_idx < CONFIG_BT_CTLR_LLCP_CONN; ctx_idx++) {
		ctxs[ctx_idx] = llcp_create_local_procedure(PROC_VERSION_EXCHANGE);
	}

#if defined(LLCP_TX_CTRL_BUF_QUEUE_ENABLE)
	/* Check alloc flow */
	for (i = 0; i < CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM; i++) {
		zassert_true(llcp_tx_alloc_peek(&conn[0], ctxs[0]), NULL);
		tx[tx_alloc_idx] = llcp_tx_alloc(&conn[0], ctxs[0]);
		zassert_not_null(tx[tx_alloc_idx], NULL);
		tx_alloc_idx++;
	}
	for (i = 0; i < CONFIG_BT_CTLR_LLCP_COMMON_TX_CTRL_BUF_NUM; i++) {
		zassert_true(llcp_tx_alloc_peek(&conn[0], ctxs[0]), NULL);
		tx[tx_alloc_idx] = llcp_tx_alloc(&conn[0], ctxs[0]);
		zassert_not_null(tx[tx_alloc_idx], NULL);
		tx_alloc_idx++;
	}
	zassert_false(llcp_tx_alloc_peek(&conn[0], ctxs[0]), NULL);
	zassert_equal(ctxs[0]->wait_reason, WAITING_FOR_TX_BUFFER, NULL);

	for (int j = 1; j < CONFIG_BT_CTLR_LLCP_CONN; j++) {
		/* Now global pool is exausted, but conn pool is not */
		for (i = 0; i < CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM; i++) {
			zassert_true(llcp_tx_alloc_peek(&conn[j], ctxs[j]), NULL);
			tx[tx_alloc_idx] = llcp_tx_alloc(&conn[j], ctxs[j]);
			zassert_not_null(tx[tx_alloc_idx], NULL);
			tx_alloc_idx++;
		}

		zassert_false(llcp_tx_alloc_peek(&conn[j], ctxs[j]), NULL);
		zassert_equal(ctxs[j]->wait_reason, WAITING_FOR_TX_BUFFER, NULL);
	}

	ull_cp_release_tx(&conn[0], tx[1]);

	/* global pool is now 'open' again, but ctxs[1] is NOT next in line */
	zassert_false(llcp_tx_alloc_peek(&conn[1], ctxs[1]), NULL);

	/* ... ctxs[0] is */
	zassert_true(llcp_tx_alloc_peek(&conn[0], ctxs[0]), NULL);
	tx[tx_alloc_idx] = llcp_tx_alloc(&conn[0], ctxs[0]);
	zassert_not_null(tx[tx_alloc_idx], NULL);
	tx_alloc_idx++;
	ull_cp_release_tx(&conn[0], tx[tx_alloc_idx - 1]);

	/* global pool does not allow as ctxs[2] is NOT next up */
	zassert_false(llcp_tx_alloc_peek(&conn[2], ctxs[2]), NULL);

#if (CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM > 0)
	/* Release conn[2] held tx, to confirm alloc is allowed after releasing pre-alloted buf */
	zassert_true(!(conn[2].llcp.tx_buffer_alloc <
		       CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM), NULL);
	ull_cp_release_tx(&conn[2], tx[CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM +
			  CONFIG_BT_CTLR_LLCP_COMMON_TX_CTRL_BUF_NUM +
			  CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM]);
	zassert_true((conn[2].llcp.tx_buffer_alloc <
		       CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM), NULL);

	/* global pool does not allow as ctxs[2] is not next up, but pre alloted is now avail */
	zassert_equal(ctxs[2]->wait_reason, WAITING_FOR_TX_BUFFER, NULL);
	zassert_not_null(ctxs[2]->wait_node.next, NULL);
	zassert_true(llcp_tx_alloc_peek(&conn[2], ctxs[2]), NULL);
	tx[tx_alloc_idx] = llcp_tx_alloc(&conn[2], ctxs[2]);
	zassert_not_null(tx[tx_alloc_idx], NULL);
	tx_alloc_idx++;

	/* No longer waiting in line */
	zassert_equal(ctxs[2]->wait_reason, WAITING_FOR_NOTHING, NULL);
	zassert_is_null(ctxs[2]->wait_node.next, NULL);
#endif /* (CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM > 0) */

	/* now ctxs[1] is next up */
	zassert_true(llcp_tx_alloc_peek(&conn[1], ctxs[1]), NULL);
	tx[tx_alloc_idx] = llcp_tx_alloc(&conn[0], ctxs[0]);
	zassert_not_null(tx[tx_alloc_idx], NULL);
	tx_alloc_idx++;

#else /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */
	/* Test that there are exactly LLCP_CONN * LLCP_TX_CTRL_BUF_NUM_MAX
	 * buffers available
	 */
	for (i = 0;
	     i < CONFIG_BT_CTLR_LLCP_TX_PER_CONN_TX_CTRL_BUF_NUM_MAX * CONFIG_BT_CTLR_LLCP_CONN;
	     i++) {
		zassert_true(llcp_tx_alloc_peek(&conn[0], ctxs[0]), NULL);
		tx[tx_alloc_idx] = llcp_tx_alloc(&conn[0], ctxs[0]);
		zassert_not_null(tx[tx_alloc_idx], NULL);
		tx_alloc_idx++;
	}
	zassert_false(llcp_tx_alloc_peek(&conn[0], ctxs[0]), NULL);
#endif /* LLCP_TX_CTRL_BUF_QUEUE_ENABLE */
}

void test_main(void)
{
	ztest_test_suite(
		tx_buffer_alloc, ztest_unit_test_setup_teardown(test_tx_buffer_alloc, setup,
								unit_test_noop));

	ztest_run_test_suite(tx_buffer_alloc);
}
