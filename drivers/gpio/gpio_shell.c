/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2021 Dennis Ruffer <daruffer@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Use "device list" command for GPIO port names
 */

#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <zephyr/init.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <stdlib.h>
#include <ctype.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

LOG_MODULE_REGISTER(gpio_shell);

struct args_index {
	uint8_t port;
	uint8_t index;
	uint8_t mode;
	uint8_t value;
};

static const struct args_index args_indx = {
	.port = 1,
	.index = 2,
	.mode = 3,
	.value = 3,
};

static int cmd_gpio_conf(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t index = 0U;
	int type = GPIO_OUTPUT;
	const struct device *dev;

	if (isdigit((unsigned char)argv[args_indx.index][0]) != 0 &&
	    isalpha((unsigned char)argv[args_indx.mode][0]) != 0) {
		index = (uint8_t)atoi(argv[args_indx.index]);
		if (!strcmp(argv[args_indx.mode], "in")) {
			type = GPIO_INPUT;
		} else if (!strcmp(argv[args_indx.mode], "inu")) {
			type = GPIO_INPUT | GPIO_PULL_UP;
		} else if (!strcmp(argv[args_indx.mode], "ind")) {
			type = GPIO_INPUT | GPIO_PULL_DOWN;
		} else if (!strcmp(argv[args_indx.mode], "out")) {
			type = GPIO_OUTPUT;
		} else {
			return 0;
		}
	} else {
		shell_error(sh, "Wrong parameters for conf");
		return -ENOTSUP;
	}

	dev = device_get_binding(argv[args_indx.port]);

	if (dev != NULL) {
		index = (uint8_t)atoi(argv[args_indx.index]);
		shell_print(sh, "Configuring %s pin %d",
			    argv[args_indx.port], index);
		gpio_pin_configure(dev, index, type);
	}

	return 0;
}

static int cmd_gpio_get(const struct shell *sh,
			size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t index = 0U;
	int rc;

	if (isdigit((unsigned char)argv[args_indx.index][0]) != 0) {
		index = (uint8_t)atoi(argv[args_indx.index]);
	} else {
		shell_error(sh, "Wrong parameters for get");
		return -EINVAL;
	}

	dev = device_get_binding(argv[args_indx.port]);

	if (dev != NULL) {
		index = (uint8_t)atoi(argv[2]);
		shell_print(sh, "Reading %s pin %d",
			    argv[args_indx.port], index);
		rc = gpio_pin_get(dev, index);
		if (rc >= 0) {
			shell_print(sh, "Value %d", rc);
		} else {
			shell_error(sh, "Error %d reading value", rc);
			return -EIO;
		}
	}

	return 0;
}

static int cmd_gpio_set(const struct shell *sh,
			size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t index = 0U;
	uint8_t value = 0U;

	if (isdigit((unsigned char)argv[args_indx.index][0]) != 0 &&
	    isdigit((unsigned char)argv[args_indx.value][0]) != 0) {
		index = (uint8_t)atoi(argv[args_indx.index]);
		value = (uint8_t)atoi(argv[args_indx.value]);
	} else {
		shell_print(sh, "Wrong parameters for set");
		return -EINVAL;
	}
	dev = device_get_binding(argv[args_indx.port]);

	if (dev != NULL) {
		index = (uint8_t)atoi(argv[2]);
		shell_print(sh, "Writing to %s pin %d",
			    argv[args_indx.port], index);
		gpio_pin_set(dev, index, value);
	}

	return 0;
}


/* 500 msec = 1/2 sec */
#define SLEEP_TIME_MS   500

static int cmd_gpio_blink(const struct shell *sh,
			  size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t index = 0U;
	uint8_t value = 0U;
	size_t count = 0;
	char data;

	if (isdigit((unsigned char)argv[args_indx.index][0]) != 0) {
		index = (uint8_t)atoi(argv[args_indx.index]);
	} else {
		shell_error(sh, "Wrong parameters for blink");
		return -EINVAL;
	}
	dev = device_get_binding(argv[args_indx.port]);

	if (dev != NULL) {
		index = (uint8_t)atoi(argv[2]);
		shell_fprintf(sh, SHELL_NORMAL, "Blinking port %s index %d.", argv[1], index);
		shell_fprintf(sh, SHELL_NORMAL, " Hit any key to exit");

		/* dummy read to clear any pending input */
		(void)sh->iface->api->read(sh->iface, &data, sizeof(data), &count);

		while (true) {
			(void)sh->iface->api->read(sh->iface, &data, sizeof(data), &count);
			if (count != 0) {
				break;
			}
			gpio_pin_set(dev, index, value);
			value = !value;
			k_msleep(SLEEP_TIME_MS);
		}

		shell_fprintf(sh, SHELL_NORMAL, "\n");
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_gpio,
			       SHELL_CMD_ARG(conf, NULL, "Configure GPIO", cmd_gpio_conf, 4, 0),
			       SHELL_CMD_ARG(get, NULL, "Get GPIO value", cmd_gpio_get, 3, 0),
			       SHELL_CMD_ARG(set, NULL, "Set GPIO", cmd_gpio_set, 4, 0),
			       SHELL_CMD_ARG(blink, NULL, "Blink GPIO", cmd_gpio_blink, 3, 0),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
			       );

SHELL_CMD_REGISTER(gpio, &sub_gpio, "GPIO commands", NULL);
