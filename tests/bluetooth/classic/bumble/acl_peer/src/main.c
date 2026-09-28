/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(test_acl_peer, LOG_LEVEL_DBG);

/* One semaphore per event: the peer may disconnect before the test thread has
 * seen the connection.
 */
static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static uint8_t connected_err;

static void br_connected(struct bt_conn *conn, uint8_t conn_err)
{
	LOG_DBG("connected: conn %p err 0x%02x", (void *)conn, conn_err);
	connected_err = conn_err;
	k_sem_give(&connected_sem);
}

static void br_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("disconnected: conn %p reason 0x%02x", (void *)conn, reason);
	k_sem_give(&disconnected_sem);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = br_connected,
	.disconnected = br_disconnected,
};

static void *bt_setup(void)
{
	int err;

	err = bt_enable(NULL);
	zassert_equal(err, 0, "Bluetooth init failed (err %d)", err);

	return NULL;
}

/* Accept the connection of whoever pages this device, and wait for that
 * device to disconnect again.
 */
ZTEST(acl_peer, test_01_accept)
{
	int err;

	err = bt_br_set_connectable(true, NULL);
	zassert_equal(err, 0, "Failed to set connectable (err %d)", err);

	err = k_sem_take(&connected_sem, K_SECONDS(60));
	zassert_equal(err, 0, "Connection timeout (err %d)", err);
	zassert_equal(connected_err, 0, "Connection failed (err 0x%02x)", connected_err);

	err = k_sem_take(&disconnected_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Disconnection timeout (err %d)", err);

	err = bt_br_set_connectable(false, NULL);
	zassert_equal(err, 0, "Failed to clear connectable (err %d)", err);
}

ZTEST_SUITE(acl_peer, NULL, bt_setup, NULL, NULL, NULL);
