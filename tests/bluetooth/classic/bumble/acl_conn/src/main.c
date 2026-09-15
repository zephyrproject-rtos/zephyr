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

#include "nsi_main.h"
#include "soc.h"
#include "cmdline.h" /* native_sim command line options header */

LOG_MODULE_REGISTER(test_acl_conn, LOG_LEVEL_DBG);

static bt_addr_t peer_addr;

static K_SEM_DEFINE(conn_changed_sem, 0, 1);
static struct bt_conn *acl_conn;

static void br_connected(struct bt_conn *conn, uint8_t conn_err)
{
	LOG_DBG("connected: conn %p err 0x%02x", (void *)conn, conn_err);
	if (conn_err == 0 && acl_conn == NULL) {
		acl_conn = bt_conn_ref(conn);
	}
	k_sem_give(&conn_changed_sem);
}

static void br_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("disconnected: conn %p reason 0x%02x", (void *)conn, reason);
	if (acl_conn != NULL) {
		bt_conn_unref(acl_conn);
		acl_conn = NULL;
	}
	k_sem_give(&conn_changed_sem);
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

/* The central pages the peer directly by its (known) address, verifies the
 * connection comes up, then disconnects and verifies it comes down.
 */
ZTEST(acl_central, test_01_connect)
{
	struct bt_conn *conn;
	int err;

	k_sem_reset(&conn_changed_sem);

	conn = bt_conn_create_br(&peer_addr, BT_BR_CONN_PARAM_DEFAULT);
	zassert_true(conn != NULL, "BR connection creating failed");

	err = k_sem_take(&conn_changed_sem, K_SECONDS(CONFIG_BT_CREATE_CONN_TIMEOUT));
	zassert_equal(err, 0, "Connection timeout (err %d)", err);
	zassert_true(acl_conn != NULL, "Connection failed");
	zassert_equal(acl_conn, conn, "Connection mismatch %p != %p", acl_conn, conn);
	bt_conn_unref(conn);

	err = bt_conn_disconnect(acl_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	zassert_equal(err, 0, "Failed to disconnect (err %d)", err);

	err = k_sem_take(&conn_changed_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Disconnection timeout (err %d)", err);
	zassert_true(acl_conn == NULL, "Disconnection failed");
}

ZTEST_SUITE(acl_central, NULL, bt_setup, NULL, NULL, NULL);

/* The peripheral makes itself connectable and waits for the central to
 * connect and then disconnect.
 */
ZTEST(acl_peripheral, test_01_connect)
{
	int err;

	err = bt_br_set_connectable(true, NULL);
	zassert_equal(err, 0, "Failed to set connectable (err %d)", err);

	k_sem_reset(&conn_changed_sem);

	err = k_sem_take(&conn_changed_sem, K_SECONDS(60));
	zassert_equal(err, 0, "Connection timeout (err %d)", err);
	zassert_true(acl_conn != NULL, "Connection failed");

	err = k_sem_take(&conn_changed_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Disconnection timeout (err %d)", err);
	zassert_true(acl_conn == NULL, "Disconnection failed");

	err = bt_br_set_connectable(false, NULL);
	zassert_equal(err, 0, "Failed to clear connectable (err %d)", err);
}

ZTEST_SUITE(acl_peripheral, NULL, bt_setup, NULL, NULL, NULL);

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

static void acl_conn_args(void)
{
	static struct args_struct_t args[] = {
		{
			.is_mandatory = true,
			.option = "peer_bd_address",
			.name = "XX:XX:XX:XX:XX:XX",
			.type = 's',
			.call_when_found = cmd_peer_bd_address_found,
			.descript = "Bluetooth address of the peer device",
		},
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(args);
}

NATIVE_TASK(acl_conn_args, PRE_BOOT_1, 20);
