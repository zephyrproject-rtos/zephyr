/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ina23x

#include <sys/byteorder.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <logging/log.h>
#include <drivers/sensor.h>

#include "ina23x.h"

LOG_MODULE_REGISTER(INA23X, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Macro used to test if the current's sign bit is set
 */
#define CURRENT_SIGN_BIT        0x8000

/**
 * @brief Macro used to check if the current's LSB is 1mA
 */
#define CURRENT_LSB_1MA         1

/**
 * @brief Macro for creating the INA23X calibration value
 *        CALIB = (5120 / (current_lsb * rshunt))
 *        NOTE: The 5120 value is a fixed value internal to the
 *              INA23X that is used to ensure scaling is properly
 *              maintained.
 *
 * @param current_lsb Value of the Current register LSB in milliamps
 * @param rshunt Shunt resistor value in milliohms
 */
 #define INA23X_CALIB(current_lsb, rshunt) (5120 / ((current_lsb) * (rshunt)))

/**
 * @brief Macro to convert raw Bus voltage to millivolts when current_lsb is
 *        set to 1mA.
 *
 * reg value read from bus voltage register
 * clsb value of current_lsb
 */
#define INA23X_BUS_MV(reg) ((reg) * 125 / 100)

/**
 * @brief Macro to convert raw power value to milliwatts when current_lsb is
 *        set to 1mA.
 *
 * reg value read from power register
 * clsb value of current_lsb
 */
#define INA23X_POW_MW(reg) ((reg) * 25)

static int ina23x_reg_read(const struct device *dev, uint8_t reg, int16_t *val)
{
	const struct ina23x_config *const config = dev->config;
	uint8_t data[2];
	int ret;

	ret = i2c_burst_read(config->bus, config->i2c_slv_addr, reg, data, 2);
	if (ret < 0) {
		return ret;
	}

	*val = sys_get_be16(data);

	return ret;
}

static int ina23x_reg_write(const struct device *dev, uint8_t reg, uint16_t val)
{
	const struct ina23x_config *const config = dev->config;
	uint8_t tx_buf[3];

	tx_buf[0] = reg;
	sys_put_be16(val, &tx_buf[1]);

	return i2c_write(config->bus, tx_buf, sizeof(tx_buf), config->i2c_slv_addr);
}

/**
 * @brief sensor value get
 *
 * @return 0 for success
 * @return -ENOTSUP for unsupported channels
 */
static int ina23x_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ina23x_data *ina23x = dev->data;
	const struct ina23x_config *const config = dev->config;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		if (config->current_lsb == CURRENT_LSB_1MA) {
			val->val1 = INA23X_BUS_MV(ina23x->bus_voltage) / 1000U;
			val->val2 = (INA23X_BUS_MV(ina23x->bus_voltage) % 1000) * 1000;
		} else {
			val->val1 = ina23x->bus_voltage;
			val->val2 = 0;
		}
		break;

	case SENSOR_CHAN_CURRENT:
		if (config->current_lsb == CURRENT_LSB_1MA) {
			/**
			 * If current is negative, convert it to a
			 * magnitude and return the negative of that
			 * magnitude.
			 */
			if (ina23x->current & CURRENT_SIGN_BIT) {
				uint16_t current_mag = (~ina23x->current + 1);

				val->val1 = -(current_mag / 1000U);
				val->val2 = -(current_mag % 1000) * 1000;
			} else {
				val->val1 = ina23x->current / 1000U;
				val->val2 = (ina23x->current % 1000) * 1000;
			}
		} else {
			val->val1 = ina23x->current;
			val->val2 = 0;
		}
		break;

	case SENSOR_CHAN_POWER:
		if (config->current_lsb == CURRENT_LSB_1MA) {
			val->val1 = INA23X_POW_MW(ina23x->power) / 1000U;
			val->val2 = (INA23X_POW_MW(ina23x->power) % 1000) * 1000;
		} else {
			val->val1 = ina23x->power;
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
 * @return 0 for success
 * @return -ENOTSUP for unsupported channels
 */
static int ina23x_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct ina23x_data *ina23x = dev->data;
	int ret;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		ret = ina23x_reg_read(dev, INA23X_REG_BUS_VOLT, &ina23x->bus_voltage);
		if (ret < 0) {
			LOG_ERR("Failed to read bus voltage");
			return ret;
		}
		break;

	case SENSOR_CHAN_CURRENT:
		ret = ina23x_reg_read(dev, INA23X_REG_CURRENT, &ina23x->current);
		if (ret < 0) {
			LOG_ERR("Failed to read current");
			return ret;
		}
		break;

	case SENSOR_CHAN_POWER:
		ret = ina23x_reg_read(dev, INA23X_REG_POWER, &ina23x->power);
		if (ret < 0) {
			LOG_ERR("Failed to read power");
			return ret;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief sensor attribute set
 *
 * @return 0 for success
 * @return -ENOTSUP for unsupported channels
 * @return -EIO for i2c write failure
 */
static int ina23x_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	uint16_t data = val->val1;

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		return ina23x_reg_write(dev, INA23X_REG_CONFIG, data);
	case SENSOR_ATTR_CALIBRATION:
		return ina23x_reg_write(dev, INA23X_REG_CALIB, data);
	case SENSOR_ATTR_FEATURE_MASK:
		return ina23x_reg_write(dev, INA23X_REG_MASK, data);
	case SENSOR_ATTR_ALERT:
		return ina23x_reg_write(dev, INA23X_REG_ALERT, data);
	default:
		LOG_ERR("INA23X attribute not supported.");
		return -ENOTSUP;
	}
}

/**
 * @brief sensor attribute get
 *
 * @return 0 for success
 * @return -ENOTSUP for unsupported channels
 * @return -EIO for i2c read failure
 */
static int ina23x_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   struct sensor_value *val)
{
	uint16_t data;
	int ret;

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		ret = ina23x_reg_read(dev, INA23X_REG_CONFIG, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_CALIBRATION:
		ret = ina23x_reg_read(dev, INA23X_REG_CALIB, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_FEATURE_MASK:
		ret = ina23x_reg_read(dev, INA23X_REG_MASK, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_ALERT:
		ret = ina23x_reg_read(dev, INA23X_REG_ALERT, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		LOG_ERR("INA23X attribute not supported.");
		return -ENOTSUP;
	}

	val->val1 = data;
	val->val2 = 0;

	return 0;
}

/**
 * @brief Initialize the INA23x
 *
 * @return 0 for success
 * @return -EINVAL on error
 */
static int ina23x_init(const struct device *dev)
{
	const struct ina23x_config *const config = dev->config;
	uint16_t cal;
	int ret;

	if (!device_is_ready(config->bus)) {
		LOG_ERR("Device %s is not ready", config->bus->name);
		return -ENODEV;
	}

	ret = ina23x_reg_write(dev, INA23X_REG_CONFIG, config->config);
	if (ret < 0) {
		LOG_ERR("Failed to write configuration register!");
		return ret;
	}

	cal = INA23X_CALIB(config->current_lsb, config->rshunt);

	ret = ina23x_reg_write(dev, INA23X_REG_CALIB, cal);
	if (ret < 0) {
		LOG_ERR("Failed to write calibration register!");
		return ret;
	}

#ifdef CONFIG_INA23X_TRIGGER
	if (config->trig_enabled) {
		ret = ina23x_trigger_mode_init(dev);
		if (ret < 0) {
			LOG_ERR("Failed to init trigger mode\n");
			return ret;
		}

		ret = ina23x_reg_write(dev, INA23X_REG_ALERT,
				       config->alert_limit);
		if (ret < 0) {
			LOG_ERR("Failed to write alert register!");
			return ret;
		}

		ret = ina23x_reg_write(dev, INA23X_REG_MASK, config->mask);
		if (ret < 0) {
			LOG_ERR("Failed to write mask register!");
			return ret;
		}
	}
#endif  /* CONFIG_INA23X_TRIGGER */

	return 0;
}

static const struct sensor_driver_api ina23x_driver_api = {
	.attr_set = ina23x_attr_set,
	.attr_get = ina23x_attr_get,
#ifdef CONFIG_INA23X_TRIGGER
	.trigger_set = ina23x_trigger_set,
#endif
	.sample_fetch = ina23x_sample_fetch,
	.channel_get = ina23x_channel_get,
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 0,
	     "No compatible ina23x instances found");

#ifdef CONFIG_INA23X_TRIGGER
#define INA23X_CFG_IRQ(inst)				\
	.trig_enabled = true,				\
	.mask = DT_INST_PROP(inst, mask),		\
	.alert_limit = DT_INST_PROP(inst, alert_limit),	\
	.gpio_alert = GPIO_DT_SPEC_INST_GET(inst, irq_gpios)
#else
#define INA23X_CFG_IRQ(inst)
#endif /* CONFIG_INA23X_TRIGGER */

#define INA23X_DRIVER_INIT(inst)				    \
	static struct ina23x_data drv_data_##inst;		    \
	static const struct ina23x_config drv_config_##inst = {	    \
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),	    \
		.i2c_slv_addr = DT_INST_REG_ADDR(inst),		    \
		.config = DT_INST_PROP(inst, config),		    \
		.current_lsb = DT_INST_PROP(inst, current_lsb),	    \
		.rshunt = DT_INST_PROP(inst, rshunt),		    \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, irq_gpios), \
			    (INA23X_CFG_IRQ(inst)), ())		    \
	};							    \
	DEVICE_DT_INST_DEFINE(inst,				    \
			      &ina23x_init,			    \
			      NULL,				    \
			      &drv_data_##inst,			    \
			      &drv_config_##inst,		    \
			      POST_KERNEL,			    \
			      CONFIG_SENSOR_INIT_PRIORITY,	    \
			      &ina23x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INA23X_DRIVER_INIT)
