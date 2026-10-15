/* main_l2cap_server_unregister.c - L2CAP server unregistration */

/*
 * Copyright (c) 2026 Xiaomi Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <testlib/addr.h>
#include <testlib/adv.h>
#include <testlib/conn.h>
#include <testlib/scan.h>

#include <babblekit/flags.h>
#include <babblekit/testcase.h>

#include "bstests.h"

LOG_MODULE_REGISTER(main_l2cap_server_unregister, LOG_LEVEL_INF);

/* The DUT serves both PSMs, so that a lookup has to pick one server out of
 * the list. The tester serves PSM A as well, so that an outgoing channel on
 * the DUT carries the PSM of the DUT's own server without having been
 * accepted by it.
 */
#define TEST_DATA_L2CAP_PSM_A 0x0080U
#define TEST_DATA_L2CAP_PSM_B 0x0081U
#define TEST_DATA_DUT_ADDR    BT_TESTLIB_ADDR_LE_RANDOM_C0_00_00_00_00_(0x01)

static int chan_recv_cb(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	ARG_UNUSED(chan);
	ARG_UNUSED(buf);

	return 0;
}

DEFINE_FLAG_STATIC(flag_accepted_connected);
DEFINE_FLAG_STATIC(flag_accepted_disconnected);

/* What unregistering server B from inside its own accept() returned. */
static int unregister_in_accept;

static void accepted_connected_cb(struct bt_l2cap_chan *chan)
{
	ARG_UNUSED(chan);

	SET_FLAG(flag_accepted_connected);
}

static void accepted_disconnected_cb(struct bt_l2cap_chan *chan)
{
	ARG_UNUSED(chan);

	SET_FLAG(flag_accepted_disconnected);
}

static struct bt_l2cap_le_chan dut_accepted_chan = {
	.chan.ops =
		&(const struct bt_l2cap_chan_ops){
			.connected = accepted_connected_cb,
			.disconnected = accepted_disconnected_cb,
			.recv = chan_recv_cb,
		},
};

static struct bt_l2cap_le_chan dut_outgoing_chan = {
	.chan.ops =
		&(const struct bt_l2cap_chan_ops){
			.recv = chan_recv_cb,
		},
};

static int accept_unexpected_cb(struct bt_conn *conn, struct bt_l2cap_server *server,
				struct bt_l2cap_chan **chan)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(server);
	ARG_UNUSED(chan);

	TEST_FAIL("server A was not supposed to get a connection request");

	return -ENOMEM;
}

static int dut_accept_cb(struct bt_conn *conn, struct bt_l2cap_server *server,
			 struct bt_l2cap_chan **chan)
{
	ARG_UNUSED(conn);

	/* The request being handled holds a reference to this server, so it
	 * cannot be unregistered from here.
	 */
	unregister_in_accept = bt_l2cap_server_unregister(server);

	*chan = &dut_accepted_chan.chan;

	return 0;
}

static struct bt_l2cap_server server_a = {
	.accept = accept_unexpected_cb,
	.psm = TEST_DATA_L2CAP_PSM_A,
};

static struct bt_l2cap_server server_b = {
	.accept = dut_accept_cb,
	.psm = TEST_DATA_L2CAP_PSM_B,
};

void entrypoint_dut(void)
{
	struct bt_conn *conn = NULL;
	int err;

	TEST_START("dut");

	err = bt_id_create(&TEST_DATA_DUT_ADDR, NULL);
	TEST_ASSERT(err == 0, "bt_id_create: %d", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "bt_enable: %d", err);

	err = bt_l2cap_server_register(&server_a);
	TEST_ASSERT(err == 0, "register server A: %d", err);

	err = bt_l2cap_server_register(&server_b);
	TEST_ASSERT(err == 0, "register server B: %d", err);

	err = bt_testlib_adv_conn(&conn, BT_ID_DEFAULT, NULL);
	TEST_ASSERT(err == 0, "adv_conn: %d", err);

	/* The tester opens a channel that server B accepts. */
	WAIT_FOR_FLAG(flag_accepted_connected);

	TEST_ASSERT(unregister_in_accept == -EBUSY, "unregister from accept(): %d",
		    unregister_in_accept);

	err = bt_l2cap_server_unregister(&server_b);
	TEST_ASSERT(err == -EBUSY, "unregister with an accepted channel: %d", err);

	/* Server A has no channels, so it goes even while server B is busy. */
	err = bt_l2cap_server_unregister(&server_a);
	TEST_ASSERT(err == 0, "unregister the idle server: %d", err);

	err = bt_l2cap_server_register(&server_a);
	TEST_ASSERT(err == 0, "register server A again on the same PSM: %d", err);

	err = bt_l2cap_chan_disconnect(&dut_accepted_chan.chan);
	TEST_ASSERT(err == 0, "chan disconnect: %d", err);
	WAIT_FOR_FLAG(flag_accepted_disconnected);

	err = bt_l2cap_server_unregister(&server_b);
	TEST_ASSERT(err == 0, "unregister once the channel is gone: %d", err);

	err = bt_l2cap_server_unregister(&server_b);
	TEST_ASSERT(err == -ENOENT, "unregister a second time: %d", err);

	/* An outgoing channel carries the PSM of server A, but it is not a
	 * channel that server accepted, so it must not block unregistering.
	 */
	err = bt_l2cap_chan_connect(conn, &dut_outgoing_chan.chan, TEST_DATA_L2CAP_PSM_A);
	TEST_ASSERT(err == 0, "chan connect: %d", err);

	while (!atomic_test_bit(dut_outgoing_chan.chan.status, BT_L2CAP_STATUS_OUT)) {
		k_sleep(K_MSEC(10));
	}

	err = bt_l2cap_server_unregister(&server_a);
	TEST_ASSERT(err == 0, "unregister with only an outgoing channel: %d", err);

	TEST_PASS_AND_EXIT("dut");
}

static struct bt_l2cap_le_chan tester_accepted_chan = {
	.chan.ops =
		&(const struct bt_l2cap_chan_ops){
			.recv = chan_recv_cb,
		},
};

static struct bt_l2cap_le_chan tester_outgoing_chan = {
	.chan.ops =
		&(const struct bt_l2cap_chan_ops){
			.recv = chan_recv_cb,
		},
};

static int tester_accept_cb(struct bt_conn *conn, struct bt_l2cap_server *server,
			    struct bt_l2cap_chan **chan)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(server);

	*chan = &tester_accepted_chan.chan;

	return 0;
}

static struct bt_l2cap_server test_server = {
	.accept = tester_accept_cb,
	.psm = TEST_DATA_L2CAP_PSM_A,
};

void entrypoint_tester(void)
{
	struct bt_conn *conn = NULL;
	int err;

	TEST_START("tester");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "bt_enable: %d", err);

	/* The DUT opens a channel to this server at the end of the test. */
	err = bt_l2cap_server_register(&test_server);
	TEST_ASSERT(err == 0, "register: %d", err);

	err = bt_testlib_connect(&TEST_DATA_DUT_ADDR, &conn);
	TEST_ASSERT(err == 0, "connect: %d", err);

	err = bt_l2cap_chan_connect(conn, &tester_outgoing_chan.chan, TEST_DATA_L2CAP_PSM_B);
	TEST_ASSERT(err == 0, "chan connect: %d", err);

	while (!atomic_test_bit(tester_outgoing_chan.chan.status, BT_L2CAP_STATUS_OUT)) {
		k_sleep(K_MSEC(10));
	}

	TEST_PASS("tester");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "dut",
		.test_descr = "DUT L2CAP server unregister",
		.test_main_f = entrypoint_dut,
	},
	{
		.test_id = "tester",
		.test_descr = "Tester L2CAP server unregister",
		.test_main_f = entrypoint_tester,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_main_l2cap_server_unregister_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
