/*
 * Copyright (c) 2018 Diego Sueiro
 * Copyright (c) 2022 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/drivers/w1.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>

#define BUF_SIZE CONFIG_W1_SHELL_BUFFER_SIZE
static uint8_t msg_buf[BUF_SIZE];

#define W1DEV_X_NOT_FOUND "1-Wire device not found: \"%s\""

#define OPTION_HELP_RESET "-r Perform bus reset before executing cmd."

static const char *w1_settings_name[W1_SETINGS_TYPE_COUNT] = {
	[W1_SETTING_SPEED] = "speed",
	[W1_SETTING_STRONG_PULLUP] = "spu",
};

static int read_io_options(const struct shell *sh, int pos, char **argv,
			   bool *reset)
{
	char *arg = argv[pos];

	if (arg[0] != '-') {
		return pos;
	}

	for (arg = &arg[1]; *arg; arg++) {
		switch (*arg) {
		case 'r':
			*reset = true;
			break;
		default:
			shell_error(sh, "Unknown option %c", *arg);
			return -EINVAL;
		}
	}

	return ++pos;
}

/* 1-Wire reset bus <device> */
static int cmd_w1_reset_bus(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, W1DEV_X_NOT_FOUND, argv[1]);
		return -EINVAL;
	}

	(void)w1_lock_bus(dev);
	ret = w1_reset_bus(dev);
	if (ret < 0) {
		shell_error(sh, "Failed to reset bus [%d]", ret);
	}

	(void)w1_unlock_bus(dev);
	return ret;
}

/* 1-Wire read_bit <device> */
static int cmd_w1_read_bit(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, W1DEV_X_NOT_FOUND, argv[1]);
		return -EINVAL;
	}

	(void)w1_lock_bus(dev);
	ret = w1_read_bit(dev);
	if (ret < 0) {
		shell_error(sh, "Failed to read bit [%d]", ret);
	} else {
		shell_print(sh, "Output: 0b%x", ret);
	}

	(void)w1_unlock_bus(dev);
	return ret;
}

/* 1-Wire read_byte <device> */
static int cmd_w1_read_byte(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, W1DEV_X_NOT_FOUND, argv[1]);
		return -EINVAL;
	}

	(void)w1_lock_bus(dev);
	ret = w1_read_byte(dev);
	if (ret < 0) {
		shell_error(sh, "Failed to read byte [%d]", ret);
	} else {
		shell_print(sh, "Output: 0x%x", ret);
	}

	(void)w1_unlock_bus(dev);
	return ret;
}

/* 1-Wire read_block <device> [num_bytes] */
static int cmd_w1_read_block(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	char *end_ptr;
	size_t read_len;
	int ret;

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, W1DEV_X_NOT_FOUND, argv[1]);
		return -EINVAL;
	}

	read_len = strtoul(argv[2], &end_ptr, 0);
	if (*end_ptr != '\0') {
		shell_error(sh, "num_bytes is not a number");
		return -EINVAL;
	}
	if (read_len > BUF_SIZE) {
		shell_error(sh, "num_bytes limited to: %u", BUF_SIZE);
		return -EINVAL;
	}

	(void)w1_lock_bus(dev);
	ret = w1_read_block(dev, msg_buf, read_len);
	if (ret < 0) {
		shell_error(sh, "Failed to read byte [%d]", ret);
		goto out;
	}

	shell_fprintf(sh, SHELL_NORMAL, "Output:");
	for (int i = 0; i < read_len; i++) {
		shell_fprintf(sh, SHELL_NORMAL, " 0x%02x", msg_buf[i]);
	}
	shell_fprintf(sh, SHELL_NORMAL, "\n");

out:
	(void)w1_unlock_bus(dev);
	return ret;
}

/* 1-Wire write_bit <device> <bit_value> */
static int cmd_w1_write_bit(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	unsigned long input = strtoul(argv[2], NULL, 0);
	int ret;

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, W1DEV_X_NOT_FOUND, argv[1]);
		return -EINVAL;
	}
	if (input > 1UL) {
		shell_error(sh, "input must not be > 0b1");
		return -EINVAL;
	}

	(void)w1_lock_bus(dev);
	ret = w1_write_byte(dev, (bool)input);
	if (ret < 0) {
		shell_error(sh, "Failed to write bit [%d]", ret);
	}

	(void)w1_unlock_bus(dev);
	return ret;
}

/* 1-Wire write_byte <device> <byte_value> */
static int cmd_w1_write_byte(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	unsigned long input;
	int pos = 1;
	bool reset = false;
	int ret;

	dev = device_get_binding(argv[pos]);
	if (!dev) {
		shell_error(sh, W1DEV_X_NOT_FOUND, argv[pos]);
		return -EINVAL;
	}
	pos++;

	pos = read_io_options(sh, pos, argv, &reset);
	if (pos < 0) {
		return -EINVAL;
	}
	if (argc <= pos) {
		shell_error(sh, "Missing data to be written.");
		return -EINVAL;
	}

	input = strtoul(argv[pos], NULL, 0);
	if (input > 0xFFUL) {
		shell_error(sh, "input must not be > 0xFF");
		return -EINVAL;
	}

	(void)w1_lock_bus(dev);
	if (reset) {
		ret = w1_reset_bus(dev);
		if (ret <= 0) {
			shell_error(sh, "Failed to reset bus [%d]", ret);
			goto out;
		}
	}

	ret = w1_write_byte(dev, (uint8_t)input);
	if (ret < 0) {
		shell_error(sh, "Failed to write byte [%d]", ret);
	}
out:
	(void)w1_unlock_bus(dev);
	return ret;
}

/* 1-Wire write_block <device> <byt1> [byte2, ...] */
static int cmd_w1_write_block(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int i;
	int pos = 1;
	bool reset = false;
	int ret;

	dev = device_get_binding(argv[pos]);
	if (!dev) {
		shell_error(sh, W1DEV_X_NOT_FOUND, argv[1]);
		return -EINVAL;
	}
	pos++;

	pos = read_io_options(sh, pos, argv, &reset);
	if (pos < 0) {
		return -EINVAL;
	}
	if (argc <= pos) {
		shell_error(sh, "Missing data to be written.");
		return -EINVAL;
	}
	if ((argc - pos) > BUF_SIZE) {
		shell_error(sh, "Too much data to be written.");
		return -EINVAL;
	}

	(void)w1_lock_bus(dev);
	i = 0;
	do {
		msg_buf[i] = (uint8_t)strtoul(argv[i + pos], NULL, 16);
		i++;
	} while ((i + pos) < argc);

	if (reset) {
		ret = w1_reset_bus(dev);
		if (ret <= 0) {
			shell_error(sh, "Failed to reset bus [%d]", ret);
			goto out;
		}
	}

	ret = w1_write_block(dev, msg_buf, i);
	if (ret < 0) {
		shell_error(sh, "Failed to write block [%d]", ret);
	}
out:
	(void)w1_unlock_bus(dev);
	return ret;
}

/* 1-Wire config <device> <type> <value> */
static int cmd_w1_configure(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	char *type_endptr;
	char *type_name = argv[2];
	int ret;
	uint32_t type = strtoul(type_name, &type_endptr, 0);
	uint32_t value = strtoul(argv[3], NULL, 0);

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, W1DEV_X_NOT_FOUND, argv[1]);
		return -EINVAL;
	}

	/* if type is not given as number, search it via the name */
	if (*type_endptr != '\0') {
		for (type = 0; type < ARRAY_SIZE(w1_settings_name); type++) {
			if (strcmp(type_name, w1_settings_name[type]) == 0) {
				break;
			}
		}

		if (type == ARRAY_SIZE(w1_settings_name)) {
			shell_error(sh, "Unknown config name (%s)", type_name);
			return -ENOTSUP;
		}
	}

	if (type > W1_SETINGS_TYPE_COUNT) {
		shell_error(sh, "invalid type %u", type);
		return -EINVAL;
	}

	(void)w1_lock_bus(dev);
	ret = w1_configure(dev, type, value);
	if (ret < 0) {
		shell_error(sh, "Failed to configure [%d]", ret);
		goto out;
	}

	shell_info(sh, "Applied config: %s = %u (0x%08x)",
		   w1_settings_name[type], value, value);

out:
	(void)w1_unlock_bus(dev);
	return ret;
}

static void search_callback(struct w1_rom rom, void *user_data)
{
	const struct shell *sh = (const struct shell *)user_data;

	shell_print(sh, "ROM found: %016llx", w1_rom_to_uint64(&rom));
}

/* 1-Wire search <device> */
static int cmd_w1_search(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, W1DEV_X_NOT_FOUND, argv[1]);
		return -EINVAL;
	}

	(void)w1_lock_bus(dev);
	ret = w1_search_rom(dev, search_callback, (void *)sh);
	if (ret < 0) {
		shell_error(sh, "Failed to initiate search [%d]", ret);
	} else {
		shell_print(sh, "Found %d device(s)", ret);
	}

	(void)w1_unlock_bus(dev);
	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_w1,
	SHELL_CMD_ARG(reset, NULL,
		      "Reset 1-Wire bus.\n"
		      "Usage: <device>",
		      cmd_w1_reset_bus, 2, 0),
	SHELL_CMD_ARG(read_bit, NULL,
		      "Read 1-Wire bit.\n"
		      "Usage: <device>",
		      cmd_w1_read_bit, 2, 0),
	SHELL_CMD_ARG(read_byte, NULL,
		      "Read 1-Wire byte.\n"
		      "Usage: <device>",
		      cmd_w1_read_byte, 2, 0),
	SHELL_CMD_ARG(read_block, NULL,
		      "Read 1-Wire block.\n"
		      "Usage: <device> <num_bytes>",
		      cmd_w1_read_block, 3, 0),
	SHELL_CMD_ARG(write_bit, NULL,
		      "Write 1-Wire bit.\n"
		      "Usage: <device> <bit>",
		      cmd_w1_write_bit, 3, 0),
	SHELL_CMD_ARG(write_byte, NULL,
		      "Write 1-Wire byte.\n"
		      "Usage: <device> [-r] <byte>\n"
		      OPTION_HELP_RESET,
		      cmd_w1_write_byte, 3, 1),
	SHELL_CMD_ARG(write_block, NULL,
		      "Write 1-Wire block.\n"
		      "Usage: <device> [-r] <byte1> [<byte2>, ...]\n"
		      OPTION_HELP_RESET,
		      cmd_w1_write_block, 3, BUF_SIZE),
	SHELL_CMD_ARG(config, NULL,
		      "Configure 1-Wire host.\n"
		      "Usage: <device> <type> <value>\n"
		      "<type> is either a name or an id.",
		      cmd_w1_configure, 4, 0),
	SHELL_CMD_ARG(search, NULL,
		      "1-Wire devices.\n"
		      "Usage: <device>",
		      cmd_w1_search, 2, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_ARG_REGISTER(w1, &sub_w1, "1-Wire commands", NULL, 2, 0);
