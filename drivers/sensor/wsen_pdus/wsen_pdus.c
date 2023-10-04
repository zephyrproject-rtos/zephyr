/*
 * Copyright (c) 2023 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_pdus

#include <stdlib.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "wsen_pdus.h"

LOG_MODULE_REGISTER(WSEN_PDUS, CONFIG_SENSOR_LOG_LEVEL);

static int pdus_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct pdus_config *const config = dev->config;
	struct pdus_data *data = dev->data;
	float pressure;
	float temperature;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (PDUS_getPressureAndTemperature_float(&data->sensor_interface,
						 config->sensor_type, &pressure,
						 &temperature) != WE_SUCCESS) {
		LOG_ERR("Failed to fetch data sample");
		return -EIO;
	}

	data->pressure_k_pa = pressure;
	data->temperature_deg_c = temperature;

	return 0;
}

static int pdus_channel_get(const struct device *dev, enum sensor_channel chan,
			    struct sensor_value *value)
{
	struct pdus_data *data = dev->data;

	if (chan == SENSOR_CHAN_PRESS) {
		value->val1 = (int32_t)data->pressure_k_pa;
		value->val2 = (((int32_t)(data->pressure_k_pa * 1000)) % 1000) * 1000;
	} else if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		value->val1 = (int32_t)data->temperature_deg_c;
		value->val2 =
			(((int32_t)(data->temperature_deg_c * 1000)) % 1000) * 1000;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api pdus_driver_api = { .sample_fetch = pdus_sample_fetch,
							  .channel_get = pdus_channel_get };

static int pdus_init(const struct device *dev)
{
	const struct pdus_config *const config = dev->config;
	struct pdus_data *data = dev->data;

	/* Initialize WE sensor interface */
	PDUS_getDefaultInterface(&data->sensor_interface);
	data->sensor_interface.interfaceType = WE_i2c;

	switch (data->sensor_interface.interfaceType) {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	case WE_i2c:
		data->sensor_interface.handle = (void *)&config->bus_cfg.i2c;
		break;
#endif
	default:
		LOG_ERR("Invalid interface type");
		return -EINVAL;
	}

	return 0;
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "PDUS driver enabled without any devices"
#endif

/*
 * Main instantiation macro.
 */
#define PDUS_DEFINE(inst)                                                         \
	static struct pdus_data pdus_data_##inst;                                     \
	static const struct pdus_config pdus_config_##inst =                          \
		{                                                                         \
		.bus_cfg = {                                                              \
			.i2c = I2C_DT_SPEC_INST_GET(inst),                                    \
		},                                                                        \
		.sensor_type = (PDUS_SensorType_t) DT_INST_ENUM_IDX(inst, sensor_type)    \
		};                                                                        \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,             \
			    pdus_init,                         \
			    NULL,                              \
			    &pdus_data_##inst,                 \
			    &pdus_config_##inst,               \
			    POST_KERNEL,                       \
			    CONFIG_SENSOR_INIT_PRIORITY,       \
			    &pdus_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PDUS_DEFINE)
