/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/drivers/led.h>
#include <stdlib.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_shell);

#define MAX_CHANNEL_ARGS 8

enum {
	arg_idx_dev		= 1,
	arg_idx_led		= 2,
	arg_idx_value		= 3,
};

static int parse_common_args(const struct shell *sh, char **argv,
			     const struct device * *dev, uint32_t *led)
{
	char *end_ptr;

	*dev = device_get_binding(argv[arg_idx_dev]);
	if (!*dev) {
		shell_error(sh,
			    "LED device %s not found", argv[arg_idx_dev]);
		return -ENODEV;
	}

	*led = strtoul(argv[arg_idx_led], &end_ptr, 0);
	if (*end_ptr != '\0') {
		shell_error(sh, "Invalid LED number parameter %s",
			    argv[arg_idx_led]);
		return -EINVAL;
	}

	return 0;
}

static int cmd_off(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint32_t led;
	int err;

	err = parse_common_args(sh, argv, &dev, &led);
	if (err < 0) {
		return err;
	}

	shell_print(sh, "%s: turning off LED %d", dev->name, led);

	err = led_off(dev, led);
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static int cmd_on(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint32_t led;
	int err;

	err = parse_common_args(sh, argv, &dev, &led);
	if (err < 0) {
		return err;
	}

	shell_print(sh, "%s: turning on LED %d", dev->name, led);

	err = led_on(dev, led);
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static int cmd_get_info(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint32_t led;
	int err;
	const struct led_info *info;
	int i;

	err = parse_common_args(sh, argv, &dev, &led);
	if (err < 0) {
		return err;
	}

	shell_print(sh, "%s: getting LED %d information", dev->name, led);

	err = led_get_info(dev, led, &info);
	if (err) {
		shell_error(sh, "Error: %d", err);
		return err;
	}

	shell_print(sh, "Label      : %s", info->label ? : "<NULL>");
	shell_print(sh, "Index      : %d", info->index);
	shell_print(sh, "Num colors : %d", info->num_colors);
	if (info->color_mapping) {
		shell_fprintf(sh, SHELL_NORMAL, "Colors     : %d",
			      info->color_mapping[0]);
		for (i = 1; i < info->num_colors; i++) {
			shell_fprintf(sh, SHELL_NORMAL, ":%d",
				      info->color_mapping[i]);
		}
		shell_fprintf(sh, SHELL_NORMAL, "\n");
	}

	return 0;
}

static int cmd_set_brightness(const struct shell *sh,
			      size_t argc, char **argv)
{
	const struct device *dev;
	uint32_t led;
	int err;
	char *end_ptr;
	unsigned long value;

	err = parse_common_args(sh, argv, &dev, &led);
	if (err < 0) {
		return err;
	}

	value = strtoul(argv[arg_idx_value], &end_ptr, 0);
	if (*end_ptr != '\0') {
		shell_error(sh, "Invalid LED brightness parameter %s",
			     argv[arg_idx_value]);
		return -EINVAL;
	}
	if (value > 100) {
		shell_error(sh, "Invalid LED brightness value %lu (max 100)",
			    value);
		return -EINVAL;
	}

	shell_print(sh, "%s: setting LED %d brightness to %lu",
		    dev->name, led, value);

	err = led_set_brightness(dev, led, (uint8_t) value);
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static int cmd_set_color(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint32_t led;
	int err;
	size_t num_colors;
	uint8_t i;
	uint8_t color[MAX_CHANNEL_ARGS];

	err = parse_common_args(sh, argv, &dev, &led);
	if (err < 0) {
		return err;
	}

	num_colors = argc - arg_idx_value;
	if (num_colors > MAX_CHANNEL_ARGS) {
		shell_error(sh,
			    "Invalid number of colors %d (max %d)",
			     num_colors, MAX_CHANNEL_ARGS);
		return -EINVAL;
	}

	for (i = 0; i < num_colors; i++) {
		char *end_ptr;
		unsigned long col;

		col = strtoul(argv[arg_idx_value + i], &end_ptr, 0);
		if (*end_ptr != '\0') {
			shell_error(sh, "Invalid LED color parameter %s",
				    argv[arg_idx_value + i]);
			return -EINVAL;
		}
		if (col > 255) {
			shell_error(sh,
				    "Invalid LED color value %lu (max 255)",
				    col);
			return -EINVAL;
		}
		color[i] = col;
	}

	shell_fprintf(sh, SHELL_NORMAL, "%s: setting LED %d color to %d",
		      dev->name, led, color[0]);
	for (i = 1; i < num_colors; i++) {
		shell_fprintf(sh, SHELL_NORMAL, ":%d", color[i]);
	}
	shell_fprintf(sh, SHELL_NORMAL, "\n");

	err = led_set_color(dev, led, num_colors, color);
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static int cmd_set_channel(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint32_t channel;
	int err;
	char *end_ptr;
	unsigned long value;

	err = parse_common_args(sh, argv, &dev, &channel);
	if (err < 0) {
		return err;
	}

	value = strtoul(argv[arg_idx_value], &end_ptr, 0);
	if (*end_ptr != '\0') {
		shell_error(sh, "Invalid channel value parameter %s",
			     argv[arg_idx_value]);
		return -EINVAL;
	}
	if (value > 255) {
		shell_error(sh, "Invalid channel value %lu (max 255)",
			    value);
		return -EINVAL;
	}

	shell_print(sh, "%s: setting channel %d to %lu",
		    dev->name, channel, value);

	err = led_set_channel(dev, channel, (uint8_t) value);
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static int
cmd_write_channels(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint32_t start_channel;
	int err;
	size_t num_channels;
	uint8_t i;
	uint8_t value[MAX_CHANNEL_ARGS];

	err = parse_common_args(sh, argv, &dev, &start_channel);
	if (err < 0) {
		return err;
	}

	num_channels = argc - arg_idx_value;
	if (num_channels > MAX_CHANNEL_ARGS) {
		shell_error(sh,
			    "Can't write %d channels (max %d)",
			     num_channels, MAX_CHANNEL_ARGS);
		return -EINVAL;
	}

	for (i = 0; i < num_channels; i++) {
		char *end_ptr;
		unsigned long val;

		val = strtoul(argv[arg_idx_value + i], &end_ptr, 0);
		if (*end_ptr != '\0') {
			shell_error(sh,
				    "Invalid channel value parameter %s",
				    argv[arg_idx_value + i]);
			return -EINVAL;
		}
		if (val > 255) {
			shell_error(sh,
				    "Invalid channel value %lu (max 255)", val);
			return -EINVAL;
		}
		value[i] = val;
	}

	shell_fprintf(sh, SHELL_NORMAL, "%s: writing from channel %d: %d",
		      dev->name, start_channel, value[0]);
	for (i = 1; i < num_channels; i++) {
		shell_fprintf(sh, SHELL_NORMAL, " %d", value[i]);
	}
	shell_fprintf(sh, SHELL_NORMAL, "\n");

	err = led_write_channels(dev, start_channel, num_channels, value);
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_led, SHELL_CMD_ARG(off, &dsub_device_name, "<device> <led>", cmd_off, 3, 0),
	SHELL_CMD_ARG(on, &dsub_device_name, "<device> <led>", cmd_on, 3, 0),
	SHELL_CMD_ARG(get_info, &dsub_device_name, "<device> <led>", cmd_get_info, 3, 0),
	SHELL_CMD_ARG(set_brightness, &dsub_device_name, "<device> <led> <value [0-100]>",
		      cmd_set_brightness, 4, 0),
	SHELL_CMD_ARG(set_color, &dsub_device_name,
		      "<device> <led> <color 0 [0-255]> ... <color N>", cmd_set_color, 4,
		      MAX_CHANNEL_ARGS - 1),
	SHELL_CMD_ARG(set_channel, &dsub_device_name, "<device> <channel> <value [0-255]>",
		      cmd_set_channel, 4, 0),
	SHELL_CMD_ARG(write_channels, &dsub_device_name,
		      "<device> <chan> <value 0 [0-255]> ... <value N>", cmd_write_channels, 4,
		      MAX_CHANNEL_ARGS - 1),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(led, &sub_led, "LED commands", NULL);
