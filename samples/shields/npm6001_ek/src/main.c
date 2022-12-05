/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/dt-bindings/gpio/nordic-npm6001-gpio.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>

#include <getopt.h>

static const struct device *const gpio = DEVICE_DT_GET_ONE(nordic_npm6001_gpio);
static const struct device *const wdt = DEVICE_DT_GET_ONE(nordic_npm6001_wdt);

void main(void)
{
	if (!device_is_ready(gpio)) {
		printk("nPM6001 GPIO device not ready\n");
		return;
	}

	if (!device_is_ready(wdt)) {
		printk("nPM6001 Watchdog device not ready\n");
		return;
	}
}

static int cmd_gpio_configure(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	int opt, long_index = 0;
	static int high_drive, pull_down, cmos;
	gpio_pin_t pin = 0U;
	gpio_flags_t flags = 0U;

	static const struct option long_options[] = {
		{"pin", required_argument, NULL, 'p'},
		{"direction", required_argument, NULL, 'd'},
		{"high-drive", no_argument, &high_drive, 1},
		{"pull-down", no_argument, &pull_down, 1},
		{"cmos", no_argument, &cmos, 1},
		{NULL, 0, NULL, 0},
	};

	high_drive = 0;
	pull_down = 0;
	cmos = 0;

	while ((opt = getopt_long(argc, argv, "p:d:", long_options,
				  &long_index)) != -1) {
		switch (opt) {
		case 0:
			/* options setting a flag, do nothing */
			break;
		case 'p':
			pin = atoi(optarg);
			break;
		case 'd':
			if (strcmp(optarg, "in") == 0) {
				flags |= GPIO_INPUT;
			} else if (strcmp(optarg, "out") == 0) {
				flags |= GPIO_OUTPUT;
			} else if (strcmp(optarg, "outh") == 0) {
				flags |= GPIO_OUTPUT_HIGH;
			} else if (strcmp(optarg, "outl") == 0) {
				flags |= GPIO_OUTPUT_LOW;
			}
			break;
		default:
			shell_error(sh, "Invalid option: %c", opt);
			return -EINVAL;
		}
	}

	if (pull_down == 1) {
		flags |= GPIO_PULL_DOWN;
	}

	if (high_drive == 1) {
		flags |= NPM6001_GPIO_DRIVE_HIGH;
	}

	if (cmos == 1) {
		flags |= NPM6001_GPIO_SENSE_CMOS;
	}

	ret = gpio_pin_configure(gpio, pin, flags);
	if (ret < 0) {
		shell_error(sh, "Configuration failed (%d)", ret);
		return ret;
	}

	return 0;
}

static int cmd_gpio_get(const struct shell *sh, size_t argc, char **argv)
{
	gpio_pin_t pin;
	int val;

	pin = (gpio_pin_t)atoi(argv[1]);

	val = gpio_pin_get(gpio, pin);
	if (val < 0) {
		shell_error(sh, "Could not get pin level (%d)", val);
		return val;
	}

	shell_print(sh, "Level: %d", val);

	return 0;
}

static int cmd_gpio_set(const struct shell *sh, size_t argc, char **argv)
{
	gpio_pin_t pin;
	int val;
	int ret;

	pin = (gpio_pin_t)atoi(argv[1]);
	val = atoi(argv[2]);

	ret = gpio_pin_set(gpio, pin, val);
	if (ret < 0) {
		shell_error(sh, "Could not set pin level (%d)", ret);
		return ret;
	}

	return 0;
}

static int cmd_gpio_toggle(const struct shell *sh, size_t argc, char **argv)
{
	gpio_pin_t pin;
	int ret;

	pin = (gpio_pin_t)atoi(argv[1]);

	ret = gpio_pin_toggle(gpio, pin);
	if (ret < 0) {
		shell_error(sh, "Could not toggle pin level (%d)", ret);
		return ret;
	}

	return 0;
}

static int cmd_wdt_enable(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	struct wdt_timeout_cfg cfg = { 0 };

	cfg.window.max = (uint32_t)strtoul(argv[1], NULL, 10) * 1000U;

	ret = wdt_install_timeout(wdt, &cfg);
	if (ret < 0) {
		shell_error(sh, "Could not install watchdog timeout (%d)", ret);
	}

	return 0;
}

static int cmd_wdt_disable(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ret = wdt_disable(wdt);
	if (ret < 0) {
		shell_error(sh, "Could not disable watchdog (%d)", ret);
	}

	return 0;
}

static int cmd_wdt_kick(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ret = wdt_feed(wdt, 0);
	if (ret < 0) {
		shell_error(sh, "Could not kick watchdog (%d)", ret);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_npm6001_gpio_cmds,
			       SHELL_CMD_ARG(configure, NULL, "Configure GPIO",
					     cmd_gpio_configure, 5, 3),
			       SHELL_CMD_ARG(get, NULL, "Get GPIO level",
					     cmd_gpio_get, 2, 0),
			       SHELL_CMD_ARG(set, NULL, "Set GPIO level",
					     cmd_gpio_set, 3, 0),
			       SHELL_CMD_ARG(toggle, NULL, "Toggle GPIO level",
					     cmd_gpio_toggle, 2, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_npm6001_wdt_cmds,
			       SHELL_CMD_ARG(enable, NULL, "Enable watchdog",
					     cmd_wdt_enable, 2, 0),
			       SHELL_CMD(disable, NULL, "Disable watchdog",
					 cmd_wdt_disable),
			       SHELL_CMD(kick, NULL, "Kick watchdog",
					 cmd_wdt_kick),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_npm6001_cmds,
			       SHELL_CMD(gpio, &sub_npm6001_gpio_cmds, "GPIO",
					 NULL),
			       SHELL_CMD(wdt, &sub_npm6001_wdt_cmds, "Watchdog",
					 NULL),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(npm6001, &sub_npm6001_cmds, "nPM6001 commands", NULL);
