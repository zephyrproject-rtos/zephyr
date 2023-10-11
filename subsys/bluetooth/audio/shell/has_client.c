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

static struct bt_has *inst;

static void has_client_discover_cb(struct bt_conn *conn, int err, struct bt_has *has,
				   enum bt_has_hearing_aid_type type,
				   enum bt_has_capabilities caps)
{
	if (err) {
		shell_error(ctx_shell, "HAS discovery (err %d)", err);
		return;
	}

	shell_print(ctx_shell, "HAS discovered %p type 0x%02x caps 0x%02x for conn %p",
		    has, type, caps, conn);

	inst = has;
}

static void has_client_preset_switch_cb(struct bt_has *has, int err, uint8_t index)
{
	if (err != 0) {
		shell_error(ctx_shell, "HAS %p preset switch error (err %d)", has, err);
	} else {
		shell_print(ctx_shell, "HAS %p preset switch index 0x%02x", has, index);
	}
}

static void has_client_preset_read_rsp_cb(struct bt_has *has, int err,
					  const struct bt_has_preset_record *record, bool is_last)
{
	if (err) {
		shell_error(ctx_shell, "Preset Read operation failed (err %d)", err);
		return;
	}

	shell_print(ctx_shell, "Preset Index: 0x%02x\tProperties: 0x%02x\tName: %s",
		    record->index, record->properties, record->name);

	if (is_last) {
		shell_print(ctx_shell, "Preset Read operation complete");
	}
}

static const struct bt_has_client_cb has_client_cb = {
	.discover = has_client_discover_cb,
	.preset_switch = has_client_preset_switch_cb,
	.preset_read_rsp = has_client_preset_read_rsp_cb,
};

static int cmd_has_client_init(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	if (!ctx_shell) {
		ctx_shell = sh;
	}

	err = bt_has_client_cb_register(&has_client_cb);
	if (err != 0) {
		shell_error(sh, "bt_has_client_cb_register (err %d)", err);
	}

	return err;
}

static int cmd_has_client_discover(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!ctx_shell) {
		ctx_shell = sh;
	}

	err = bt_has_client_discover(default_conn);
	if (err != 0) {
		shell_error(sh, "bt_has_client_discover (err %d)", err);
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

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!inst) {
		shell_error(sh, "No instance discovered");
		return -ENOEXEC;
	}

	err = bt_has_client_presets_read(inst, index, count);
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

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!inst) {
		shell_error(sh, "No instance discovered");
		return -ENOEXEC;
	}

	err = bt_has_client_preset_set(inst, index, sync);
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

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!inst) {
		shell_error(sh, "No instance discovered");
		return -ENOEXEC;
	}

	err = bt_has_client_preset_next(inst, sync);
	if (err != 0) {
		shell_error(sh, "bt_has_client_preset_next (err %d)", err);
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

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!inst) {
		shell_error(sh, "No instance discovered");
		return -ENOEXEC;
	}

	err = bt_has_client_preset_prev(inst, sync);
	if (err != 0) {
		shell_error(sh, "bt_has_client_preset_prev (err %d)", err);
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
	SHELL_CMD_ARG(discover, NULL, HELP_NONE, cmd_has_client_discover, 1, 0),
	SHELL_CMD_ARG(presets-read, NULL, "<start_index_hex> <max_count_dec>",
		      cmd_has_client_read_presets, 3, 0),
	SHELL_CMD_ARG(preset-set, NULL, "<index_hex> [sync]", cmd_has_client_preset_set, 2, 1),
	SHELL_CMD_ARG(preset-next, NULL, "[sync]", cmd_has_client_preset_next, 1, 1),
	SHELL_CMD_ARG(preset-prev, NULL, "[sync]", cmd_has_client_preset_prev, 1, 1),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(has_client, &has_client_cmds, "Bluetooth HAS client shell commands",
		       cmd_has_client, 1, 1);
