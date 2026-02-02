/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

#define LOG_MODULE_NAME test_gap_discovery_peripheral
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_TEST_GAP_DISCOVERY_LOG_LEVEL);

static K_SEM_DEFINE(br_discover_sem, 0, 1);

static ATOMIC_DEFINE(test_flags, 32);

#define TEST_FLAG_CONN_CONNECTED    0
#define TEST_FLAG_CONN_DISCONNECTED 1

static void br_connected(struct bt_conn *conn, uint8_t conn_err)
{
	LOG_DBG("connected: conn %p err 0x%02x", (void *)conn, conn_err);
	if (conn_err == 0) {
		k_sem_give(&br_discover_sem);
		atomic_set_bit(test_flags, TEST_FLAG_CONN_CONNECTED);
	}
}

static void br_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("disconnected: conn %p reason 0x%02x", (void *)conn, reason);
	k_sem_give(&br_discover_sem);
	atomic_set_bit(test_flags, TEST_FLAG_CONN_DISCONNECTED);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = br_connected,
	.disconnected = br_disconnected,
};

ZTEST(gap_peripheral, test_01_gap_peripheral_general_discovery)
{
	int err;

	err = bt_br_set_connectable(true, NULL);
	zassert_equal(err, 0, "Failed to set connectable (err %d)", err);

	err = bt_br_set_discoverable(true, false);
	zassert_equal(err, 0, "Failed to set discoverable (err %d)", err);

	err = k_sem_take(&br_discover_sem, K_SECONDS(60));
	zassert_equal(err, 0, "Connection timeout (err %d)", err);
	zassert_true(atomic_test_bit(test_flags, TEST_FLAG_CONN_CONNECTED), "Connection failed");

	k_sem_reset(&br_discover_sem);

	err = k_sem_take(&br_discover_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Disconnection timeout (err %d)", err);
	zassert_true(atomic_test_bit(test_flags, TEST_FLAG_CONN_DISCONNECTED),
		     "Disconnection failed");

	err = bt_br_set_discoverable(false, false);
	zassert_equal(err, 0, "Failed to clear discoverable (err %d)", err);

	err = bt_br_set_connectable(false, NULL);
	zassert_equal(err, 0, "Failed to clear connectable (err %d)", err);
}

ZTEST(gap_peripheral, test_02_gap_peripheral_limited_discovery)
{
	int err;

	err = bt_br_set_connectable(true, NULL);
	zassert_equal(err, 0, "Failed to set connectable (err %d)", err);

	err = bt_br_set_discoverable(true, true);
	zassert_equal(err, 0, "Failed to set discoverable (err %d)", err);

	err = k_sem_take(&br_discover_sem, K_SECONDS(30 + CONFIG_BT_LIMITED_DISCOVERABLE_DURATION));
	zassert_equal(err, 0, "Connection timeout (err %d)", err);
	zassert_true(atomic_test_bit(test_flags, TEST_FLAG_CONN_CONNECTED), "Connection failed");

	k_sem_reset(&br_discover_sem);

	err = k_sem_take(&br_discover_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Disconnection timeout (err %d)", err);
	zassert_true(atomic_test_bit(test_flags, TEST_FLAG_CONN_DISCONNECTED),
		     "Disconnection failed");

	err = bt_br_set_connectable(false, NULL);
	zassert_equal(err, 0, "Failed to clear connectable (err %d)", err);
}

static void *setup(void)
{
	int err;

	LOG_DBG("Initializing Bluetooth");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	zassert_equal(err, 0, "Bluetooth init failed (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	return NULL;
}

static void teardown(void *f)
{
	int err;

	LOG_DBG("Disabling Bluetooth");

	/* De-initialize the Bluetooth Subsystem */
	err = bt_disable();
	zassert_equal(err, 0, "Bluetooth de-init failed (err %d)", err);

	LOG_DBG("Bluetooth de-initialized");
}

static void before(void *f)
{
	k_sem_reset(&br_discover_sem);

	atomic_clear_bit(test_flags, TEST_FLAG_CONN_CONNECTED);
	atomic_clear_bit(test_flags, TEST_FLAG_CONN_DISCONNECTED);
}

ZTEST_SUITE(gap_peripheral, NULL, setup, before, NULL, teardown);
