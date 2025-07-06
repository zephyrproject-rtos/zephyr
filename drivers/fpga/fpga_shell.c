/*
 * Copyright (c) 2021 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <zephyr/version.h>
#include <stdlib.h>
#include <zephyr/drivers/fpga.h>

static int parse_common_args(const struct shell *sh, char **argv,
			     const struct device **dev)
{
	*dev = shell_device_get_binding(argv[1]);
	if (!*dev) {
		shell_error(sh, "FPGA device %s not found", argv[1]);
		return -ENODEV;
	}
	return 0;
}

static int cmd_on(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err;

	err = parse_common_args(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	shell_print(sh, "%s: turning on", dev->name);

	err = fpga_on(dev);
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static int cmd_off(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err;

	err = parse_common_args(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	shell_print(sh, "%s: turning off", dev->name);

	err = fpga_off(dev);
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static int cmd_reset(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err;

	err = parse_common_args(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	shell_print(sh, "%s: resetting FPGA", dev->name);

	err = fpga_reset(dev);
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static int cmd_load(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err;

	err = parse_common_args(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	shell_print(sh, "%s: loading bitstream", dev->name);

	err = fpga_load(dev, (uint32_t *)strtol(argv[2], NULL, 0),
			(uint32_t)atoi(argv[3]));
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static int cmd_get_status(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err;

	err = parse_common_args(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	shell_print(sh, "%s status: %d", dev->name, fpga_get_status(dev));

	return err;
}

static int cmd_get_info(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err;

	err = parse_common_args(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	shell_print(sh, "%s", fpga_get_info(dev));

	return err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_fpga, SHELL_CMD_ARG(off, NULL, "<device>", cmd_off, 2, 0),
	SHELL_CMD_ARG(on, NULL, "<device>", cmd_on, 2, 0),
	SHELL_CMD_ARG(reset, NULL, "<device>", cmd_reset, 2, 0),
	SHELL_CMD_ARG(load, NULL, "<device> <address> <size in bytes>",
		      cmd_load, 4, 0),
	SHELL_CMD_ARG(get_status, NULL, "<device>", cmd_get_status, 2, 0),
	SHELL_CMD_ARG(get_info, NULL, "<device>", cmd_get_info, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(fpga, &sub_fpga, "FPGA commands", NULL);
