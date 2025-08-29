/*
 * Copyright 2024 NXP
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* TODO: Add functionality for Trigger. */

#define DT_DRV_COMPAT ti_ina226

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/ina226.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/sys/util_macro.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include "ina226.h"

LOG_MODULE_REGISTER(INA226, CONFIG_SENSOR_LOG_LEVEL);

/** @brief Calibration scaling value (scaled by 10^-5) */
#define INA226_CAL_SCALING		512ULL

/** @brief The LSB value for the bus voltage register, in microvolts/LSB. */
#define INA226_BUS_VOLTAGE_TO_uV(x)	((x) * 1250U)

/** @brief The LSB value for the shunt voltage register, in microvolts/LSB. */
#define INA226_SHUNT_VOLTAGE_TO_uV(x)	((x) * 2500U / 1000U)

/** @brief Power scaling (need factor of 0.2) */
#define INA226_POWER_TO_uW(x)		((x) * 25ULL)

static void micro_s32_to_sensor_value(struct sensor_value *val, int32_t value_microX)
{
	val->val1 = value_microX / 1000000L;
	val->val2 = value_microX % 1000000L;
}

static int ina226_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct ina226_data *data = dev->data;
	const struct ina2xx_config *config = dev->config;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		micro_s32_to_sensor_value(val, INA226_BUS_VOLTAGE_TO_uV(data->bus_voltage));
		break;
	case SENSOR_CHAN_CURRENT:
		/* see datasheet "Current and Power calculations" section */
		micro_s32_to_sensor_value(val, data->current * config->current_lsb);
		break;
	case SENSOR_CHAN_POWER:
		/* power in uW is power_reg * current_lsb * 0.2 */
		micro_s32_to_sensor_value(val,
			INA226_POWER_TO_uW((uint32_t)data->power * config->current_lsb));
		break;
#ifdef CONFIG_INA226_VSHUNT
	case SENSOR_CHAN_VSHUNT:
		micro_s32_to_sensor_value(val, INA226_SHUNT_VOLTAGE_TO_uV(data->shunt_voltage));
		break;
#endif /* CONFIG_INA226_VSHUNT */
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ina226_read_data(const struct device *dev)
{
	struct ina226_data *data = dev->data;
	const struct ina2xx_config *config = dev->config;
	int ret;

	if ((data->chan == SENSOR_CHAN_ALL) || (data->chan == SENSOR_CHAN_VOLTAGE)) {
		ret = ina2xx_reg_read_16(&config->bus, INA226_REG_BUS_VOLT, &data->bus_voltage);
		if (ret < 0) {
			LOG_ERR("Failed to read bus voltage");
			return ret;
		}
	}

	if ((data->chan == SENSOR_CHAN_ALL) || (data->chan == SENSOR_CHAN_CURRENT)) {
		ret = ina2xx_reg_read_16(&config->bus, INA226_REG_CURRENT, &data->current);
		if (ret < 0) {
			LOG_ERR("Failed to read current");
			return ret;
		}
	}

	if ((data->chan == SENSOR_CHAN_ALL) || (data->chan == SENSOR_CHAN_POWER)) {
		ret = ina2xx_reg_read_16(&config->bus, INA226_REG_POWER, &data->power);
		if (ret < 0) {
			LOG_ERR("Failed to read power");
			return ret;
		}
	}

#ifdef CONFIG_INA226_VSHUNT
	if ((data->chan == SENSOR_CHAN_ALL) || (data->chan == SENSOR_CHAN_VSHUNT)) {
		ret = ina2xx_reg_read_16(&config->bus, INA226_REG_SHUNT_VOLT, &data->shunt_voltage);
		if (ret < 0) {
			LOG_ERR("Failed to read shunt voltage");
			return ret;
		}
	}
#endif /* CONFIG_INA226_VSHUNT */

	return 0;
}

static int ina226_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ina226_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE
	    && chan != SENSOR_CHAN_CURRENT && chan != SENSOR_CHAN_POWER
#ifdef CONFIG_INA226_VSHUNT
	    && chan != SENSOR_CHAN_VSHUNT
#endif /* CONFIG_INA226_VSHUNT */
	    ) {
		return -ENOTSUP;
	}

	data->chan = chan;

	return ina226_read_data(dev);
}

static DEVICE_API(sensor, ina226_driver_api) = {
	.attr_set = ina2xx_attr_set,
	.attr_get = ina2xx_attr_get,
	.sample_fetch = ina226_sample_fetch,
	.channel_get = ina226_channel_get,
};

#define INA226_DT_CONFIG(inst)                                                  \
		(DT_INST_ENUM_IDX(inst, avg_count) << 9) |                              \
		(DT_INST_ENUM_IDX(inst, vbus_conversion_time_us) << 6) |                \
		(DT_INST_ENUM_IDX(inst, vshunt_conversion_time_us) << 3) |              \
		(DT_INST_ENUM_IDX(inst, operating_mode))

#define INA226_DT_CAL(inst)                                                     \
	INA226_CAL_SCALING *                                                        \
	DT_INST_PROP(inst, current_lsb_microamps) *                                 \
	DT_INST_PROP(inst, rshunt_micro_ohms) / 10000000ULL

#define INA226_DRIVER_INIT(inst)                                                \
	static struct ina226_data ina226_data_##inst;                               \
	static const struct ina2xx_config ina226_config_##inst = {                  \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                      \
		.current_lsb = DT_INST_PROP(inst, current_lsb_microamps),               \
		.cal = INA226_DT_CAL(inst),                                             \
		.config = INA226_DT_CONFIG(inst),                                       \
		.id_reg = INA226_REG_MANUFACTURER_ID,                                   \
		.config_reg = INA226_REG_CONFIG,                                        \
		.adc_config_reg = -1,                                                   \
		.cal_reg = INA226_REG_CALIB,                                            \
	};                                                                          \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,                                          \
				     &ina2xx_init,                                              \
				     NULL,                                                      \
				     &ina226_data_##inst,                                       \
				     &ina226_config_##inst,                                     \
				     POST_KERNEL,                                               \
				     CONFIG_SENSOR_INIT_PRIORITY,                               \
				     &ina226_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INA226_DRIVER_INIT)
