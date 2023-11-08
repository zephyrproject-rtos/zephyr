/**
 * @file
 * @brief Bluetooth Hearing Access Service (HAS) shell.
 *
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/has.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <stdio.h>

#include "shell/bt.h"

static struct bt_has_client *default_client;

static void has_client_connected_cb(struct bt_has_client *client, int err)
{
	if (err) {
		shell_error(ctx_shell, "Client %p connect failed (err %d)", client, err);
		return;
	}

	shell_print(ctx_shell, "Client %p connected", client);
}

static void has_client_disconnected_cb(struct bt_has_client *client)
{
	shell_print(ctx_shell, "Client %p disconnected", client);
}

static void has_client_unbound_cb(struct bt_has_client *client, int err)
{
	shell_print(ctx_shell, "Client %p unbound err %d", client, err);
}

static void has_client_preset_switch_cb(struct bt_has_client *client, uint8_t index)
{
	shell_print(ctx_shell, "Client %p preset switch index 0x%02x", client, index);
}

static void has_client_preset_read_rsp_cb(struct bt_has_client *client,
					  const struct bt_has_preset_record *record, bool is_last)
{
	shell_print(ctx_shell, "Preset Index: 0x%02x\tProperties: 0x%02x\tName: %s",
		    record->index, record->properties, record->name);

	if (is_last) {
		shell_print(ctx_shell, "Preset Read operation complete");
	}
}

static void has_cmd_status_cb(struct bt_has_client *client, uint8_t err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Client %p command (err 0x%02x)", client, err);

	} else {
		shell_print(ctx_shell, "Client %p command complete", client);
	}
}

static const struct bt_has_client_cb has_client_cb = {
	.connected = has_client_connected_cb,
	.disconnected = has_client_disconnected_cb,
	.unbound = has_client_unbound_cb,
	.preset_switch = has_client_preset_switch_cb,
	.preset_read_rsp = has_client_preset_read_rsp_cb,
	.cmd_status = has_cmd_status_cb,
};

static int cmd_has_client_init(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	if (!ctx_shell) {
		ctx_shell = sh;
	}

	err = bt_has_client_init(&has_client_cb);
	if (err != 0) {
		shell_error(sh, "bt_has_client_init (err %d)", err);
	}

	return err;
}

static int cmd_has_client_bind(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	if (!ctx_shell) {
		ctx_shell = sh;
	}

	if (default_client == NULL) {
		shell_error(sh, "No client");
		return -ENOEXEC;
	}

	err = bt_has_client_bind(default_conn, &default_client);
	if (err != 0) {
		shell_error(sh, "bt_has_client_bind (err %d)", err);
	}

	return err;
}

static int cmd_has_client_unbind(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	if (!ctx_shell) {
		ctx_shell = sh;
	}

	if (default_client == NULL) {
		shell_error(sh, "No client");
		return -ENOEXEC;
	}

	err = bt_has_client_unbind(default_client);
	if (err != 0) {
		shell_error(sh, "bt_has_client_unbind (err %d)", err);
	}

	return err;
}

static int cmd_has_client_read_presets(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	const uint8_t index = shell_strtoul(argv[1], 16, &err);
	const uint8_t count = shell_strtoul(argv[2], 10, &err);

	if (err < 0) {
		shell_error(sh, "Invalid command parameter (err %d)", err);
		return err;
	}

	if (default_client == NULL) {
		shell_error(sh, "No client");
		return -ENOEXEC;
	}

	err = bt_has_client_cmd_presets_read(default_client, index, count);
	if (err != 0) {
		shell_error(sh, "bt_has_client_discover (err %d)", err);
	}

	return err;
}

static int cmd_has_client_preset_set(const struct shell *sh, size_t argc, char **argv)
{
	bool sync = false;
	uint8_t index;
	int err = 0;

	index = shell_strtoul(argv[1], 16, &err);
	if (err < 0) {
		shell_error(sh, "Invalid command parameter (err %d)", err);
		return -ENOEXEC;
	}

	for (size_t argn = 2; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "sync")) {
			sync = true;
		} else {
			shell_error(sh, "Invalid argument");
			return -ENOEXEC;
		}
	}

	if (default_client == NULL) {
		shell_error(sh, "No client");
		return -ENOEXEC;
	}

	err = bt_has_client_cmd_preset_set(default_client, index, sync);
	if (err != 0) {
		shell_error(sh, "bt_has_client_preset_switch (err %d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_has_client_preset_next(const struct shell *sh, size_t argc, char **argv)
{
	bool sync = false;
	int err;

	for (size_t argn = 1; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "sync")) {
			sync = true;
		} else {
			shell_error(sh, "Invalid argument");
			return -ENOEXEC;
		}
	}

	if (default_client == NULL) {
		shell_error(sh, "No client");
		return -ENOEXEC;
	}

	err = bt_has_client_cmd_preset_next(default_client, sync);
	if (err != 0) {
		shell_error(sh, "bt_has_client_cmd_preset_next (err %d)", err);
		return -ENOEXEC;
	}

	return err;
}

static int cmd_has_client_preset_prev(const struct shell *sh, size_t argc, char **argv)
{
	bool sync = false;
	int err;

	for (size_t argn = 1; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "sync")) {
			sync = true;
		} else {
			shell_error(sh, "Invalid argument");
			return -ENOEXEC;
		}
	}

	if (default_client == NULL) {
		shell_error(sh, "No client");
		return -ENOEXEC;
	}

	err = bt_has_client_cmd_preset_prev(default_client, sync);
	if (err != 0) {
		shell_error(sh, "bt_has_client_cmd_preset_prev (err %d)", err);
		return -ENOEXEC;
	}

	return err;
}

static int cmd_has_client_preset_write(const struct shell *sh, size_t argc, char **argv)
{
	int err = 0;
	const uint8_t index = shell_strtoul(argv[1], 16, &err);

	if (err < 0) {
		shell_print(sh, "Invalid command parameter (err %d)", err);
		return err;
	}

	if (default_client == NULL) {
		shell_error(sh, "No client");
		return -ENOEXEC;
	}

	err = bt_has_client_cmd_preset_write(default_client, index, argv[2]);
	if (err != 0) {
		shell_error(sh, "bt_has_client_cmd_preset_write (err %d)", err);
		return -ENOEXEC;
	}

	return err;
}

static int cmd_has_client(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	} else {
		shell_error(sh, "%s missing subcomand", argv[0]);
	}

	return -ENOEXEC;
}

#define HELP_NONE "[none]"

SHELL_STATIC_SUBCMD_SET_CREATE(has_client_cmds,
	SHELL_CMD_ARG(init, NULL, HELP_NONE, cmd_has_client_init, 1, 0),
	SHELL_CMD_ARG(bind, NULL, HELP_NONE, cmd_has_client_bind, 1, 0),
	SHELL_CMD_ARG(unbind, NULL, HELP_NONE, cmd_has_client_unbind, 1, 0),
	SHELL_CMD_ARG(presets-read, NULL, "<start_index_hex> <max_count_dec>",
		      cmd_has_client_read_presets, 3, 0),
	SHELL_CMD_ARG(preset-set, NULL, "<index_hex> [sync]", cmd_has_client_preset_set, 2, 1),
	SHELL_CMD_ARG(preset-next, NULL, "[sync]", cmd_has_client_preset_next, 1, 1),
	SHELL_CMD_ARG(preset-prev, NULL, "[sync]", cmd_has_client_preset_prev, 1, 1),
	SHELL_CMD_ARG(preset-write, NULL, "<index_hex> <name>", cmd_has_client_preset_write, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(has_client, &has_client_cmds, "Bluetooth HAS client shell commands",
		       cmd_has_client, 1, 1);
