/** @file
 *  @brief Bluetooth Hearing Access Service (HAS) server shell.
 *
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <bluetooth/conn.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/audio/has.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <stdio.h>

#include "bt.h"

static struct bt_has *has;

static const uint8_t g_universal_idx = 1;
static const uint8_t g_outdoor_idx = 5;
static const uint8_t g_noisy_idx = 8;
static const uint8_t g_office_idx = 22;

static int set_active_preset_cb(struct bt_has *has, uint8_t index, bool sync)
{
	int err;

	shell_print(ctx_shell, "Set active preset index 0x%02x sync %d", index, sync);

	err = bt_has_preset_active_set(has, index);
	if (err < 0) {
		shell_error(ctx_shell, "Set active failed (err %d)", err);
	}

	return err;
}

struct bt_has_ops preset_ops = {
	.active_set = set_active_preset_cb,
};

static int cmd_has(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcomand", argv[0]);
	}

	return -ENOEXEC;
}

static int cmd_has_init(const struct shell *sh, size_t argc, char **argv)
{
	int result;
	struct bt_has_register_param register_param = {
		.preset_param = {
			{
				.index = g_universal_idx,
				.properties = BT_HAS_PROP_WRITABLE | BT_HAS_PROP_AVAILABLE,
				.name = "Universal",
			},
			{
				.index = g_outdoor_idx,
				.properties = BT_HAS_PROP_WRITABLE | BT_HAS_PROP_AVAILABLE,
				.name = "Outdoor",
			},
			{
				.index = g_noisy_idx,
				.properties = BT_HAS_PROP_WRITABLE | BT_HAS_PROP_AVAILABLE,
				.name = "Noisy environment",
			},
			{
				.index = g_office_idx,
				.properties = BT_HAS_PROP_WRITABLE | BT_HAS_PROP_AVAILABLE,
				.name = "Office",
			},
		},
		.ops = &preset_ops,
	};

	if (!ctx_shell) {
		ctx_shell = sh;
	}

	result = bt_has_register(&register_param, &has);
	if (result < 0) {
		shell_error(ctx_shell, "HAS preset ops register failed (err %d)", result);
	} else {
		shell_print(ctx_shell, "HAS server initialized");
	}

	return result;
}

static int cmd_has_set_active(const struct shell *sh, size_t argc, char **argv)
{
	int result;
	const int index = strtol(argv[1], NULL, 0);

	result = bt_has_preset_active_set(has, index);
	if (result < 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_has_set_available(const struct shell *sh, size_t argc, char **argv)
{
	int result;
	const int index = strtol(argv[1], NULL, 0);
	const char *available = argv[2];

	if (!strcmp(available, "on")) {
		result = bt_has_preset_availability_set(has, index, true);
	} else if (!strcmp(available, "off")) {
		result = bt_has_preset_availability_set(has, index, false);
	} else {
		shell_error(sh, "Invalid argument");
		return -EINVAL;
	}

	if (result < 0) {
		shell_error(sh, "Failed to set preset availability (err %d)", result);
	}

	return result;
}

static int cmd_has_set_visible(const struct shell *sh, size_t argc, char **argv)
{
	int result;
	const int index = strtol(argv[1], NULL, 0);
	const char *visible = argv[2];

	if (!strcmp(visible, "on")) {
		result = bt_has_preset_visibility_set(has, index, true);
	} else if (!strcmp(visible, "off")) {
		result = bt_has_preset_visibility_set(has, index, false);
	} else {
		shell_error(sh, "Invalid argument");
		return -EINVAL;
	}

	if (result < 0) {
		shell_error(sh, "Failed to set preset availability (err %d)", result);
	}

	return result;
}

static int cmd_has_set_preset_name(const struct shell *sh, size_t argc, char **argv)
{
	int result;
	const int index = strtol(argv[1], NULL, 0);
	const char *name = argv[2];

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_has_preset_name_set(has, index, name);
	if (result < 0) {
		shell_print(sh, "Failed to set preset name (err %d)", result);
	}

	return result;
}

#define HELP_NONE "[none]"

SHELL_STATIC_SUBCMD_SET_CREATE(has_cmds,
	SHELL_CMD_ARG(init, NULL, HELP_NONE, cmd_has_init, 1, 0),
	SHELL_CMD_ARG(set-active, NULL, "<index>", cmd_has_set_active, 2, 0),
	SHELL_CMD_ARG(set-available, NULL, "<index> <on, off>", cmd_has_set_available, 3, 0),
	SHELL_CMD_ARG(set-visible, NULL, "<index> <on, off>", cmd_has_set_visible, 3, 0),
	SHELL_CMD_ARG(set-name, NULL, "<index> <name>", cmd_has_set_preset_name, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(has, &has_cmds, "Bluetooth HAS shell commands", cmd_has, 1, 1);
