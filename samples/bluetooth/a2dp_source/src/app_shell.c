/*
 * Copyright 2020 - 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/a2dp.h>

#include "app_discover.h"
#include "app_connect.h"

static int cmd_a2dp_source_bt_discover(const struct shell *sh, size_t argc, char *argv[])
{
	app_discover();
	return 0;
}

static int cmd_a2dp_source_bt_connect(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t select_index = 0;
	char *ch = argv[1];
	uint8_t *addr;

	if (argc < 2) {
		printk("the parameter count is wrong\r\n");
		return SHELL_CMD_HELP_PRINTED;
	}

	for (select_index = 0; select_index < strlen(ch); ++select_index) {
		if ((ch[select_index] < '0') || (ch[select_index] > '9')) {
			printk("the parameter is wrong\r\n");
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	switch (strlen(ch)) {
	case 1:
		select_index = ch[0] - '0';
		break;
	case 2:
		select_index = (ch[0] - '0') * 10 + (ch[1] - '0');
		break;
	default:
		printk("the parameter is wrong\r\n");
		break;
	}

	if (select_index == 0U) {
		printk("the parameter is wrong\r\n");
	}
	addr = app_get_addr(select_index - 1);
	app_connect(addr);
	return 0;
}

static int cmd_a2dp_source_bt_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	app_disconnect();
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(a2dp_source_bt_cmds,
	SHELL_CMD(discover, NULL, "start to find BT devices", cmd_a2dp_source_bt_discover),
	SHELL_CMD_ARG(connect, NULL,
		"connect to the device that is found, for example: bt connectdevice n (from 1)",
		cmd_a2dp_source_bt_connect, 2, 0),
	SHELL_CMD(disconnect, NULL, "disconnect current connection",
		cmd_a2dp_source_bt_disconnect),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(bt, &a2dp_source_bt_cmds, "a2dp source demo shell commands", NULL);
