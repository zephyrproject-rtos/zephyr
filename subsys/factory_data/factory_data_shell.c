/*
 * Copyright (c) 2022 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/factory_data/factory_data.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

static int cmd_write(const struct shell *shell_ptr, size_t argc, char *argv[])
{
	int ret;
	const char *name = argv[1];
	const char *hex = argv[2];
	uint8_t binary[CONFIG_SHELL_CMD_BUFF_SIZE / 2];
	size_t number_of_bytes;

	ret = factory_data_init();
	if (ret) {
		shell_error(shell_ptr, "Failed to initialize: %d", ret);
		return -EIO;
	}

	number_of_bytes = hex2bin(hex, strlen(hex), binary, sizeof(binary));
	if (!number_of_bytes) {
		shell_error(shell_ptr, "Failed to parse hexstring");
		return -EINVAL;
	}

	ret = factory_data_save_one(name, &binary, number_of_bytes);
	if (ret == -ENOSPC) {
		shell_error(shell_ptr, "No more space left");
		return ret;
	}
	if (ret == -EEXIST) {
		shell_error(shell_ptr, "Value exists");
		return ret;
	}
	if (ret) {
		shell_error(shell_ptr, "Failed to save: %d", ret);
		return -EIO;
	}

	return 0;
}

struct cmd_list_callback_context {
	const struct shell *shell_ptr;
	const char *name;
	size_t found;
};

static int cmd_read_print_value_for_name_callback(const char *name, const uint8_t *value,
						  size_t len, const void *param)
{
	struct cmd_list_callback_context *ctx = (struct cmd_list_callback_context *)param;

	if (strcmp(name, ctx->name) == 0) {
		ctx->found++;
		shell_hexdump(ctx->shell_ptr, value, len);
	}

	return 0;
}

static int cmd_read(const struct shell *shell_ptr, size_t argc, char *argv[])
{
	int ret;
	struct cmd_list_callback_context ctx = {
		.name = argv[1],
		.shell_ptr = shell_ptr,
		.found = 0,
	};

	ret = factory_data_init();
	if (ret) {
		shell_error(shell_ptr, "Failed to initialize: %d", ret);
		return -EIO;
	}

	ret = factory_data_load(cmd_read_print_value_for_name_callback, &ctx);
	if (ret) {
		shell_error(shell_ptr, "Failed to read: %d", ret);
	}
	if (ctx.found == 0) {
		shell_error(shell_ptr, "Variable not found");
		return -ENOENT;
	} else if (ctx.found > 1) {
		shell_error(shell_ptr, "Variable found more than once!");
		return -ENFILE; /* Better error code? */
	}

	return 0;
}

static int cmd_erase(const struct shell *shell_ptr, size_t argc, char *argv[])
{
	int ret;

	ret = factory_data_init();
	if (ret) {
		shell_error(shell_ptr, "Failed to initialize: %d", ret);
		return -EIO;
	}

	ret = factory_data_erase();
	if (ret) {
		shell_error(shell_ptr, "Failed to erase: %d", ret);
		return -EIO;
	}

	return 0;
}

static int cmd_list_print_name_callback(const char *name, const uint8_t *value, size_t len,
					const void *param)
{
	const struct shell *shell_ptr = param;

	shell_print(shell_ptr, "%s", name);

	return 0;
}

static int cmd_list(const struct shell *shell_ptr, size_t argc, char *argv[])
{
	int ret;

	ret = factory_data_init();
	if (ret) {
		shell_error(shell_ptr, "Failed to initialize: %d", ret);
		return -EIO;
	}

	ret = factory_data_load(cmd_list_print_name_callback, shell_ptr);
	if (ret) {
		shell_error(shell_ptr, "Failed to load: %d", ret);
		return -EIO;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(factory_data_cmds,
			       SHELL_CMD(list, NULL, "list all entries", cmd_list),
			       SHELL_CMD_ARG(write, NULL, "<name> <hex>", cmd_write, 3, 0),
			       SHELL_CMD_ARG(read, NULL, "<name>", cmd_read, 2, 0),
			       SHELL_CMD(erase, NULL, "start over", cmd_erase),
			       SHELL_SUBCMD_SET_END);

static int cmd_factory_data(const struct shell *shell_ptr, size_t argc, char **argv)
{
	shell_error(shell_ptr, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(factory_data, &factory_data_cmds, "Factory data shell commands",
		       cmd_factory_data, 2, 0);
