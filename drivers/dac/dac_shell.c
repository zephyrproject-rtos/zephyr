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
};

static const struct args_index args_indx = {
	.device = 1,
	.channel = 2,
	.value = 3,
	.resolution = 3,
};

static int cmd_setup(const struct shell *shell, size_t argc, char **argv)
{
	struct dac_channel_cfg cfg;
	const struct device *dac;
	int err;

	dac = device_get_binding(argv[args_indx.device]);
	if (!dac) {
		shell_error(shell, "DAC device not found");
		return -EINVAL;
	}

	cfg.channel_id = strtoul(argv[args_indx.channel], NULL, 0);
	cfg.resolution = strtoul(argv[args_indx.resolution], NULL, 0);

	err = dac_channel_setup(dac, &cfg);
	if (err) {
		shell_error(shell, "Failed to setup DAC channel (err %d)", err);
		return err;
	}

	return 0;
}

static int cmd_write_value(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dac;
	uint8_t channel;
	uint32_t value;
	int err;

	dac = device_get_binding(argv[args_indx.device]);
	if (!dac) {
		shell_error(shell, "DAC device not found");
		return -EINVAL;
	}

	channel = strtoul(argv[args_indx.channel], NULL, 0);
	value = strtoul(argv[args_indx.value], NULL, 0);

	err = dac_write_value(dac, channel, value);
	if (err) {
		shell_error(shell, "Failed to write DAC value (err %d)", err);
		return err;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(dac_cmds,
	SHELL_CMD_ARG(setup, NULL, "<device> <channel> <resolution>",
		      cmd_setup, 4, 0),
	SHELL_CMD_ARG(write_value, NULL, "<device> <channel> <value>",
		      cmd_write_value, 4, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(dac, &dac_cmds, "DAC shell commands", NULL);
