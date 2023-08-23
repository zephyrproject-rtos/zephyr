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

static int cmd_gpio_conf(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	unsigned long index;
	unsigned long type;
	int rc = 0;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "GPIO device %s not available", argv[1]);
		return -ENODEV;
	}

	if (!strcmp(argv[3], "in")) {
		type = GPIO_INPUT;
	} else if (!strcmp(argv[3], "inu")) {
		type = GPIO_INPUT | GPIO_PULL_UP;
	} else if (!strcmp(argv[3], "ind")) {
		type = GPIO_INPUT | GPIO_PULL_DOWN;
	} else if (!strcmp(argv[3], "out")) {
		type = GPIO_OUTPUT;
	} else {
		type = shell_strtoul(argv[3], 0, &rc);
	}

	index = shell_strtoul(argv[2], 10, &rc);
	if ((rc < 0) || (index > UINT8_MAX)) {
		shell_error(sh, "Wrong parameters for conf");
		return rc;
	}

	shell_print(sh, "Configuring %s pin %lu with mask %lu", argv[1], index, type);

	rc = gpio_pin_configure(dev, index, type);
	if (rc < 0) {
		shell_error(sh, "Error %d during config", rc);
	}

	return rc;
}

static int cmd_gpio_get(const struct shell *sh,
			size_t argc, char **argv)
{
	const struct device *dev;
	unsigned long index;
	int rc = 0;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "GPIO device %s not available", argv[1]);
		return -ENODEV;
	}

	index = shell_strtoul(argv[2], 10, &rc);
	if ((rc < 0) || (index > UINT8_MAX)) {
		shell_error(sh, "Wrong parameters for get");
		return rc;
	}

	shell_print(sh, "Reading %s pin %lu", argv[1], index);

	rc = gpio_pin_get(dev, index);
	if (rc < 0) {
		shell_error(sh, "Error %d reading value", rc);
		return -EIO;
	}

	shell_print(sh, "Value %d", rc);
	return 0;
}

static int cmd_gpio_set(const struct shell *sh,
			size_t argc, char **argv)
{
	const struct device *dev;
	unsigned long index;
	uint8_t value = 0U;
	int rc = 0;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "GPIO device %s not available", argv[1]);
		return -ENODEV;
	}

	index = shell_strtoul(argv[2], 10, &rc);
	value = shell_strtoul(argv[3], 10, &rc);

	if ((rc < 0) || (index > UINT8_MAX)) {
		shell_print(sh, "Wrong parameters for set");
		return rc;
	}

	shell_print(sh, "Writing to %s pin %lu", argv[1], index);

	rc = gpio_pin_set(dev, index, value);
	if (rc < 0) {
		shell_error(sh, "Error %d writing value", rc);
	}

	return rc;
}


/* 500 msec = 1/2 sec */
#define SLEEP_TIME_MS   500

static int cmd_gpio_blink(const struct shell *sh,
			  size_t argc, char **argv)
{
	const struct device *dev;
	unsigned long index = 0U;
	uint8_t value = 0U;
	size_t count = 0;
	char data;
	int rc = 0;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "GPIO device %s not available", argv[1]);
		return -ENODEV;
	}

	index = shell_strtoul(argv[2], 10, &rc);
	if ((rc < 0) || (index > UINT8_MAX)) {
		shell_error(sh, "Wrong parameters for blink");
		return rc;
	}

	shell_fprintf(sh, SHELL_NORMAL, "Blinking port %s index %lu.", argv[1], index);
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

	return 0;
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
	sub_gpio,
	SHELL_CMD_ARG(conf, &dsub_device_name,
		     "Configure GPIO\n"
		     "Usage: conf <device> <pin> <mode>\n"
		     "The mode argument accepts these values:\n"
		     "'in' - Input\n"
		     "'inu' - Input with pullup\n"
		     "'ind' - Input with pulldown\n"
		     "'out' - Output\n"
		     "An integer - Passed to gpio_pin_configure flags argument",
		     cmd_gpio_conf, 4, 0),
	SHELL_CMD_ARG(get, &dsub_device_name,
		     "Get GPIO value\n"
		     "Usage: get <device> <pin>",
		     cmd_gpio_get, 3, 0),
	SHELL_CMD_ARG(set, &dsub_device_name,
		     "Set GPIO\n"
		     "Usage: set <device> <pin> <value>",
		     cmd_gpio_set, 4, 0),
	SHELL_CMD_ARG(blink, &dsub_device_name,
		     "Blink GPIO\n"
		     "Usage: blink <device> <pin>",
		     cmd_gpio_blink, 3, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(gpio, &sub_gpio, "GPIO commands", NULL);
