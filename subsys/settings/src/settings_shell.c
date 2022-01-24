/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <settings/settings.h>
#include <shell/shell.h>
#include <sys/util.h>
#include <toolchain.h>

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

	return 0;
}

struct settings_read_callback_params {
	const struct shell *shell_ptr;
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

	shell_hexdump(params->shell_ptr, buffer, num_read_bytes);

	if (len > SETTINGS_MAX_VAL_LEN) {
		shell_print(params->shell_ptr, "(The output has been truncated)");
	}

	return 0;
}

static int cmd_read(const struct shell *shell_ptr, size_t argc, char *argv[])
{
	int err;
	const char *name = argv[1];

	struct settings_read_callback_params params = {
		.shell_ptr = shell_ptr,
		.value_found = false
	};

	err = settings_load_subtree_direct(name, settings_read_callback, &params);

	if (err) {
		shell_error(shell_ptr, "Failed to load settings: %d", err);
	} else if (!params.value_found) {
		shell_error(shell_ptr, "Setting not found");
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(settings_cmds,
			       SHELL_CMD_ARG(list, NULL, "[<subtree>]", cmd_list, 1, 1),
			       SHELL_CMD_ARG(read, NULL, "<name>", cmd_read, 2, 0),
			       SHELL_SUBCMD_SET_END
			       );

static int cmd_settings(const struct shell *shell_ptr, size_t argc, char **argv)
{
	shell_error(shell_ptr, "%s unknown parameter: %s", argv[0], argv[1]);
	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(settings, &settings_cmds, "Settings shell commands",
		       cmd_settings, 2, 0);
