/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fan shell commands.
 */

#include <zephyr/drivers/fan.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>

#define FAN_SET_HELP SHELL_HELP("Set fan speed", "<device> <percent>")
#define FAN_GET_HELP SHELL_HELP("Get configured fan speed", "<device>")

#define FAN_ARGS_DEVICE  1
#define FAN_ARGS_PERCENT 2

static int cmd_set(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint32_t percent;
	int ret;

	dev = shell_device_get_binding(argv[FAN_ARGS_DEVICE]);
	if (dev == NULL) {
		shell_error(sh, "fan device not found");
		return -EINVAL;
	}

	percent = shell_strtoul(argv[FAN_ARGS_PERCENT], 10, &ret);
	if (ret < 0) {
		shell_error(sh, "failed to parse percent (%d)", ret);
		return ret;
	}

	if (percent > FAN_SPEED_MAX) {
		shell_error(sh, "percent must be 0 to %u", FAN_SPEED_MAX);
		return -EINVAL;
	}

	ret = fan_set_speed(dev, (uint8_t)percent);
	if (ret < 0) {
		shell_error(sh, "failed to set fan speed (%d)", ret);
	}

	return ret;
}

static int cmd_get(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t percent;
	int ret;

	ARG_UNUSED(argc);

	dev = shell_device_get_binding(argv[FAN_ARGS_DEVICE]);
	if (dev == NULL) {
		shell_error(sh, "fan device not found");
		return -EINVAL;
	}

	ret = fan_get_speed(dev, &percent);
	if (ret < 0) {
		shell_error(sh, "failed to get fan speed (%d)", ret);
		return ret;
	}

	shell_print(sh, "%u%%", percent);

	return 0;
}

static bool device_is_fan(const struct device *dev)
{
	return DEVICE_API_IS(fan, dev);
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_fan);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(fan_cmds,
	SHELL_CMD_ARG(set, &dsub_device_name, FAN_SET_HELP, cmd_set, 3, 0),
	SHELL_CMD_ARG(get, &dsub_device_name, FAN_GET_HELP, cmd_get, 2, 0),
	SHELL_SUBCMD_SET_END);
/* clang-format on */

SHELL_CMD_REGISTER(fan, &fan_cmds, "Fan shell commands", NULL);
