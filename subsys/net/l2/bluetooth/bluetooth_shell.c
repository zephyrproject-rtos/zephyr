/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_bt_shell, CONFIG_NET_L2_BT_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <string.h>
#include <errno.h>

#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/bt.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

static int shell_cmd_connect(const struct shell *sh,
			     size_t argc, char *argv[])
{
	int err;
	bt_addr_le_t addr;
	struct net_if *iface = net_if_get_default();

	if (argc < 3) {
		shell_help(sh);
		return -ENOEXEC;
	}

	err = bt_addr_le_from_str(argv[1], argv[2], &addr);
	if (err) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Invalid peer address (err %d)\n", err);
		return 0;
	}

	if (net_mgmt(NET_REQUEST_BT_CONNECT, iface, &addr, sizeof(addr))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Connection failed\n");
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "Connection pending\n");
	}

	return 0;
}

static int shell_cmd_scan(const struct shell *sh,
			  size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();

	if (argc < 2) {
		shell_help(sh);
		return -ENOEXEC;
	}

	if (net_mgmt(NET_REQUEST_BT_SCAN, iface, argv[1], strlen(argv[1]))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Scan failed\n");
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "Scan in progress\n");
	}

	return 0;
}

static int shell_cmd_disconnect(const struct shell *sh,
				size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();

	if (net_mgmt(NET_REQUEST_BT_DISCONNECT, iface, NULL, 0)) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Disconnect failed\n");
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "Disconnected\n");
	}

	return 0;
}

static int shell_cmd_advertise(const struct shell *sh,
			       size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();

	if (argc < 2) {
		shell_help(sh);
		return -ENOEXEC;
	}

	if (net_mgmt(NET_REQUEST_BT_ADVERTISE, iface, argv[1],
		     strlen(argv[1]))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Advertise failed\n");
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "Advertise in progress\n");
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(bt_commands,
	SHELL_CMD(advertise, NULL,
		  "on/off",
		  shell_cmd_advertise),
	SHELL_CMD(connect, NULL,
		  "<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>",
		  shell_cmd_connect),
	SHELL_CMD(scan, NULL,
		  "<on/off/active/passive>",
		  shell_cmd_scan),
	SHELL_CMD(disconnect, NULL,
		  "",
		  shell_cmd_disconnect),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(net_bt, &bt_commands, "Net Bluetooth commands", NULL);

void net_bt_shell_init(void)
{
}
