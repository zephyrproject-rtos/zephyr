/*
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>
#define LOG_MODULE_NAME rsi_bt_shell
#include "rs9116w_ble_conn.h"

#define RS9116_EN atomic_test_bit(bt_dev_flags, BT_DEV_ENABLE)

static const struct bt_data ad_conn[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static int ble_adv_connectable(const struct shell *shell, int argc, char **argv)
{
	if (!RS9116_EN) {
		shell_error(shell, "Device not enabled");
		return 1;
	}
	if (get_active_le_conns() < CONFIG_BT_MAX_CONN) {
		int err;

		err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad_conn,
				      ARRAY_SIZE(ad_conn), NULL, 0);
		if (err) {
			shell_error(shell,
				    "Advertising failed to start (err %d)",
				    err);
			return 1;
		}
		shell_print(shell, "Advertising successfully started");
		return 0;
	}
	shell_error(shell,
		    "Maximum number of simultaneous connections reached");
	return 1;
}

static int ble_adv_stop(const struct shell *shell, int argc, char **argv)
{
	if (!RS9116_EN) {
		shell_error(shell, "Device not enabled");
		return 1;
	}
	int err;

	err = bt_le_adv_stop();
	if (err) {
		shell_error(shell, "Advertising stop failed (err%d)\n", err);
		return 1;
	}
	shell_print(shell, "Stopped Advertising");
	return 0;
}

static int ble_list_connected(const struct shell *shell, int argc, char **argv)
{
	if (!RS9116_EN) {
		shell_error(shell, "Device not enabled");
		return 1;
	}
	int count = get_active_le_conns();

	if (count) {
		shell_print(shell, "%d active connection(s):", count);
		int n = 1;

		for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
			if (get_acl_conn(i)->ref > 0 &&
			    get_acl_conn(i)->type == BT_CONN_TYPE_LE) {
				uint8_t *addr = get_acl_conn(i)->le.dst.a.val;

				shell_print(shell,
					    "\t%02d: ADDR = "
					    "%02X-%02X-%02X-%02X-%02X-%02X",
					    n, addr[0], addr[1], addr[2],
					    addr[3], addr[4], addr[5]);
				n++;
			}
		}
	} else {
		shell_print(shell, "No active connections");
	}

	return 0;
}

static int ble_disconn_all(const struct shell *shell, int argc, char **argv)
{
	if (!RS9116_EN) {
		shell_error(shell, "Device not enabled");
		return 1;
	}
	int count = get_active_le_conns();

	if (count) {
		for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
			if (get_acl_conn(i)->ref > 0) {
				bt_conn_disconnect(get_acl_conn(i), 0);
			}
		}
		shell_print(shell, "Disconnected from %d client(s)", count);
		return 0;
	}
	shell_print(shell, "No active clients");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	ble_9116_adv_sub,
	SHELL_CMD(start, NULL, "Advertise start command.", ble_adv_connectable),
	SHELL_CMD(stop, NULL, "Advertise stop command.", ble_adv_stop),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	ble_9116_conn_sub,
	SHELL_CMD(list, NULL, "Connection list command.", ble_list_connected),
	SHELL_CMD(disconnect, NULL, "Disconnect all command.", ble_disconn_all),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(ble_9116_sub,
			       SHELL_CMD(adv, &ble_9116_adv_sub,
					 "Advertise control command.", NULL),
			       SHELL_CMD(conn, &ble_9116_conn_sub,
					 "Connection control command.", NULL),
			       SHELL_SUBCMD_SET_END);

/* Creating root (level 0) command "ble9116" */
SHELL_CMD_REGISTER(ble9116, &ble_9116_sub, "RS9116 BLE test commands", NULL);
