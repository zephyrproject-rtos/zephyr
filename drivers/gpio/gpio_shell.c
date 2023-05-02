/*
 * Copyright (C) 2018-2023 Intel Corporation
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
	const struct gpio_driver_config *cfg;
	uint32_t type = GPIO_OUTPUT;
	const struct device *dev;
	uint8_t index = 0U;
	int ret;

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
		shell_error(sh, "Wrong parameters for GPIO configuration");
		return -EINVAL;
	}

	dev = device_get_binding(argv[args_indx.port]);

	if (dev != NULL) {
		cfg = (const struct gpio_driver_config *)dev->config;
		if (cfg == NULL) {
			shell_error(sh, "Device Configuration is NULL");
			return -EIO;
		}
		if (index < find_msb_set(cfg->port_pin_mask)) {
			shell_print(sh, "Configuring %s pin %u",
			    argv[args_indx.port], index);
		} else {
			shell_error(sh, "Invalid GPIO pin");
			return -EINVAL;
		}
		ret = gpio_pin_configure(dev, index, type);
		if (ret < 0) {
			shell_error(sh, "Pin Configuration failed. Error code: %d", ret);
			return -EIO;
		}
	} else {
		shell_error(sh, "No such GPIO device");
		return -ENODEV;
	}

	return 0;
}

static int cmd_gpio_get(const struct shell *sh,
			size_t argc, char **argv)
{
	const struct gpio_driver_config *cfg;
	const struct device *dev;
	uint8_t index = 0U;
	int ret;

	if (isdigit((unsigned char)argv[args_indx.index][0]) != 0) {
		index = (uint8_t)atoi(argv[args_indx.index]);
	} else {
		shell_error(sh, "Wrong parameters for GPIO get");
		return -EINVAL;
	}

	dev = device_get_binding(argv[args_indx.port]);

	if (dev != NULL) {
		cfg = (const struct gpio_driver_config *)dev->config;
		if (cfg == NULL) {
			shell_error(sh, "Device Configuration is NULL");
			return -EIO;
		}
		if (index < find_msb_set(cfg->port_pin_mask)) {
			shell_print(sh, "Reading %s pin %u",
			    argv[args_indx.port], index);
		} else {
			shell_error(sh, "Invalid GPIO pin");
			return -EINVAL;
		}
		ret = gpio_pin_get(dev, index);
		if (ret >= 0) {
			shell_print(sh, "Value %d", ret);
		} else {
			shell_error(sh, "Error %d while reading value", ret);
			return -EIO;
		}
	} else {
		shell_error(sh, "No such GPIO device");
		return -ENODEV;
	}

	return 0;
}

static int cmd_gpio_set(const struct shell *sh,
			size_t argc, char **argv)
{
	const struct gpio_driver_config *cfg;
	const struct device *dev;
	uint8_t index = 0U;
	uint8_t value = 0U;
	int ret;

	if (isdigit((unsigned char)argv[args_indx.index][0]) != 0 &&
	    isdigit((unsigned char)argv[args_indx.value][0]) != 0) {
		index = (uint8_t)atoi(argv[args_indx.index]);
		value = (uint8_t)atoi(argv[args_indx.value]);
	} else {
		shell_print(sh, "Wrong parameters for GPIO set");
		return -EINVAL;
	}
	dev = device_get_binding(argv[args_indx.port]);

	if (dev != NULL) {
		cfg = (const struct gpio_driver_config *)dev->config;
		if (cfg == NULL) {
			shell_error(sh, "Device Configuration is NULL");
			return -EIO;
		}
		if (index < find_msb_set(cfg->port_pin_mask)) {
			shell_print(sh, "Writing to %s pin %u",
			    argv[args_indx.port], index);
		} else {
			shell_error(sh, "Invalid GPIO pin");
			return -EINVAL;
		}
		ret = gpio_pin_set(dev, index, value);
		if (ret < 0) {
			shell_error(sh, "Error %d while writing value", ret);
			return -EIO;
		}
	} else {
		shell_error(sh, "No such GPIO device");
		return -ENODEV;
	}

	return 0;
}


/* 500 msec = 1/2 sec */
#define SLEEP_TIME_MS   500

static int cmd_gpio_blink(const struct shell *sh,
			  size_t argc, char **argv)
{
	const struct gpio_driver_config *cfg;
	const struct device *dev;
	uint8_t index = 0U;
	uint8_t value = 0U;
	size_t count = 0;
	char data;
	int ret;

	if (isdigit((unsigned char)argv[args_indx.index][0]) != 0) {
		index = (uint8_t)atoi(argv[args_indx.index]);
	} else {
		shell_error(sh, "Wrong parameters for GPIO blink");
		return -EINVAL;
	}
	dev = device_get_binding(argv[args_indx.port]);

	if (dev != NULL) {
		cfg = (const struct gpio_driver_config *)dev->config;
		if (cfg == NULL) {
			shell_error(sh, "Device Configuration is NULL");
			return -EIO;
		}
		if (index < find_msb_set(cfg->port_pin_mask)) {
			shell_fprintf(sh, SHELL_NORMAL,
			"Blinking port %s pin %u.", argv[1], index);
			shell_fprintf(sh, SHELL_NORMAL, " Hit any key to exit");
		} else {
			shell_error(sh, "Invalid GPIO pin");
			return -EINVAL;
		}

		while (true) {
			(void)sh->iface->api->read(sh->iface, &data, sizeof(data), &count);
			if (count != 0) {
				break;
			}
			ret = gpio_pin_set(dev, index, value);
			if (ret < 0) {
				shell_error(sh, "Error %d while writing value", ret);
				return -EIO;
			}
			value = !value;
			k_msleep(SLEEP_TIME_MS);
		}

		shell_fprintf(sh, SHELL_NORMAL, "\n");
	} else {
		shell_error(sh, "No such GPIO device");
		return -ENODEV;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_gpio,
			       SHELL_CMD_ARG(conf, NULL,
					"Configure GPIO: conf <gpio_node_id> <pin> <mode(in/out)>",
					cmd_gpio_conf, 4, 0),
			       SHELL_CMD_ARG(get, NULL,
					"Get GPIO value: get <gpio_node_id> <pin>",
					cmd_gpio_get, 3, 0),
			       SHELL_CMD_ARG(set, NULL,
					"Set GPIO: set <gpio_node_id> <pin> <value(0/1)>",
					cmd_gpio_set, 4, 0),
			       SHELL_CMD_ARG(blink, NULL,
					"Blink GPIO: blink <gpio_node_id> <pin>",
					cmd_gpio_blink, 3, 0),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
			       );

SHELL_CMD_REGISTER(gpio, &sub_gpio, "GPIO commands", NULL);
