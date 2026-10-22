/*
 * Copyright (c) 2026 Martin Moya
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_emu_temp

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#include <sl_hal_emu.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(silabs_emu_temp, CONFIG_SENSOR_LOG_LEVEL);

struct silabs_emu_temp_data {
	float temp_c;
};

static int silabs_emu_temp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct silabs_emu_temp_data *data;

	if (dev == NULL || dev->data == NULL) {
		return -EINVAL;
	}

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	data = dev->data;

	/* sl_hal_emu_get_temperature() returns the die temperature directly in
	 * degrees Celsius (TCelsius = TKelvin - 273.15).
	 */
	data->temp_c = sl_hal_emu_get_temperature();

	return 0;
}

static int silabs_emu_temp_channel_get(const struct device *dev, enum sensor_channel chan,
				       struct sensor_value *val)
{
	struct silabs_emu_temp_data *data;

	if (dev == NULL || dev->data == NULL || val == NULL) {
		return -EINVAL;
	}

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	data = dev->data;

	return sensor_value_from_float(val, data->temp_c);
}

static DEVICE_API(sensor, silabs_emu_temp_driver_api) = {
	.sample_fetch = silabs_emu_temp_sample_fetch,
	.channel_get = silabs_emu_temp_channel_get,
};

static int silabs_emu_temp_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* The EMU temperature sensor runs periodically in all energy modes
	 * except EM4 and requires no initialization.
	 */
	return 0;
}

#define SILABS_EMU_TEMP_DEFINE(inst)								\
	static struct silabs_emu_temp_data silabs_emu_temp_data_##inst;				\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, silabs_emu_temp_init, NULL,				\
				     &silabs_emu_temp_data_##inst, NULL,			\
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,			\
				     &silabs_emu_temp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SILABS_EMU_TEMP_DEFINE)
