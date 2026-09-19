/* Copyright (c) 2026 Xiaomi Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>

#include <testlib/addr.h>
#include <testlib/conn.h>
#include <testlib/scan.h>

#include <babblekit/flags.h>
#include <babblekit/testcase.h>

#include "data.h"

LOG_MODULE_REGISTER(tester, LOG_LEVEL_INF);

static int chan_recv_cb(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	return 0;
}

static struct bt_l2cap_le_chan accepted_chan = {
	.chan.ops =
		&(const struct bt_l2cap_chan_ops){
			.recv = chan_recv_cb,
		},
};

static struct bt_l2cap_le_chan outgoing_chan = {
	.chan.ops =
		&(const struct bt_l2cap_chan_ops){
			.recv = chan_recv_cb,
		},
};

static int server_accept_cb(struct bt_conn *conn, struct bt_l2cap_server *server,
			    struct bt_l2cap_chan **chan)
{
	*chan = &accepted_chan.chan;

	return 0;
}

static struct bt_l2cap_server test_server = {
	.accept = server_accept_cb,
	.psm = TEST_DATA_L2CAP_PSM,
};

void entrypoint_tester(void)
{
	struct bt_conn *conn = NULL;
	int err;

	TEST_START("tester");

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(!err);

	/* The DUT opens a channel to this server at the end of the test. */
	err = bt_l2cap_server_register(&test_server);
	TEST_ASSERT(err == 0, "register: %d", err);

	err = bt_testlib_connect(&TEST_DATA_DUT_ADDR, &conn);
	__ASSERT_NO_MSG(!err);

	err = bt_l2cap_chan_connect(conn, &outgoing_chan.chan, TEST_DATA_L2CAP_PSM);
	TEST_ASSERT(err == 0, "chan connect: %d", err);

	while (!atomic_test_bit(outgoing_chan.chan.status, BT_L2CAP_STATUS_OUT)) {
		k_sleep(K_MSEC(10));
	}

	TEST_PASS("tester");
}
