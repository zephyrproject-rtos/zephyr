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

LOG_MODULE_DECLARE(INA2XX, CONFIG_SENSOR_LOG_LEVEL);

/** @brief Calibration scaling value (scaled by 10^-5) */
#define INA226_CAL_SCALING		512ULL

INA2XX_REG_DEFINE(ina226_config, INA226_REG_CONFIG, 16);
INA2XX_REG_DEFINE(ina226_cal, INA226_REG_CALIB, 16);
INA2XX_REG_DEFINE(ina226_id, INA226_REG_MANUFACTURER_ID, 16);

INA2XX_CHANNEL_DEFINE(ina226_shunt_voltage, INA226_REG_SHUNT_VOLT, 16, 0, 2500, 1000);
INA2XX_CHANNEL_DEFINE(ina226_bus_voltage, INA226_REG_BUS_VOLT, 16, 0, 1250, 1);
INA2XX_CHANNEL_DEFINE(ina226_current, INA226_REG_CURRENT, 16, 0, 1, 1);
INA2XX_CHANNEL_DEFINE(ina226_power, INA226_REG_POWER, 16, 0, 25, 1);

static struct ina2xx_channels ina226_channels = {
	.voltage = &ina226_bus_voltage,
	.vshunt = &ina226_shunt_voltage,
	.current = &ina226_current,
	.power = &ina226_power,
};

static DEVICE_API(sensor, ina226_driver_api) = {
	.attr_set = ina2xx_attr_set,
	.attr_get = ina2xx_attr_get,
	.sample_fetch = ina2xx_sample_fetch,
	.channel_get = ina2xx_channel_get,
};

#define INA226_DT_CONFIG(inst)                                     \
		(DT_INST_ENUM_IDX(inst, avg_count) << 9) |                 \
		(DT_INST_ENUM_IDX(inst, vbus_conversion_time_us) << 6) |   \
		(DT_INST_ENUM_IDX(inst, vshunt_conversion_time_us) << 3) | \
		(DT_INST_ENUM_IDX(inst, operating_mode))

#define INA226_DT_CAL(inst)                                          \
	INA226_CAL_SCALING * DT_INST_PROP(inst, current_lsb_microamps) * \
	DT_INST_PROP(inst, rshunt_micro_ohms) / 10000000ULL

#define INA226_DRIVER_INIT(inst)                                  \
	static struct ina2xx_data ina226_data_##inst;                 \
	static const struct ina2xx_config ina226_config_##inst = {    \
		.bus = I2C_DT_SPEC_INST_GET(inst),                        \
		.current_lsb = DT_INST_PROP(inst, current_lsb_microamps), \
		.cal = INA226_DT_CAL(inst),                               \
		.config = INA226_DT_CONFIG(inst),                         \
		.id_reg = &ina226_id,                                     \
		.config_reg = &ina226_config,                             \
		.adc_config_reg = NULL,                                   \
		.cal_reg = &ina226_cal,                                   \
		.channels = &ina226_channels,                             \
	};                                                            \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,                            \
				     &ina2xx_init,                                \
				     NULL,                                        \
				     &ina226_data_##inst,                         \
				     &ina226_config_##inst,                       \
				     POST_KERNEL,                                 \
				     CONFIG_SENSOR_INIT_PRIORITY,                 \
				     &ina226_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INA226_DRIVER_INIT)
