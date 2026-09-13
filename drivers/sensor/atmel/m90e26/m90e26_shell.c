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

LOG_MODULE_REGISTER(m90e26_shell, CONFIG_SENSOR_LOG_LEVEL);

#include "m90e26.h"

static int cmd_m90e26_read_register(const struct shell *m90e26_shell, size_t argc, char **argv)
{
	int ret = 0;
	m90e26_data_value_t value = {0};
	m90e26_data_value_t mean = {0};
	m90e26_register_t reg = (m90e26_register_t)strtoul(argv[2], NULL, 16);
	const size_t N = argc == 4 ? strtol(argv[3], NULL, 10) : 1;
	const struct device *dev;
	const struct m90e26_config *config = NULL;

	if (argc < 3 || argc > 4) {
		shell_error(m90e26_shell,
			    "Usage: m90e26 read_register [device] [hex_reg_addr] <N>");
		return -EINVAL;
	}

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(m90e26_shell, "Device %s not found.", argv[1]);
		return -ENODEV;
	}

	config = (struct m90e26_config *)dev->config;

	for (size_t i = 0; i < N; i++) {
		ret = config->bus_io->read(dev, reg, &value);
		if (ret < 0) {
			shell_error(m90e26_shell, "Error on read register from %s. [%d]", argv[1],
				    ret);
			return ret;
		}
		mean.uint16 += value.uint16 / N;
	}

	shell_print(m90e26_shell, "Register: [0x%04X] | Value: [0x%04X]", reg, mean.uint16);

	return ret;
}

static int cmd_m90e26_write_register(const struct shell *m90e26_shell, size_t argc, char **argv)
{
	int ret = 0;
	m90e26_data_value_t value = {0};
	m90e26_register_t reg = (m90e26_register_t)strtoul(argv[2], NULL, 16);
	const struct device *dev;
	const struct m90e26_config *config = NULL;

	if (argc != 4) {
		shell_error(m90e26_shell,
			    "Usage: m90e26 write_register [device] [hex_reg_addr] [hex_value]");
		return -EINVAL;
	}

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(m90e26_shell, "Device %s not found.", argv[1]);
		return -ENODEV;
	}

	config = (const struct m90e26_config *)dev->config;

	value.uint16 = (uint16_t)strtoul(argv[3], NULL, 16);

	ret = config->bus_io->write(dev, reg, &value);
	if (ret < 0) {
		shell_error(m90e26_shell, "Error on write register to %s. [%d]", argv[1], ret);
		return ret;
	}

	shell_print(m90e26_shell, "Wrote Register: [0x%04X] | Value: [0x%04X]", reg, value.uint16);

	return ret;
}

#define M90E26_SHELL_DESCRIPTION_READ_REGISTER                                                     \
	"Read M90E26 register. <N> is optional for averaging N samples.\n"                         \
	"Usage: read_register [<device>] [hex_reg_addr] <N>"

#define M90E26_SHELL_DESCRIPTION_WRITE_REGISTER                                                    \
	"Write M90E26 register.\n"                                                                 \
	"Usage: write_register [<device>] [hex_reg_addr] [hex_value]"

SHELL_STATIC_SUBCMD_SET_CREATE(m90e26_cmds,
			       SHELL_CMD_ARG(read_register, NULL,
					     M90E26_SHELL_DESCRIPTION_READ_REGISTER,
					     cmd_m90e26_read_register, 3, 1),
			       SHELL_CMD_ARG(write_register, NULL,
					     M90E26_SHELL_DESCRIPTION_WRITE_REGISTER,
					     cmd_m90e26_write_register, 4, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(m90e26, &m90e26_cmds, "Atmel M90E26 sensor commands", NULL);
