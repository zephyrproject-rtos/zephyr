/*
 * Copyright 2021 The Chromium OS Authors
 * Copyright 2021 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ina230.h"

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

static int ina230_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct ina230_config *config = dev->config;
	const struct ina2xx_config *common = &config->common;
	uint16_t data = val->val1;

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		return ina2xx_reg_write(&common->bus, INA230_REG_CONFIG, data);
	case SENSOR_ATTR_CALIBRATION:
		return ina2xx_reg_write(&common->bus, INA230_REG_CALIB, data);
	case SENSOR_ATTR_FEATURE_MASK:
		return ina2xx_reg_write(&common->bus, INA230_REG_MASK, data);
	case SENSOR_ATTR_ALERT:
		return ina2xx_reg_write(&common->bus, INA230_REG_ALERT, data);
	default:
		LOG_ERR("INA230 attribute not supported.");
		return -ENOTSUP;
	}
}

static int ina230_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	const struct ina230_config *config = dev->config;
	const struct ina2xx_config *common = &config->common;
	uint16_t data;
	int ret;

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		ret = ina2xx_reg_read_16(&common->bus, INA230_REG_CONFIG, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_CALIBRATION:
		ret = ina2xx_reg_read_16(&common->bus, INA230_REG_CALIB, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_FEATURE_MASK:
		ret = ina2xx_reg_read_16(&common->bus, INA230_REG_MASK, &data);
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_ALERT:
		ret = ina2xx_reg_read_16(&common->bus, INA230_REG_ALERT, &data);
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

static int ina230_init(const struct device *dev)
{
	int ret;

	ret = ina2xx_init(dev);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_INA230_TRIGGER
	if (config->trig_enabled) {
		const struct ina230_config *const config = dev->config;
		const struct ina2xx_config *const common = &config->common;

		ret = ina230_trigger_mode_init(dev);
		if (ret < 0) {
			LOG_ERR("Failed to init trigger mode\n");
			return ret;
		}

		ret = ina2xx_reg_write(&common->bus, INA230_REG_ALERT, config->alert_limit);
		if (ret < 0) {
			LOG_ERR("Failed to write alert register!");
			return ret;
		}

		ret = ina2xx_reg_write(&common->bus, INA230_REG_MASK, config->mask);
		if (ret < 0) {
			LOG_ERR("Failed to write mask register!");
			return ret;
		}
	}
#endif /* CONFIG_INA230_TRIGGER */

	return 0;
}

static DEVICE_API(sensor, ina230_driver_api) = {
	.attr_set = ina230_attr_set,
	.attr_get = ina230_attr_get,
#ifdef CONFIG_INA230_TRIGGER
	.trigger_set = ina230_trigger_set,
#endif
	.sample_fetch = ina2xx_sample_fetch,
	.channel_get = ina2xx_channel_get,
};

#ifdef CONFIG_INA230_TRIGGER
#define INA230_CFG_IRQ(inst)                                                                       \
	.trig_enabled = true, .mask = DT_INST_PROP(inst, mask),                                    \
	.alert_limit = DT_INST_PROP(inst, alert_limit),                                            \
	.alert_gpio = GPIO_DT_SPEC_INST_GET(inst, alert_gpios)
#else
#define INA230_CFG_IRQ(inst)
#endif /* CONFIG_INA230_TRIGGER */

#define INA230_DT_CONFIG(inst)                                 \
	(DT_INST_PROP_OR(inst, high_precision, 0) << 12) |         \
	(DT_INST_ENUM_IDX(inst, avg_count) << 9) |                 \
	(DT_INST_ENUM_IDX(inst, vbus_conversion_time_us) << 6) |   \
	(DT_INST_ENUM_IDX(inst, vshunt_conversion_time_us) << 3) | \
	(DT_INST_ENUM_IDX(inst, adc_mode))

#define INA230_DT_CAL(inst)                                \
	(uint16_t)(((INA230_CAL_SCALING * 10000000ULL) /       \
	((uint64_t)DT_INST_PROP(inst, current_lsb_microamps) * \
	DT_INST_PROP(inst, rshunt_micro_ohms))) >>             \
	(DT_INST_PROP_OR(inst, high_precision, 0) << 1))

/**
 * @brief INA230/INA236 sensor instance initialization macro.
 *
 * Note: this file/macro is used for both the INA230 and INA236 which
 * requires the REG/CHANNEL definitions to be done per instance to avoid
 * unused structure warnings. This could result in multiple instances of the
 * channels structure and thus wastes some memory.  It may be worth fixing later.
 */
#define INA230_DRIVER_INIT(inst, type)                                         \
	INA2XX_REG_DEFINE(type##_config##inst, INA230_REG_CONFIG, 16, 0);          \
	INA2XX_REG_DEFINE(type##_cal##inst, INA230_REG_CALIB, 16, 0);              \
	INA2XX_CHANNEL_DEFINE(type##_current##inst, INA230_REG_CURRENT,            \
		16, 0, 1, 1);                                                          \
	INA2XX_CHANNEL_DEFINE(type##_bus_voltage##inst, INA230_REG_BUS_VOLT,       \
		16, 0, type##_BUS_VOLTAGE_UV_LSB, 1);                                  \
	INA2XX_CHANNEL_DEFINE(type##_power##inst, INA230_REG_POWER,                \
		16, 0, type##_POWER_SCALING, 1);                                       \
	                                                                           \
	static struct ina2xx_channels drv_channels_##type##inst = {                \
		.bus_voltage = &type##_bus_voltage##inst,                              \
		.shunt_voltage = NULL,                                                 \
		.current = &type##_current##inst,                                      \
		.power = &type##_power##inst,                                          \
		.die_temp = NULL,                                                      \
		.energy = NULL,                                                        \
		.charge = NULL,                                                        \
	};                                                                         \
	static struct ina230_data drv_data_##type##inst;                           \
	static const struct ina230_config drv_config_##type##inst = {              \
		.common = {                                                            \
			.bus = I2C_DT_SPEC_INST_GET(inst),                                 \
			.current_lsb = DT_INST_PROP(inst, current_lsb_microamps),          \
			.config = INA230_DT_CONFIG(inst),                                  \
			.cal = INA230_DT_CAL(inst),                                        \
			.id_reg = NULL,                                                    \
			.config_reg = &type##_config##inst,                                \
			.adc_config_reg = NULL,                                            \
			.cal_reg = &type##_cal##inst,                                      \
			.channels = &drv_channels_##type##inst,                            \
		},                                                                     \
		.uv_lsb = type##_BUS_VOLTAGE_UV_LSB,                                   \
		.power_scale = type##_POWER_SCALING,                                   \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, alert_gpios),                  \
				(INA230_CFG_IRQ(inst)), ())};                                  \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &ina230_init, NULL,                     \
				&drv_data_##type##inst, &drv_config_##type##inst,              \
				POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &ina230_driver_api);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_ina230
DT_INST_FOREACH_STATUS_OKAY_VARGS(INA230_DRIVER_INIT, INA230)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_ina236
DT_INST_FOREACH_STATUS_OKAY_VARGS(INA230_DRIVER_INIT, INA236)
