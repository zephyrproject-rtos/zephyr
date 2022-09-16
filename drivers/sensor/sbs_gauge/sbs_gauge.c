/*
 * Copyright (c) 2021 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sbs_sbs_gauge

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>

#include "sbs_gauge.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sbs_gauge, CONFIG_SENSOR_LOG_LEVEL);

static int sbs_cmd_reg_read(const struct device *dev,
			    uint8_t reg_addr,
			    uint16_t *val)
{
	const struct sbs_gauge_config *cfg;
	uint8_t i2c_data[2];
	int status;

	cfg = dev->config;
	status = i2c_burst_read(cfg->i2c_dev, cfg->i2c_addr, reg_addr,
				i2c_data, ARRAY_SIZE(i2c_data));
	if (status < 0) {
		LOG_ERR("Unable to read register");
		return status;
	}

	*val = sys_get_le16(i2c_data);

	return 0;
}

/**
 * @brief sensor value get
 *
 * @return -ENOTSUP for unsupported channels
 */
static int sbs_gauge_channel_get(const struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct sbs_gauge_data *data;
	int32_t int_temp;

	data = dev->data;
	val->val2 = 0;

	switch (chan) {
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		val->val1 = data->voltage / 1000;
		val->val2 = (data->voltage % 1000) * 1000;
		break;

	case SENSOR_CHAN_GAUGE_AVG_CURRENT:
		val->val1 = data->avg_current / 1000;
		val->val2 = (data->avg_current % 1000) * 1000;
		break;

	case SENSOR_CHAN_GAUGE_TEMP:
		int_temp = (data->internal_temperature * 10);
		int_temp = int_temp - 27315;
		val->val1 = int_temp / 100;
		val->val2 = (int_temp % 100) * 1000000;
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
		val->val1 = data->state_of_charge;
		break;

	case SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY:
		val->val1 = data->full_charge_capacity;
		break;

	case SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY:
		val->val1 = data->remaining_charge_capacity;
		break;

	case SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY:
		val->val1 = data->nom_avail_capacity;
		break;

	case SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY:
		val->val1 = data->full_avail_capacity;
		break;

	case SENSOR_CHAN_GAUGE_TIME_TO_EMPTY:
		val->val1 = data->time_to_empty;
		break;

	case SENSOR_CHAN_GAUGE_TIME_TO_FULL:
		val->val1 = data->time_to_full;
		break;

	case SENSOR_CHAN_GAUGE_CYCLE_COUNT:
		val->val1 = data->cycle_count;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static const uint16_t all_channels[] = {
	SENSOR_CHAN_GAUGE_VOLTAGE,
	SENSOR_CHAN_GAUGE_AVG_CURRENT,
	SENSOR_CHAN_GAUGE_TEMP,
	SENSOR_CHAN_GAUGE_STATE_OF_CHARGE,
	SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY,
	SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY,
	SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY,
	SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY,
	SENSOR_CHAN_GAUGE_TIME_TO_EMPTY,
	SENSOR_CHAN_GAUGE_TIME_TO_FULL,
	SENSOR_CHAN_GAUGE_CYCLE_COUNT
};

static int sbs_gauge_sample_fetch(const struct device *dev,
			    enum sensor_channel chan)
{
	struct sbs_gauge_data *data;
	int status = 0;

	data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		status = sbs_cmd_reg_read(dev,
					  SBS_GAUGE_CMD_VOLTAGE,
					  &data->voltage);
		if (status < 0) {
			LOG_ERR("Failed to read voltage");
		}
		break;

	case SENSOR_CHAN_GAUGE_AVG_CURRENT:
		status = sbs_cmd_reg_read(dev,
					  SBS_GAUGE_CMD_AVG_CURRENT,
					  &data->avg_current);
		if (status < 0) {
			LOG_ERR("Failed to read average current ");
		}
		break;

	case SENSOR_CHAN_GAUGE_TEMP:
		status = sbs_cmd_reg_read(dev,
					  SBS_GAUGE_CMD_TEMP,
					  &data->internal_temperature);
		if (status < 0) {
			LOG_ERR("Failed to read internal temperature");
		}
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
		status = sbs_cmd_reg_read(dev,
					  SBS_GAUGE_CMD_ASOC,
					  &data->state_of_charge);
		if (status < 0) {
			LOG_ERR("Failed to read state of charge");
		}
		break;

	case SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY:
		status = sbs_cmd_reg_read(dev,
					  SBS_GAUGE_CMD_FULL_CAPACITY,
					  &data->full_charge_capacity);
		if (status < 0) {
			LOG_ERR("Failed to read full charge capacity");
		}
		break;

	case SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY:
		status = sbs_cmd_reg_read(dev,
					  SBS_GAUGE_CMD_REM_CAPACITY,
					  &data->remaining_charge_capacity);
		if (status < 0) {
			LOG_ERR("Failed to read remaining charge capacity");
		}
		break;

	case SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY:
		status = sbs_cmd_reg_read(dev,
					  SBS_GAUGE_CMD_NOM_CAPACITY,
					  &data->nom_avail_capacity);
		if (status < 0) {
			LOG_ERR("Failed to read nominal available capacity");
		}
		break;

	case SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY:
		status = sbs_cmd_reg_read(dev,
					  SBS_GAUGE_CMD_FULL_CAPACITY,
					  &data->full_avail_capacity);
		if (status < 0) {
			LOG_ERR("Failed to read full available capacity");
		}
		break;

	case SENSOR_CHAN_GAUGE_TIME_TO_EMPTY:
		status = sbs_cmd_reg_read(dev,
					  SBS_GAUGE_CMD_AVG_TIME2EMPTY,
					  &data->time_to_empty);
		data->time_to_empty = (data->time_to_empty) & 0x00FF;

		if (status < 0) {
			LOG_ERR("Failed to read time to empty");
		}
		break;

	case SENSOR_CHAN_GAUGE_TIME_TO_FULL:
		status = sbs_cmd_reg_read(dev,
					  SBS_GAUGE_CMD_AVG_TIME2FULL,
					  &data->time_to_full);
		data->time_to_full = (data->time_to_full) & 0x00FF;

		if (status < 0) {
			LOG_ERR("Failed to read time to full");
		}
		break;

	case SENSOR_CHAN_GAUGE_CYCLE_COUNT:
		status = sbs_cmd_reg_read(dev,
					  SBS_GAUGE_CMD_CYCLE_COUNT,
					  &data->cycle_count);
		data->cycle_count = (data->cycle_count) & 0x00FF;

		if (status < 0) {
			LOG_ERR("Failed to read cycle count");
		}
		break;

	case SENSOR_CHAN_ALL:
		for (int i = 0; i < ARRAY_SIZE(all_channels); i++) {
			status = sbs_gauge_sample_fetch(dev, all_channels[i]);
			if (status != 0) {
				break;
			}
		}

		break;

	default:
		return -ENOTSUP;
	}

	return status;
}

/**
 * @brief initialize the fuel gauge
 *
 * @return 0 for success
 */
static int sbs_gauge_init(const struct device *dev)
{
	const struct sbs_gauge_config *cfg;

	cfg = dev->config;

	if (!device_is_ready(cfg->i2c_dev)) {
		LOG_ERR("%s device is not ready", cfg->i2c_dev->name);
		return -ENODEV;
	}

	return 0;
}

static const struct sensor_driver_api sbs_gauge_driver_api = {
	.sample_fetch = sbs_gauge_sample_fetch,
	.channel_get = sbs_gauge_channel_get,
};

#define SBS_GAUGE_INIT(index) \
	static struct sbs_gauge_data sbs_gauge_driver_##index; \
\
	static const struct sbs_gauge_config sbs_gauge_config_##index = { \
		.i2c_dev = DEVICE_DT_GET(DT_INST_BUS(index)), \
		.i2c_addr = DT_INST_REG_ADDR(index), \
	}; \
\
	DEVICE_DT_INST_DEFINE(index, \
			    &sbs_gauge_init, \
			    NULL, \
			    &sbs_gauge_driver_##index, \
			    &sbs_gauge_config_##index, POST_KERNEL, \
			    CONFIG_SENSOR_INIT_PRIORITY, \
			    &sbs_gauge_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SBS_GAUGE_INIT)
