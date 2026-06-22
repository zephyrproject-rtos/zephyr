/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util_macro.h>

#include <testlib/addr.h>
#include <testlib/adv.h>
#include <testlib/conn.h>
#include <testlib/scan.h>

#include <babblekit/flags.h>
#include <babblekit/testcase.h>

#include "data.h"

LOG_MODULE_REGISTER(tester, LOG_LEVEL_INF);

static int tester_chan_recv_cb(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	__ASSERT(false, "Unexpected recv in tester");
	return 0;
};

static struct bt_l2cap_le_chan le_chan = {
	.chan.ops =
		&(const struct bt_l2cap_chan_ops){
			.recv = tester_chan_recv_cb,
		},
};

NET_BUF_POOL_DEFINE(test_pool, 1, BT_L2CAP_SDU_BUF_SIZE(0), CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

void entrypoint_tester(void)
{
	struct net_buf *sdu;
	struct bt_conn *conn = NULL;
	int err;

	TEST_START("tester");

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(!err);

	err = bt_testlib_connect(&TEST_DATA_DUT_ADDR, &conn);
	__ASSERT_NO_MSG(!err);

	err = bt_l2cap_chan_connect(conn, &le_chan.chan, TEST_DATA_L2CAP_PSM);
	__ASSERT_NO_MSG(!err);

	/* Wait for async L2CAP connect */
	while (!atomic_test_bit(le_chan.chan.status, BT_L2CAP_STATUS_OUT)) {
		k_sleep(K_MSEC(100));
	}

	sdu = net_buf_alloc(&test_pool, K_NO_WAIT);
	__ASSERT_NO_MSG(sdu);
	net_buf_reserve(sdu, BT_L2CAP_SDU_CHAN_SEND_RESERVE);

	err = bt_l2cap_chan_send(&le_chan.chan, sdu);
	__ASSERT(!err, "err: %d", err);

	TEST_PASS("tester");
}
