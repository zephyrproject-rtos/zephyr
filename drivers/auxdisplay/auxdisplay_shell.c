/*
 * Copyright 2026 Karl Palsson <karl.palsson@marel.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(auxdisplay_shell, CONFIG_AUXDISPLAY_LOG_LEVEL);

#define AUXD_ARGV_DEVICE 1
#define AUXD_ARGV_PATTERN 2
#define AUXD_ARGV_X 3
#define AUXD_ARGV_Y 4

static int cmd_clear(const struct shell *sh, size_t argc, char **argv)
{
	(void)argc;
	const struct device *dev;

	dev = shell_device_get_binding(argv[AUXD_ARGV_DEVICE]);
	if (!dev) {
		shell_error(sh, "Auxiliary display device not found");
		return -ENOENT;
	}

	return auxdisplay_clear(dev);
}

static int cmd_size(const struct shell *sh, size_t argc, char **argv)
{
	(void)argc;
	const struct device *dev;

	dev = shell_device_get_binding(argv[AUXD_ARGV_DEVICE]);
	if (!dev) {
		shell_error(sh, "Auxiliary display device not found");
		return -ENOENT;
	}

	struct auxdisplay_capabilities capabilities;
	int ret = auxdisplay_capabilities_get(dev, &capabilities);

	if (ret) {
		shell_error(sh, "Failed to get display capabilities (err %d)", ret);
		return ret;
	}
	shell_print(sh, "%zu rows, %zu columns", capabilities.rows, capabilities.columns);
	return 0;
}

static int cmd_write(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err;
	int x = 0;
	int y = 0;

	dev = shell_device_get_binding(argv[AUXD_ARGV_DEVICE]);
	if (!dev) {
		shell_error(sh, "Auxiliary display device not found");
		return -ENOENT;
	}

	err = 0;
	if (argc > AUXD_ARGV_X) {
		x = shell_strtol(argv[AUXD_ARGV_X], 0, &err);
		if (err) {
			shell_error(sh, "Error parsing X position");
			return -EINVAL;
		}
	}
	err = 0;
	if (argc > AUXD_ARGV_Y) {
		y = shell_strtol(argv[AUXD_ARGV_Y], 0, &err);
		if (err) {
			shell_error(sh, "Error parsing Y position");
			return -EINVAL;
		}
	}

	err = auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, x, y);
	if (err) {
		shell_error(sh, "Failed to set cursor position (err %d)", err);
		return err;
	}

	err = auxdisplay_write(dev, argv[AUXD_ARGV_PATTERN], strlen(argv[AUXD_ARGV_PATTERN]));
	if (err) {
		shell_error(sh, "Failed to write to display (err %d)", err);
		return err;
	}

	return 0;
}

static bool device_is_auxdisplay(const struct device *dev)
{
	return DEVICE_API_IS(auxdisplay, dev);
}

/* Device name autocompletion support */
static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_auxdisplay);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(auxdisplay_cmds,
	SHELL_CMD_ARG(size, &dsub_device_name, "<device>", cmd_size, 2, 0),
	SHELL_CMD_ARG(write, &dsub_device_name, "<device> <pattern> [x] [y]", cmd_write, 3, 2),
	SHELL_CMD_ARG(clear, &dsub_device_name, "<device>", cmd_clear, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(auxdisplay, &auxdisplay_cmds, "Auxiliary display shell commands", NULL);
