/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include <shell/shell.h>
#include <init.h>
#include <string.h>
#include <stdio.h>
#include <drivers/gpio.h>
#include <stdlib.h>
#include <ctype.h>
#include <logging/log.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

LOG_MODULE_REGISTER(gpio_shell);

struct args_index {
	u8_t port;
	u8_t index;
	u8_t mode;
	u8_t value;
};

struct args_number {
	u8_t conf;
	u8_t set;
	u8_t get;
	u8_t listen;
};

static const struct args_index args_indx = {
	.port = 1,
	.index = 2,
	.mode = 3,
	.value = 3,
};

static const struct args_number args_no = {
	.conf = 4,
	.set = 4,
	.get = 3,
	.listen = 7
};

static int cmd_gpio_conf(const struct shell *shell, size_t argc, char **argv)
{
	u8_t index = 0U;
	int type = GPIO_DIR_OUT;
	struct device *dev;

	if (argc == args_no.conf &&
	    isdigit((unsigned char)argv[args_indx.index][0]) &&
	    isalpha((unsigned char)argv[args_indx.mode][0])) {
		index = (u8_t)atoi(argv[args_indx.index]);
		if (!strcmp(argv[args_indx.mode], "in")) {
			type = GPIO_DIR_IN;
		} else if (!strcmp(argv[args_indx.mode], "out")) {
			type = GPIO_DIR_OUT;
		} else {
			return 0;
		}
	} else {
		shell_error(shell, "Wrong parameters for conf");
		return -ENOTSUP;
	}

	dev = device_get_binding(argv[args_indx.port]);

	if (dev != NULL) {
		index = (uint8_t)atoi(argv[args_indx.index]);
		shell_print(shell, "Configuring %s pin %d",
			    argv[args_indx.port], index);
		gpio_pin_configure(dev, index, type);
	}

	return 0;
}

static int cmd_gpio_get(const struct shell *shell,
			size_t argc, char **argv)
{
	struct device *dev;
	u8_t index = 0U;
	u32_t value = 0U;
	int rc;

	if (argc == args_no.get && isdigit((unsigned char)argv[args_indx.index][0])) {
		index = (uint8_t)atoi(argv[args_indx.index]);
	} else {
		shell_error(shell, "Wrong parameters for get");
		return -EINVAL;
	}

	dev = device_get_binding(argv[args_indx.port]);

	if (dev != NULL) {
		index = (u8_t)atoi(argv[2]);
		shell_print(shell, "Reading %s pin %d",
			     argv[args_indx.port], index);
		rc = gpio_pin_read(dev, index, &value);
		if (rc == 0) {
			shell_print(shell, "Value %d", value);
		} else {
			shell_error(shell, "Error reading value");
			return -EIO;
		}
	}

	return 0;
}

static int cmd_gpio_set(const struct shell *shell,
			size_t argc, char **argv)
{
	struct device *dev;
	u8_t index = 0U;
	u8_t value = 0U;

	if (argc == args_no.set &&
	    isdigit((unsigned char)argv[args_indx.index][0]) &&
	    isdigit((unsigned char)argv[args_indx.value][0])) {
		index = (uint8_t)atoi(argv[args_indx.index]);
		value = (uint8_t)atoi(argv[args_indx.value]);
	} else {
		shell_print(shell, "Wrong parameters for set");
		return -EINVAL;
	}
	dev = device_get_binding(argv[args_indx.port]);

	if (dev != NULL) {
		index = (u8_t)atoi(argv[2]);
		shell_print(shell, "Writing to %s pin %d",
			    argv[args_indx.port], index);
		gpio_pin_write(dev, index, value);
	}

	return 0;
}


SHELL_STATIC_SUBCMD_SET_CREATE(sub_gpio,
	SHELL_CMD(conf, NULL, "Configure GPIO", cmd_gpio_conf),
	SHELL_CMD(get, NULL, "Get GPIO value", cmd_gpio_get),
	SHELL_CMD(set, NULL, "Set GPIO", cmd_gpio_set),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(gpio, &sub_gpio, "GPIO commands", NULL);
