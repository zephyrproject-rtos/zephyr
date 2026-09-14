/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <testlib/conn.h>
#include <testlib/scan.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/printk.h>

#include "babblekit/sync.h"
#include "babblekit/testcase.h"

#define ROUNDS 2U

static struct bt_conn *connect_to_peripheral(void)
{
	struct bt_conn *conn = NULL;
	bt_addr_le_t peer;
	int err;

	err = bt_testlib_scan_find_name(&peer, CONFIG_BT_DEVICE_NAME);
	TEST_ASSERT(err == 0, "Scanning failed (err %d)", err);

	err = bt_testlib_connect(&peer, &conn);
	TEST_ASSERT(err == 0, "Connecting failed (err %d)", err);

	return conn;
}

void test_central_main(void)
{
	struct bt_conn *other_conn;
	struct bt_conn_info info;
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Backchannel init failed (err %d)", err);

	/* The peripheral first advertises with its default identity. That connection has to
	 * stay up while the identities of the following rounds are deleted and reset.
	 */
	other_conn = connect_to_peripheral();

	for (uint32_t round = 0U; round < ROUNDS; round++) {
		struct bt_conn *conn = connect_to_peripheral();

		bt_testlib_wait_disconnected(conn);
		bt_conn_drop(&conn);
		printk("Round %u disconnected\n", round);

		err = bt_conn_get_info(other_conn, &info);
		TEST_ASSERT(err == 0, "bt_conn_get_info failed (err %d)", err);
		TEST_ASSERT(info.state == BT_CONN_STATE_CONNECTED,
			    "The connection to the other identity went down (state %d)",
			    info.state);
	}

	/* Let the peripheral observe its own disconnection before ending the simulation */
	bk_sync_wait();
	TEST_PASS_AND_EXIT("Central done");
}
