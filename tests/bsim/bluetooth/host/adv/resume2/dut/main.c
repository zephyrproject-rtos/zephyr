/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bstests.h>
#include <testlib/conn.h>
#include <testlib/scan.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic_builtin.h>
#include <zephyr/sys/atomic_types.h>

LOG_MODULE_REGISTER(dut, LOG_LEVEL_INF);

atomic_t connected_count;

extern enum bst_result_t bst_result;

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

static void disconnect(struct bt_conn *conn, void *user_data)
{
	int err;

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	__ASSERT_NO_MSG(!err);
}

int main(void)
{
	int err;
	bt_addr_le_t connectable_addr;
	struct bt_conn *conn = NULL;

	bst_result = In_progress;

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(!err);

	LOG_INF("👉 Preflight test: Advertiser restarts to fill connection capacity.");

	err = bt_set_name("dut");
	__ASSERT_NO_MSG(!err);

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME_AD, NULL, 0, NULL, 0);
	__ASSERT_NO_MSG(!err);

	LOG_INF("Waiting for connections...");
	while (atomic_get(&connected_count) < CONFIG_BT_MAX_CONN) {
		k_msleep(1000);
	}

	LOG_INF("✅ Ok");
	LOG_INF("🧹 Clean up");

	err = bt_le_adv_stop();
	__ASSERT_NO_MSG(!err);

	bt_conn_foreach(BT_CONN_TYPE_LE, disconnect, NULL);

	LOG_INF("Waiting for disconnections...");
	while (atomic_get(&connected_count) != 0) {
		k_msleep(1000);
	}

	LOG_INF("✨ Ok");

	LOG_INF("🚧 Setup: Connect one central connection");

	err = bt_testlib_scan_find_name(&connectable_addr, "connectable");
	__ASSERT_NO_MSG(!err);

	err = bt_testlib_connect(&connectable_addr, &conn);
	__ASSERT_NO_MSG(!err);

	LOG_INF("✅ Ok");

	LOG_INF("🚧 Setup: Start advertiser. Let it fill the connection limit.");

	err = bt_set_name("dut");
	__ASSERT_NO_MSG(!err);

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME_AD, NULL, 0, NULL, 0);
	__ASSERT_NO_MSG(!err);

	LOG_INF("Waiting for connections...");
	while (atomic_get(&connected_count) < CONFIG_BT_MAX_CONN) {
		k_msleep(1000);
	}

	LOG_INF("✅ Ok");

	LOG_INF("👉 Main test: Disconnect, wait and connect the central connection.");

	LOG_INF("💣 Disconnect");
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	bt_testlib_wait_disconnected(conn);
	bt_testlib_conn_unref(&conn);

	LOG_INF("💣 Wait to bait the advertiser");
	k_msleep(5000);

	LOG_INF("❓ Connect");
	err = bt_testlib_connect(&connectable_addr, &conn);
	__ASSERT_NO_MSG(!err);
	LOG_INF("✅ Ok");

	bst_result = Passed;
	LOG_INF("🌈 Test complete");

	return 0;
}
