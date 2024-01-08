/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/settings/settings.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include <stdint.h>
#include <stddef.h>

struct settings_list_callback_params {
	const struct shell *shell_ptr;
	const char *subtree;
};

static int settings_list_callback(const char      *key,
				  size_t len,
				  settings_read_cb read_cb,
				  void            *cb_arg,
				  void            *param)
{
	ARG_UNUSED(len);
	ARG_UNUSED(read_cb);
	ARG_UNUSED(cb_arg);

	struct settings_list_callback_params *params = param;

	if (params->subtree != NULL) {
		shell_print(params->shell_ptr, "%s/%s", params->subtree, key);
	} else {
		shell_print(params->shell_ptr, "%s", key);
	}

	return 0;
}

static int cmd_list(const struct shell *shell_ptr, size_t argc, char *argv[])
{
	int err;

	struct settings_list_callback_params params = {
		.shell_ptr = shell_ptr,
		.subtree = (argc == 2 ? argv[1] : NULL)
	};

	err = settings_load_subtree_direct(params.subtree, settings_list_callback, &params);

	if (err) {
		shell_error(shell_ptr, "Failed to load settings: %d", err);
	}

	return err;
}

enum settings_value_types {
	SETTINGS_VALUE_HEX,
	SETTINGS_VALUE_STRING,
};

struct settings_read_callback_params {
	const struct shell *shell_ptr;
	const enum settings_value_types value_type;
	bool value_found;
};

static int settings_read_callback(const char *key,
				  size_t len,
				  settings_read_cb read_cb,
				  void            *cb_arg,
				  void            *param)
{
	uint8_t buffer[SETTINGS_MAX_VAL_LEN];
	ssize_t num_read_bytes = MIN(len, SETTINGS_MAX_VAL_LEN);
	struct settings_read_callback_params *params = param;

	/* Process only the exact match and ignore descendants of the searched name */
	if (settings_name_next(key, NULL) != 0) {
		return 0;
	}

	params->value_found = true;
	num_read_bytes = read_cb(cb_arg, buffer, num_read_bytes);

	if (num_read_bytes < 0) {
		shell_error(params->shell_ptr, "Failed to read value: %d", (int) num_read_bytes);
		return 0;
	}

	if (num_read_bytes == 0) {
		shell_warn(params->shell_ptr, "Value is empty");
		return 0;
	}

	switch (params->value_type) {
	case SETTINGS_VALUE_HEX:
		shell_hexdump(params->shell_ptr, buffer, num_read_bytes);
		break;
	case SETTINGS_VALUE_STRING:
		if (buffer[num_read_bytes - 1] != '\0') {
			shell_error(params->shell_ptr, "Value is not a string");
			return 0;
		}
		shell_print(params->shell_ptr, "%s", buffer);
		break;
	}

	if (len > SETTINGS_MAX_VAL_LEN) {
		shell_print(params->shell_ptr, "(The output has been truncated)");
	}

	return 0;
}

static int settings_parse_type(const char *type, enum settings_value_types *value_type)
{
	if (strcmp(type, "string") == 0) {
		*value_type = SETTINGS_VALUE_STRING;
	} else if (strcmp(type, "hex") == 0) {
		*value_type = SETTINGS_VALUE_HEX;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int cmd_read(const struct shell *shell_ptr, size_t argc, char *argv[])
{
	int err;

	enum settings_value_types value_type = SETTINGS_VALUE_HEX;

	if (argc > 2) {
		err = settings_parse_type(argv[1], &value_type);
		if (err) {
			shell_error(shell_ptr, "Invalid type: %s", argv[1]);
			return err;
		}
	}

	struct settings_read_callback_params params = {
		.shell_ptr = shell_ptr,
		.value_type = value_type,
		.value_found = false
	};

	err = settings_load_subtree_direct(argv[argc - 1], settings_read_callback, &params);

	if (err) {
		shell_error(shell_ptr, "Failed to load setting: %d", err);
	} else if (!params.value_found) {
		err = -ENOENT;
		shell_error(shell_ptr, "Setting not found");
	}

	return err;
}

static int cmd_write(const struct shell *shell_ptr, size_t argc, char *argv[])
{
	int err;
	uint8_t buffer[CONFIG_SHELL_CMD_BUFF_SIZE / 2];
	size_t buffer_len = 0;
	enum settings_value_types value_type = SETTINGS_VALUE_HEX;

	if (argc > 3) {
		err = settings_parse_type(argv[1], &value_type);
		if (err) {
			shell_error(shell_ptr, "Invalid type: %s", argv[1]);
			return err;
		}
	}

	switch (value_type) {
	case SETTINGS_VALUE_HEX:
		buffer_len = hex2bin(argv[argc - 1], strlen(argv[argc - 1]),
			buffer, sizeof(buffer));
		break;
	case SETTINGS_VALUE_STRING:
		buffer_len = strlen(argv[argc - 1]) + 1;
		memcpy(buffer, argv[argc - 1], buffer_len);
		break;
	}

	if (buffer_len == 0) {
		shell_error(shell_ptr, "Failed to parse value");
		return -EINVAL;
	}

	err = settings_save_one(argv[argc - 2], buffer, buffer_len);

	if (err) {
		shell_error(shell_ptr, "Failed to write setting: %d", err);
	}

	return err;
}

static int cmd_delete(const struct shell *shell_ptr, size_t argc, char *argv[])
{
	int err;

	err = settings_delete(argv[1]);

	if (err) {
		shell_error(shell_ptr, "Failed to delete setting: %d", err);
	}

	return err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(settings_cmds,
	SHELL_CMD_ARG(list, NULL,
		"List all settings in a subtree (omit to list all)\n"
		"Usage: settings list [subtree]",
		cmd_list, 1, 1),
	SHELL_CMD_ARG(read, NULL,
		"Read a specific setting\n"
		"Usage: settings read [type] <name>\n"
		"type: string or hex (default: hex)",
		cmd_read, 2, 1),
	SHELL_CMD_ARG(write, NULL,
		"Write to a specific setting\n"
		"Usage: settings write [type] <name> <value>\n"
		"type: string or hex (default: hex)",
		cmd_write, 3, 1),
	SHELL_CMD_ARG(delete, NULL,
		"Delete a specific setting\n"
		"Usage: settings delete <name>",
		cmd_delete, 2, 0),
	SHELL_SUBCMD_SET_END);

static int cmd_settings(const struct shell *shell_ptr, size_t argc, char **argv)
{
	shell_error(shell_ptr, "%s unknown parameter: %s", argv[0], argv[1]);
	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(settings, &settings_cmds, "Settings shell commands",
		       cmd_settings, 2, 0);
