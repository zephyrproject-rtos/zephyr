/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/actuator.h>
#include <zephyr/sys/q31.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>

#include <stdlib.h>

#define SETPOINT_MAX 1000
#define SETPOINT_MIN -1000

static int get_device_from_str(const struct shell *sh,
			       const char *dev_str,
			       const struct device **dev)
{
	*dev = shell_device_get_binding(dev_str);

	if (*dev == NULL) {
		shell_error(sh, "%s not %s", dev_str, "found");
		return -ENODEV;
	}

	if (!device_is_ready(*dev)) {
		shell_error(sh, "%s not %s", dev_str, "ready");
		return -ENODEV;
	}

	return 0;
}

static int get_setpoint_from_str(const struct shell *sh,
				 const char *setpoint_str,
				 q31_t *setpoint)
{
	long value;
	char *end;

	value = strtol(setpoint_str, &end, 10);
	if ((*end != '\0') ||
	    (value < SETPOINT_MIN) ||
	    (value > SETPOINT_MAX)) {
		shell_error(sh, "%s not %s", setpoint_str, "valid");
		return -EINVAL;
	}

	*setpoint = SYS_Q31_MILLI(value);
	return 0;
}

static int cmd_set_setpoint(const struct shell *sh, size_t argc, char **argv)
{
	const char *dev_str;
	const char *setpoint_str;
	int ret;
	const struct device *dev;
	q31_t setpoint;

	ARG_UNUSED(argc);

	dev_str = argv[1];
	ret = get_device_from_str(sh, dev_str, &dev);
	if (ret < 0) {
		return ret;
	}

	setpoint_str = argv[2];
	ret = get_setpoint_from_str(sh, setpoint_str, &setpoint);
	if (ret < 0) {
		return ret;
	}

	ret = actuator_set_setpoint(dev, setpoint);
	if (ret < 0) {
		shell_error(sh, "failed to %s %s", "set", "setpoint");
		return -EIO;
	}

	return 0;
}

static bool device_is_actuator(const struct device *dev)
{
	return DEVICE_API_IS(actuator, dev);
}

static void dsub_device_lookup(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_actuator);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_0, dsub_device_lookup);

#define SET_SETPOINT_HELP									\
	SHELL_HELP(										\
		"Set actuator setpoint",							\
		"<device> <setpoint>\n"								\
		"setpoint: min=-1000 max=1000"							\
	)

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_actuator,
	SHELL_CMD_ARG(
		set_setpoint,
		&dsub_device_0,
		SET_SETPOINT_HELP,
		cmd_set_setpoint,
		3,
		0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(actuator, &sub_actuator, "Actuator device commands", NULL);
