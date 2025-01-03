/**
 * @file
 * @brief Bluetooth Hearing Access Service (HAS) shell.
 *
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/audio/has.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

static int preset_select(uint8_t index, bool sync)
{
	return 0;
}

static void preset_name_changed(uint8_t index, const char *name)
{
	bt_shell_print("Preset name changed index %u name %s", index, name);
}

static const struct bt_has_preset_ops preset_ops = {
	.select = preset_select,
	.name_changed = preset_name_changed,
};

static int cmd_preset_reg(const struct shell *sh, size_t argc, char **argv)
{
	int err = 0;
	struct bt_has_preset_register_param param = {
		.index = shell_strtoul(argv[1], 16, &err),
		.properties = shell_strtoul(argv[2], 16, &err),
		.name = argv[3],
		.ops = &preset_ops,
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

static int cmd_features_set(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	struct bt_has_features_param param = {
		.type = BT_HAS_HEARING_AID_TYPE_MONAURAL,
		.preset_sync_support = false,
		.independent_presets = false
	};

	for (size_t argn = 1; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (strcmp(arg, "binaural") == 0) {
			param.type = BT_HAS_HEARING_AID_TYPE_BINAURAL;
		} else if (strcmp(arg, "monaural") == 0) {
			param.type = BT_HAS_HEARING_AID_TYPE_MONAURAL;
		} else if (strcmp(arg, "banded") == 0) {
			param.type = BT_HAS_HEARING_AID_TYPE_BANDED;
		} else if (strcmp(arg, "sync") == 0) {
			param.preset_sync_support = true;
		} else if (strcmp(arg, "independent") == 0) {
			param.independent_presets = true;
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	err = bt_has_features_set(&param);
	if (err != 0) {
		shell_error(sh, "Could not set features: %d", err);
		return err;
	}

	return 0;
}

static int cmd_has_register(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	struct bt_has_features_param param = {
		.type = BT_HAS_HEARING_AID_TYPE_MONAURAL,
		.preset_sync_support = false,
		.independent_presets = false
	};

	for (size_t argn = 1; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (strcmp(arg, "binaural") == 0) {
			param.type = BT_HAS_HEARING_AID_TYPE_BINAURAL;
		} else if (strcmp(arg, "monaural") == 0) {
			param.type = BT_HAS_HEARING_AID_TYPE_MONAURAL;
		} else if (strcmp(arg, "banded") == 0) {
			param.type = BT_HAS_HEARING_AID_TYPE_BANDED;
		} else if (strcmp(arg, "sync") == 0) {
			param.preset_sync_support = true;
		} else if (strcmp(arg, "independent") == 0) {
			param.independent_presets = true;
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	err = bt_has_register(&param);
	if (err != 0) {
		shell_error(sh, "Could not register HAS: %d", err);
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

static int cmd_preset_avail(const struct shell *sh, size_t argc, char **argv)
{
	int err = 0;
	const uint8_t index = shell_strtoul(argv[1], 16, &err);

	if (err < 0) {
		shell_print(sh, "Invalid command parameter (err %d)", err);
		return err;
	}

	err = bt_has_preset_available(index);
	if (err < 0) {
		shell_print(sh, "Preset availability set failed (err %d)", err);
		return err;
	}

	return 0;
}

static int cmd_preset_unavail(const struct shell *sh, size_t argc, char **argv)
{
	int err = 0;
	const uint8_t index = shell_strtoul(argv[1], 16, &err);

	if (err < 0) {
		shell_print(sh, "Invalid command parameter (err %d)", err);
		return err;
	}

	err = bt_has_preset_unavailable(index);
	if (err < 0) {
		shell_print(sh, "Preset availability set failed (err %d)", err);
		return err;
	}

	return 0;
}

static int cmd_preset_active_set(const struct shell *sh, size_t argc, char **argv)
{
	int err = 0;
	const uint8_t index = shell_strtoul(argv[1], 16, &err);

	if (err < 0) {
		shell_print(sh, "Invalid command parameter (err %d)", err);
		return err;
	}

	err = bt_has_preset_active_set(index);
	if (err < 0) {
		shell_print(sh, "Preset selection failed (err %d)", err);
		return err;
	}

	return 0;
}

static int cmd_preset_active_get(const struct shell *sh, size_t argc, char **argv)
{
	const uint8_t index = bt_has_preset_active_get();

	shell_print(sh, "Active index 0x%02x", index);

	return 0;
}

static int cmd_preset_active_clear(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_has_preset_active_clear();
	if (err < 0) {
		shell_print(sh, "Preset selection failed (err %d)", err);
		return err;
	}

	return 0;
}

static int cmd_preset_name_set(const struct shell *sh, size_t argc, char **argv)
{
	int err = 0;
	const uint8_t index = shell_strtoul(argv[1], 16, &err);

	if (err < 0) {
		shell_print(sh, "Invalid command parameter (err %d)", err);
		return err;
	}

	err = bt_has_preset_name_change(index, argv[2]);
	if (err < 0) {
		shell_print(sh, "Preset name change failed (err %d)", err);
		return err;
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
	SHELL_CMD_ARG(register, NULL,
		      "Initialize the service and register type "
		      "[binaural | monaural(default) | banded] [sync] [independent]",
		      cmd_has_register, 1, 3),
	SHELL_CMD_ARG(preset_reg, NULL, "Register preset <index> <properties> <name>",
		      cmd_preset_reg, 4, 0),
	SHELL_CMD_ARG(preset_unreg, NULL, "Unregister preset <index>", cmd_preset_unreg, 2, 0),
	SHELL_CMD_ARG(preset_list, NULL, "List all presets", cmd_preset_list, 1, 0),
	SHELL_CMD_ARG(preset_set_avail, NULL, "Set preset as available <index>",
		      cmd_preset_avail, 2, 0),
	SHELL_CMD_ARG(preset_set_unavail, NULL, "Set preset as unavailable <index>",
		      cmd_preset_unavail, 2, 0),
	SHELL_CMD_ARG(preset_active_set, NULL, "Set active preset <index>",
		      cmd_preset_active_set, 2, 0),
	SHELL_CMD_ARG(preset_active_get, NULL, "Get active preset", cmd_preset_active_get, 1, 0),
	SHELL_CMD_ARG(preset_active_clear, NULL, "Clear selected preset",
		      cmd_preset_active_clear, 1, 0),
	SHELL_CMD_ARG(set_name, NULL, "Set preset name <index> <name>", cmd_preset_name_set, 3, 0),
	SHELL_CMD_ARG(features_set, NULL, "Set hearing aid features "
		      "[binaural | monaural(default) | banded] [sync] [independent]",
		      cmd_features_set, 1, 3),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(has, &has_cmds, "Bluetooth HAS shell commands", cmd_has, 1, 1);
