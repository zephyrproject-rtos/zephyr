/*
 * Copyright (c) 2026 Kent Hauser
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Builds the controller with RX, TX and connection meta data enabled, and checks that pdu[] lands
 * on the test descriptors' data.
 */

#include <stddef.h>

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "util/util.h"
#include "util/memq.h"
#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"
#include "lll.h"
#include "lll_conn.h"

BUILD_ASSERT(offsetof(struct node_tx, pdu) ==
	     offsetof(struct node_tx, tx_pdu_meta) + offsetof(lll_tx_pdu_meta_t, data));
BUILD_ASSERT(offsetof(struct node_rx_pdu, pdu) ==
	     offsetof(struct node_rx_pdu, rx_pdu_meta) + offsetof(lll_rx_pdu_meta_t, data));
/* pdu[] is 2-byte aligned but not 4-byte aligned, as the test descriptors place it */
BUILD_ASSERT((offsetof(struct node_tx, pdu) % 4U) == 2U);
BUILD_ASSERT((offsetof(struct node_rx_pdu, pdu) % 4U) == 2U);

void lll_conn_meta_init(struct lll_conn *lll)
{
	lll->conn_meta.initialized = 1U;
}

ZTEST_SUITE(test_bluetooth, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_bluetooth, test_ctrl_pdu_meta)
{
	zassert_false(bt_enable(NULL), "Bluetooth ctrl_pdu_meta failed");
}
