/*
 * Copyright 2024 Xiaomi Corporation
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

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/map_client.h>
#include <zephyr/shell/shell.h>

#include "bt.h"

#define MAP_PATH       "telecom/msg/inbox"
#define MAP_SET_FOLDER "telecom/msg/inbox"
#define MAP_SET_FLAG   2

#define HELP_NONE "[none]"

struct bt_map_client *default_map;

void map_client_connected(struct bt_map_client *client)
{
	shell_print(ctx_shell, "map connected:%p", client);
}

void map_client_disconnected(struct bt_map_client *client)
{
	shell_print(ctx_shell, "map disconnected:%p", client);
}
void map_client_set_path_finished(struct bt_map_client *client)
{
	shell_print(ctx_shell, "map set path finished:%p", client);
}

void map_client_recv(struct bt_map_client *client, struct bt_map_result *result, uint8_t array_size)
{
	shell_print(ctx_shell, "map client:%p, type:%d, len:%d", client, result->type, result->len);
}

struct bt_map_client_cb map_client_cb = {
	.connected = map_client_connected,
	.disconnected = map_client_disconnected,
	.set_path_finished = map_client_set_path_finished,
	.recv = map_client_recv,
};

static int cmd_connect(const struct shell *sh, int32_t argc, char *argv[])
{
	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	default_map = bt_map_client_connect(default_conn, MAP_PATH, &map_client_cb);
	if (!default_map) {
		shell_error(sh, "map client fail to connect");
	}

	return 0;
}

static int cmd_disconnect(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;

	if (!default_map) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	ret = bt_map_client_disconnect(default_map);
	if (ret < 0) {
		shell_error(sh, "map client fail to disconnect map client, err:%d", ret);
	}

	return 0;
}

static int cmd_get_message(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;
	if (!default_map) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	ret = bt_map_client_get_message(default_map, MAP_PATH);
	if (ret < 0) {
		shell_error(sh, "map client fail to get size, err:%d", ret);
	}

	return 0;
}

static int cmd_set_path(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;
	if (!default_map) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	ret = bt_map_client_setpath(default_map, MAP_SET_FOLDER);
	if (ret < 0) {
		shell_error(sh, "map client fail to get size, err:%d", ret);
	}

	return 0;
}

static int cmd_set_folder(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;
	if (!default_map) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	ret = bt_map_client_set_folder(default_map, MAP_SET_FOLDER, MAP_SET_FLAG);
	if (ret < 0) {
		shell_error(sh, "map client fail to get size, err:%d", ret);
	}

	return 0;
}

static int cmd_get_msglist(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;

	if (!default_map) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	ret = bt_map_client_get_messages_listing(default_map, 1, 0x0a);
	if (ret) {
		shell_error(sh, "map client fail to get pb, err:%d", ret);
	}

	return 0;
}

static int cmd_get_folderlist(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;

	if (!default_map) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	ret = bt_map_client_get_folder_listing(default_map);
	if (ret) {
		shell_error(sh, "map client fail to set patch, err:%d", ret);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(map_client_cmds,
			       SHELL_CMD_ARG(connect, NULL, HELP_NONE, cmd_connect, 1, 0),
			       SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_disconnect, 1, 0),
			       SHELL_CMD_ARG(getmsg, NULL, HELP_NONE, cmd_get_message, 1, 0),
			       SHELL_CMD_ARG(setpath, NULL, HELP_NONE, cmd_set_path, 1, 0),
			       SHELL_CMD_ARG(setfolder, NULL, HELP_NONE, cmd_set_folder, 1, 0),
			       SHELL_CMD_ARG(getmsglist, NULL, HELP_NONE, cmd_get_msglist, 1, 0),
			       SHELL_CMD_ARG(getfolderlist, NULL, HELP_NONE, cmd_get_folderlist, 1,
					     0),
			       SHELL_SUBCMD_SET_END);

static int cmd_map(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		/* sh returns 1 when help is printed */
		return 1;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(mapc, &map_client_cmds, "Bluetooth MAP Client sh commands", cmd_map, 1, 1);
