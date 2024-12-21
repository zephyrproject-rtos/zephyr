/*
 * Copyright 2021 The Chromium OS Authors
 * Copyright 2021 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ina230.h"
#include "ina23x_common.h"

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(INA230, CONFIG_SENSOR_LOG_LEVEL);

/** @brief Calibration scaling value (value scaled by 100000) */
#define INA230_CAL_SCALING 512ULL

/** @brief The LSB value for the bus voltage register, in microvolts/LSB. */
#define INA230_BUS_VOLTAGE_UV_LSB 1250U
#define INA236_BUS_VOLTAGE_UV_LSB 1600U

/** @brief The scaling for the power register. */
#define INA230_POWER_SCALING 25
#define INA236_POWER_SCALING 32

static int ina230_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ina230_data *data = dev->data;
	const struct ina230_config *const config = dev->config;
	uint32_t bus_uv, power_uw;
	int32_t current_ua;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		bus_uv = data->bus_voltage * config->uv_lsb;

		/* convert to fractional volts (units for voltage channel) */
		val->val1 = bus_uv / 1000000U;
		val->val2 = bus_uv % 1000000U;
		break;

	case SENSOR_CHAN_CURRENT:
		/* see datasheet "Programming" section for reference */
		current_ua = data->current * config->current_lsb;

		/* convert to fractional amperes */
		val->val1 = current_ua / 1000000L;
		val->val2 = current_ua % 1000000L;
		break;

	case SENSOR_CHAN_POWER:
		power_uw = data->power * config->power_scale * config->current_lsb;

		/* convert to fractional watts */
		val->val1 = (int32_t)(power_uw / 1000000U);
		val->val2 = (int32_t)(power_uw % 1000000U);

		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ina230_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ina230_data *data = dev->data;
	const struct ina230_config *config = dev->config;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE && chan != SENSOR_CHAN_CURRENT &&
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

static int ina230_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
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

static int ina230_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
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

static int ina230_calibrate(const struct device *dev)
{
	const struct ina230_config *config = dev->config;
	int ret;

	/* See datasheet "Programming" section */
	ret = ina23x_reg_write(&config->bus, INA230_REG_CALIB, config->cal);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

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

		ret = ina23x_reg_write(&config->bus, INA230_REG_ALERT, config->alert_limit);
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
#endif /* CONFIG_INA230_TRIGGER */

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

#ifdef CONFIG_INA230_TRIGGER
#define INA230_CFG_IRQ(inst)                                                                       \
	.trig_enabled = true, .mask = DT_INST_PROP(inst, mask),                                    \
	.alert_limit = DT_INST_PROP(inst, alert_limit),                                            \
	.alert_gpio = GPIO_DT_SPEC_INST_GET(inst, alert_gpios)
#else
#define INA230_CFG_IRQ(inst)
#endif /* CONFIG_INA230_TRIGGER */

#define INA230_DRIVER_INIT(inst, type)                                                             \
	static struct ina230_data drv_data_##inst;                                                 \
	static const struct ina230_config drv_config_##type##inst = {                              \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.config = (DT_INST_PROP_OR(inst, high_precision, 0) << 12) |                       \
			  DT_INST_PROP(inst, config) | (DT_INST_ENUM_IDX(inst, avg_count) << 9) |  \
			  (DT_INST_ENUM_IDX(inst, vbus_conversion_time_us) << 6) |                 \
			  (DT_INST_ENUM_IDX(inst, vshunt_conversion_time_us) << 3) |               \
			  DT_INST_ENUM_IDX(inst, adc_mode),                                        \
		.current_lsb = DT_INST_PROP(inst, current_lsb_microamps),                          \
		.uv_lsb = type##_BUS_VOLTAGE_UV_LSB,                                               \
		.power_scale = type##_POWER_SCALING,                                               \
		.cal = (uint16_t)(((INA230_CAL_SCALING * 10000000ULL) /                            \
				   ((uint64_t)DT_INST_PROP(inst, current_lsb_microamps) *          \
				    DT_INST_PROP(inst, rshunt_micro_ohms))) >>                     \
				  (DT_INST_PROP_OR(inst, high_precision, 0) << 1)),                \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, alert_gpios), (INA230_CFG_IRQ(inst)),      \
			    ())};   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &ina230_init, NULL, &drv_data_##inst,                   \
				     &drv_config_##type##inst, POST_KERNEL,                        \
				     CONFIG_SENSOR_INIT_PRIORITY, &ina230_driver_api);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_ina230
DT_INST_FOREACH_STATUS_OKAY_VARGS(INA230_DRIVER_INIT, INA230)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_ina236
DT_INST_FOREACH_STATUS_OKAY_VARGS(INA230_DRIVER_INIT, INA236)
