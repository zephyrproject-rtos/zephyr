/*
 * Copyright (c) 2024 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_pdus_25131308xxxxx

#include <stdlib.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "wsen_pdus_25131308XXXXX.h"

LOG_MODULE_REGISTER(WSEN_PDUS_25131308XXXXX, CONFIG_SENSOR_LOG_LEVEL);

static int pdus_25131308XXXXX_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct pdus_25131308XXXXX_data *data = dev->data;
	uint16_t pressure_dummy;

	switch (chan) {
	case SENSOR_CHAN_ALL: {
		if (PDUS_getRawPressureAndTemperature(&data->sensor_interface, &data->pressure,
						      &data->temperature) != WE_SUCCESS) {
			LOG_ERR("Failed to fetch data sample");
			return -EIO;
		}
		break;
	}
	case SENSOR_CHAN_AMBIENT_TEMP: {
		if (PDUS_getRawPressureAndTemperature(&data->sensor_interface, &pressure_dummy,
						      &data->temperature) != WE_SUCCESS) {
			LOG_ERR("Failed to fetch data sample");
			return -EIO;
		}
		break;
	}
	case SENSOR_CHAN_PRESS: {
		if (PDUS_getRawPressure(&data->sensor_interface, &data->pressure) != WE_SUCCESS) {
			LOG_ERR("Failed to fetch data sample");
			return -EIO;
		}
		break;
	}
	default:
		LOG_ERR("Fetching is not supported on channel %d.", chan);
		return -ENOTSUP;
	}

	return 0;
}

static int pdus_25131308XXXXX_channel_get(const struct device *dev, enum sensor_channel chan,
					  struct sensor_value *value)
{
	struct pdus_25131308XXXXX_data *data = dev->data;
	const struct pdus_25131308XXXXX_config *config = dev->config;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP: {
		int32_t temperature_mega = ((int32_t)(data->temperature - T_MIN_VAL_PDUS)) * 4272;

		value->val1 = temperature_mega / 1000000;
		value->val2 = temperature_mega % 1000000;
		break;
	}
	case SENSOR_CHAN_PRESS: {
		int32_t pressure_temp = ((int32_t)(data->pressure - P_MIN_VAL_PDUS));

		/*
		 * these values are conversion factors based on the sensor type defined in the user
		 * manual of the respective sensor
		 */
		switch (config->sensor_type) {
		case PDUS_pdus0:
			value->val1 = ((pressure_temp * 763) - 10000000) / 100000000;
			value->val2 = (((pressure_temp * 763) - 10000000) % 100000000) / 100;
			break;
		case PDUS_pdus1:
			value->val1 = ((pressure_temp * 763) - 10000000) / 10000000;
			value->val2 = (((pressure_temp * 763) - 10000000) % 10000000) / 10;
			break;
		case PDUS_pdus2:
			value->val1 = ((pressure_temp * 763) - 10000000) / 1000000;
			value->val2 = ((pressure_temp * 763) - 10000000) % 1000000;
			break;
		case PDUS_pdus3:
			value->val1 = (pressure_temp * 3815) / 1000000;
			value->val2 = (pressure_temp * 3815) % 1000000;
			break;
		case PDUS_pdus4:
			value->val1 = ((pressure_temp * 4196) - 10000000) / 100000;
			value->val2 =
				((pressure_temp * 4196) - 10000000) % 100000 * (1000000 / 100000);
			break;
		case PDUS_pdus5:
			value->val1 = (pressure_temp * 5722) / 100000;
			value->val2 = (pressure_temp * 5722) % 100000 * (1000000 / 100000);
			break;
		default:
			LOG_ERR("Sensor type doesn't exist");
			return -ENOTSUP;
		}
		break;
	}
	default:
		LOG_ERR("Channel not supported %d", chan);
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, pdus_25131308XXXXX_driver_api) = {
	.sample_fetch = pdus_25131308XXXXX_sample_fetch,
	.channel_get = pdus_25131308XXXXX_channel_get
};

static int pdus_25131308XXXXX_init(const struct device *dev)
{
	struct pdus_25131308XXXXX_data *data = dev->data;
	const struct pdus_25131308XXXXX_config *config = dev->config;

	/* Initialize WE sensor interface */
	PDUS_getDefaultInterface(&data->sensor_interface);
	data->sensor_interface.interfaceType = WE_i2c;

	if (!i2c_is_ready_dt(&config->bus_cfg.i2c)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	data->sensor_interface.handle = (void *)&config->bus_cfg.i2c;

	return 0;
}

/*
 * Main instantiation macro.
 */
#define PDUS_25131308XXXXX_DEFINE(inst)                                                            \
	static struct pdus_25131308XXXXX_data pdus_25131308XXXXX_data_##inst;                      \
	static const struct pdus_25131308XXXXX_config pdus_25131308XXXXX_config_##inst = {         \
		.bus_cfg =                                                                         \
			{                                                                          \
				.i2c = I2C_DT_SPEC_INST_GET(inst),                                 \
			},                                                                         \
		.sensor_type = (PDUS_SensorType_t)DT_INST_ENUM_IDX(inst, sensor_type)};            \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, pdus_25131308XXXXX_init, NULL,                          \
				     &pdus_25131308XXXXX_data_##inst,                              \
				     &pdus_25131308XXXXX_config_##inst, POST_KERNEL,               \
				     CONFIG_SENSOR_INIT_PRIORITY, &pdus_25131308XXXXX_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PDUS_25131308XXXXX_DEFINE)
