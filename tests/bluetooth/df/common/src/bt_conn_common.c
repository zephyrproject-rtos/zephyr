/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <stddef.h>
#include <ztest.h>

#include <util/util.h>
#include <util/memq.h>
#include <util/dbuf.h>

#include <hal/ccm.h>

#include <zephyr/bluetooth/hci.h>
#include <pdu.h>
#include <lll.h>
#include <lll/lll_df_types.h>
#include <lll_conn.h>

#include <ull_tx_queue.h>
#include <ull_conn_types.h>
#include <ull_conn_internal.h>

#define PEER_FEATURES_ARE_VALID 1U

uint16_t ut_bt_create_connection(void)
{
	struct ll_conn *conn;

	conn = ll_conn_acquire();
	zassert_not_equal(conn, NULL, "Failed acquire ll_conn instance");

	conn->lll.latency = 0;
	conn->lll.handle = ll_conn_handle_get(conn);

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
	conn->lll.df_rx_cfg.is_initialized = 0U;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	conn->llcp.cte_req.is_enabled = 0U;

	conn->llcp.fex.features_used |= BIT(BT_LE_FEAT_BIT_CONN_CTE_REQ);
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */

	return conn->lll.handle;
}

void ut_bt_destroy_connection(uint16_t handle)
{
	struct ll_conn *conn;

	conn = ll_conn_get(handle);
	zassert_not_equal(conn, NULL, "Failed ll_conn instance for given handle");

	ll_conn_release(conn);
}

void ut_bt_set_peer_features(uint16_t conn_handle, uint64_t features)
{
	struct ll_conn *conn;

	conn = ll_conn_get(conn_handle);
	zassert_not_equal(conn, NULL, "Failed ll_conn instance for given handle");

	conn->llcp.fex.valid = PEER_FEATURES_ARE_VALID;
	conn->llcp.fex.features_peer = features;
}

void ut_bt_set_periph_latency(uint16_t conn_handle, uint16_t periph_latency)
{
	struct ll_conn *conn;

	conn = ll_conn_get(conn_handle);
	zassert_not_equal(conn, NULL, "Failed ll_conn instance for given handle");

	conn->lll.latency = periph_latency;
}
