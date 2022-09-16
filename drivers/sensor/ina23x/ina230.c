/*
 * Copyright 2021 The Chromium OS Authors
 * Copyright 2021 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ina230

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include "ina230.h"
#include "ina23x_common.h"

LOG_MODULE_REGISTER(INA230, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Internal fixed value of INA230 that is used to ensure
 *	  scaling is properly maintained.
 *
 */
#define INA230_INTERNAL_FIXED_SCALING_VALUE 5120

/**
 * @brief The LSB value for the bus voltage register.
 *
 */
#define INA230_BUS_VOLTAGE_LSB 125

/**
 * @brief The LSB value for the power register.
 *
 */
#define INA230_POWER_VALUE_LSB 25

/**
 * @brief sensor value get
 *
 * @retval 0 for success
 * @retval -ENOTSUP for unsupported channels
 */
static int ina230_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ina230_data *data = dev->data;
	const struct ina230_config *const config = dev->config;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		if (config->current_lsb == INA23X_CURRENT_LSB_1MA) {
			uint32_t bus_mv = ((data->bus_voltage *
					 INA230_BUS_VOLTAGE_LSB) / 100);

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

				val->val1 = -(current_mag / 1000U);
				val->val2 = -(current_mag % 1000) * 1000;
			} else {
				val->val1 = data->current / 1000U;
				val->val2 = (data->current % 1000) * 1000;
			}
		} else {
			val->val1 = data->current;
			val->val2 = 0;
		}
		break;

	case SENSOR_CHAN_POWER:
		if (config->current_lsb == INA23X_CURRENT_LSB_1MA) {
			uint32_t power_mw = data->power * INA230_POWER_VALUE_LSB;

			val->val1 = power_mw / 1000U;
			val->val2 = (power_mw % 1000) * 1000;
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
static int ina230_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct ina230_data *data = dev->data;
	const struct ina230_config *config = dev->config;
	int ret;

	if (chan != SENSOR_CHAN_ALL &&
	    chan != SENSOR_CHAN_VOLTAGE &&
	    chan != SENSOR_CHAN_CURRENT &&
	    chan != SENSOR_CHAN_POWER) {
		return -ENOTSUP;
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_VOLTAGE)) {
		ret = ina23x_reg_read_16(&config->bus, INA230_REG_BUS_VOLT, &data->bus_voltage);
		if (ret < 0) {
			LOG_ERR("Failed to read bus voltage");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_CURRENT)) {
		ret = ina23x_reg_read_16(&config->bus, INA230_REG_CURRENT, &data->current);
		if (ret < 0) {
			LOG_ERR("Failed to read current");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_POWER)) {
		ret = ina23x_reg_read_16(&config->bus, INA230_REG_POWER, &data->power);
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
static int ina230_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	const struct ina230_config *config = dev->config;
	uint16_t data = val->val1;

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		return ina23x_reg_write(&config->bus, INA230_REG_CONFIG, data);
	case SENSOR_ATTR_CALIBRATION:
		return ina23x_reg_write(&config->bus, INA230_REG_CALIB, data);
	case SENSOR_ATTR_FEATURE_MASK:
		return ina23x_reg_write(&config->bus, INA230_REG_MASK, data);
	case SENSOR_ATTR_ALERT:
		return ina23x_reg_write(&config->bus, INA230_REG_ALERT, data);
	default:
		LOG_ERR("INA230 attribute not supported.");
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
static int ina230_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   struct sensor_value *val)
{
	const struct ina230_config *config = dev->config;
	uint16_t data;
	int ret;

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		ret = ina23x_reg_read_16(&config->bus, INA230_REG_CONFIG, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_CALIBRATION:
		ret = ina23x_reg_read_16(&config->bus, INA230_REG_CALIB, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_FEATURE_MASK:
		ret = ina23x_reg_read_16(&config->bus, INA230_REG_MASK, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_ALERT:
		ret = ina23x_reg_read_16(&config->bus, INA230_REG_ALERT, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		LOG_ERR("INA230 attribute not supported.");
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
static int ina230_calibrate(const struct device *dev)
{
	const struct ina230_config *config = dev->config;
	uint16_t val;
	int ret;

	val = (INA230_INTERNAL_FIXED_SCALING_VALUE / (config->current_lsb * config->rshunt));

	ret = ina23x_reg_write(&config->bus, INA230_REG_CALIB, val);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/**
 * @brief Initialize the INA230
 *
 * @retval 0 for success
 * @retval -EINVAL on error
 */
static int ina230_init(const struct device *dev)
{
	const struct ina230_config *const config = dev->config;
	int ret;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C bus %s is not ready", config->bus.bus->name);
		return -ENODEV;
	}

	ret = ina23x_reg_write(&config->bus, INA230_REG_CONFIG, config->config);
	if (ret < 0) {
		LOG_ERR("Failed to write configuration register!");
		return ret;
	}

	ret = ina230_calibrate(dev);
	if (ret < 0) {
		LOG_ERR("Failed to write calibration register!");
		return ret;
	}

#ifdef CONFIG_INA230_TRIGGER
	if (config->trig_enabled) {
		ret = ina230_trigger_mode_init(dev);
		if (ret < 0) {
			LOG_ERR("Failed to init trigger mode\n");
			return ret;
		}

		ret = ina23x_reg_write(&config->bus, INA230_REG_ALERT,
				       config->alert_limit);
		if (ret < 0) {
			LOG_ERR("Failed to write alert register!");
			return ret;
		}

		ret = ina23x_reg_write(&config->bus, INA230_REG_MASK, config->mask);
		if (ret < 0) {
			LOG_ERR("Failed to write mask register!");
			return ret;
		}
	}
#endif  /* CONFIG_INA230_TRIGGER */

	return 0;
}

static const struct sensor_driver_api ina230_driver_api = {
	.attr_set = ina230_attr_set,
	.attr_get = ina230_attr_get,
#ifdef CONFIG_INA230_TRIGGER
	.trigger_set = ina230_trigger_set,
#endif
	.sample_fetch = ina230_sample_fetch,
	.channel_get = ina230_channel_get,
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 0,
	     "No compatible ina230 instances found");

#ifdef CONFIG_INA230_TRIGGER
#define INA230_CFG_IRQ(inst)				\
	.trig_enabled = true,				\
	.mask = DT_INST_PROP(inst, mask),		\
	.alert_limit = DT_INST_PROP(inst, alert_limit),	\
	.gpio_alert = GPIO_DT_SPEC_INST_GET(inst, irq_gpios)
#else
#define INA230_CFG_IRQ(inst)
#endif /* CONFIG_INA230_TRIGGER */

#define INA230_DRIVER_INIT(inst)				    \
	static struct ina230_data drv_data_##inst;		    \
	static const struct ina230_config drv_config_##inst = {	    \
		.bus = I2C_DT_SPEC_INST_GET(inst),		    \
		.config = DT_INST_PROP(inst, config),		    \
		.current_lsb = DT_INST_PROP(inst, current_lsb),	    \
		.rshunt = DT_INST_PROP(inst, rshunt),		    \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, irq_gpios), \
			    (INA230_CFG_IRQ(inst)), ())		    \
	};							    \
	DEVICE_DT_INST_DEFINE(inst,				    \
			      &ina230_init,			    \
			      NULL,				    \
			      &drv_data_##inst,			    \
			      &drv_config_##inst,		    \
			      POST_KERNEL,			    \
			      CONFIG_SENSOR_INIT_PRIORITY,	    \
			      &ina230_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INA230_DRIVER_INIT)
