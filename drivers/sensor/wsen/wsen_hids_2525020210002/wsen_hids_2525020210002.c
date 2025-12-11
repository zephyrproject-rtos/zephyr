/*
 * Copyright (c) 2024 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_hids_2525020210002

#include <stdlib.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "wsen_hids_2525020210002.h"

LOG_MODULE_REGISTER(WSEN_HIDS_2525020210002, CONFIG_SENSOR_LOG_LEVEL);

static const hids_measureCmd_t precision_cmds[] = {HIDS_MEASURE_LPM, HIDS_MEASURE_MPM,
						   HIDS_MEASURE_HPM};

static const hids_measureCmd_t heater_cmds[] = {HIDS_HEATER_200_MW_01_S, HIDS_HEATER_200_MW_100_MS,
						HIDS_HEATER_110_MW_01_S, HIDS_HEATER_110_MW_100_MS,
						HIDS_HEATER_20_MW_01_S,  HIDS_HEATER_20_MW_100_MS};

static int hids_2525020210002_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct hids_2525020210002_data *data = dev->data;

	hids_measureCmd_t cmd;

	int32_t raw_temperature;
	int32_t raw_humidity;

	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_AMBIENT_TEMP:
	case SENSOR_CHAN_HUMIDITY:
		break;
	default:
		LOG_ERR("Fetching is not supported on channel %d.", chan);
		return -ENOTSUP;
	}

	if (data->sensor_precision == hids_2525020210002_precision_High &&
	    data->sensor_heater != hids_2525020210002_heater_Off) {
		cmd = heater_cmds[data->sensor_heater - 1];
	} else {
		cmd = precision_cmds[data->sensor_precision];
	}

	if (HIDS_Sensor_Measure_Raw(&data->sensor_interface, cmd, &raw_temperature,
				    &raw_humidity) != WE_SUCCESS) {
		LOG_ERR("Failed to fetch data sample");
		return -EIO;
	}

	switch (chan) {
	case SENSOR_CHAN_ALL:
		data->temperature = raw_temperature;
		data->humidity = raw_humidity;
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		data->temperature = raw_temperature;
		break;
	case SENSOR_CHAN_HUMIDITY:
		data->humidity = raw_humidity;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int hids_2525020210002_channel_get(const struct device *dev, enum sensor_channel chan,
					  struct sensor_value *val)
{
	struct hids_2525020210002_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		val->val1 = data->temperature / 1000;
		val->val2 = ((int32_t)data->temperature % 1000) * (1000000 / 1000);
		break;
	case SENSOR_CHAN_HUMIDITY:
		val->val1 = data->humidity / 1000;
		val->val2 = ((int32_t)data->humidity % 1000) * (1000000 / 1000);
		break;
	default:
		LOG_ERR("Channel not supported %d", chan);
		return -ENOTSUP;
	}

	return 0;
}

/* Set precision configuration. */
static int hids_2525020210002_precision_set(const struct device *dev,
					    const struct sensor_value *precision)
{
	struct hids_2525020210002_data *data = dev->data;

	if (precision->val1 < hids_2525020210002_precision_Low ||
	    precision->val1 > hids_2525020210002_precision_High || precision->val2 != 0) {
		LOG_ERR("Bad precision configuration %d", precision->val1);
		return -EINVAL;
	}

	data->sensor_precision = (hids_2525020210002_precision_t)precision->val1;

	return 0;
}

/* Set heater option. */
static int hids_2525020210002_heater_set(const struct device *dev,
					 const struct sensor_value *heater)
{
	struct hids_2525020210002_data *data = dev->data;

	if (heater->val1 < hids_2525020210002_heater_Off ||
	    heater->val1 > hids_2525020210002_heater_On_20mW_100ms || heater->val2 != 0) {
		LOG_ERR("Bad heater option %d", heater->val1);
		return -EINVAL;
	}

	data->sensor_heater = (hids_2525020210002_heater_t)heater->val1;

	return 0;
}

static int hids_2525020210002_attr_set(const struct device *dev, enum sensor_channel chan,
				       enum sensor_attribute attr, const struct sensor_value *val)
{

	if (chan != SENSOR_CHAN_ALL) {
		LOG_WRN("attr_set() is not supported on channel %d.", chan);
		return -ENOTSUP;
	}

	switch ((int)attr) {
	case SENSOR_ATTR_WSEN_HIDS_2525020210002_PRECISION:
		return hids_2525020210002_precision_set(dev, val);
	case SENSOR_ATTR_WSEN_HIDS_2525020210002_HEATER:
		return hids_2525020210002_heater_set(dev, val);
	default:
		LOG_ERR("Operation not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int hids_2525020210002_attr_get(const struct device *dev, enum sensor_channel chan,
				       enum sensor_attribute attr, struct sensor_value *val)
{
	struct hids_2525020210002_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL) {
		LOG_WRN("attr_get() is not supported on channel %d.", chan);
		return -ENOTSUP;
	}

	if (val == NULL) {
		LOG_WRN("address of passed value is NULL.");
		return -EFAULT;
	}

	switch ((int)attr) {
	case SENSOR_ATTR_WSEN_HIDS_2525020210002_PRECISION:
		val->val1 = data->sensor_precision;
		val->val2 = 0;
		break;
	case SENSOR_ATTR_WSEN_HIDS_2525020210002_HEATER:
		val->val1 = data->sensor_heater;
		val->val2 = 0;
		break;
	default:
		LOG_ERR("Operation not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, hids_2525020210002_driver_api) = {
	.attr_set = hids_2525020210002_attr_set,
	.attr_get = hids_2525020210002_attr_get,
	.sample_fetch = hids_2525020210002_sample_fetch,
	.channel_get = hids_2525020210002_channel_get,
};

static int hids_2525020210002_init(const struct device *dev)
{
	const struct hids_2525020210002_config *const config = dev->config;
	struct hids_2525020210002_data *data = dev->data;
	struct sensor_value precision, heater;

	/* Initialize WE sensor interface */
	HIDS_Get_Default_Interface(&data->sensor_interface);
	data->sensor_interface.interfaceType = WE_i2c;
	if (!i2c_is_ready_dt(&config->bus_cfg.i2c)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}
	data->sensor_interface.handle = (void *)&config->bus_cfg.i2c;

	/* First communication test - check device ID */
	if (HIDS_Sensor_Init(&data->sensor_interface) != WE_SUCCESS) {
		LOG_ERR("Failed to read device ID.");
		return -EIO;
	}

	precision.val1 = config->precision;
	precision.val2 = 0;

	if (hids_2525020210002_precision_set(dev, &precision) < 0) {
		LOG_ERR("Failed to set precision configuration.");
		return -EIO;
	}

	heater.val1 = config->heater;
	heater.val2 = 0;

	if (hids_2525020210002_heater_set(dev, &heater) < 0) {
		LOG_ERR("Failed to set heater option.");
		return -EIO;
	}

	return 0;
}

/*
 * Main instantiation macro.
 */
#define HIDS_2525020210002_DEFINE(inst)                                                            \
	static struct hids_2525020210002_data hids_2525020210002_data_##inst;                      \
	static const struct hids_2525020210002_config hids_2525020210002_config_##inst = {         \
		.bus_cfg = {.i2c = I2C_DT_SPEC_INST_GET(inst)},                                    \
		.precision = (hids_2525020210002_precision_t)(DT_INST_ENUM_IDX(inst, precision)),  \
		.heater = (hids_2525020210002_heater_t)(DT_INST_ENUM_IDX(inst, heater)),           \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, hids_2525020210002_init, NULL,                          \
				     &hids_2525020210002_data_##inst,                              \
				     &hids_2525020210002_config_##inst, POST_KERNEL,               \
				     CONFIG_SENSOR_INIT_PRIORITY, &hids_2525020210002_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HIDS_2525020210002_DEFINE)
