/**
 * @file
 * @brief Bluetooth Hearing Access Service (HAS) shell.
 *
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <bluetooth/conn.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/audio/has.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <stdio.h>

#include "bt.h"

static int cmd_preset_reg(const struct shell *sh, size_t argc, char **argv)
{
	int err = 0;
	struct bt_has_preset_register_param param = {
		.index = shell_strtoul(argv[1], 16, &err),
		.properties = shell_strtoul(argv[2], 16, &err),
		.name = argv[3],
	};

	if (err < 0) {
		shell_print(sh, "Invalid command parameter (err %d)", err);
		return err;
	}

	err = bt_has_preset_register(&param);
	if (err < 0) {
		shell_error(sh, "Preset register failed (err %d)", err);
		return err;
	}

	return 0;
}

static int cmd_preset_unreg(const struct shell *sh, size_t argc, char **argv)
{
	int err = 0;
	const uint8_t index = shell_strtoul(argv[1], 16, &err);

	if (err < 0) {
		shell_print(sh, "Invalid command parameter (err %d)", err);
		return err;
	}

	err = bt_has_preset_unregister(index);
	if (err < 0) {
		shell_print(sh, "Preset unregister failed (err %d)", err);
		return err;
	}

	return 0;
}

struct print_list_entry_data {
	int num;
	const struct shell *sh;
};

static uint8_t print_list_entry(uint8_t index, enum bt_has_properties properties,
				const char *name, void *user_data)
{
	struct print_list_entry_data *data = user_data;

	shell_print(data->sh, "%d: index 0x%02x prop 0x%02x name %s", ++data->num, index,
		    properties, name);

	return BT_HAS_PRESET_ITER_CONTINUE;
}

static int cmd_preset_list(const struct shell *sh, size_t argc, char **argv)
{
	struct print_list_entry_data data = {
		.sh = sh,
	};

	bt_has_preset_foreach(0, print_list_entry, &data);

	if (data.num == 0) {
		shell_print(sh, "No presets registered");
	}

	return 0;
}

static int cmd_has(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	} else {
		shell_error(sh, "%s missing subcomand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(has_cmds,
	SHELL_CMD_ARG(preset-reg, NULL, "Register preset <index> <properties> <name>",
		      cmd_preset_reg, 4, 0),
	SHELL_CMD_ARG(preset-unreg, NULL, "Unregister preset <index>", cmd_preset_unreg, 2, 0),
	SHELL_CMD_ARG(preset-list, NULL, "List all presets", cmd_preset_list, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(has, &has_cmds, "Bluetooth HAS shell commands", cmd_has, 1, 1);
