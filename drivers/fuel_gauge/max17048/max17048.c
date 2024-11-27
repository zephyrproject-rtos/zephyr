/* max17048.c - Driver for max17048 battery fuel gauge */

/*
 * Copyright (c) 2023 Alvaro Garcia Gomez <maxpowel@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max17048

#include "max17048.h"

#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MAX17048);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "MAX17048 driver enabled without any devices"
#endif

/**
 * Storage for the fuel gauge basic information
 */
struct max17048_data {
	/* Charge as percentage */
	uint8_t charge;
	/* Voltage as uV */
	uint32_t voltage;

	/* Time in minutes */
	uint16_t time_to_full;
	uint16_t time_to_empty;
	/* True if battery chargin, false if discharging */
	bool charging;
};

/**
 * I2C communication
 * The way we read a value is first writing the address we want to read and then
 * wait for 2 bytes containing the data.
 */
int max17048_read_register(const struct device *dev, uint8_t registerId, uint16_t *response)
{
	uint8_t max17048_buffer[2];
	const struct max17048_config *cfg = dev->config;
	int rc = i2c_write_read_dt(&cfg->i2c, &registerId, sizeof(registerId), max17048_buffer,
				   sizeof(max17048_buffer));
	if (rc != 0) {
		LOG_ERR("Unable to read register, error %d", rc);
		return rc;
	}

	*response = sys_get_be16(max17048_buffer);
	return 0;
}

/**
 * Raw value from the internal ADC
 */
int max17048_adc(const struct device *i2c_dev, uint16_t *response)
{
	return max17048_read_register(i2c_dev, REGISTER_VCELL, response);
}

/**
 * Battery voltage
 */
int max17048_voltage(const struct device *i2c_dev, uint32_t *response)
{
	uint16_t raw_voltage;
	int rc = max17048_adc(i2c_dev, &raw_voltage);

	if (rc < 0) {
		return rc;
	}
	/**
	 * Once the value is read, it has to be converted to volts. The datasheet
	 * https://www.analog.com/media/en/technical-documentation/data-sheets/
	 * MAX17048-MAX17049.pdf
	 * Page 10, Table 2. Register Summary: 78.125µV/cell
	 * Max17048 only supports one cell so we just have to multiply the value by 78.125 to
	 * obtain µV
	 */

	*response = (uint32_t)raw_voltage * 78.125;
	return 0;
}

/**
 * Battery percentage still available
 */
int max17048_percent(const struct device *i2c_dev, uint8_t *response)
{
	uint16_t data;
	int rc = max17048_read_register(i2c_dev, REGISTER_SOC, &data);

	if (rc < 0) {
		return rc;
	}
	/**
	 * Once the value is read, it has to be converted to percentage. The datasheet
	 * https://www.analog.com/media/en/technical-documentation/data-she4ets/
	 * MAX17048-MAX17049.pdf
	 * Page 10, Table 2. Register Summary: 1%/256
	 * So to obtain the total percentaje we just divide the read value by 256
	 */
	*response = data / 256;
	return 0;
}

/**
 * Percentage of the total battery capacity per hour, positive is charging or
 * negative if discharging
 */
int max17048_crate(const struct device *i2c_dev, int16_t *response)
{
	int rc = max17048_read_register(i2c_dev, REGISTER_CRATE, response);

	if (rc < 0) {
		return rc;
	}

	/**
	 * Once the value is read, it has to be converted to something useful. The datasheet
	 * https://www.analog.com/media/en/technical-documentation/data-sheets/
	 * MAX17048-MAX17049.pdf
	 * Page 11, Table 2. Register Summary (continued): 0.208%/hr
	 * To avoid floats, the value will be multiplied by 208 instead of 0.208, taking into
	 * account that the value will be 1000 times higher
	 */
	*response = *response * 208;
	return 0;
}

/**
 * Initialize and verify the chip. The datasheet says that the version register
 * should be 0x10. If not, or the chip is malfunctioning or it is not a MAX17048 at all
 */
static int max17048_init(const struct device *dev)
{
	const struct max17048_config *cfg = dev->config;
	uint16_t version;
	int rc = max17048_read_register(dev, REGISTER_VERSION, &version);

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	if (rc < 0) {
		LOG_ERR("Cannot read from I2C");
		return rc;
	}

	version = version & 0xFFF0;
	if (version != 0x10) {
		LOG_ERR("Something found at the provided I2C address, but it is not a MAX17048");
		LOG_ERR("The version registers should be 0x10 but got %x. Maybe your wiring is "
			"wrong or it is a fake chip\n",
			version);
		return -ENODEV;
	}

	return 0;
}

/**
 * Get a single property from the fuel gauge
 */
static int max17048_get_single_prop_impl(const struct device *dev, fuel_gauge_prop_t prop,
					 union fuel_gauge_prop_val *val)
{
	struct max17048_data *data = dev->data;
	int rc = 0;

	switch (prop) {
	case FUEL_GAUGE_RUNTIME_TO_EMPTY:
		val->runtime_to_empty = data->time_to_empty;
		break;
	case FUEL_GAUGE_RUNTIME_TO_FULL:
		val->runtime_to_full = data->time_to_full;
		break;
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE:
		val->relative_state_of_charge = data->charge;
		break;
	case FUEL_GAUGE_VOLTAGE:
		val->voltage = data->voltage;
		break;
	default:
		rc = -ENOTSUP;
	}

	return rc;
}

/**
 * Get properties from the fuel gauge
 */
static int max17048_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
			     union fuel_gauge_prop_val *val)
{
	struct max17048_data *data = dev->data;
	int rc = max17048_percent(dev, &data->charge);
	int16_t crate;
	int ret;

	if (rc < 0) {
		LOG_ERR("Error while reading battery percentage");
		return rc;
	}

	rc = max17048_voltage(dev, &data->voltage);
	if (rc < 0) {
		LOG_ERR("Error while reading battery voltage");
		return rc;
	}

	/**
	 * Crate (current rate) is the current percentage of the battery charged or drained
	 * per hour
	 */
	rc = max17048_crate(dev, &crate);
	if (rc < 0) {
		LOG_ERR("Error while reading battery current rate");
		return rc;
	}

	if (crate != 0) {

		/**
		 * May take some time until the chip detects the change between discharging to
		 * charging (and vice versa) especially if your device consumes little power
		 */
		data->charging = crate > 0;

		/**
		 * In the following code, we multiply by 1000 the charge to increase the
		 * precision. If we just truncate the division without this multiplier,
		 * the precision lost is very significant when converting it into minutes
		 * (the value given is in hours)
		 *
		 * The value coming from crate is already 1000 times higher (check the
		 * function max17048_crate to see the reason) so the multiplier for the
		 * charge will be 1000000
		 */
		if (data->charging) {
			uint8_t percentage_pending = 100 - data->charge;
			uint32_t hours_pending = percentage_pending * 1000000 / crate;

			data->time_to_empty = 0;
			data->time_to_full = hours_pending * 60 / 1000;
		} else {
			/* Discharging */
			uint32_t hours_pending = data->charge * 1000000 / -crate;

			data->time_to_empty = hours_pending * 60 / 1000;
			data->time_to_full = 0;
		}
	} else {
		/**
		 * This case is to avoid a division by 0 when the charge rate is the same
		 * than consumption rate. It could also happen when the sensor is still
		 * calibrating the battery
		 */
		data->charging = false;
		data->time_to_full = 0;
		data->time_to_empty = 0;
	}

	ret = max17048_get_single_prop_impl(dev, prop, val);

	return ret;
}

static DEVICE_API(fuel_gauge, max17048_driver_api) = {
	.get_property = &max17048_get_prop,
};

#define MAX17048_DEFINE(inst)                                                                      \
	static struct max17048_data max17048_data_##inst;                                          \
                                                                                                   \
	static const struct max17048_config max17048_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst)};                                                \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &max17048_init, NULL, &max17048_data_##inst,                   \
			&max17048_config_##inst, POST_KERNEL,                                \
			CONFIG_FUEL_GAUGE_INIT_PRIORITY, &max17048_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX17048_DEFINE)
