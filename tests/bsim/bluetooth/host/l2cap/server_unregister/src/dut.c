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
#include <testlib/adv.h>
#include <testlib/conn.h>

#include <babblekit/flags.h>
#include <babblekit/testcase.h>

#include "data.h"

LOG_MODULE_REGISTER(dut, LOG_LEVEL_INF);

DEFINE_FLAG(flag_accepted_connected);
DEFINE_FLAG(flag_accepted_disconnected);

static int chan_recv_cb(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	return 0;
}

static void accepted_connected_cb(struct bt_l2cap_chan *chan)
{
	SET_FLAG(flag_accepted_connected);
}

static void accepted_disconnected_cb(struct bt_l2cap_chan *chan)
{
	SET_FLAG(flag_accepted_disconnected);
}

static struct bt_l2cap_le_chan accepted_chan = {
	.chan.ops =
		&(const struct bt_l2cap_chan_ops){
			.connected = accepted_connected_cb,
			.disconnected = accepted_disconnected_cb,
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

void entrypoint_dut(void)
{
	struct bt_conn *conn = NULL;
	int err;

	TEST_START("dut");

	err = bt_id_create(&TEST_DATA_DUT_ADDR, NULL);
	__ASSERT_NO_MSG(!err);

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(!err);

	err = bt_l2cap_server_register(&test_server);
	TEST_ASSERT(err == 0, "register: %d", err);

	err = bt_testlib_adv_conn(&conn, BT_ID_DEFAULT, NULL);
	__ASSERT_NO_MSG(!err);

	/* The tester opens a channel that this server accepts. */
	WAIT_FOR_FLAG(flag_accepted_connected);

	err = bt_l2cap_server_unregister(&test_server);
	TEST_ASSERT(err == -EBUSY, "unregister with an accepted channel: %d", err);

	err = bt_l2cap_chan_disconnect(&accepted_chan.chan);
	TEST_ASSERT(err == 0, "chan disconnect: %d", err);
	WAIT_FOR_FLAG(flag_accepted_disconnected);

	err = bt_l2cap_server_unregister(&test_server);
	TEST_ASSERT(err == 0, "unregister once the channel is gone: %d", err);

	err = bt_l2cap_server_unregister(&test_server);
	TEST_ASSERT(err == -ENOENT, "unregister a second time: %d", err);

	err = bt_l2cap_server_register(&test_server);
	TEST_ASSERT(err == 0, "register again on the same PSM: %d", err);

	/* An outgoing channel carries the same PSM number, but it is not a
	 * channel this server accepted, so it must not block unregistering.
	 */
	err = bt_l2cap_chan_connect(conn, &outgoing_chan.chan, TEST_DATA_L2CAP_PSM);
	TEST_ASSERT(err == 0, "chan connect: %d", err);

	while (!atomic_test_bit(outgoing_chan.chan.status, BT_L2CAP_STATUS_OUT)) {
		k_sleep(K_MSEC(10));
	}

	err = bt_l2cap_server_unregister(&test_server);
	TEST_ASSERT(err == 0, "unregister with only an outgoing channel: %d", err);

	TEST_PASS_AND_EXIT("dut");
}
