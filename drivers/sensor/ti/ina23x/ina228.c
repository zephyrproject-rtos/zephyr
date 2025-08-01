/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ina228

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor/ina228.h>
#include <zephyr/sys/byteorder.h>

#include "ina23x_common.h"

LOG_MODULE_REGISTER(INA228, CONFIG_SENSOR_LOG_LEVEL);

#define INA228_REG_CONFIG     0x00
#define INA228_REG_ADC_CONFIG 0x01
#define INA228_REG_SHUNT_CAL  0x02
#define INA228_REG_SHUNT_VOLT 0x04
#define INA228_REG_BUS_VOLT   0x05
#define INA228_REG_DIETEMP    0x06
#define INA228_REG_CURRENT    0x07
#define INA228_REG_POWER      0x08
#define INA228_REG_ENERGY     0x09
#define INA228_REG_CHARGE     0x0A
#define INA228_REG_ALERT      0x0B
#define INA228_REG_SOVL       0x0C
#define INA228_REG_SUVL       0x0D
#define INA228_REG_BOVL       0x0E
#define INA228_REG_BUVL       0x0F
#define INA228_REG_TEMP_LIMIT 0x10
#define INA228_REG_PWR_LIMIT  0x11
#define INA228_REG_MANUFACTURER_ID 0x3E

#define INA228_MANUFACTURER_ID 0x5449

/** @brief Calibration scaling value (scaled by 10^-5) */
#define INA228_CAL_SCALING 131072ULL

struct ina228_data {
	const struct device *dev;
	uint64_t energy;
	uint64_t charge;
	int32_t shunt_voltage;
	int32_t bus_voltage;
	int32_t current;
	uint32_t power;
	int16_t die_temp;
};

struct ina228_config {
	struct i2c_dt_spec bus;
	const struct gpio_dt_spec alert_gpio; /**< Alerts not yet supported */
	uint32_t current_lsb;
	uint16_t adc_config;
	uint16_t cal;
};

static int ina228_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct ina228_data *data = dev->data;
	const struct ina228_config *config = dev->config;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		/* Scaled by 195.3125 uV/LSB; 20-bits, lower 4 bits are zero */
		return sensor_value_from_micro(val,
			((data->bus_voltage >> 4) * 3125) / 16);
	case SENSOR_CHAN_CURRENT:
		/* Scaled by CURRENT_LSB; 20-bits, lower 4 bits are zero*/
		return sensor_value_from_micro(val,
			(data->current >> 4) * config->current_lsb);
	case SENSOR_CHAN_POWER:
		/* Scaled by 3.2 * CURRENT_LSB; 24-bits */
		return sensor_value_from_micro(val,
			(data->power * config->current_lsb * 16) / 5);
	case SENSOR_CHAN_VSHUNT:
		/* Scaled by 312.5 nV/LSB; 20-bits, lower 4 bits are zero */
		return sensor_value_from_micro(val,
			((data->shunt_voltage >> 4) * 5000) / 16);
	case SENSOR_CHAN_DIE_TEMP:
		/* Scaled by 7.8125 m°C/LSB; 16-bits */
		return sensor_value_from_micro(val, (data->die_temp * 125000) / 16);
	case SENSOR_CHAN_INA228_ENERGY:
		/* Scaled by 16 * 3.2 * CURRENT_LSB; 40-bits */
		return sensor_value_from_micro(val,
			((data->energy * config->current_lsb * 256) / 5));
	case SENSOR_CHAN_INA228_CHARGE:
		/* Scaled by CURRENT_LSB; 40-bits */
		return sensor_value_from_micro(val, data->charge * config->current_lsb);
	default:
		return -ENOTSUP;
	}
}

static int ina228_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ina228_data *data = dev->data;
	const struct ina228_config *config = dev->config;
	bool rstacc = false;
	int ret = -ENOTSUP;

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_VSHUNT)) {
		ret = ina23x_reg_read_24(&config->bus, INA228_REG_SHUNT_VOLT, &data->shunt_voltage);
		if (ret < 0) {
			LOG_ERR("failed to read shunt voltage");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_VOLTAGE)) {
		ret = ina23x_reg_read_24(&config->bus, INA228_REG_BUS_VOLT, &data->bus_voltage);
		if (ret < 0) {
			LOG_ERR("failed to read bus voltage");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_DIE_TEMP)) {
		ret = ina23x_reg_read_16(&config->bus, INA228_REG_DIETEMP, &data->die_temp);
		if (ret < 0) {
			LOG_ERR("failed to read temperature");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_CURRENT)) {
		ret = ina23x_reg_read_24(&config->bus, INA228_REG_CURRENT, &data->current);
		if (ret < 0) {
			LOG_ERR("failed to read current");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_POWER)) {
		ret = ina23x_reg_read_24(&config->bus, INA228_REG_POWER, &data->power);
		if (ret < 0) {
			LOG_ERR("failed to read power");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == (enum sensor_channel)SENSOR_CHAN_INA228_ENERGY)) {
		ret = ina23x_reg_read_40(&config->bus, INA228_REG_ENERGY, &data->energy);
		if (ret < 0) {
			LOG_ERR("failed to read energy accumulator");
			return ret;
		}

		rstacc = true;
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == (enum sensor_channel)SENSOR_CHAN_INA228_CHARGE)) {
		ret = ina23x_reg_read_40(&config->bus, INA228_REG_CHARGE, &data->charge);
		if (ret < 0) {
			LOG_ERR("failed to read charge accumulator");
			return ret;
		}

		rstacc = true;
	}

	if (rstacc) {
		/* Reset accumulators */
		ret = ina23x_reg_write(&config->bus, INA228_REG_CONFIG, (1 << 14));
		if (ret < 0) {
			LOG_ERR("failed to reset accumulators");
			return ret;
		}
	}

	return ret;
}

static int ina228_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct ina228_config *config = dev->config;
	uint16_t data = val->val1;

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		return ina23x_reg_write(&config->bus, INA228_REG_CONFIG, data);
	case SENSOR_ATTR_CALIBRATION:
		return ina23x_reg_write(&config->bus, INA228_REG_SHUNT_CAL, data);
	default:
		LOG_ERR("INA228 attribute not supported.");
		return -ENOTSUP;
	}
}

static int ina228_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	const struct ina228_config *config = dev->config;
	uint16_t data;
	int ret;

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		ret = ina23x_reg_read_16(&config->bus, INA228_REG_CONFIG, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_CALIBRATION:
		ret = ina23x_reg_read_16(&config->bus, INA228_REG_SHUNT_CAL, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		LOG_ERR("INA228 attribute not supported.");
		return -ENOTSUP;
	}

	val->val1 = data;
	val->val2 = 0;

	return 0;
}

static int ina228_init(const struct device *dev)
{
	struct ina228_data *data = dev->data;
	const struct ina228_config *config = dev->config;
	uint16_t id;
	int ret;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C bus %s is not ready", config->bus.bus->name);
		return -ENODEV;
	}

	data->dev = dev;

	ret = ina23x_reg_read_16(&config->bus, INA228_REG_MANUFACTURER_ID, &id);
	if (ret < 0) {
		LOG_ERR("failed to read manufacturer register!");
		return ret;
	}

	if (id != INA228_MANUFACTURER_ID) {
		LOG_ERR("manufacturer ID doesn't match!");
		return -ENODEV;
	}

	ret = ina23x_reg_write(&config->bus, INA228_REG_ADC_CONFIG, config->adc_config);
	if (ret < 0) {
		LOG_ERR("failed to write ADC configuration register!");
		return ret;
	}

	ret = ina23x_reg_write(&config->bus, INA228_REG_SHUNT_CAL, config->cal);
	if (ret < 0) {
		LOG_ERR("failed to write calibration register!");
		return ret;
	}

	return 0;
}

static DEVICE_API(sensor, ina228_driver_api) = {
	.attr_set = ina228_attr_set,
	.attr_get = ina228_attr_get,
	.sample_fetch = ina228_sample_fetch,
	.channel_get = ina228_channel_get,
};

#define INA228_DRIVER_INIT(inst)                                                \
	static struct ina228_data ina228_data_##inst;                               \
	static const struct ina228_config ina228_config_##inst = {                  \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                      \
		.adc_config = (DT_INST_ENUM_IDX(inst, adc_mode) << 12) |                \
			(DT_INST_ENUM_IDX(inst, vbus_conversion_time_us) << 9) |            \
			(DT_INST_ENUM_IDX(inst, vshunt_conversion_time_us) << 6) |          \
			(DT_INST_ENUM_IDX(inst, temp_conversion_time_us) << 3) |            \
			(DT_INST_ENUM_IDX(inst, avg_count)),                                \
		.current_lsb = DT_INST_PROP(inst, current_lsb_microamps),               \
		.cal = INA228_CAL_SCALING *                                             \
			DT_INST_PROP(inst, current_lsb_microamps) *                         \
			DT_INST_PROP(inst, rshunt_micro_ohms) / 10000000ULL,                \
		.alert_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, alert_gpios, {0}),         \
	};                                                                          \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &ina228_init, NULL, &ina228_data_##inst, \
				     &ina228_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &ina228_driver_api);             \

DT_INST_FOREACH_STATUS_OKAY(INA228_DRIVER_INIT)
