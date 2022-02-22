/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2021 Dennis Ruffer <daruffer@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Use "device list" command for GPIO port names
 */

#include <sys/printk.h>
#include <shell/shell.h>
#include <init.h>
#include <string.h>
#include <stdio.h>
#include <drivers/gpio.h>
#include <drivers/gpio/gpio_name.h>
#include <stdlib.h>
#include <ctype.h>
#include <logging/log.h>

#ifdef CONFIG_GPIO_NAME
#include "gpio_name_shell.h"
#endif

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

LOG_MODULE_REGISTER(gpio_shell);

enum arg_idx {
	ARG_PORT_OR_NAME        = 1,    /* Port or name argument */
	ARG_INDEX               = 2,    /* pin index argument (after port) */
	ARG_NAME_NEXT           = 2,    /* Next argument after name */
	ARG_PORT_NEXT           = 3,    /* Next argument after port and index */
};

/*
 * Decode arguments 1 and optionally 2 as either a
 * name, or as device and separate pin index.
 * Return the next argument index (2 or 3), and set the
 * port and index, or -1 for error.
 */
static int cmd_gpio_port_index(size_t argc, char **argv,
			       const struct device **dev, gpio_pin_t *index)
{
#ifdef CONFIG_GPIO_NAME
	/* Must be at least one argument */
	if (argc < 2) {
		return -1;
	}
	/* Check and see if the name is a match.  */
	if (gpio_pin_by_name(argv[ARG_PORT_OR_NAME], dev, index) == 0) {
		/* Matched name, step to argument after name */
		return ARG_NAME_NEXT;
	}
#endif
	/* Must be at least 2 arguments */
	if (argc < 3) {
		return -1;
	}
	*dev = device_get_binding(argv[ARG_PORT_OR_NAME]);
	if (*dev == NULL) {
		/* unknown device */
		*index = 0;
		return -1;
	}
	*index = atoi(argv[ARG_INDEX]);
	if (*index < 0) {
		*dev = NULL;
		*index = 0;
		return -1;
	}
	/* Found GPIO port and index */
	return ARG_PORT_NEXT;
}

static int cmd_gpio_conf(const struct shell *sh, size_t argc, char **argv)
{
	gpio_pin_t index;
	int next_arg;
	int type = GPIO_OUTPUT;
	const struct device *dev;

	next_arg = cmd_gpio_port_index(argc, argv, &dev, &index);
	if (next_arg < 0) {
		shell_error(sh, "Cannot find GPIO");
		return -ENOENT;
	}
	/* Make sure there is a mode argument */
	if (next_arg >= argc) {
		shell_error(sh, "Missing 'in' or 'out' parameter");
		return -ENOTSUP;
	}
	if (!strcmp(argv[next_arg], "in")) {
		type = GPIO_INPUT;
	} else if (!strcmp(argv[next_arg], "out")) {
		type = GPIO_OUTPUT;
	} else {
		return 0;
	}

	shell_print(sh, "Configuring %s pin %d", dev->name, index);
	gpio_pin_configure(dev, index, type);

	return 0;
}

static int cmd_gpio_get(const struct shell *sh,
			size_t argc, char **argv)
{
	const struct device *dev;
	gpio_pin_t index;
	int rc;
	int next_arg;

#ifdef CONFIG_GPIO_NAME
	/* No arguments, display all named GPIOs */
	if (argc == 1) {
		cmd_gpio_name_show_all(sh);
		return 0;
	}
	/* Try the name */
	if (argc == 2) {
		if (cmd_gpio_name_show(sh, argv[ARG_PORT_OR_NAME]) == 0) {
			return 0;
		}
	}
#endif

	next_arg = cmd_gpio_port_index(argc, argv, &dev, &index);
	if (next_arg < 0) {
		shell_error(sh, "Wrong parameters for get");
		return -EINVAL;
	}
	shell_print(sh, "Reading %s pin %d", dev->name, index);
	rc = gpio_pin_get(dev, index);
	if (rc >= 0) {
		shell_print(sh, "Value %d", rc);
	} else {
		shell_error(sh, "Error %d reading value", rc);
		return -EIO;
	}

	return 0;
}

static int cmd_gpio_set(const struct shell *sh,
			size_t argc, char **argv)
{
	const struct device *dev;
	gpio_pin_t index;
	int value;
	int next_arg;

	next_arg = cmd_gpio_port_index(argc, argv, &dev, &index);
	if (next_arg < 0) {
		shell_error(sh, "Wrong parameters for set");
		return -EINVAL;
	}
	/* Make sure there is a value argument */
	if (next_arg >= argc) {
		shell_error(sh, "Missing value parameter");
		return -EINVAL;
	}
	value = atoi(argv[next_arg]);
	if (value != 0 && value != 1) {
		shell_error(sh, "Illegal value");
		return -EINVAL;
	}

	shell_print(sh, "Writing to %s pin %d value %d", dev->name, index, value);
	gpio_pin_set(dev, index, value);

	return 0;
}


/* 500 msec = 1/2 sec */
#define SLEEP_TIME_MS   500

static int cmd_gpio_blink(const struct shell *sh,
			  size_t argc, char **argv)
{
	const struct device *dev;
	gpio_pin_t index;
	int value = 0U;
	size_t count = 0;
	char data;

	if (cmd_gpio_port_index(argc, argv, &dev, &index) < 0) {
		shell_error(sh, "Wrong parameters for blink");
		return -EINVAL;
	}
	shell_fprintf(sh, SHELL_NORMAL, "Blinking port %s index %d.", dev->name, index);
	shell_fprintf(sh, SHELL_NORMAL, " Hit any key to exit");

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

SHELL_STATIC_SUBCMD_SET_CREATE(sub_gpio,
			       SHELL_CMD_ARG(conf, NULL, "Configure GPIO", cmd_gpio_conf, 3, 1),
			       SHELL_CMD_ARG(get, NULL, "Get GPIO value", cmd_gpio_get, 1, 2),
			       SHELL_CMD_ARG(set, NULL, "Set GPIO", cmd_gpio_set, 3, 1),
			       SHELL_CMD_ARG(blink, NULL, "Blink GPIO", cmd_gpio_blink, 2, 1),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
			       );

SHELL_CMD_REGISTER(gpio, &sub_gpio, "GPIO commands", NULL);
