/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/logging/log.h>

#include "testlib/scan.h"
#include "testlib/conn.h"

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"

/* local includes */
#include "data.h"

LOG_MODULE_REGISTER(central, CONFIG_APP_LOG_LEVEL);

static struct bt_l2cap_le_chan le_chan;

static void sent_cb(struct bt_l2cap_chan *chan)
{
	TEST_FAIL("Tester should not send data");
}

static int recv_cb(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	LOG_DBG("received %d bytes", buf->len);

	return 0;
}

static void l2cap_chan_connected_cb(struct bt_l2cap_chan *chan)
{
	LOG_DBG("%p", chan);
}

static void l2cap_chan_disconnected_cb(struct bt_l2cap_chan *chan)
{
	LOG_DBG("%p", chan);
}

static int server_accept_cb(struct bt_conn *conn, struct bt_l2cap_server *server,
			    struct bt_l2cap_chan **chan)
{
	static struct bt_l2cap_chan_ops ops = {
		.connected = l2cap_chan_connected_cb,
		.disconnected = l2cap_chan_disconnected_cb,
		.recv = recv_cb,
		.sent = sent_cb,
	};

	memset(&le_chan, 0, sizeof(le_chan));
	le_chan.chan.ops = &ops;
	*chan = &le_chan.chan;

	return 0;
}

static int l2cap_server_register(bt_security_t sec_level)
{
	static struct bt_l2cap_server test_l2cap_server = {.accept = server_accept_cb};

	test_l2cap_server.psm = 0;
	test_l2cap_server.sec_level = sec_level;

	int err = bt_l2cap_server_register(&test_l2cap_server);

	TEST_ASSERT(err == 0, "Failed to register l2cap server (err %d)", err);

	return test_l2cap_server.psm;
}

static void acl_connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		LOG_ERR("Failed to connect to %s (0x%02x)", addr, err);
		return;
	}

	LOG_DBG("Connected to %s", addr);
}

static void acl_disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("Disconnected from %s (reason 0x%02x)", addr, reason);
}

/* Read the comments on `entrypoint_dut()` first. */
void entrypoint_central(void)
{
	int err;
	struct bt_conn *conn = NULL;
	bt_addr_le_t dut;
	static struct bt_conn_cb central_cb = {
		.connected = acl_connected,
		.disconnected = acl_disconnected,
	};

	/* Mark test as in progress. */
	TEST_START("central");

	/* Initialize Bluetooth */
	err = bt_conn_cb_register(&central_cb);
	TEST_ASSERT(err == 0, "Can't register callbacks (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	int psm = l2cap_server_register(BT_SECURITY_L1);

	LOG_DBG("Registered server PSM %x", psm);

	/* The device address will not change. Scan only once in order to reduce
	 * test time.
	 */
	err = bt_testlib_scan_find_name(&dut, DUT_NAME);
	TEST_ASSERT(!err, "Failed to start scan (err %d)", err);

	/* DUT will terminate all devices when it's done. Mark the device as
	 * "passed" so bsim doesn't return a nonzero err code when the
	 * termination happens.
	 */
	TEST_PASS("central");

	while (true) {
		/* Create a connection using that address */
		err = bt_testlib_connect(&dut, &conn);
		TEST_ASSERT(!err, "Failed to initiate connection (err %d)", err);

		LOG_DBG("Connected");

		/* Receive in the background */
		k_sleep(K_MSEC(1000));

		/* Disconnect and destroy connection object */
		err = bt_testlib_disconnect(&conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		TEST_ASSERT(!err, "Failed to disconnect (err %d)", err);

		LOG_DBG("Disconnected");

		/* Simulate the central going in and out of range. In the real world, it is unlikely
		 * to drop a connection and re-establish it after only a few milliseconds.
		 */
		k_sleep(K_MSEC(200));
	}
}
