/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bs_tracing.h>
#include <bstests.h>
#include <stdint.h>
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

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	atomic_t count = atomic_dec(&connected_count) - 1;

	LOG_INF("Disconnected. Current count %d", count);
}

/* Advertiser */

static K_MUTEX_DEFINE(globals_lock);
/* Initialized to zero, which means restarting is disabled. */
uint8_t target_peripheral_count;

static void restart_work_handler(struct k_work *work);
struct k_work restart_work = {.handler = restart_work_handler};

static void on_connected(struct bt_conn *conn, uint8_t conn_err)
{
	atomic_t count = atomic_inc(&connected_count) + 1;

	k_work_submit(&restart_work);
	LOG_INF("Connected. Current count %d", count);
}

#define MY_ADV_PARAMS                                                                              \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME |                       \
				BT_LE_ADV_OPT_FORCE_NAME_IN_AD | BT_LE_ADV_OPT_ONE_TIME,           \
			BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL)

static int my_adv_start(void)
{
	int err;

	k_mutex_lock(&globals_lock, K_FOREVER);
	err = bt_le_adv_start(MY_ADV_PARAMS, NULL, 0, NULL, 0);

	k_mutex_unlock(&globals_lock);
	return err;
}

static int start_resumptious_advertising(uint8_t max_peripherals)
{
	int err;

	k_mutex_lock(&globals_lock, K_FOREVER);
	target_peripheral_count = max_peripherals;

	err = my_adv_start();

	k_mutex_unlock(&globals_lock);
	return err;
}

static int stop_advertising(void)
{
	int err;

	k_mutex_lock(&globals_lock, K_FOREVER);
	target_peripheral_count = 0;

	err = bt_le_adv_stop();

	k_mutex_unlock(&globals_lock);
	return err;
}

static void _count_conn_marked_peripheral_loop(struct bt_conn *conn, void *count_)
{
	size_t *count = count_;
	struct bt_conn_info conn_info;

	bt_conn_get_info(conn, &conn_info);

	if (conn_info.role == BT_CONN_ROLE_PERIPHERAL) {
		(*count)++;
	}
}

static size_t count_conn_marked_peripheral(void)
{
	size_t count = 0;

	bt_conn_foreach(BT_CONN_TYPE_LE, _count_conn_marked_peripheral_loop, &count);

	return count;
}

static bool should_restart(void)
{
	uint8_t peripheral_count;

	peripheral_count = count_conn_marked_peripheral();

	return peripheral_count < target_peripheral_count;
}

static int try_restart_ignore_oom(void)
{
	int err;

	err = my_adv_start();

	switch (err) {
	case -EALREADY:
	case -ECONNREFUSED:
	case -ENOMEM:
		/* Retry later */
		return 0;
	default:
		return err;
	}
}

static void restart_work_handler(struct k_work *self)
{
	int err;

	/* The timeout is defence-in-depth. The lock has a dependency
	 * the blocking Bluetooth API. This can form a deadlock if the
	 * Bluetooth API happens to have a dependency on the work queue.
	 */
	err = k_mutex_lock(&globals_lock, K_MSEC(100));
	if (err) {
		LOG_DBG("reshed");
		k_work_submit(self);

		/* We did not get the lock. */
		return;
	}

	if (should_restart()) {
		err = try_restart_ignore_oom();
		if (err) {
			LOG_ERR("Failed to restart advertising (err %d)", err);
		}
	}

	k_mutex_unlock(&globals_lock);
}

static void on_conn_recycled(void)
{
	k_work_submit(&restart_work);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = on_connected,
	.disconnected = on_disconnected,
	.recycled = on_conn_recycled,
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

	err = start_resumptious_advertising(CONFIG_BT_MAX_CONN);
	__ASSERT_NO_MSG(!err);

	LOG_INF("Waiting for connections...");
	while (atomic_get(&connected_count) < CONFIG_BT_MAX_CONN) {
		k_msleep(1000);
	}

	LOG_INF("✅ Ok");
	LOG_INF("🧹 Clean up");

	err = stop_advertising();
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

	err = start_resumptious_advertising(CONFIG_BT_MAX_CONN - 1);
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
	bs_trace_silent_exit(0);

	return 0;
}
