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
	return bt_le_adv_start(MY_ADV_PARAMS, NULL, 0, NULL, 0);
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
