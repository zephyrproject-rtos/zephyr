/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/dt-bindings/gpio/nordic-npm6001-gpio.h>
#include <zephyr/dt-bindings/regulator/npm6001.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>

#include <getopt.h>

struct regulators_map {
	const char *name;
	const struct device *dev;
};

static const struct device *const gpio = DEVICE_DT_GET_ONE(nordic_npm6001_gpio);
static const struct device *const wdt = DEVICE_DT_GET_ONE(nordic_npm6001_wdt);
static const struct regulators_map regulators[] = {
	{ "BUCK0", DEVICE_DT_GET_OR_NULL(DT_NODELABEL(npm6001_ek_buck0)) },
	{ "BUCK1", DEVICE_DT_GET_OR_NULL(DT_NODELABEL(npm6001_ek_buck1)) },
	{ "BUCK2", DEVICE_DT_GET_OR_NULL(DT_NODELABEL(npm6001_ek_buck2)) },
	{ "BUCK3", DEVICE_DT_GET_OR_NULL(DT_NODELABEL(npm6001_ek_buck3)) },
	{ "LDO0", DEVICE_DT_GET_OR_NULL(DT_NODELABEL(npm6001_ek_ldo0)) },
	{ "LDO1", DEVICE_DT_GET_OR_NULL(DT_NODELABEL(npm6001_ek_ldo1)) },
};

static const struct device *name2reg(const char *name)
{
	for (size_t i = 0U; i < ARRAY_SIZE(regulators); i++) {
		if (strcmp(name, regulators[i].name) == 0) {
			return regulators[i].dev;
		}
	}

	return NULL;
}

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

	for (size_t i = 0U; i < ARRAY_SIZE(regulators); i++) {
		if ((regulators[i].dev != NULL) &&
		    !device_is_ready(regulators[i].dev)) {
			printk("nPM6001 %s regulator device not ready\n",
			       regulators[i].name);
			return;
		}
	}
}

static int cmd_regulator_list(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	for (size_t i = 0U; i < ARRAY_SIZE(regulators); i++) {
		if (regulators[i].dev != NULL) {
			shell_print(sh, "%s", regulators[i].name);
		}
	}

	return 0;
}

static int cmd_regulator_voltages(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	unsigned int volt_cnt;

	ARG_UNUSED(argc);

	dev = name2reg(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Invalid regulator: %s", argv[1]);
		return -ENODEV;
	}

	volt_cnt = regulator_count_voltages(dev);

	for (unsigned int i = 0U; i < volt_cnt; i++) {
		int32_t volt_uv;

		(void)regulator_list_voltage(dev, i, &volt_uv);
		shell_print(sh, "%d mV", volt_uv / 1000);
	}

	return 0;
}

static int cmd_regulator_enable(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	ARG_UNUSED(argc);

	dev = name2reg(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Invalid regulator: %s", argv[1]);
		return -ENODEV;
	}

	ret = regulator_enable(dev);
	if (ret < 0) {
		shell_error(sh, "Could not enable regulator (%d)", ret);
		return ret;
	}

	return 0;
}

static int cmd_regulator_disable(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	ARG_UNUSED(argc);

	dev = name2reg(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Invalid regulator: %s", argv[1]);
		return -ENODEV;
	}

	ret = regulator_disable(dev);
	if (ret < 0) {
		shell_error(sh, "Could not disable regulator (%d)", ret);
		return ret;
	}

	return 0;
}

static int cmd_regulator_set(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int32_t min_uv, max_uv;
	int ret;

	dev = name2reg(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Invalid regulator: %s", argv[1]);
		return -ENODEV;
	}

	min_uv = (int32_t)strtoul(argv[2], NULL, 10) * 1000;
	if (argc == 4) {
		max_uv = (int32_t)strtoul(argv[3], NULL, 10) * 1000;
	} else {
		max_uv = min_uv;
	}

	ret = regulator_set_voltage(dev, min_uv, max_uv);
	if (ret < 0) {
		shell_error(sh, "Could not set voltage (%d)", ret);
		return ret;
	}

	return 0;
}

static int cmd_regulator_get(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int32_t volt_uv;
	int ret;

	ARG_UNUSED(argc);

	dev = name2reg(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Invalid regulator: %s", argv[1]);
		return -ENODEV;
	}

	ret = regulator_get_voltage(dev, &volt_uv);
	if (ret < 0) {
		shell_error(sh, "Could not get voltage (%d)", ret);
		return ret;
	}

	shell_print(sh, "%d mV", volt_uv / 1000);

	return 0;
}

static int cmd_regulator_modeset(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	regulator_mode_t mode;
	int ret;

	ARG_UNUSED(argc);

	dev = name2reg(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Invalid regulator: %s", argv[1]);
		return -ENODEV;
	}

	if (strcmp(argv[2], "pwm") == 0) {
		mode = NPM6001_MODE_PWM;
	} else if (strcmp(argv[2], "hys") == 0) {
		mode = NPM6001_MODE_HYS;
	} else {
		shell_error(sh, "Invalid mode: %s", argv[1]);
		return -EINVAL;
	}

	ret = regulator_set_mode(dev, mode);
	if (ret < 0) {
		shell_error(sh, "Could not set mode (%d)", ret);
		return ret;
	}

	return 0;
}

static int cmd_regulator_modeget(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	regulator_mode_t mode;
	int ret;

	ARG_UNUSED(argc);

	dev = name2reg(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Invalid regulator: %s", argv[1]);
		return -ENODEV;
	}

	ret = regulator_get_mode(dev, &mode);
	if (ret < 0) {
		shell_error(sh, "Could not get mode (%d)", ret);
		return ret;
	}

	if (mode == NPM6001_MODE_PWM) {
		shell_print(sh, "PWM");
	} else if (mode == NPM6001_MODE_HYS) {
		shell_print(sh, "Hysteretic");
	} else {
		shell_error(sh, "Invalid mode: %u", mode);
		return -EINVAL;
	}

	return 0;
}

static int cmd_regulator_errors(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	regulator_error_flags_t errors;
	int ret;

	ARG_UNUSED(argc);

	dev = name2reg(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Invalid regulator: %s", argv[1]);
		return -ENODEV;
	}

	ret = regulator_get_error_flags(dev, &errors);
	if (ret < 0) {
		shell_error(sh, "Could not get error flags (%d)", ret);
		return ret;
	}

	shell_print(sh, "Overcurrent:\t[%s]",
		    ((errors & REGULATOR_ERROR_OVER_CURRENT) != 0U) ? "X" : " ");
	shell_print(sh, "Overtemp.:\t[%s]",
		    ((errors & REGULATOR_ERROR_OVER_TEMP) != 0U) ? "X" : " ");

	return 0;
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

SHELL_STATIC_SUBCMD_SET_CREATE(sub_npm6001_regulator_cmds,
			       SHELL_CMD(list, NULL, "List regulator names",
					     cmd_regulator_list),
			       SHELL_CMD_ARG(voltages, NULL, "List voltages",
					     cmd_regulator_voltages, 2, 0),
			       SHELL_CMD_ARG(enable, NULL, "Enable regulator",
					     cmd_regulator_enable, 2, 0),
			       SHELL_CMD_ARG(disable, NULL, "Disable regulator",
					     cmd_regulator_disable, 2, 0),
			       SHELL_CMD_ARG(set, NULL, "Set voltage",
					     cmd_regulator_set, 3, 1),
			       SHELL_CMD_ARG(get, NULL, "Get voltage",
					     cmd_regulator_get, 2, 0),
			       SHELL_CMD_ARG(modeset, NULL, "Set mode PWM/HYS",
					     cmd_regulator_modeset, 3, 0),
			       SHELL_CMD_ARG(modeget, NULL, "Get mode PWM/HYS",
					     cmd_regulator_modeget, 2, 0),
			       SHELL_CMD_ARG(errors, NULL, "Get active errors",
					     cmd_regulator_errors, 2, 0),
			       SHELL_SUBCMD_SET_END);

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
			       SHELL_CMD(regulator, &sub_npm6001_regulator_cmds,
					 "Regulators", NULL),
			       SHELL_CMD(gpio, &sub_npm6001_gpio_cmds, "GPIO",
					 NULL),
			       SHELL_CMD(wdt, &sub_npm6001_wdt_cmds, "Watchdog",
					 NULL),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(npm6001, &sub_npm6001_cmds, "nPM6001 commands", NULL);
