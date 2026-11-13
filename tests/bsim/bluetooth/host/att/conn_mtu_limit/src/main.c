/*
 * Copyright (c) 2026 Embeint Holdings Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>

#include "babblekit/sync.h"
#include "babblekit/testcase.h"
#include "testlib/adv.h"
#include "testlib/att.h"
#include "testlib/conn.h"
#include "testlib/scan.h"

#define PEER_NAME          "ATT MTU Limit Peer"
/* Minimum ATT MTU on LE (Core Vol 3, Part F, 3.2.8) */
#define ATT_DEFAULT_LE_MTU 23U
#define MTU_LIMIT          64U

/* Check the result of the MTU exchange, and that the limit can no longer be changed */
static void check_exchanged(struct bt_conn *conn)
{
	uint16_t mtu = bt_gatt_get_mtu(conn);
	int err;

	TEST_ASSERT(mtu == MTU_LIMIT, "ATT MTU is %u, expected %u", mtu, MTU_LIMIT);

	err = bt_gatt_conn_mtu_limit(conn, MTU_LIMIT);
	TEST_ASSERT(err == -EALREADY, "Post-exchange MTU limit returned %d", err);
}

/* @p exchange initiates the MTU exchange, or is NULL if the central initiates it */
static void peripheral_procedure(int (*exchange)(struct bt_conn *conn))
{
	struct bt_conn *conn = NULL;
	int err;

	TEST_START("peripheral");

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Sync init failed");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_testlib_adv_conn(&conn, BT_ID_DEFAULT, PEER_NAME);
	TEST_ASSERT(err == 0, "Advertising failed (err %d)", err);

	bk_sync_wait();

	if (exchange != NULL) {
		err = exchange(conn);
		TEST_ASSERT(err == 0, "MTU exchange failed (err %d)", err);
	}

	check_exchanged(conn);
	bk_sync_send();

	if (exchange != NULL) {
		bk_sync_wait();
	}

	err = bt_testlib_wait_disconnected(conn);
	TEST_ASSERT(err == 0, "Waiting for disconnect failed (err %d)", err);
	bt_testlib_conn_unref(&conn);

	TEST_PASS("peripheral");
}

static void peripheral_local(void)
{
	peripheral_procedure(NULL);
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral_local",
		.test_main_f = peripheral_local,
	},
	BSTEST_END_MARKER,
};

/* The cases below initiate the MTU exchange from the local device, which needs the GATT
 * client. The server-only build leaves them out, as the build does not optimize away calls.
 */
#if defined(CONFIG_BT_GATT_CLIENT)
static struct bt_conn *connect_to_peer(void)
{
	bt_addr_le_t peer = {};
	struct bt_conn *conn = NULL;
	int err;

	err = bt_testlib_scan_find_name(&peer, PEER_NAME);
	TEST_ASSERT(err == 0, "Peer not found (err %d)", err);

	err = bt_testlib_connect(&peer, &conn);
	TEST_ASSERT(err == 0, "Connection failed (err %d)", err);

	return conn;
}

static void central_init(void)
{
	int err;

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Sync init failed");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);
}

static void central_procedure(bool local_initiated)
{
	struct bt_conn *conn;
	int err;

	TEST_START("central %s initiated", local_initiated ? "local" : "peer");

	central_init();

	conn = connect_to_peer();

	err = bt_gatt_conn_mtu_limit(conn, ATT_DEFAULT_LE_MTU - 1U);
	TEST_ASSERT(err == -EINVAL, "Invalid MTU limit returned %d", err);

	err = bt_gatt_conn_mtu_limit(conn, MTU_LIMIT);
	TEST_ASSERT(err == 0, "Failed to set MTU limit (err %d)", err);

	if (local_initiated) {
		err = bt_testlib_att_exchange_mtu(conn);
		TEST_ASSERT(err == 0, "MTU exchange failed (err %d)", err);

		check_exchanged(conn);
		bk_sync_send();
		bk_sync_wait();
	} else {
		bk_sync_send();
		bk_sync_wait();

		check_exchanged(conn);
		bk_sync_send();
	}

	err = bt_testlib_disconnect(&conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Disconnect failed (err %d)", err);

	TEST_PASS("central %s initiated", local_initiated ? "local" : "peer");
}

static void central_local(void)
{
	central_procedure(true);
}

static void central_peer(void)
{
	central_procedure(false);
}

static void peripheral_peer(void)
{
	peripheral_procedure(bt_testlib_att_exchange_mtu);
}

#if defined(CONFIG_BT_GATT_AUTO_UPDATE_MTU)
static K_SEM_DEFINE(mtu_updated, 0, 1);

/* Also called with the default MTU when the ATT bearer connects, before the exchange */
static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	if (tx > ATT_DEFAULT_LE_MTU) {
		k_sem_give(&mtu_updated);
	}
}

/* The host initiates the MTU exchange before the connected callback, so the limit is set
 * immediately after creating the connection.
 */
static void central_auto(void)
{
	static struct bt_gatt_cb gatt_cb = {
		.att_mtu_updated = att_mtu_updated,
	};
	bt_addr_le_t peer = {};
	struct bt_conn *conn = NULL;
	int err;

	TEST_START("central auto initiated");

	central_init();
	bt_gatt_cb_register(&gatt_cb);

	err = bt_testlib_scan_find_name(&peer, PEER_NAME);
	TEST_ASSERT(err == 0, "Peer not found (err %d)", err);

	err = bt_conn_le_create(&peer, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &conn);
	TEST_ASSERT(err == 0, "Connection creation failed (err %d)", err);

	err = bt_gatt_conn_mtu_limit(conn, MTU_LIMIT);
	TEST_ASSERT(err == 0, "Failed to set MTU limit (err %d)", err);

	k_sem_take(&mtu_updated, K_FOREVER);

	check_exchanged(conn);
	bk_sync_send();
	bk_sync_wait();

	err = bt_testlib_disconnect(&conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Disconnect failed (err %d)", err);

	TEST_PASS("central auto initiated");
}
#endif /* CONFIG_BT_GATT_AUTO_UPDATE_MTU */

static const struct bst_test_instance client_test_def[] = {
	{
		.test_id = "central_local",
		.test_main_f = central_local,
	},
	{
		.test_id = "central_peer",
		.test_main_f = central_peer,
	},
	{
		.test_id = "peripheral_peer",
		.test_main_f = peripheral_peer,
	},
#if defined(CONFIG_BT_GATT_AUTO_UPDATE_MTU)
	{
		.test_id = "central_auto",
		.test_main_f = central_auto,
	},
#endif /* CONFIG_BT_GATT_AUTO_UPDATE_MTU */
	BSTEST_END_MARKER,
};
#endif /* CONFIG_BT_GATT_CLIENT */

static struct bst_test_list *install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_def);
#if defined(CONFIG_BT_GATT_CLIENT)
	tests = bst_add_tests(tests, client_test_def);
#endif /* CONFIG_BT_GATT_CLIENT */

	return tests;
}

bst_test_install_t test_installers[] = {install, NULL};

int main(void)
{
	bst_main();

	return 0;
}
