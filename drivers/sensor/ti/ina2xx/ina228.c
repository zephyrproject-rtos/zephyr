/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ina228

#include "ina2xx_common.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(INA2XX, CONFIG_SENSOR_LOG_LEVEL);

INA2XX_REG_DEFINE(ina228_config, 0x00, 16, 0);
INA2XX_REG_DEFINE(ina228_adc_config, 0x01, 16, 0);
INA2XX_REG_DEFINE(ina228_shunt_cal, 0x02, 16, 0);
INA2XX_REG_DEFINE(ina228_mfr_id, 0x3E, 16, 0);

INA2XX_CHANNEL_DEFINE(ina228_shunt_voltage, 0x04, 24, 4, 5000, 16);
INA2XX_CHANNEL_DEFINE(ina228_bus_voltage, 0x05, 24, 4, 3125, 16);
INA2XX_CHANNEL_DEFINE(ina228_die_temp, 0x06, 16, 0, 125000, 16);
INA2XX_CHANNEL_DEFINE(ina228_current, 0x07, 24, 4, 1, 1);
INA2XX_CHANNEL_DEFINE(ina228_power, 0x08, 24, 0, 16, 5);
INA2XX_CHANNEL_DEFINE(ina228_energy, 0x09, 40, 0, 256, 5);
INA2XX_CHANNEL_DEFINE(ina228_charge, 0x0A, 40, 0, 1, 1);

static struct ina2xx_channels ina228_channels = {
	.bus_voltage = &ina228_bus_voltage,
	.shunt_voltage = &ina228_shunt_voltage,
	.current = &ina228_current,
	.power = &ina228_power,
	.die_temp = &ina228_die_temp,
	.energy = &ina228_energy,
	.charge = &ina228_charge,
};

/**
 * @brief INA228 sensor fetch.
 *
 * This overrides ina2xx_sample fetch to clear the energy and charge
 * accumulators after a successful read.
 */
int ina228_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct ina2xx_config *config = dev->config;
	int ret;

	ret = ina2xx_sample_fetch(dev, chan);
	if (ret < 0) {
		return ret;
	}

	if ((chan == SENSOR_CHAN_ALL) ||
		(chan == (enum sensor_channel)SENSOR_CHAN_INA2XX_ENERGY) ||
	    (chan == (enum sensor_channel)SENSOR_CHAN_INA2XX_CHARGE)) {
		/* Reset energy and charge accumulators */
		ret = ina2xx_reg_write(&config->bus, config->config_reg->addr, (1 << 14));
		if (ret < 0) {
			LOG_ERR("failed to reset accumulators");
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(sensor, ina228_driver_api) = {
	.attr_set = ina2xx_attr_set,
	.attr_get = ina2xx_attr_get,
	.sample_fetch = ina228_sample_fetch,
	.channel_get = ina2xx_channel_get,
};

/** @brief INA228 ADC configuration */
#define INA228_DT_ADC_CONFIG(inst)                             \
	(DT_INST_ENUM_IDX(inst, adc_mode) << 12) |                 \
	(DT_INST_ENUM_IDX(inst, vbus_conversion_time_us) << 9) |   \
	(DT_INST_ENUM_IDX(inst, vshunt_conversion_time_us) << 6) | \
	(DT_INST_ENUM_IDX(inst, temp_conversion_time_us) << 3) |   \
	(DT_INST_ENUM_IDX(inst, avg_count))

/** @brief INA228 shunt calibration (scaled by 10^-5) */
#define INA228_DT_CAL(inst)                                 \
	131072ULL * DT_INST_PROP(inst, current_lsb_microamps) * \
	DT_INST_PROP(inst, rshunt_micro_ohms) / 10000000ULL

#define INA228_DRIVER_INIT(inst)                                               \
	static struct ina2xx_data ina228_data_##inst;                              \
	static const struct ina2xx_config ina228_config_##inst = {                 \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                     \
		.current_lsb = DT_INST_PROP(inst, current_lsb_microamps),              \
		.config = 0,                                                           \
		.adc_config = INA228_DT_ADC_CONFIG(inst),                              \
		.cal = INA228_DT_CAL(inst),                                            \
		.id_reg = &ina228_mfr_id,                                              \
		.config_reg = &ina228_config,                                          \
		.adc_config_reg = &ina228_adc_config,                                  \
		.cal_reg = &ina228_shunt_cal,                                          \
		.channels = &ina228_channels,                                          \
	};                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &ina2xx_init, NULL,                     \
				&ina228_data_##inst, &ina228_config_##inst, POST_KERNEL,       \
				CONFIG_SENSOR_INIT_PRIORITY, &ina228_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INA228_DRIVER_INIT)
