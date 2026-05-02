/*
 * Copyright 2021 The Chromium OS Authors
 * Copyright 2021 Grinn
 * Copyright 2025 Nova Dynamics LLC
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
#define INA232_BUS_VOLTAGE_UV_LSB 1600U
#define INA236_BUS_VOLTAGE_UV_LSB 1600U

/** @brief The scaling for the power register. */
#define INA230_POWER_SCALING 25
#define INA232_POWER_SCALING 32
#define INA236_POWER_SCALING 32

INA2XX_REG_DEFINE(ina230_config, INA230_REG_CONFIG, 16);
INA2XX_REG_DEFINE(ina230_cal, INA230_REG_CALIB, 16);

#if DT_HAS_COMPAT_STATUS_OKAY(ti_ina230)
INA2XX_CHANNEL_DEFINE(ina230_current, INA230_REG_CURRENT, 16, 0, 1, 1);
INA2XX_CHANNEL_DEFINE(ina230_bus_voltage, INA230_REG_BUS_VOLT, 16, 0, INA230_BUS_VOLTAGE_UV_LSB, 1);
INA2XX_CHANNEL_DEFINE(ina230_power, INA230_REG_POWER, 16, 0, INA230_POWER_SCALING, 1);

static struct ina2xx_channels ina230_channels = {
	.voltage = &ina230_bus_voltage,
	.current = &ina230_current,
	.power = &ina230_power,
};
#endif /* ti_ina230 */

#if DT_HAS_COMPAT_STATUS_OKAY(ti_ina232)
INA2XX_CHANNEL_DEFINE(ina232_current, INA230_REG_CURRENT, 16, 0, 1, 1);
INA2XX_CHANNEL_DEFINE(ina232_bus_voltage, INA230_REG_BUS_VOLT, 16, 0, INA232_BUS_VOLTAGE_UV_LSB, 1);
INA2XX_CHANNEL_DEFINE(ina232_power, INA230_REG_POWER, 16, 0, INA232_POWER_SCALING, 1);

static struct ina2xx_channels ina232_channels = {
	.voltage = &ina232_bus_voltage,
	.current = &ina232_current,
	.power = &ina232_power,
};
#endif /* ti_ina232 */

#if DT_HAS_COMPAT_STATUS_OKAY(ti_ina236)
INA2XX_CHANNEL_DEFINE(ina236_current, INA230_REG_CURRENT, 16, 0, 1, 1);
INA2XX_CHANNEL_DEFINE(ina236_bus_voltage, INA230_REG_BUS_VOLT, 16, 0, INA236_BUS_VOLTAGE_UV_LSB, 1);
INA2XX_CHANNEL_DEFINE(ina236_power, INA230_REG_POWER, 16, 0, INA236_POWER_SCALING, 1);

static struct ina2xx_channels ina236_channels = {
	.voltage = &ina236_bus_voltage,
	.current = &ina236_current,
	.power = &ina236_power,
};
#endif /* ti_ina236 */

static int ina230_set_feature_mask(const struct device *dev, const struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	uint16_t data = val->val1;

	return ina2xx_reg_write(&config->bus, INA230_REG_MASK, data);
}

static int ina230_set_alert(const struct device *dev, const struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	uint16_t data = val->val1;

	return ina2xx_reg_write(&config->bus, INA230_REG_ALERT, data);
}

static int ina230_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	if (attr == SENSOR_ATTR_FEATURE_MASK) {
		return ina230_set_feature_mask(dev, val);
	}

	if (attr == SENSOR_ATTR_ALERT) {
		return ina230_set_alert(dev, val);
	}

	return ina2xx_attr_set(dev, chan, attr, val);
}

static int ina230_get_feature_mask(const struct device *dev, struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	uint16_t data;
	int ret;

	ret = ina2xx_reg_read_16(&config->bus, INA230_REG_MASK, &data);
	if (ret < 0) {
		return ret;
	}

	val->val1 = data;
	val->val2 = 0;

	return 0;
}

static int ina230_get_alert(const struct device *dev, struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	uint16_t data;
	int ret;

	ret = ina2xx_reg_read_16(&config->bus, INA230_REG_ALERT, &data);
	if (ret < 0) {
		return ret;
	}

	val->val1 = data;
	val->val2 = 0;

	return 0;
}

static int ina230_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	if (attr == SENSOR_ATTR_FEATURE_MASK) {
		return ina230_get_feature_mask(dev, val);
	}

	if (attr == SENSOR_ATTR_ALERT) {
		return ina230_get_alert(dev, val);
	}

	return ina2xx_attr_get(dev, chan, attr, val);
}

static int ina230_init_trigger(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_INA230_TRIGGER)) {
		const struct ina230_config *const config = dev->config;
		const struct i2c_dt_spec *bus = &config->common.bus;
		int ret;

		if (!config->trig_enabled) {
			return 0;
		}

		ret = ina230_trigger_mode_init(dev);
		if (ret < 0) {
			LOG_ERR("Failed to init trigger mode\n");
			return ret;
		}

		ret = ina2xx_reg_write(bus, INA230_REG_ALERT, config->alert_limit);
		if (ret < 0) {
			LOG_ERR("Failed to write alert register!");
			return ret;
		}

		ret = ina2xx_reg_write(bus, INA230_REG_MASK, config->mask);
		if (ret < 0) {
			LOG_ERR("Failed to write mask register!");
			return ret;
		}
	}

	return 0;
}

static int ina230_init(const struct device *dev)
{
	int ret;

	ret = ina2xx_init(dev);
	if (ret < 0) {
		return ret;
	}

	ret = ina230_init_trigger(dev);
	if (ret < 0) {
		return ret;
	}

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

#define INA230_DT_CONFIG(inst)                                                                     \
	(DT_INST_PROP_OR(inst, high_precision, 0) << 12) |                                         \
		(DT_INST_ENUM_IDX(inst, avg_count) << 9) |                                         \
		(DT_INST_ENUM_IDX(inst, vbus_conversion_time_us) << 6) |                           \
		(DT_INST_ENUM_IDX(inst, vshunt_conversion_time_us) << 3) |                         \
		(DT_INST_ENUM_IDX(inst, adc_mode))

#define INA230_DT_CAL(inst)                                                                        \
	(uint16_t)(((INA230_CAL_SCALING * 10000000ULL) /                                           \
		    ((uint64_t)DT_INST_PROP(inst, current_lsb_microamps) *                         \
		     DT_INST_PROP(inst, rshunt_micro_ohms))) >>                                    \
		   (DT_INST_PROP_OR(inst, high_precision, 0) << 1))

#define INA230_DRIVER_INIT(inst)                                                                   \
	static struct ina230_data ina230_data_##inst;                                              \
	static const struct ina230_config ina230_config_##inst = {                                 \
		.common =                                                                          \
			{                                                                          \
				.bus = I2C_DT_SPEC_INST_GET(inst),                                 \
				.current_lsb = DT_INST_PROP(inst, current_lsb_microamps),          \
				.config = INA230_DT_CONFIG(inst),                                  \
				.cal = INA230_DT_CAL(inst),                                        \
				.id_reg = NULL,                                                    \
				.config_reg = &ina230_config,                                      \
				.adc_config_reg = NULL,                                            \
				.cal_reg = &ina230_cal,                                            \
				.channels = &ina230_channels,                                      \
			},                                                                         \
		.uv_lsb = INA230_BUS_VOLTAGE_UV_LSB,                                               \
		.power_scale = INA230_POWER_SCALING,                                               \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, alert_gpios),                              \
				(INA230_CFG_IRQ(inst)), ())};                                      \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &ina230_init, NULL, &ina230_data_##inst,                \
				     &ina230_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &ina230_driver_api);

#define INA232_DRIVER_INIT(inst)                                                                   \
	static struct ina230_data ina232_data_##inst;                                              \
	static const struct ina230_config ina232_config_##inst = {                                 \
		.common =                                                                          \
			{                                                                          \
				.bus = I2C_DT_SPEC_INST_GET(inst),                                 \
				.current_lsb = DT_INST_PROP(inst, current_lsb_microamps),          \
				.config = INA230_DT_CONFIG(inst),                                  \
				.cal = INA230_DT_CAL(inst),                                        \
				.id_reg = NULL,                                                    \
				.config_reg = &ina230_config,                                      \
				.adc_config_reg = NULL,                                            \
				.cal_reg = &ina230_cal,                                            \
				.channels = &ina232_channels,                                      \
			},                                                                         \
		.uv_lsb = INA232_BUS_VOLTAGE_UV_LSB,                                               \
		.power_scale = INA232_POWER_SCALING,                                               \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, alert_gpios),                              \
				(INA230_CFG_IRQ(inst)), ())};                                      \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &ina230_init, NULL, &ina232_data_##inst,                \
				     &ina232_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &ina230_driver_api);

#define INA236_DRIVER_INIT(inst)                                                                   \
	static struct ina230_data ina236_data_##inst;                                              \
	static const struct ina230_config ina236_config_##inst = {                                 \
		.common =                                                                          \
			{                                                                          \
				.bus = I2C_DT_SPEC_INST_GET(inst),                                 \
				.current_lsb = DT_INST_PROP(inst, current_lsb_microamps),          \
				.config = INA230_DT_CONFIG(inst),                                  \
				.cal = INA230_DT_CAL(inst),                                        \
				.id_reg = NULL,                                                    \
				.config_reg = &ina230_config,                                      \
				.adc_config_reg = NULL,                                            \
				.cal_reg = &ina230_cal,                                            \
				.channels = &ina236_channels,                                      \
			},                                                                         \
		.uv_lsb = INA236_BUS_VOLTAGE_UV_LSB,                                               \
		.power_scale = INA236_POWER_SCALING,                                               \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, alert_gpios),                              \
				(INA230_CFG_IRQ(inst)), ())};                                      \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &ina230_init, NULL, &ina236_data_##inst,                \
				     &ina236_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &ina230_driver_api);

#define DT_DRV_COMPAT ti_ina230
DT_INST_FOREACH_STATUS_OKAY(INA230_DRIVER_INIT)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT ti_ina232
DT_INST_FOREACH_STATUS_OKAY(INA232_DRIVER_INIT)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT ti_ina236
DT_INST_FOREACH_STATUS_OKAY(INA236_DRIVER_INIT)
#undef DT_DRV_COMPAT
