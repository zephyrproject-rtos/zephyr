/*
 * Copyright (c) 2025 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Haptics shell commands.
 */

#include <zephyr/shell/shell.h>
#include <zephyr/drivers/haptics.h>
#include <stdlib.h>

#define HAPTICS_START_HELP SHELL_HELP("Start haptic output", "<device>")
#define HAPTICS_STOP_HELP  SHELL_HELP("Stop haptic output", "<device>")

#define HAPTICS_ARGS_DEVICE 1

static int cmd_start(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int error;

	dev = shell_device_get_binding(argv[HAPTICS_ARGS_DEVICE]);
	if (dev == NULL) {
		shell_error(sh, "haptic device not found");
		return -EINVAL;
	}

	error = haptics_start_output(dev);
	if (error < 0) {
		shell_error(sh, "failed to start haptic output (%d)", error);
	}

	return error;
}

static int cmd_stop(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int error;

	dev = shell_device_get_binding(argv[HAPTICS_ARGS_DEVICE]);
	if (dev == NULL) {
		shell_error(sh, "haptic device not found");
		return -EINVAL;
	}

	error = haptics_stop_output(dev);
	if (error < 0) {
		shell_error(sh, "Failed to stop haptic output (%d)", error);
	}

	return error;
}

static bool device_is_haptics(const struct device *dev)
{
	return DEVICE_API_IS(haptics, dev);
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_haptics);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(
	haptic_cmds, SHELL_CMD_ARG(start, &dsub_device_name, HAPTICS_START_HELP, cmd_start, 2, 0),
	SHELL_CMD_ARG(stop, &dsub_device_name, HAPTICS_STOP_HELP, cmd_stop, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(haptics, &haptic_cmds, "Haptic shell commands", NULL);
