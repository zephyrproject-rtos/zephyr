/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

#include "nsi_main.h"
#include "soc.h"
#include "cmdline.h"

LOG_MODULE_REGISTER(test_gap_discovery, CONFIG_TEST_GAP_DISCOVERY_LOG_LEVEL);

static bt_addr_t peer_addr;
static atomic_t peer_cod;
static uint8_t connected_err;
static K_SEM_DEFINE(discovery_sem, 0, 1);
static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);

static void br_discover_timeout(const struct bt_br_discovery_result *results, size_t count)
{
	LOG_DBG("BR discovery done, found %zu devices", count);
	k_sem_give(&discovery_sem);
}

static void br_discover_recv(const struct bt_br_discovery_result *result)
{
	if (bt_addr_eq(&peer_addr, &result->addr)) {
		atomic_set(&peer_cod, sys_get_le24(result->cod));
		k_sem_give(&discovery_sem);
	}
}

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

static bool peer_device_discovery(bool limited, uint8_t length)
{
	static struct bt_br_discovery_result results[10];
	struct bt_br_discovery_param param = {
		.length = length,
		.limited = limited,
	};
	int err;

	atomic_set(&peer_cod, -1);
	k_sem_reset(&discovery_sem);
	memset(results, 0, sizeof(results));

	err = bt_br_discovery_start(&param, results, ARRAY_SIZE(results));
	zassert_equal(err, 0, "Bluetooth inquiry failed (err %d)", err);

	err = k_sem_take(&discovery_sem, K_MSEC(length * 1280U + 5000U));
	zassert_equal(err, 0, "Failed to wait for discovery done (err %d)", err);

	err = bt_br_discovery_stop();
	zassert_true(err == 0 || err == -EALREADY,
		     "Failed to stop GAP discovery procedure (err %d)", err);

	return atomic_get(&peer_cod) != -1;
}

static void peer_device_connect(void)
{
	struct bt_conn *conn;
	int err;

	conn = bt_conn_create_br(&peer_addr, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(conn, "BR connection creating failed");

	err = k_sem_take(&connected_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Connection timeout (err %d)", err);
	zassert_equal(connected_err, 0, "Connection failed (err 0x%02x)", connected_err);

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	zassert_equal(err, 0, "Disconnection ACL failed (err %d)", err);

	err = k_sem_take(&disconnected_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Disconnection timeout (err %d)", err);
	bt_conn_unref(conn);
}

ZTEST(gap_central, test_01_general_discovery)
{
	zassert_true(peer_device_discovery(false, 10U), "Peer device not found");
	peer_device_connect();
}

ZTEST(gap_central, test_02_limited_discovery)
{
	zassert_true(peer_device_discovery(true, 10U), "Peer device not found");
	zassert_not_equal(atomic_get(&peer_cod) & BT_COD_MAJOR_SVC_CLASS_LIMITED_DISCOVER, 0,
			  "Invalid COD (limited bit not set)");

	k_sleep(K_SECONDS(CONFIG_BT_LIMITED_DISCOVERABLE_DURATION + 1));

	zassert_false(peer_device_discovery(true, 2U),
		      "Peer still discoverable after the limited period");
	peer_device_connect();
}

static void wait_for_peer_connection(void)
{
	int err;

	err = k_sem_take(&connected_sem, K_SECONDS(60));
	zassert_equal(err, 0, "Connection timeout (err %d)", err);
	zassert_equal(connected_err, 0, "Connection failed (err 0x%02x)", connected_err);

	err = k_sem_take(&disconnected_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Disconnection timeout (err %d)", err);
}

ZTEST(gap_peripheral, test_01_general_discovery)
{
	int err;

	err = bt_br_set_connectable(true, NULL);
	zassert_equal(err, 0, "Failed to set connectable (err %d)", err);

	err = bt_br_set_discoverable(true, false);
	zassert_equal(err, 0, "Failed to set discoverable (err %d)", err);

	wait_for_peer_connection();

	err = bt_br_set_discoverable(false, false);
	zassert_equal(err, 0, "Failed to clear discoverable (err %d)", err);

	err = bt_br_set_connectable(false, NULL);
	zassert_equal(err, 0, "Failed to clear connectable (err %d)", err);
}

ZTEST(gap_peripheral, test_02_limited_discovery)
{
	int err;

	err = bt_br_set_connectable(true, NULL);
	zassert_equal(err, 0, "Failed to set connectable (err %d)", err);

	err = bt_br_set_discoverable(true, true);
	zassert_equal(err, 0, "Failed to set discoverable (err %d)", err);

	wait_for_peer_connection();

	err = bt_br_set_connectable(false, NULL);
	zassert_equal(err, 0, "Failed to clear connectable (err %d)", err);
}

static void *setup(void)
{
	static struct bt_br_discovery_cb br_discover = {
		.recv = br_discover_recv,
		.timeout = br_discover_timeout,
	};
	int err;

	err = bt_enable(NULL);
	zassert_equal(err, 0, "Bluetooth init failed (err %d)", err);
	bt_br_discovery_cb_register(&br_discover);

	return NULL;
}

static void teardown(void *fixture)
{
	int err;

	err = bt_disable();
	zassert_equal(err, 0, "Bluetooth de-init failed (err %d)", err);
}

static void before(void *fixture)
{
	k_sem_reset(&discovery_sem);
	k_sem_reset(&connected_sem);
	k_sem_reset(&disconnected_sem);
}

ZTEST_SUITE(gap_central, NULL, setup, before, NULL, teardown);
ZTEST_SUITE(gap_peripheral, NULL, setup, before, NULL, teardown);

static void cmd_peer_bd_address_found(char *argv, int offset)
{
	char *addr_str = &argv[offset];
	int err;

	err = bt_addr_from_str(addr_str, &peer_addr);
	if (err != 0) {
		LOG_ERR("Failed to parse peer Bluetooth address: %s (err %d)", addr_str, err);
		nsi_exit(err);
	}
}

static void gap_discovery_args(void)
{
	static struct args_struct_t args[] = {
		{
			.is_mandatory = true,
			.option = "peer_bd_address",
			.name = "XX:XX:XX:XX:XX:XX",
			.type = 's',
			.call_when_found = cmd_peer_bd_address_found,
			.descript = "Bluetooth peer device address for GAP discovery test",
		},
		ARG_TABLE_ENDMARKER,
	};

	native_add_command_line_opts(args);
}

NATIVE_TASK(gap_discovery_args, PRE_BOOT_1, 20);
