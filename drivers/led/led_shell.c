/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <drivers/led.h>
#include <stdlib.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(led_shell);

enum {
	arg_idx_dev		= 1,
	arg_idx_led		= 2,
	arg_idx_brightness	= 3,
};

static int parse_common_args(const struct shell *shell, char **argv,
			   struct device **dev, u32_t *led)
{
	char *end_ptr;

	*dev = device_get_binding(argv[arg_idx_dev]);
	if (!*dev) {
		shell_error(shell,
			    "LED device %s not found", argv[arg_idx_dev]);
		return -ENODEV;
	}

	*led = strtoul(argv[arg_idx_led], &end_ptr, 0);
	if (*end_ptr != '\0') {
		shell_error(shell, "Invalid LED number parameter %s",
			    argv[arg_idx_led]);
		return -EINVAL;
	}

	return 0;
}

static int cmd_off(const struct shell *shell, size_t argc, char **argv)
{
	struct device *dev;
	u32_t led;
	int err;

	err = parse_common_args(shell, argv, &dev, &led);
	if (err < 0) {
		return err;
	}

	shell_print(shell, "%s: turning off LED %d", dev->name, led);

	err = led_off(dev, led);
	if (err) {
		shell_error(shell, "Error: %d", err);
	}

	return err;
}

static int cmd_on(const struct shell *shell, size_t argc, char **argv)
{
	struct device *dev;
	u32_t led;
	int err;

	err = parse_common_args(shell, argv, &dev, &led);
	if (err < 0) {
		return err;
	}

	shell_print(shell, "%s: turning on LED %d", dev->name, led);

	err = led_on(dev, led);
	if (err) {
		shell_error(shell, "Error: %d", err);
	}

	return err;
}

static int cmd_set_brightness(const struct shell *shell,
			      size_t argc, char **argv)
{
	struct device *dev;
	u32_t led;
	int err;
	char *end_ptr;
	unsigned long brightness;

	err = parse_common_args(shell, argv, &dev, &led);
	if (err < 0) {
		return err;
	}

	brightness = strtoul(argv[arg_idx_brightness], &end_ptr, 0);
	if (*end_ptr != '\0') {
		shell_error(shell, "Invalid LED brightness parameter %s",
			     argv[arg_idx_brightness]);
		return -EINVAL;
	}
	if (brightness > 255) {
		shell_error(shell, "Invalid LED brightness value %d (max 255)",
			     brightness);
		return -EINVAL;
	}

	shell_print(shell, "%s: setting LED %d brightness to %d",
		    dev->name, led, brightness);

	err = led_set_brightness(dev, led, (u8_t) brightness);
	if (err) {
		shell_error(shell, "Error: %d", err);
	}

	return err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_led,
	SHELL_CMD_ARG(off, NULL, "<device> <led_num>", cmd_off, 3, 0),
	SHELL_CMD_ARG(on, NULL, "<device> <led_num>", cmd_on, 3, 0),
	SHELL_CMD_ARG(set_brightness, NULL,
		      "<device> <led_num> <brightness [0-255]>",
		      cmd_set_brightness, 4, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(led, &sub_led, "LED commands", NULL);
