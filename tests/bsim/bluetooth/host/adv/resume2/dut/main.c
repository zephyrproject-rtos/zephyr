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

#include "adv_resumer.h"

extern enum bst_result_t bst_result;
extern atomic_t connected_count;
extern size_t count_conn_marked_peripheral(void);

LOG_MODULE_REGISTER(dut, LOG_LEVEL_INF);

/* Note `BT_LE_ADV_OPT_ONE_TIME`. The stack is not responsible
 * for resuming.
 */
#define MY_ADV_PARAMS                                                                              \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME |                       \
				BT_LE_ADV_OPT_FORCE_NAME_IN_AD | BT_LE_ADV_OPT_ONE_TIME,           \
			BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL)

/* Obs! Synchronization: Do not modify when running. */
static uint8_t target_peripheral_count;

static bool should_restart(void)
{
	return count_conn_marked_peripheral() < target_peripheral_count;
}

static int my_adv_start(void)
{
	if (!should_restart()) {
		/* Returning 0 here will allow the resumer to be enabled even
                 * when it's not possible to start the advertiser. To faithfully
                 * emulate the legacy behavior, return -ECONNREFUSED instead.
                 */
		return 0;
	}

	return bt_le_adv_start(MY_ADV_PARAMS, NULL, 0, NULL, 0);
}

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

	target_peripheral_count = CONFIG_BT_MAX_CONN;
	err = adv_resumer_start(my_adv_start);
	__ASSERT_NO_MSG(!err);

	LOG_INF("Waiting for connections...");
	while (atomic_get(&connected_count) < CONFIG_BT_MAX_CONN) {
		k_msleep(1000);
	}

	LOG_INF("✅ Ok");
	LOG_INF("🧹 Clean up");

	err = adv_resumer_stop();
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

	target_peripheral_count = CONFIG_BT_MAX_CONN - 1;
	err = adv_resumer_start(my_adv_start);
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
