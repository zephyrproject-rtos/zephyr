/*
 * Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief DAC shell commands.
 */

#include <zephyr/shell/shell.h>
#include <zephyr/drivers/dac.h>
#include <stdlib.h>

struct args_index {
	uint8_t device;
	uint8_t channel;
	uint8_t value;
	uint8_t resolution;
	uint8_t options;
};

static const struct args_index args_indx = {
	.device = 1,
	.channel = 2,
	.value = 3,
	.resolution = 3,
	.options = 4,
};

static int cmd_setup(const struct shell *sh, size_t argc, char **argv)
{
	struct dac_channel_cfg cfg = {0};
	const struct device *dac;
	int argidx;
	int err;

	dac = shell_device_get_binding(argv[args_indx.device]);
	if (!dac) {
		shell_error(sh, "DAC device not found");
		return -EINVAL;
	}

	cfg.channel_id = strtoul(argv[args_indx.channel], NULL, 0);
	cfg.resolution = strtoul(argv[args_indx.resolution], NULL, 0);

	argidx = args_indx.options;
	while (argidx < argc && strncmp(argv[argidx], "-", 1) == 0) {
		if (strcmp(argv[argidx], "-b") == 0) {
			cfg.buffered = true;
			argidx++;
		} else if (strcmp(argv[argidx], "-i") == 0) {
			cfg.internal = true;
			argidx++;
		} else {
			shell_error(sh, "unsupported option %s", argv[argidx]);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	err = dac_channel_setup(dac, &cfg);
	if (err) {
		shell_error(sh, "Failed to setup DAC channel (err %d)", err);
		return err;
	}

	return 0;
}

static int cmd_write_value(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dac;
	uint8_t channel;
	uint32_t value;
	int err;

	dac = shell_device_get_binding(argv[args_indx.device]);
	if (!dac) {
		shell_error(sh, "DAC device not found");
		return -EINVAL;
	}

	channel = strtoul(argv[args_indx.channel], NULL, 0);
	value = strtoul(argv[args_indx.value], NULL, 0);

	err = dac_write_value(dac, channel, value);
	if (err) {
		shell_error(sh, "Failed to write DAC value (err %d)", err);
		return err;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(dac_cmds,
	SHELL_CMD_ARG(setup, NULL,
		      "Setup DAC channel\n"
		      "Usage: setup <device> <channel> <resolution> [-b] [-i]\n"
		      "-b Enable output buffer\n"
		      "-i Connect internally",
		      cmd_setup, 4, 2),
	SHELL_CMD_ARG(write_value, NULL,
		      "Write DAC value\n"
		      "Usage: write <device> <channel> <value>",
		      cmd_write_value, 4, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(dac, &dac_cmds, "DAC shell commands", NULL);
