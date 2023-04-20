/*
 * Copyright 2021 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ina237

#include "ina237.h"
#include "ina23x_common.h"

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/ina237.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(INA237, CONFIG_SENSOR_LOG_LEVEL);

/** @brief Calibration scaling value (scaled by 10^-5) */
#define INA237_CAL_SCALING 8192U

/** @brief The LSB value for the bus voltage register, in microvolts/LSB. */
#define INA237_BUS_VOLTAGE_UV_LSB 3125

/** @brief Power scaling (scaled by 10) */
#define INA237_POWER_SCALING 2

static int ina237_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ina237_data *data = dev->data;
	const struct ina237_config *config = dev->config;
	uint32_t bus_uv, current_ua, power_uw;
	int32_t sign;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		bus_uv = data->bus_voltage * INA237_BUS_VOLTAGE_UV_LSB;

		val->val1 = bus_uv / 1000000U;
		val->val2 = bus_uv % 1000000U;
		break;

	case SENSOR_CHAN_CURRENT:
		if (data->current & INA23X_CURRENT_SIGN_BIT) {
			current_ua = ~data->current + 1U;
			sign = -1;
		} else {
			current_ua = data->current;
			sign = 1;
		}

		/* see datasheet "Current and Power calculations" section */
		current_ua = current_ua * config->current_lsb;

		/* convert to fractional amperes */
		val->val1 = sign * (int32_t)(current_ua / 1000000U);
		val->val2 = sign * (int32_t)(current_ua % 1000000U);
		break;

	case SENSOR_CHAN_POWER:
		/* see datasheet "Current and Power calculations" section */
		power_uw = (data->power * INA237_POWER_SCALING *
			    config->current_lsb) / 10000U;

		/* convert to fractional watts */
		val->val1 = (int32_t)(power_uw / 1000000U);
		val->val2 = (int32_t)(power_uw % 1000000U);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief sensor operation mode check
 *
 * @retval true if set or false if not set
 */
static bool ina237_is_triggered_mode_set(const struct device *dev)
{
	const struct ina237_config *config = dev->config;

	uint8_t mode = (config->adc_config & GENMASK(15, 12)) >> 12;

	switch (mode) {
	case INA237_OPER_MODE_BUS_VOLTAGE_TRIG:
	case INA237_OPER_MODE_SHUNT_VOLTAGE_TRIG:
	case INA237_OPER_MODE_SHUNT_BUS_VOLTAGE_TRIG:
	case INA237_OPER_MODE_TEMP_TRIG:
	case INA237_OPER_MODE_TEMP_BUS_VOLTAGE_TRIG:
	case INA237_OPER_MODE_TEMP_SHUNT_VOLTAGE_TRIG:
	case INA237_OPER_MODE_BUS_SHUNT_VOLTAGE_TEMP_TRIG:
		return true;
	default:
		return false;
	}
}

/**
 * @brief request for one shot measurement
 *
 * @retval 0 for success
 * @retval negative errno code on fail
 */
static int ina237_trigg_one_shot_request(const struct device *dev)
{
	const struct ina237_config *config = dev->config;
	int ret;

	ret = ina23x_reg_write(&config->bus, INA237_REG_ADC_CONFIG, config->adc_config);
	if (ret < 0) {
		LOG_ERR("Failed to write ADC configuration register!");
		return ret;
	}

	return 0;
}

/**
 * @brief sensor data read
 *
 * @retval 0 for success
 * @retval -EIO in case of input / output error
 */
static int ina237_read_data(const struct device *dev)
{
	struct ina237_data *data = dev->data;
	const struct ina237_config *config = dev->config;
	int ret;

	if ((data->chan == SENSOR_CHAN_ALL) || (data->chan == SENSOR_CHAN_VOLTAGE)) {
		ret = ina23x_reg_read_16(&config->bus, INA237_REG_BUS_VOLT, &data->bus_voltage);
		if (ret < 0) {
			LOG_ERR("Failed to read bus voltage");
			return ret;
		}
	}

	if ((data->chan == SENSOR_CHAN_ALL) || (data->chan == SENSOR_CHAN_CURRENT)) {
		ret = ina23x_reg_read_16(&config->bus, INA237_REG_CURRENT, &data->current);
		if (ret < 0) {
			LOG_ERR("Failed to read current");
			return ret;
		}
	}

	if ((data->chan == SENSOR_CHAN_ALL) || (data->chan == SENSOR_CHAN_POWER)) {
		ret = ina23x_reg_read_24(&config->bus, INA237_REG_POWER, &data->power);
		if (ret < 0) {
			LOG_ERR("Failed to read power");
			return ret;
		}
	}

	return 0;
}

/**
 * @brief sensor sample fetch
 *
 * @retval 0 for success
 * @retval negative errno code on fail
 */
static int ina237_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct ina237_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL &&
	    chan != SENSOR_CHAN_VOLTAGE &&
	    chan != SENSOR_CHAN_CURRENT &&
	    chan != SENSOR_CHAN_POWER) {
		return -ENOTSUP;
	}

	data->chan = chan;

	if (ina237_is_triggered_mode_set(dev)) {
		return ina237_trigg_one_shot_request(dev);
	} else {
		return ina237_read_data(dev);
	}
}

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

static int ina237_calibrate(const struct device *dev)
{
	const struct ina237_config *config = dev->config;
	uint16_t val;
	int ret;

	/* see datasheet "Current and Power calculations" section */
	val = (INA237_CAL_SCALING * config->current_lsb * config->rshunt) /
	       10000000U;

	ret = ina23x_reg_write(&config->bus, INA237_REG_CALIB, val);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static void ina237_trigger_work_handler(struct k_work *work)
{
	struct ina23x_trigger *trigg = CONTAINER_OF(work, struct ina23x_trigger, conversion_work);
	struct ina237_data *data = CONTAINER_OF(trigg, struct ina237_data, trigger);
	const struct ina237_config *config = data->dev->config;
	int ret;
	uint16_t reg_alert;

	/* Read reg alert to clear alerts */
	ret = ina23x_reg_read_16(&config->bus, INA237_REG_ALERT, &reg_alert);
	if (ret < 0) {
		LOG_ERR("Failed to read alert register!");
		return;
	}

	ret = ina237_read_data(data->dev);
	if (ret < 0) {
		LOG_WRN("Unable to read data, ret %d", ret);
	}

	if (data->trigger.handler_alert) {
		data->trigger.handler_alert(data->dev, data->trigger.trig_alert);
	}
}

static int ina237_init(const struct device *dev)
{
	struct ina237_data *data = dev->data;
	const struct ina237_config *config = dev->config;
	uint16_t id;
	int ret;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C bus %s is not ready", config->bus.bus->name);
		return -ENODEV;
	}

	data->dev = dev;

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

	if (ina237_is_triggered_mode_set(dev)) {
		if ((config->alert_config & GENMASK(15, 14)) != GENMASK(15, 14)) {
			LOG_ERR("ALATCH and CNVR bits must be enabled in triggered mode!");
			return -ENODEV;
		}

		k_work_init(&data->trigger.conversion_work, ina237_trigger_work_handler);

		ret = ina23x_trigger_mode_init(&data->trigger, &config->alert_gpio);
		if (ret < 0) {
			LOG_ERR("Failed to init trigger mode");
			return ret;
		}

		ret = ina23x_reg_write(&config->bus, INA237_REG_ALERT, config->alert_config);
		if (ret < 0) {
			LOG_ERR("Failed to write alert configuration register!");
			return ret;
		}
	}

	return 0;
}

static int ina237_trigger_set(const struct device *dev,
			      const struct sensor_trigger *trig,
			      sensor_trigger_handler_t handler)
{
	ARG_UNUSED(trig);
	struct ina237_data *ina237 = dev->data;

	if (!ina237_is_triggered_mode_set(dev)) {
		return -ENOTSUP;
	}

	ina237->trigger.handler_alert = handler;
	ina237->trigger.trig_alert = trig;

	return 0;
}

static const struct sensor_driver_api ina237_driver_api = {
	.attr_set = ina237_attr_set,
	.attr_get = ina237_attr_get,
	.trigger_set = ina237_trigger_set,
	.sample_fetch = ina237_sample_fetch,
	.channel_get = ina237_channel_get,
};

#define INA237_DRIVER_INIT(inst)						\
	static struct ina237_data ina237_data_##inst;				\
	static const struct ina237_config ina237_config_##inst = {		\
		.bus = I2C_DT_SPEC_INST_GET(inst),				\
		.config = DT_INST_PROP(inst, config),				\
		.adc_config = DT_INST_PROP(inst, adc_config),			\
		.current_lsb = DT_INST_PROP(inst, current_lsb_microamps),	\
		.rshunt = DT_INST_PROP(inst, rshunt_milliohms),			\
		.alert_config = DT_INST_PROP_OR(inst, alert_config, 0x01),	\
		.alert_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, alert_gpios, {0}),	\
	};							    \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,			    \
			      &ina237_init,			    \
			      NULL,				    \
			      &ina237_data_##inst,		    \
			      &ina237_config_##inst,		    \
			      POST_KERNEL,			    \
			      CONFIG_SENSOR_INIT_PRIORITY,	    \
			      &ina237_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INA237_DRIVER_INIT)
