/*
 * Copyright (c) 2026 Embeint Holdings Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Tests of the att_mtu_limit callback of struct bt_gatt_cb.
 *
 * Both images have a default ATT MTU of 247, and one of the devices limits the local MTU of
 * the connection to MTU_LIMIT. The central always initiates the MTU exchange, as the
 * peripheral image is built without the GATT client. The central image runs the connection
 * callbacks of the central sample, which initiate the exchange in the connected callback.
 *
 * - local: the central limits the MTU of the exchange it initiates.
 * - peer: the server-only peripheral limits the MTU of the exchange initiated by the central.
 * - auto: as local, with the central built with CONFIG_BT_GATT_AUTO_UPDATE_MTU, so the host
 *   initiates the exchange before the connected callback.
 * - multi: the auto central connects to two peripherals and limits only the first
 *   connection. The first connection uses a long connection interval, so that the exchange
 *   on the second connection starts while the response on the first one is pending. The
 *   devices do not synchronize, as the babblekit sync only pairs two devices.
 *
 * No device disconnects, as the disconnected callback of the peripheral sample expects the
 * sample to be running.
 */

#include <stdbool.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>

#include "babblekit/sync.h"
#include "babblekit/testcase.h"

#define PEER_NAME          "ATT MTU Limit Peer"
#define PEER_NAME_MULTI_1  "ATT MTU Limit Peer 1"
#define PEER_NAME_MULTI_2  "ATT MTU Limit Peer 2"
/* Minimum ATT MTU on LE (Core Vol 3, Part F, 3.2.8) */
#define ATT_DEFAULT_LE_MTU 23U
/* Default local ATT MTU of both images */
#define DEFAULT_MTU        247U
#define MTU_LIMIT          64U
#define WAIT_TIMEOUT       K_SECONDS(2)
/* Longer than two connection events of the multi case's first connection */
#define MULTI_WAIT_TIMEOUT K_SECONDS(5)

static K_SEM_DEFINE(mtu_updated_sem, 0, 2);

static void setup(bool sync)
{
	int err;

	if (sync) {
		err = bk_sync_init();
		TEST_ASSERT(err == 0, "Sync init failed");
	}

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);
}

/* Also called with the default MTU when the ATT bearer connects, before the exchange */
static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	if (tx > ATT_DEFAULT_LE_MTU) {
		k_sem_give(&mtu_updated_sem);
	}
}

static uint16_t att_mtu_limit(struct bt_conn *conn)
{
	return MTU_LIMIT;
}

/* Limit the MTU of all connections from now on */
static void register_limit(void)
{
	static struct bt_gatt_cb gatt_cb = {
		.att_mtu_limit = att_mtu_limit,
	};

	bt_gatt_cb_register(&gatt_cb);
}

static void check_mtu(struct bt_conn *conn, uint16_t expected)
{
	uint16_t mtu = bt_gatt_get_mtu(conn);

	TEST_ASSERT(mtu == expected, "ATT MTU is %u, expected %u", mtu, expected);
}

static void check_exchanged(struct bt_conn *conn)
{
	check_mtu(conn, MTU_LIMIT);
}

#if defined(CONFIG_BT_PERIPHERAL)
/* At file scope, as BT_DATA_BYTES is not a constant initializer */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, PEER_NAME, sizeof(PEER_NAME) - 1),
};
static K_SEM_DEFINE(connected_sem, 0, 1);
static struct bt_conn *peripheral_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT(err == 0, "Connection failed (err 0x%02x)", err);

	peripheral_conn = bt_conn_ref(conn);
	k_sem_give(&connected_sem);
}

static void peripheral_procedure(bool limit)
{
	static struct bt_conn_cb conn_cb = {
		.connected = connected,
	};
	int err;

	TEST_START("peripheral %s", limit ? "limit" : "check");

	setup(true);

	if (limit) {
		register_limit();
	}

	err = bt_conn_cb_register(&conn_cb);
	TEST_ASSERT(err == 0, "Callback registration failed (err %d)", err);

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	TEST_ASSERT(err == 0, "Advertising failed (err %d)", err);

	err = k_sem_take(&connected_sem, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "No connection (err %d)", err);

	/* Wait for the central to complete the exchange */
	bk_sync_wait();

	check_exchanged(peripheral_conn);
	bk_sync_send();

	TEST_PASS("peripheral %s", limit ? "limit" : "check");
}

/* Check the MTU of the exchange initiated by the central in the multi case */
static void peripheral_multi_procedure(const char *name, uint16_t expected)
{
	static struct bt_conn_cb conn_cb = {
		.connected = connected,
	};
	static struct bt_gatt_cb gatt_cb = {
		.att_mtu_updated = att_mtu_updated,
	};
	const struct bt_data multi_ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		BT_DATA(BT_DATA_NAME_COMPLETE, name, strlen(name)),
	};
	int err;

	TEST_START("peripheral %s", name);

	setup(false);
	bt_gatt_cb_register(&gatt_cb);

	err = bt_conn_cb_register(&conn_cb);
	TEST_ASSERT(err == 0, "Callback registration failed (err %d)", err);

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, multi_ad, ARRAY_SIZE(multi_ad), NULL, 0);
	TEST_ASSERT(err == 0, "Advertising failed (err %d)", err);

	err = k_sem_take(&connected_sem, MULTI_WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "No connection (err %d)", err);

	err = k_sem_take(&mtu_updated_sem, MULTI_WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "No MTU update from the exchange (err %d)", err);

	check_mtu(peripheral_conn, expected);

	TEST_PASS("peripheral %s", name);
}

/* The central limits the MTU, the peripheral checks the result */
static void limit_peripheral_local(void)
{
	peripheral_procedure(false);
}

/* The peripheral limits the MTU, the central initiates the exchange */
static void limit_peripheral_peer(void)
{
	peripheral_procedure(true);
}

/* The first connection of the multi case, which the central limits */
static void limit_peripheral_multi_1(void)
{
	peripheral_multi_procedure(PEER_NAME_MULTI_1, MTU_LIMIT);
}

/* The second connection of the multi case, which the central does not limit */
static void limit_peripheral_multi_2(void)
{
	peripheral_multi_procedure(PEER_NAME_MULTI_2, DEFAULT_MTU);
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "limit_peripheral_local",
		.test_descr = "Check the MTU limited by the central",
		.test_main_f = limit_peripheral_local,
	},
	{
		.test_id = "limit_peripheral_peer",
		.test_descr = "Limit the MTU on a server-only peripheral",
		.test_main_f = limit_peripheral_peer,
	},
	{
		.test_id = "limit_peripheral_multi_1",
		.test_descr = "Check the MTU of the limited connection of the multi case",
		.test_main_f = limit_peripheral_multi_1,
	},
	{
		.test_id = "limit_peripheral_multi_2",
		.test_descr = "Check the MTU of the unlimited connection of the multi case",
		.test_main_f = limit_peripheral_multi_2,
	},
	BSTEST_END_MARKER,
};
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
static K_SEM_DEFINE(found_sem, 0, 1);
static bt_addr_le_t peer_addr;
static const char *scan_name;

static bool ad_has_peer_name(struct bt_data *data, void *user_data)
{
	bool *found = user_data;

	if ((data->type == BT_DATA_NAME_COMPLETE) && (data->data_len == strlen(scan_name)) &&
	    (memcmp(data->data, scan_name, data->data_len) == 0)) {
		*found = true;
		return false;
	}

	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	bool found = false;

	bt_data_parse(ad, ad_has_peer_name, &found);
	if (found) {
		bt_addr_le_copy(&peer_addr, addr);
		k_sem_give(&found_sem);
	}
}

/* Find the address of the peer advertising @p name */
static void find_peer(const char *name, bt_addr_le_t *addr)
{
	int err;

	scan_name = name;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	TEST_ASSERT(err == 0, "Scanning failed (err %d)", err);

	err = k_sem_take(&found_sem, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "Peer not found (err %d)", err);

	err = bt_le_scan_stop();
	TEST_ASSERT(err == 0, "Stopping scan failed (err %d)", err);

	bt_addr_le_copy(addr, &peer_addr);
}

static struct bt_conn *connect_to_addr(const bt_addr_le_t *addr,
				       const struct bt_le_conn_param *param)
{
	struct bt_conn *conn = NULL;
	int err;

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &conn);
	TEST_ASSERT(err == 0, "Connection creation failed (err %d)", err);

	return conn;
}

/* Find the peer advertising @p name, and create a connection to it */
static struct bt_conn *connect_to(const char *name, const struct bt_le_conn_param *param)
{
	bt_addr_le_t addr;

	find_peer(name, &addr);

	return connect_to_addr(&addr, param);
}

static void central_procedure(bool limit)
{
	static struct bt_gatt_cb gatt_cb = {
		.att_mtu_updated = att_mtu_updated,
	};
	struct bt_conn *conn;
	int err;

	TEST_START("central %s", limit ? "limit" : "check");

	setup(true);
	bt_gatt_cb_register(&gatt_cb);

	if (limit) {
		register_limit();
	}

	conn = connect_to(PEER_NAME, BT_LE_CONN_PARAM_DEFAULT);

	err = k_sem_take(&mtu_updated_sem, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "No MTU update from the exchange (err %d)", err);

	check_exchanged(conn);
	bk_sync_send();
	bk_sync_wait();

	bt_conn_unref(conn);

	TEST_PASS("central %s", limit ? "limit" : "check");
}

static void limit_central_local(void)
{
	central_procedure(true);
}

static void limit_central_peer(void)
{
	central_procedure(false);
}

#if defined(CONFIG_BT_GATT_AUTO_UPDATE_MTU)
static struct bt_conn *multi_limited_conn;
static K_SEM_DEFINE(multi_connected_sem, 0, 1);

static uint16_t att_mtu_limit_first(struct bt_conn *conn)
{
	return (conn == multi_limited_conn) ? MTU_LIMIT : 0U;
}

static void multi_connected(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT(err == 0, "Connection failed (err 0x%02x)", err);

	if (conn == multi_limited_conn) {
		k_sem_give(&multi_connected_sem);
	}
}

/* Connect to two peripherals, limiting only the first connection. The host initiates the
 * exchange of each connection when it is established, and the response on the first
 * connection is still pending when the second connection is established.
 */
static void limit_central_multi(void)
{
	static struct bt_gatt_cb gatt_cb = {
		.att_mtu_updated = att_mtu_updated,
		.att_mtu_limit = att_mtu_limit_first,
	};
	static struct bt_conn_cb conn_cb = {
		.connected = multi_connected,
	};
	/* 1 s connection interval, 6 s supervision timeout */
	const struct bt_le_conn_param *slow = BT_LE_CONN_PARAM(800, 800, 0, 600);
	bt_addr_le_t addr_1;
	bt_addr_le_t addr_2;
	struct bt_conn *conn;
	int err;

	TEST_START("central multi");

	setup(false);
	bt_gatt_cb_register(&gatt_cb);

	err = bt_conn_cb_register(&conn_cb);
	TEST_ASSERT(err == 0, "Callback registration failed (err %d)", err);

	/* Scanning only resumes after the first connection event of the first connection, so
	 * both peers are found before connecting.
	 */
	find_peer(PEER_NAME_MULTI_1, &addr_1);
	find_peer(PEER_NAME_MULTI_2, &addr_2);

	multi_limited_conn = connect_to_addr(&addr_1, slow);

	err = k_sem_take(&multi_connected_sem, MULTI_WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "No connection to the first peripheral (err %d)", err);

	conn = connect_to_addr(&addr_2, BT_LE_CONN_PARAM_DEFAULT);

	for (int i = 0; i < 2; i++) {
		err = k_sem_take(&mtu_updated_sem, MULTI_WAIT_TIMEOUT);
		TEST_ASSERT(err == 0, "No MTU update from exchange %d (err %d)", i, err);
	}

	check_mtu(multi_limited_conn, MTU_LIMIT);
	check_mtu(conn, DEFAULT_MTU);

	bt_conn_unref(conn);

	TEST_PASS("central multi");
}
#endif /* CONFIG_BT_GATT_AUTO_UPDATE_MTU */

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "limit_central_local",
		.test_descr = "Limit the MTU before the exchange",
		.test_main_f = limit_central_local,
	},
	{
		.test_id = "limit_central_peer",
		.test_descr = "Exchange the MTU with a peripheral that limits it",
		.test_main_f = limit_central_peer,
	},
#if defined(CONFIG_BT_GATT_AUTO_UPDATE_MTU)
	{
		.test_id = "limit_central_multi",
		.test_descr = "Limit only the first of two connections with pending exchanges",
		.test_main_f = limit_central_multi,
	},
#endif /* CONFIG_BT_GATT_AUTO_UPDATE_MTU */
	BSTEST_END_MARKER,
};
#endif /* CONFIG_BT_CENTRAL */

struct bst_test_list *test_conn_mtu_limit_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
