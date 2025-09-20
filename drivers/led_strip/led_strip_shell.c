/*
 * Copyright (c) 2025 Dmitrii Sharshakov <d3dx12.xx@gmail.com>
 *
 * Based on drivers/led/led_shell.c which is:
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/kernel.h"
#include "zephyr/sys/util.h"
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/led_strip.h>
#include <stdlib.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_shell);

#define MAX_COLOR_ARGS 255

enum {
	arg_idx_dev	= 1,
	arg_idx_value	= 2,
};

static int parse_common_args(const struct shell *sh, char **argv,
			     const struct device **dev)
{
	*dev = shell_device_get_binding(argv[arg_idx_dev]);
	if (!*dev) {
		shell_error(sh,
			    "LED device %s not found", argv[arg_idx_dev]);
		return -ENODEV;
	}

	return 0;
}

static int cmd_update_rgb(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err;
	size_t num_colors, strip_len;
	uint8_t i;
	struct led_rgb *pixels;

	err = parse_common_args(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	strip_len = led_strip_length(dev);
	num_colors = argc - arg_idx_value;
	if (num_colors > MIN(strip_len, MAX_COLOR_ARGS)) {
		shell_error(sh,
			"Invalid number of colors %d (max %d)",
			num_colors, MIN(strip_len, MAX_COLOR_ARGS));
		return -EINVAL;
	}

	pixels = k_malloc(num_colors * sizeof(struct led_rgb));

	for (i = 0; i < num_colors; i++) {
		if (strlen(argv[arg_idx_value + i]) < 6) {
			shell_error(sh, "Invalid color length for value %s, expected 6",
				argv[arg_idx_value + i]);
			return -EINVAL;
		}

		err = hex2bin(argv[arg_idx_value + i], 2, &pixels[i].r, 1);
		if (err == 0) {
			shell_error(sh, "Invalid red channel parameter %s",
				argv[arg_idx_value + i]);
			return -EINVAL;
		}

		err = hex2bin(argv[arg_idx_value + i] + 2, 2, &pixels[i].g, 1);
		if (err == 0) {
			shell_error(sh, "Invalid green channel parameter %s",
				argv[arg_idx_value + i]);
			return -EINVAL;
		}

		err = hex2bin(argv[arg_idx_value + i] + 4, 2, &pixels[i].b, 1);
		if (err == 0) {
			shell_error(sh, "Invalid blue channel parameter %s",
				argv[arg_idx_value + i]);
			return -EINVAL;
		}
	}

	shell_fprintf(sh, SHELL_NORMAL, "%s: updating %u pixels:",
		      dev->name, num_colors);
	for (i = 0; i < num_colors; i++) {
		shell_fprintf(sh, SHELL_NORMAL, " (%d, %d, %d)",
			pixels[i].r, pixels[i].g, pixels[i].b);
	}
	shell_fprintf(sh, SHELL_NORMAL, "\n");

	err = led_strip_update_rgb(dev, pixels, num_colors);
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static bool device_is_led_strip(const struct device *dev)
{
	return DEVICE_API_IS(led_strip, dev);
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_led_strip);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_led_strip,
	SHELL_CMD_ARG(update_rgb, &dsub_device_name,
		      SHELL_HELP("Set first N leds to RGB colors",
				 "<device> <color0> [... <colorN>]\n"
				 "colorN: RGB value in hex format"),
		      cmd_update_rgb, 3, MAX_COLOR_ARGS - 1),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(led_strip, &sub_led_strip, "LED strip commands", NULL);
