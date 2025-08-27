/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ina228

#include "ina228.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(INA228, CONFIG_SENSOR_LOG_LEVEL);

/** @brief Calibration scaling value (scaled by 10^-5) */
#define INA228_CAL_SCALING 131072ULL

static int ina228_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct ina228_data *data = dev->data;
	const struct ina2xx_config *config = dev->config;

	if (chan == (enum sensor_channel)SENSOR_CHAN_INA2XX_ENERGY) {
		/* Scaled by 16 * 3.2 * CURRENT_LSB; 40-bits */
		return sensor_value_from_micro(val,
			((data->energy * config->current_lsb * 256) / 5));
	}

	if (chan == (enum sensor_channel)SENSOR_CHAN_INA2XX_CHARGE) {
		/* Scaled by 16 * CURRENT_LSB; 40-bits */
		return sensor_value_from_micro(val,
			(data->charge * config->current_lsb * 16));
	}

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
	default:
		return -ENOTSUP;
	}
}

static int ina228_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ina228_data *data = dev->data;
	const struct ina2xx_config *config = dev->config;
	bool rstacc = false;
	int ret = -ENOTSUP;

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_VSHUNT)) {
		ret = ina2xx_reg_read_24(&config->bus, INA228_REG_SHUNT_VOLT, &data->shunt_voltage);
		if (ret < 0) {
			LOG_ERR("failed to read shunt voltage");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_VOLTAGE)) {
		ret = ina2xx_reg_read_24(&config->bus, INA228_REG_BUS_VOLT, &data->bus_voltage);
		if (ret < 0) {
			LOG_ERR("failed to read bus voltage");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_DIE_TEMP)) {
		ret = ina2xx_reg_read_16(&config->bus, INA228_REG_DIETEMP, &data->die_temp);
		if (ret < 0) {
			LOG_ERR("failed to read temperature");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_CURRENT)) {
		ret = ina2xx_reg_read_24(&config->bus, INA228_REG_CURRENT, &data->current);
		if (ret < 0) {
			LOG_ERR("failed to read current");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_POWER)) {
		ret = ina2xx_reg_read_24(&config->bus, INA228_REG_POWER, &data->power);
		if (ret < 0) {
			LOG_ERR("failed to read power");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == (enum sensor_channel)SENSOR_CHAN_INA2XX_ENERGY)) {
		ret = ina2xx_reg_read_40(&config->bus, INA228_REG_ENERGY, &data->energy);
		if (ret < 0) {
			LOG_ERR("failed to read energy accumulator");
			return ret;
		}

		rstacc = true;
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == (enum sensor_channel)SENSOR_CHAN_INA2XX_CHARGE)) {
		ret = ina2xx_reg_read_40(&config->bus, INA228_REG_CHARGE, &data->charge);
		if (ret < 0) {
			LOG_ERR("failed to read charge accumulator");
			return ret;
		}

		rstacc = true;
	}

	if (rstacc) {
		/* Reset accumulators */
		ret = ina2xx_reg_write(&config->bus, INA228_REG_CONFIG, (1 << 14));
		if (ret < 0) {
			LOG_ERR("failed to reset accumulators");
			return ret;
		}
	}

	return ret;
}

static DEVICE_API(sensor, ina228_driver_api) = {
	.attr_set = ina2xx_attr_set,
	.attr_get = ina2xx_attr_get,
	.sample_fetch = ina228_sample_fetch,
	.channel_get = ina228_channel_get,
};

#define INA228_DT_ADC_CONFIG(inst)                                              \
	(DT_INST_ENUM_IDX(inst, adc_mode) << 12) |                                  \
	(DT_INST_ENUM_IDX(inst, vbus_conversion_time_us) << 9) |                    \
	(DT_INST_ENUM_IDX(inst, vshunt_conversion_time_us) << 6) |                  \
	(DT_INST_ENUM_IDX(inst, temp_conversion_time_us) << 3) |                    \
	(DT_INST_ENUM_IDX(inst, avg_count))

#define INA228_DT_CAL(inst)                                                     \
	INA228_CAL_SCALING *                                                        \
	DT_INST_PROP(inst, current_lsb_microamps) *                                 \
	DT_INST_PROP(inst, rshunt_micro_ohms) / 10000000ULL

#define INA228_DRIVER_INIT(inst)                                                \
	static struct ina228_data ina228_data_##inst;                               \
	static const struct ina2xx_config ina228_config_##inst = {                  \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                      \
		.current_lsb = DT_INST_PROP(inst, current_lsb_microamps),               \
		.config = 0,                                                            \
		.adc_config = INA228_DT_ADC_CONFIG(inst),                               \
		.cal = INA228_DT_CAL(inst),                                             \
		.id_reg = INA228_REG_MANUFACTURER_ID,                                   \
		.config_reg = INA228_REG_CONFIG,                                        \
		.adc_config_reg = INA228_REG_ADC_CONFIG,                                \
		.cal_reg = INA228_REG_SHUNT_CAL,                                        \
	};                                                                          \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &ina2xx_init, NULL, &ina228_data_##inst, \
				     &ina228_config_##inst, POST_KERNEL,                        \
				     CONFIG_SENSOR_INIT_PRIORITY, &ina228_driver_api);          \

DT_INST_FOREACH_STATUS_OKAY(INA228_DRIVER_INIT)
