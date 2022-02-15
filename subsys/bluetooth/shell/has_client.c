/** @file
 *  @brief Bluetooth Hearing Access Service (HAS) client shell.
 *
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <bluetooth/audio/has.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <shell/shell.h>
#include <zephyr/types.h>

#include "bt.h"

static struct bt_has *g_has;

static void has_discover_cb(struct bt_conn *conn, struct bt_has *has, enum bt_has_hearing_aid_type type)
{
	if (has == NULL) {
		shell_error(ctx_shell, "Failed to discover HAS");
		return;
	}

	g_has = has;

	shell_print(ctx_shell, "HAS discovered (type %u)", type);
}

static void has_active_preset_cb(struct bt_has *has, int err, uint8_t index)
{
	if (err != 0) {
		shell_error(ctx_shell, "HAS preset get failed (%d) for inst %p", err, has);
	} else {
		shell_print(ctx_shell, "Preset (%u) set successful", index);
	}
}

static void has_preset_cb(struct bt_has *has, int err, uint8_t index, uint8_t properties,
			  const char *name)
{
	if (err != 0) {
		shell_error(ctx_shell, "HAS preset change failed %d for inst %p", err, has);
	} else {
		shell_print(ctx_shell, "Preset changed Index: %u Properties: 0x%02x Name: %s",
			    index, properties, name);
	}
}

static void has_preset_read_complete_cb(struct bt_has *has, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Preset read complete failed %d for inst %p", err, has);
	} else {
		shell_print(ctx_shell, "Preset read complete");
	}
}

static void mtu_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	int result;

	if (err > 0) {
		shell_error(ctx_shell, "Failed to exchange MTU (err %u)\n", err);
		return;
	}

	result = bt_has_discover(default_conn);
	if (result < 0) {
		shell_error(ctx_shell, "Fail (err %d)", result);
	}
}

static struct bt_has_client_cb has_cbs = {
	.discover = has_discover_cb,
	.active_preset = has_active_preset_cb,
	.preset = has_preset_cb,
	.preset_read_complete = has_preset_read_complete_cb,
};

static int cmd_has_discover(const struct shell *sh, size_t argc, char **argv)
{
	int result;
	static struct bt_gatt_exchange_params mtu_params = {
		.func = mtu_cb,
	};

	if (!ctx_shell) {
		ctx_shell = sh;
	}

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_gatt_exchange_mtu(default_conn, &mtu_params);
	if (result < 0) {
		shell_error(sh, "Failed to exchange MTU (err %d)\n", result);
	}

	return result;
}

static int cmd_has_client_init(const struct shell *sh, size_t argc, char **argv)
{
	int result;

	if (!ctx_shell) {
		ctx_shell = sh;
	}

	result = bt_has_client_cb_register(&has_cbs);
	if (result < 0) {
		shell_error(sh, "CB register failed (err %d)", result);
	} else {
		shell_print(sh, "HAS client initialized");
	}

	return result;
}

static int cmd_has_get_active(const struct shell *sh, size_t argc, char **argv)
{
	int result;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_has_preset_active_get(g_has);
	if (result < 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_has_set_active(const struct shell *sh, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_has_preset_active_set(g_has, index);
	if (result < 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_has_set_next(const struct shell *sh, size_t argc, char **argv)
{
	int result;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_has_preset_active_set_next(g_has);
	if (result < 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_has_set_prev(const struct shell *sh, size_t argc, char **argv)
{
	int result;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_has_preset_active_set_prev(g_has);
	if (result < 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_has_read_presets(const struct shell *sh, size_t argc, char **argv)
{
	int result;

	result = bt_has_preset_read_multiple(g_has, 0x01, 0xff);
	if (result < 0) {
		shell_print(sh, "Failed to read all presets (err %d)", result);
	}

	return result;
}

static int cmd_has_set_preset_name(const struct shell *sh, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	char *name = argv[2];

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_has_preset_name_set(g_has, index, name);
	if (result < 0) {
		shell_print(sh, "Failed to set preset name (err %d)", result);
	}

	return result;
}

static int cmd_has_client(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

#define HELP_NONE "[none]"

SHELL_STATIC_SUBCMD_SET_CREATE(
	has_client_cmds, SHELL_CMD_ARG(init, NULL, HELP_NONE, cmd_has_client_init, 1, 0),
	SHELL_CMD_ARG(discover, NULL, HELP_NONE, cmd_has_discover, 1, 0),
	SHELL_CMD_ARG(get-active, NULL, HELP_NONE, cmd_has_get_active, 1, 0),
	SHELL_CMD_ARG(set-active, NULL, "<index>", cmd_has_set_active, 2, 0),
	SHELL_CMD_ARG(set-next, NULL, HELP_NONE, cmd_has_set_next, 1, 0),
	SHELL_CMD_ARG(set-prev, NULL, HELP_NONE, cmd_has_set_prev, 1, 0),
	SHELL_CMD_ARG(read-all, NULL, HELP_NONE, cmd_has_read_presets, 1, 0),
	SHELL_CMD_ARG(set-name, NULL, "<index> <name>", cmd_has_set_preset_name, 3, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(has_client, &has_client_cmds, "Bluetooth HAS Client shell commands",
		       cmd_has_client, 1, 1);
