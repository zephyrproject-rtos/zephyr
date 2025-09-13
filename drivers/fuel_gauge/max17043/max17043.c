/* max17043.c - Driver for max17043 battery fuel gauge */

/*
 * Copyright (c) 2025  Mohammed Billoo <mab@mab-labs.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max17043

#include "max17043.h"

#include <zephyr/logging/log.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

LOG_MODULE_REGISTER(MAX17043);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "MAX17043 driver enabled without any devices"
#endif

/**
 * Storage for the fuel gauge basic information
 */
struct max17043_data {
	/* Charge as percentage */
	uint8_t charge;
	/* Voltage as uV */
	uint32_t voltage;
};

/**
 * I2C communication
 * The way we read a value is first writing the address we want to read and then
 * wait for 2 bytes containing the data.
 */
int max17043_read_register(const struct device *dev, uint8_t registerId, uint16_t *response)
{
	uint8_t max17043_buffer[2];
	const struct max17043_config *cfg = dev->config;
	int rc = i2c_write_read_dt(&cfg->i2c, &registerId, sizeof(registerId), max17043_buffer,
				   sizeof(max17043_buffer));
	if (rc != 0) {
		LOG_ERR("Unable to read register, error %d", rc);
		return rc;
	}

	*response = sys_get_be16(max17043_buffer);
	return 0;
}

/**
 * Raw value from the internal ADC
 */
int max17043_adc(const struct device *i2c_dev, uint16_t *response)
{
	return max17043_read_register(i2c_dev, REGISTER_VCELL, response);
}

/**
 * Battery voltage
 */
int max17043_voltage(const struct device *i2c_dev, uint32_t *response)
{
	uint16_t raw_voltage;
	int rc = max17043_adc(i2c_dev, &raw_voltage);

	if (rc < 0) {
		return rc;
	}
	/**
	 * Once the value is read, it has to be converted to volts. The datasheet
	 * https://www.analog.com/media/en/technical-documentation/data-sheets/
	 * MAX17043-MAX17049.pdf
	 * Page 10, Table 2. Register Summary: 78.125µV/cell
	 * Max17043 only supports one cell so we just have to multiply the value by 78.125 to
	 * obtain µV
	 */

	*response = (uint32_t)raw_voltage * 78.125;
	return 0;
}

/**
 * Battery percentage still available
 */
int max17043_percent(const struct device *i2c_dev, uint8_t *response)
{
	uint16_t data;
	int rc = max17043_read_register(i2c_dev, REGISTER_SOC, &data);

	if (rc < 0) {
		return rc;
	}
	/**
	 * Once the value is read, it has to be converted to percentage. The datasheet
	 * Page 8, Table 2. Register Summary: 1%/256
	 * So to obtain the total percentaje we just divide the read value by 256
	 */
	*response = data / 256;
	return 0;
}

static int max17043_init(const struct device *dev)
{
	const struct max17043_config *cfg = dev->config;
	uint16_t version;
	int rc = max17043_read_register(dev, REGISTER_VERSION, &version);

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	if (rc < 0) {
		LOG_ERR("Cannot read from I2C");
		return rc;
	}

	LOG_INF("MAX17043 version: %x", version);

	return 0;
}

/**
 * Get a single property from the fuel gauge
 */
int max17043_get_single_prop(const struct device *dev, fuel_gauge_prop_t prop,
			     union fuel_gauge_prop_val *val)
{
	struct max17043_data *data = dev->data;
	int rc = 0;

	switch (prop) {
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE:
		rc = max17043_percent(dev, &data->charge);

		if (rc < 0) {
			return rc;
		}

		val->relative_state_of_charge = data->charge;
		break;
	case FUEL_GAUGE_VOLTAGE:
		rc = max17043_voltage(dev, &data->voltage);
		if (rc < 0) {
			return rc;
		}
		val->voltage = data->voltage;
		break;
	default:
		rc = -ENOTSUP;
	}

	return rc;
}

static const struct fuel_gauge_driver_api max17043_driver_api = {
	.get_property = &max17043_get_single_prop,
};

#define MAX17043_DEFINE(inst)                                                                      \
	static struct max17043_data max17043_data_##inst;                                          \
                                                                                                   \
	static const struct max17043_config max17043_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst)};                                                \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &max17043_init, NULL, &max17043_data_##inst,                   \
			      &max17043_config_##inst, POST_KERNEL,                                \
			      CONFIG_FUEL_GAUGE_INIT_PRIORITY, &max17043_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX17043_DEFINE)
