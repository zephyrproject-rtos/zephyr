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

LOG_MODULE_REGISTER(connected_count, LOG_LEVEL_INF);

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
