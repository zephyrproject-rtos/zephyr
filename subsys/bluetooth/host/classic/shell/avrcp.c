/** @file
 *  @brief Audio Video Remote Control Profile shell functions.
 */

/*
 * Copyright (c) 2024 Xiaomi InC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/avrcp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"

struct bt_avrcp *default_avrcp;
static uint8_t avrcp_registered;
static void avrcp_connected(struct bt_avrcp *avrcp)
{
	default_avrcp = avrcp;
	shell_print(ctx_shell, "AVRCP connected");
}

static void avrcp_disconnected(struct bt_avrcp *avrcp)
{
	shell_print(ctx_shell, "AVRCP disconnected");
}

static struct bt_avrcp_cb avrcp_cb = {
	.connected = avrcp_connected,
	.disconnected = avrcp_disconnected,
};

static int register_cb(const struct shell *sh)
{
	int err;

	if (avrcp_registered) {
		return 0;
	}

	err = bt_avrcp_register_cb(&avrcp_cb);
	if (!err) {
		avrcp_registered = 1;
		shell_print(sh, "AVRCP callbacks registered");
	} else {
		shell_print(sh, "failed to register callbacks");
	}

	return err;
}

static int cmd_register_cb(const struct shell *sh, int32_t argc, char *argv[])
{
	if (avrcp_registered) {
		shell_print(sh, "already registered");
		return 0;
	}

	register_cb(sh);

	return 0;
}

static int cmd_connect(const struct shell *sh, int32_t argc, char *argv[])
{
	if (avrcp_registered == 0) {
		if (register_cb(sh) != 0) {
			return -ENOEXEC;
		}
	}

	if (!default_conn) {
		shell_error(sh, "BR/EDR not connected");
		return -ENOEXEC;
	}

	default_avrcp = bt_avrcp_connect(default_conn);
	if (NULL == default_avrcp) {
		shell_error(sh, "fail to connect AVRCP");
	}

	return 0;
}

static int cmd_disconnect(const struct shell *sh, int32_t argc, char *argv[])
{
	if (avrcp_registered == 0) {
		if (register_cb(sh) != 0) {
			return -ENOEXEC;
		}
	}

	if (default_avrcp != NULL) {
		bt_avrcp_disconnect(default_avrcp);
		default_avrcp = NULL;
	} else {
		shell_error(sh, "AVRCP is not connected");
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(avrcp_cmds,
	SHELL_CMD_ARG(register_cb, NULL, "register avrcp callbacks",
		cmd_register_cb, 1, 0),
	SHELL_CMD_ARG(connect, NULL, "<address>", cmd_connect, 2, 0),
	SHELL_CMD_ARG(disconnect, NULL, "<address>", cmd_disconnect, 2, 0),
	SHELL_SUBCMD_SET_END);

static int cmd_avrcp(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		/* sh returns 1 when help is printed */
		return 1;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(avrcp, &avrcp_cmds, "Bluetooth AVRCP sh commands",
				cmd_avrcp, 1, 1);
