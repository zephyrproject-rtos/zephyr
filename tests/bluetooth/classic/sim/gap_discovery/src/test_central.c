/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

#define LOG_MODULE_NAME test_gap_discovery_central
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_TEST_GAP_DISCOVERY_LOG_LEVEL);

static struct bt_br_discovery_param br_discover_param;
#define BR_DISCOVER_RESULT_COUNT 10
static struct bt_br_discovery_result br_discover_result[BR_DISCOVER_RESULT_COUNT];

static K_SEM_DEFINE(br_discover_sem, 0, 1);

bt_addr_t peer_addr;
static ATOMIC_DEFINE(test_flags, 32);

#define TEST_FLAG_DEVICE_FOUND      0
#define TEST_FLAG_CONN_CONNECTED    1
#define TEST_FLAG_CONN_DISCONNECTED 2

static void br_discover_timeout(const struct bt_br_discovery_result *results, size_t count)
{
	LOG_DBG("BR discovery done, found %zu devices", count);

	k_sem_give(&br_discover_sem);
}

static void br_discover_recv(const struct bt_br_discovery_result *result)
{
	char br_addr[BT_ADDR_STR_LEN];

	bt_addr_to_str(&result->addr, br_addr, sizeof(br_addr));

	LOG_DBG("[DEVICE]: %s, RSSI %i, COD %u", br_addr, result->rssi, sys_get_le24(result->cod));

	if (bt_addr_eq(&peer_addr, &result->addr)) {
		atomic_set_bit(test_flags, TEST_FLAG_DEVICE_FOUND);
		LOG_DBG("  Target %s is found", br_addr);
		k_sem_give(&br_discover_sem);
	}
}

static struct bt_br_discovery_cb br_discover = {
	.recv = br_discover_recv,
	.timeout = br_discover_timeout,
};

static void br_connected(struct bt_conn *conn, uint8_t conn_err)
{
	LOG_DBG("connected: conn %p err 0x%02x", (void *)conn, conn_err);
	if (conn_err == 0) {
		k_sem_give(&br_discover_sem);
		atomic_set_bit(test_flags, TEST_FLAG_CONN_CONNECTED);
	} else {
		LOG_ERR("Connection failed");
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

/* Multiplier for each discovery result to estimate timeout (seconds per device) */
#define GAP_DISCOVERY_TIMEOUT_PER_DEVICE 1.25
/* Base timeout added to the total (seconds) */
#define GAP_DISCOVERY_TIMEOUT_BASE 5.0
#define GAP_DISCOVERY_TIMEOUT(count) \
	((double)(count) * GAP_DISCOVERY_TIMEOUT_PER_DEVICE + GAP_DISCOVERY_TIMEOUT_BASE)

static bool peer_device_discovery(bool limited)
{
	int err;
	double timeout;

	atomic_clear_bit(test_flags, TEST_FLAG_DEVICE_FOUND);

	LOG_DBG("Starting Bluetooth inquiry");

	br_discover_param.length = BR_DISCOVER_RESULT_COUNT;
	br_discover_param.limited = limited;

	memset(br_discover_result, 0, sizeof(br_discover_result));

	err = bt_br_discovery_start(&br_discover_param, br_discover_result,
				    ARRAY_SIZE(br_discover_result));
	zassert_equal(err, 0, "Bluetooth inquiry failed (err %d)", err);

	/* Wait for all discovery results to be processed */
	timeout = GAP_DISCOVERY_TIMEOUT(ARRAY_SIZE(br_discover_result));
	LOG_DBG("Will wait for GAP discovery done (timeout %us)", (uint32_t)timeout);
	err = k_sem_take(&br_discover_sem, K_SECONDS(timeout));
	zassert_equal(err, 0, "Failed to wait for discovery done (err %d)", err);

	err = bt_br_discovery_stop();
	if ((err != 0) && (err != -EALREADY)) {
		LOG_ERR("Failed to stop GAP discovery procedure (err %d)", err);
	}

	LOG_DBG("Bluetooth inquiry completed");

	return atomic_test_bit(test_flags, TEST_FLAG_DEVICE_FOUND);
}

static void peer_device_connect(void)
{
	int err;
	struct bt_conn *conn;

	conn = bt_conn_create_br(&peer_addr, BT_BR_CONN_PARAM_DEFAULT);
	zassert_true(conn != NULL, "BR connection creating failed");

	err = k_sem_take(&br_discover_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Connection timeout (err %d)", err);
	zassert_true(atomic_test_bit(test_flags, TEST_FLAG_CONN_CONNECTED), "Connection failed");

	k_sem_reset(&br_discover_sem);

	k_sleep(K_SECONDS(5));

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	zassert_equal(err, 0, "Disconnection ACL failed (err %d)", err);

	err = k_sem_take(&br_discover_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Disconnection timeout (err %d)", err);
	zassert_true(atomic_test_bit(test_flags, TEST_FLAG_CONN_DISCONNECTED),
		     "Disconnection failed");

	bt_conn_unref(conn);
}

ZTEST(gap_central, test_01_gap_central_general_discovery)
{
	bool found;

	found = peer_device_discovery(false);

	zassert_true(found, "Peer device not found");

	peer_device_connect();
}

#define BT_COD_MAJOR_SVC_CLASS_LIMITED_DISCOVER BIT(13)

static bool is_limited_inquiry(void)
{
	ARRAY_FOR_EACH(br_discover_result, i) {
		uint32_t cod;

		if (!bt_addr_eq(&br_discover_result[i].addr, &peer_addr)) {
			continue;
		}

		cod = sys_get_le24(br_discover_result[i].cod);
		if ((cod & BT_COD_MAJOR_SVC_CLASS_LIMITED_DISCOVER) != 0) {
			return true;
		}
	}

	return false;
}

ZTEST(gap_central, test_02_gap_central_limited_discovery)
{
	bool found;

	found = peer_device_discovery(true);

	zassert_true(found, "Peer device not found");

	if (!is_limited_inquiry()) {
		/* There is a timing issue. Rediscovery to ensure limited bit is set. */
		found = peer_device_discovery(true);

		zassert_true(found, "Peer device not found");
	}

	zassert_true(is_limited_inquiry(), "Invalid COD (limited bit not set)");

	k_sleep(K_SECONDS(CONFIG_BT_LIMITED_DISCOVERABLE_DURATION + 5));

	found = peer_device_discovery(true);
	if (found) {
		zassert_true(!is_limited_inquiry(), "Invalid COD (limited bit is set)");
	}

	peer_device_connect();
}

static void *setup(void)
{
	int err;

	LOG_DBG("Initializing Bluetooth");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	zassert_equal(err, 0, "Bluetooth init failed (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	LOG_DBG("Register discovery callback");
	bt_br_discovery_cb_register(&br_discover);

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
	atomic_clear_bit(test_flags, TEST_FLAG_DEVICE_FOUND);
	atomic_clear_bit(test_flags, TEST_FLAG_CONN_CONNECTED);
	atomic_clear_bit(test_flags, TEST_FLAG_CONN_DISCONNECTED);

	k_sem_reset(&br_discover_sem);
}

ZTEST_SUITE(gap_central, NULL, setup, before, NULL, teardown);
