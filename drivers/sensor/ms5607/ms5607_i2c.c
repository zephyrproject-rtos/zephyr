/*
 * Copyright (c) 2021 Aurelien Jarno <aurelien@aurel32.net>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT meas_ms5607

#include <string.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include "ms5607.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ms5607);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

static int ms5607_i2c_raw_cmd(const struct ms5607_config *config, uint8_t cmd)
{
	return i2c_write(config->bus, &cmd, 1, config->bus_cfg.i2c_addr);
}

static int ms5607_i2c_reset(const struct ms5607_config *config)
{
	int err = ms5607_i2c_raw_cmd(config, MS5607_CMD_RESET);

	if (err < 0) {
		return err;
	}

	return 0;
}

static int ms5607_i2c_read_prom(const struct ms5607_config *config, uint8_t cmd,
				uint16_t *val)
{
	uint8_t valb[2];
	int err;

	err = i2c_burst_read(config->bus, config->bus_cfg.i2c_addr, cmd, valb, sizeof(valb));
	if (err < 0) {
		return err;
	}

	*val = sys_get_be16(valb);

	return 0;
}


static int ms5607_i2c_start_conversion(const struct ms5607_config *config, uint8_t cmd)
{
	return ms5607_i2c_raw_cmd(config, cmd);
}

static int ms5607_i2c_read_adc(const struct ms5607_config *config, uint32_t *val)
{
	int err;
	uint8_t valb[3];

	err = i2c_burst_read(config->bus, config->bus_cfg.i2c_addr,
			     MS5607_CMD_CONV_READ_ADC, valb, sizeof(valb));
	if (err < 0) {
		return err;
	}

	*val = (valb[0] << 16) + (valb[1] << 8) + valb[2];

	return 0;
}


static int ms5607_i2c_check(const struct ms5607_config *config)
{
	if (!device_is_ready(config->bus)) {
		LOG_DBG("I2C bus %s not ready", config->bus->name);
		return -ENODEV;
	}

	return 0;
}

const struct ms5607_transfer_function ms5607_i2c_transfer_function = {
	.bus_check = ms5607_i2c_check,
	.reset = ms5607_i2c_reset,
	.read_prom = ms5607_i2c_read_prom,
	.start_conversion = ms5607_i2c_start_conversion,
	.read_adc = ms5607_i2c_read_adc,
};

#endif
