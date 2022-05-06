/*
 * Copyright 2021 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ina237

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include "ina237.h"
#include "ina23x_common.h"

LOG_MODULE_REGISTER(INA237, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Internal fixed value of INA237 that is used to ensure
 *	  scaling is properly maintained.
 *
 */
#define INA237_INTERNAL_FIXED_SCALING_VALUE 8192

/**
 * @brief The LSB value for the bus voltage register.
 *
 */
#define INA237_BUS_VOLTAGE_LSB 3125

/**
 * @brief The LSB value for the power register.
 *
 */
#define INA237_POWER_VALUE_LSB 2

/**
 * @brief sensor value get
 *
 * @retval 0 for success
 * @retval -ENOTSUP for unsupported channels
 */
static int ina237_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ina237_data *data = dev->data;
	const struct ina237_config *config = dev->config;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		if (config->current_lsb == INA23X_CURRENT_LSB_1MA) {
			uint32_t bus_mv = ((data->bus_voltage *
					 INA237_BUS_VOLTAGE_LSB) / 1000);

			val->val1 = bus_mv / 1000U;
			val->val2 = (bus_mv % 1000) * 1000;
		} else {
			val->val1 = data->bus_voltage;
			val->val2 = 0;
		}
		break;

	case SENSOR_CHAN_CURRENT:
		if (config->current_lsb == INA23X_CURRENT_LSB_1MA) {
			/**
			 * If current is negative, convert it to a
			 * magnitude and return the negative of that
			 * magnitude.
			 */
			if (data->current & INA23X_CURRENT_SIGN_BIT) {
				uint16_t current_mag = (~data->current + 1);

				val->val1 = -(current_mag / 10000U);
				val->val2 = -(current_mag % 10000) * 100;

			} else {
				val->val1 = data->current / 10000U;
				val->val2 = (data->current % 10000) * 100;
			}
		} else {
			val->val1 = data->current;
			val->val2 = 0;
		}
		break;

	case SENSOR_CHAN_POWER:
		if (config->current_lsb == INA23X_CURRENT_LSB_1MA) {
			uint32_t power_mw = ((data->power *
					 config->current_lsb *
					 INA237_POWER_VALUE_LSB) / 10);

			val->val1 = power_mw / 10000U;
			val->val2 = (power_mw % 10000) * 100;
		} else {
			val->val1 = data->power;
			val->val2 = 0;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief sensor sample fetch
 *
 * @retval 0 for success
 * @retval -ENOTSUP for unsupported channels
 */
static int ina237_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct ina237_data *data = dev->data;
	const struct ina237_config *config = dev->config;
	int ret;

	if (chan != SENSOR_CHAN_ALL &&
	    chan != SENSOR_CHAN_VOLTAGE &&
	    chan != SENSOR_CHAN_CURRENT &&
	    chan != SENSOR_CHAN_POWER) {
		return -ENOTSUP;
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_VOLTAGE)) {
		ret = ina23x_reg_read_16(&config->bus, INA237_REG_BUS_VOLT, &data->bus_voltage);
		if (ret < 0) {
			LOG_ERR("Failed to read bus voltage");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_CURRENT)) {
		ret = ina23x_reg_read_16(&config->bus, INA237_REG_CURRENT, &data->current);
		if (ret < 0) {
			LOG_ERR("Failed to read current");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_POWER)) {
		ret = ina23x_reg_read_24(&config->bus, INA237_REG_POWER, &data->power);
		if (ret < 0) {
			LOG_ERR("Failed to read power");
			return ret;
		}
	}

	return 0;
}

/**
 * @brief sensor attribute set
 *
 * @retval 0 for success
 * @retval -ENOTSUP for unsupported channels
 * @retval -EIO for i2c write failure
 */
static int ina237_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	const struct ina237_config *config = dev->config;
	uint16_t data = val->val1;

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		return ina23x_reg_write(&config->bus, INA237_REG_CONFIG, data);
	case SENSOR_ATTR_CALIBRATION:
		return ina23x_reg_write(&config->bus, INA237_REG_CALIB, data);
	default:
		LOG_ERR("INA237 attribute not supported.");
		return -ENOTSUP;
	}
}

/**
 * @brief sensor attribute get
 *
 * @retval 0 for success
 * @retval -ENOTSUP for unsupported channels
 * @retval -EIO for i2c read failure
 */
static int ina237_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   struct sensor_value *val)
{
	const struct ina237_config *config = dev->config;
	uint16_t data;
	int ret;

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		ret = ina23x_reg_read_16(&config->bus, INA237_REG_CONFIG, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_CALIBRATION:
		ret = ina23x_reg_read_16(&config->bus, INA237_REG_CALIB, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		LOG_ERR("INA237 attribute not supported.");
		return -ENOTSUP;
	}

	val->val1 = data;
	val->val2 = 0;

	return 0;
}

/**
 * @brief sensor calibrate
 *
 * @retval 0 for success
 * @retval -EIO for i2c write failure
 */
static int ina237_calibrate(const struct device *dev)
{
	const struct ina237_config *config = dev->config;
	uint16_t val;
	int ret;

	val = ((INA237_INTERNAL_FIXED_SCALING_VALUE * config->current_lsb * config->rshunt) / 100);

	ret = ina23x_reg_write(&config->bus, INA237_REG_CALIB, val);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/**
 * @brief Initialize the INA237
 *
 * @retval 0 for success
 * @retval -EINVAL on error
 */
static int ina237_init(const struct device *dev)
{
	const struct ina237_config *config = dev->config;
	uint16_t id;
	int ret;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C bus %s is not ready", config->bus.bus->name);
		return -ENODEV;
	}

	ret = ina23x_reg_read_16(&config->bus, INA237_REG_MANUFACTURER_ID, &id);
	if (ret < 0) {
		LOG_ERR("Failed to read manufacturer register!");
		return ret;
	}

	if (id != INA237_MANUFACTURER_ID) {
		LOG_ERR("Manufacturer ID doesn't match!");
		return -ENODEV;
	}

	ret = ina23x_reg_write(&config->bus, INA237_REG_ADC_CONFIG, config->adc_config);
	if (ret < 0) {
		LOG_ERR("Failed to write ADC configuration register!");
		return ret;
	}

	ret = ina23x_reg_write(&config->bus, INA237_REG_CONFIG, config->config);
	if (ret < 0) {
		LOG_ERR("Failed to write configuration register!");
		return ret;
	}

	ret = ina237_calibrate(dev);
	if (ret < 0) {
		LOG_ERR("Failed to write calibration register!");
		return ret;
	}

	return 0;
}

static const struct sensor_driver_api ina237_driver_api = {
	.attr_set = ina237_attr_set,
	.attr_get = ina237_attr_get,
	.sample_fetch = ina237_sample_fetch,
	.channel_get = ina237_channel_get,
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 0,
	     "No compatible ina237 instances found");

#define INA237_DRIVER_INIT(inst)				    \
	static struct ina237_data ina237_data_##inst;		    \
	static const struct ina237_config ina237_config_##inst = {  \
		.bus = I2C_DT_SPEC_INST_GET(inst),		    \
		.config = DT_INST_PROP(inst, config),		    \
		.adc_config = DT_INST_PROP(inst, adc_config),	    \
		.current_lsb = DT_INST_PROP(inst, current_lsb),	    \
		.rshunt = DT_INST_PROP(inst, rshunt),		    \
	};							    \
	DEVICE_DT_INST_DEFINE(inst,				    \
			      &ina237_init,			    \
			      NULL,				    \
			      &ina237_data_##inst,		    \
			      &ina237_config_##inst,		    \
			      POST_KERNEL,			    \
			      CONFIG_SENSOR_INIT_PRIORITY,	    \
			      &ina237_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INA237_DRIVER_INIT)
