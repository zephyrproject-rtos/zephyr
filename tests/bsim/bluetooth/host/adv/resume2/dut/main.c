/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic_builtin.h>
#include <zephyr/sys/atomic_types.h>

#include <testlib/conn.h>
#include <testlib/scan.h>

#include <bs_tracing.h>
#include <bstests.h>

extern enum bst_result_t bst_result;

LOG_MODULE_REGISTER(dut, LOG_LEVEL_INF);

atomic_t connected_count;

static void on_connected(struct bt_conn *conn, uint8_t conn_err)
{
	atomic_t count = atomic_inc(&connected_count) + 1;

	LOG_INF("Connected. Current count %d", count);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	atomic_t count = atomic_dec(&connected_count) - 1;

	LOG_INF("Disconnected. Current count %d", count);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = on_connected,
	.disconnected = on_disconnected,
};

static void disconnect_all(void)
{
	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		int err;
		struct bt_conn *conn = bt_testlib_conn_unindex(BT_CONN_TYPE_LE, i);

		if (conn) {
			err = bt_testlib_disconnect(&conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			__ASSERT_NO_MSG(!err);
		}
	}
}

/* 📜:
 *   🚧Setup
 *   ✨Setup / Cleanup ok
 *   👉Test step
 *   ✅Test step passed
 *   🚩Likely triggers problematic behavior
 *   💣Checks for the bad behavior
 *   💥Bad behavior
 *   🧹Clean up
 *   🌈Test complete
 */

int main(void)
{
	int err;
	bt_addr_le_t connectable_addr;
	struct bt_conn *conn = NULL;

	bst_result = In_progress;

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(!err);

	err = bt_set_name("dut");
	__ASSERT_NO_MSG(!err);


	LOG_INF("👉 Preflight test: Advertiser fills connection capacity.");

	/* `bt_le_adv_start` is invoked once, and.. */

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME_AD, NULL, 0, NULL, 0);
	__ASSERT_NO_MSG(!err);

	/* .. the advertiser shall autoresume. Since it's not
	 * stopped, it shall continue receivng connections until
	 * the stack runs out of connection objects.
	 */

	LOG_INF("Waiting for connections...");
	while (atomic_get(&connected_count) < CONFIG_BT_MAX_CONN) {
		k_msleep(1000);
	}

	LOG_INF("✅ Ok");


	LOG_INF("👉 Disconnect one to see that it comes back");

	/* Disconnect one of the connections. It does matter
	 * which, but object with index 0 is chosen for
	 * simplicity.
	 */

	conn = bt_testlib_conn_unindex(BT_CONN_TYPE_LE, 0);
	__ASSERT_NO_MSG(conn);

	/* Disconnect, but wait with unreffing.. */

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	__ASSERT_NO_MSG(!err);
	bt_testlib_wait_disconnected(conn);

	/* Simulate a delayed unref. We delay to make sure the resume is not
	 * triggered by disconnection, but by a connection object becoming
	 * available.
	 */

	k_sleep(K_SECONDS(10));

	bt_testlib_conn_unref(&conn);

	/* Since there is a free connection object again, the
	 * advertiser shall automatically resume and receive a
	 * new connection.
	 */

	LOG_INF("Waiting for connections...");
	while (atomic_get(&connected_count) < CONFIG_BT_MAX_CONN) {
		k_msleep(1000);
	}

	LOG_INF("✅ Ok");


	LOG_INF("🧹 Clean up");

	err = bt_le_adv_stop();
	__ASSERT_NO_MSG(!err);

	disconnect_all();

	LOG_INF("✨ Ok");


	LOG_INF("🚧 Setup: Connect one central connection");

	err = bt_testlib_scan_find_name(&connectable_addr, "connectable");
	__ASSERT_NO_MSG(!err);

	err = bt_testlib_connect(&connectable_addr, &conn);
	__ASSERT_NO_MSG(!err);

	LOG_INF("✅ Ok");


	LOG_INF("🚧 Setup: Start advertiser. Let it fill the connection limit.");

	/* With one connection slot taken by the central role, we fill the rest. */

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME_AD, NULL, 0, NULL, 0);
	__ASSERT_NO_MSG(!err);

	LOG_INF("Waiting for connections...");
	while (atomic_get(&connected_count) < CONFIG_BT_MAX_CONN) {
		k_sleep(K_SECONDS(1));
	}

	LOG_INF("✅ Ok");


	LOG_INF("👉 Main test: Disconnect, wait and connect the central connection.");

	/* In this situation, disconnecting the central role should not allow
	 * the advertiser to resume. This behavior was introduced in 372c8f2d92.
	 */

	LOG_INF("🚩 Disconnect");
	err = bt_testlib_disconnect(&conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	__ASSERT_NO_MSG(!err);

	LOG_INF("🚩 Wait to bait the advertiser");
	k_sleep(K_SECONDS(5));

	LOG_INF("💣 Connect");
	err = bt_testlib_connect(&connectable_addr, &conn);
	if (err) {
		/* If the test fails, it's because the advertiser 'stole' the
		 * central's connection slot.
		 */
		__ASSERT_NO_MSG(err == -ENOMEM);

		LOG_ERR("💥 Advertiser stole the connection slot");
		bs_trace_silent_exit(1);
	}

	LOG_INF("✅ Ok");

	bst_result = Passed;
	LOG_INF("🌈 Test complete");
	bs_trace_silent_exit(0);

	return 0;
}
