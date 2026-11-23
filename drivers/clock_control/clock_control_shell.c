/*
 * Copyright (c) 2026 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h> /* strtol */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/shell/shell.h>

#define CLOCK_DEVICE_NODE DT_ALIAS(clock_control_dev)
const struct device *const clock_dev = DEVICE_DT_GET(CLOCK_DEVICE_NODE);

#define ARGV_DEV    1
#define ARGV_OUTPUT 2
#define ARGV_ARG    3

static int cmd_clock_on(const struct shell *sh, size_t argc, char **argv)
{
	int which;
	int rc;
	const struct device *dev = shell_device_get_binding(argv[ARGV_DEV]);

	if (!dev) {
		shell_error(sh, "ClockControl: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	which = strtol(argv[ARGV_OUTPUT], NULL, 10);
	rc = clock_control_on(dev, INT_TO_POINTER(which));

	shell_print(sh, "on for output %d = %d\n", which, rc);

	return 0;
}

static int cmd_clock_off(const struct shell *sh, size_t argc, char **argv)
{
	int which;
	int rc;
	const struct device *dev = shell_device_get_binding(argv[ARGV_DEV]);

	if (!dev) {
		shell_error(sh, "ClockControl: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	which = strtol(argv[ARGV_OUTPUT], NULL, 10);
	rc = clock_control_off(dev, INT_TO_POINTER(which));

	shell_print(sh, "off for output %d = %d\n", which, rc);

	return 0;
}

static int cmd_get_rate(const struct shell *sh, size_t argc, char **argv)
{
	int which;
	uint32_t rate;
	int rc = 0;
	const struct device *dev = shell_device_get_binding(argv[ARGV_DEV]);

	if (!dev) {
		shell_error(sh, "ClockControl: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	which = strtol(argv[ARGV_OUTPUT], NULL, 10);
	rc = clock_control_get_rate(dev, INT_TO_POINTER(which), &rate);

	if (rc == 0) {
		shell_print(sh, "rate on output %d = %d\n", which, rate);
	} else {
		shell_print(sh, "clock_control_get_rate returned %d\n", rc);
	}

	return 0;
}

static int cmd_get_status(const struct shell *sh, size_t argc, char **argv)
{
	int which;
	enum clock_control_status status;
	const struct device *dev = shell_device_get_binding(argv[ARGV_DEV]);

	if (!dev) {
		shell_error(sh, "ClockControl: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	which = strtol(argv[ARGV_OUTPUT], NULL, 10);
	status = clock_control_get_status(clock_dev, INT_TO_POINTER(which));

	shell_print(sh, "status on output %d = %d\n", which, (int)status);

	return 0;
}

static int cmd_set_rate(const struct shell *sh, size_t argc, char **argv)
{
	int which = 0;
	int err = 0;
	unsigned long parsed_rate = 0;
	uint32_t rate = 0;
	int rc = 0;
	const struct device *dev = shell_device_get_binding(argv[ARGV_DEV]);

	if (!dev) {
		shell_error(sh, "ClockControl: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}

	which = strtol(argv[ARGV_OUTPUT], NULL, 10);
	parsed_rate = shell_strtoul(argv[ARGV_ARG], 10, &err);

	if (err != 0 || parsed_rate > UINT32_MAX) {
		shell_error(sh, "Invalid rate: %s", argv[ARGV_ARG]);
		return (err != 0) ? err : -ERANGE;
	}

	rate = (uint32_t)parsed_rate;
	rc = clock_control_set_rate(dev, INT_TO_POINTER(which), INT_TO_POINTER(rate));

	shell_print(sh, "clock_control_set_rate returned %d\n", rc);

	return 0;
}

static bool device_is_clock_control(const struct device *dev)
{
	return DEVICE_API_IS(clock_control, dev);
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_clock_control);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_clock_control_cmds,
	SHELL_CMD_ARG(on, &dsub_device_name, SHELL_HELP("Clock On", "<device> <output>"),
		      cmd_clock_on, 3, 0),
	SHELL_CMD_ARG(off, &dsub_device_name, SHELL_HELP("Clock Off", "<device> <output>"),
		      cmd_clock_off, 3, 0),
	SHELL_CMD_ARG(get_rate, &dsub_device_name, SHELL_HELP("Get rate", "<device> <output>"),
		      cmd_get_rate, 3, 0),
	SHELL_CMD_ARG(get_status, &dsub_device_name, SHELL_HELP("Get status", "<device> <output>"),
		      cmd_get_status, 3, 0),
	SHELL_CMD_ARG(set_rate, &dsub_device_name,
		      SHELL_HELP("Get status", "<device> <output> <rate>"), cmd_set_rate, 4, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(clock_control, &sub_clock_control_cmds, "Clock control command", NULL);
