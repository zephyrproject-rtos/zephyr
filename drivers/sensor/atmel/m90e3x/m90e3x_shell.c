/*
 * Copyright (c) 2025, Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(m90e3x_shell, CONFIG_SENSOR_LOG_LEVEL);

#include "m90e3x.h"

#define M90E3X_DESCR_READ_USAGE "Usage: read_register [<device>] [hex_reg_addr] <N>"

#define M90E3X_DESCR_WRITE_USAGE "Usage: write_register [<device>] [hex_reg_addr] [hex_value]"

#define M90E3X_DESCR_GET_POWER_MODE                                                                \
	"Get M90E3X current power mode.\n"                                                         \
	"Usage: get_power_mode [<device>]"

#define M90E3X_DESCR_SET_POWER_MODE                                                                \
	"Set M90E3X power mode.\n"                                                                 \
	"Usage: set_power_mode [<device>] [mode]\n"                                                \
	"Where mode is one of: 0 (IDLE), 1 (DETECTION), 2 (PARTIAL), 3 (NORMAL)"

#define M90E3X_DESCR_READ_REGISTER                                                                 \
	"Read M90E3X register. <N> is optional for averaging N samples.\n" M90E3X_DESCR_READ_USAGE

#define M90E3X_DESCR_WRITE_REGISTER "Write M90E3X register.\n" M90E3X_DESCR_WRITE_USAGE

static int cmd_m90e3x_get_power_mode(const struct shell *m90e3x_shell, size_t argc, char **argv)
{
	const struct device *dev;
	struct m90e3x_data *data;
	enum m90e3x_power_mode power_mode;

	if (argc != 2) {
		shell_print(m90e3x_shell, "Usage: %s [device]", argv[0]);
		return -EINVAL;
	}

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_print(m90e3x_shell, "Device %s not found.", argv[1]);
		return -ENODEV;
	}

	data = (struct m90e3x_data *)dev->data;
	power_mode = data->current_power_mode;

	switch (power_mode) {
	case M90E3X_IDLE:
		shell_print(m90e3x_shell, "Power Mode: IDLE");
		break;
	case M90E3X_DETECTION:
		shell_print(m90e3x_shell, "Power Mode: DETECTION");
		break;
	case M90E3X_PARTIAL:
		shell_print(m90e3x_shell, "Power Mode: PARTIAL MEASUREMENT");
		break;
	case M90E3X_NORMAL:
		shell_print(m90e3x_shell, "Power Mode: NORMAL");
		break;
	default:
		shell_print(m90e3x_shell, "Power Mode: UNKNOWN");
		break;
	}

	return 0;
}

static int cmd_m90e3x_set_power_mode(const struct shell *m90e3x_shell, size_t argc, char **argv)
{
	const struct device *dev;
	struct m90e3x_config *config;
	struct m90e3x_data *data;
	int mode;
	int ret;

	if (argc != 3) {
		shell_print(m90e3x_shell, "Usage: %s [device] [mode]", argv[0]);
		return -EINVAL;
	}

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_print(m90e3x_shell, "Device %s not found.", argv[1]);
		return -ENODEV;
	}

	mode = shell_strtol(argv[2], 10, &ret);
	if (ret < 0) {
		shell_print(m90e3x_shell, "Invalid mode value: %s", argv[2]);
		return -EINVAL;
	}

	config = (struct m90e3x_config *)dev->config;
	data = (struct m90e3x_data *)dev->data;

	if (!config->pm_mode_ops) {
		shell_print(m90e3x_shell, "Power mode operations not defined for device %s.",
			    dev->name);
		return -ENOTSUP;
	}

	switch ((enum m90e3x_power_mode)mode) {
	case M90E3X_IDLE:
		ret = config->pm_mode_ops->enter_idle_mode(dev);
		break;
	case M90E3X_DETECTION:
		ret = config->pm_mode_ops->enter_detection_mode(dev);
		break;
	case M90E3X_PARTIAL:
		ret = config->pm_mode_ops->enter_partial_measurement_mode(dev);
		break;
	case M90E3X_NORMAL:
		ret = config->pm_mode_ops->enter_normal_mode(dev);
		break;
	default:
		shell_print(m90e3x_shell, "Invalid power mode value: %d", mode);
		return -EINVAL;
	}

	if (ret < 0) {
		shell_print(m90e3x_shell, "Failed to set power mode %d", mode);
		return ret;
	}

	data->current_power_mode = (enum m90e3x_power_mode)mode;

	shell_print(m90e3x_shell, "Power mode set to %d", mode);
	return 0;
}

static int cmd_m90e3x_read_register(const struct shell *m90e3x_shell, size_t argc, char **argv)
{
	int ret = 0;
	m90e3x_data_value_t value = {0}, mean = {0};
	m90e3x_register_t reg = strtol(argv[2], NULL, 16);
	const size_t N = argc == 4 ? strtol(argv[3], NULL, 10) : 1;
	const struct device *dev;
	struct m90e3x_config *config;

	if (argc < 3 || argc > 4) {
		shell_error(m90e3x_shell, M90E3X_DESCR_READ_USAGE);
		return -EINVAL;
	}

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(m90e3x_shell, "Device %s not found.", argv[1]);
		return -ENODEV;
	}

	config = (struct m90e3x_config *)dev->config;

	for (size_t i = 0; i < N; i++) {
		ret = config->bus_io->read(dev, reg, &value);
		if (ret < 0) {
			shell_error(m90e3x_shell, "Error on read register from %s. [%d]", argv[1],
				    ret);
			return ret;
		}
		mean.uint16 += value.uint16 / N;
	}

	shell_print(m90e3x_shell, "Register: [0x%04X] | Value: [0x%04X]", reg, mean.uint16);

	return ret;
}

static int cmd_m90e3x_write_register(const struct shell *m90e3x_shell, size_t argc, char **argv)
{
	int ret = 0;
	m90e3x_data_value_t value = {0};
	m90e3x_register_t reg = strtol(argv[2], NULL, 16);
	const struct device *dev;
	struct m90e3x_config *config;

	if (argc != 4) {
		shell_error(m90e3x_shell, M90E3X_DESCR_WRITE_USAGE);
		return -EINVAL;
	}

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(m90e3x_shell, "Device %s not found.", argv[1]);
		return -ENODEV;
	}

	config = (struct m90e3x_config *)dev->config;

	value.uint16 = strtol(argv[3], NULL, 16);

	ret = config->bus_io->write(dev, reg, &value);
	if (ret < 0) {
		shell_error(m90e3x_shell, "Error on write register to %s. [%d]", argv[1], ret);
		return ret;
	}

	shell_print(m90e3x_shell, "Wrote Register: [0x%04X] | Value: [0x%04X]", reg, value.uint16);

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(m90e3x_cmds,
			       SHELL_CMD_ARG(get_power_mode, NULL, M90E3X_DESCR_GET_POWER_MODE,
					     cmd_m90e3x_get_power_mode, 2, 0),
			       SHELL_CMD_ARG(set_power_mode, NULL, M90E3X_DESCR_SET_POWER_MODE,
					     cmd_m90e3x_set_power_mode, 3, 0),
			       SHELL_CMD_ARG(read_register, NULL, M90E3X_DESCR_READ_REGISTER,
					     cmd_m90e3x_read_register, 3, 1),
			       SHELL_CMD_ARG(write_register, NULL, M90E3X_DESCR_WRITE_REGISTER,
					     cmd_m90e3x_write_register, 4, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(m90e3x, &m90e3x_cmds, "Atmel M90E3X sensor commands", NULL);
