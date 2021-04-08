/*
 * Copyright (c) 2021 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT smart_battery

#include <drivers/i2c.h>
#include <init.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <string.h>
#include <sys/byteorder.h>

#include "smart_battery.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(smart_battery, CONFIG_SENSOR_LOG_LEVEL);

#define DEV_DATA(dev) ((struct smartbattery_data *const)(dev)->data)
#define DEV_CFG(dev) \
	((const struct smartbattery_data *const)(dev)->config)

static int smart_battery_command_reg_read(const struct device *dev, uint8_t reg_addr,
				    int16_t *val)
{
	struct smartbattery_data *smart_battery = DEV_DATA(dev);
	const struct smartbattery_config *const config = DEV_CFG(dev);
	uint8_t i2c_data[2];
	int status;

	status = i2c_burst_read(smart_battery->i2c, config->i2c_addr, reg_addr,
				i2c_data, 2);
	if (status < 0) {
		LOG_ERR("Unable to read register");
		return -EIO;
	}

	*val = sys_get_le16(i2c_data);

	return 0;
}

/**
 * @brief sensor value get
 *
 * @return -ENOTSUP for unsupported channels
 */
static int smartbattery_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct smartbattery_data *smart_battery = DEV_DATA(dev);
	int32_t int_temp;

	val->val2 = 0;

	switch (chan) {
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		val->val1 = smart_battery->voltage / 1000;
		val->val2 = (smart_battery->voltage % 1000) * 1000;
		break;

	case SENSOR_CHAN_GAUGE_AVG_CURRENT:
		val->val1 = smart_battery->avg_current / 1000;
		val->val2 = (smart_battery->avg_current % 1000) * 1000;
		break;

	case SENSOR_CHAN_GAUGE_TEMP:
		int_temp = (smart_battery->internal_temperature * 10);
		int_temp = int_temp - 27315;
		val->val1 = int_temp / 100;
		val->val2 = (int_temp % 100) * 1000000;
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
		val->val1 = smart_battery->state_of_charge;
		break;

	case SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY:
		val->val1 = smart_battery->full_charge_capacity;
		break;

	case SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY:
		val->val1 = smart_battery->remaining_charge_capacity;
		break;

	case SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY:
		val->val1 = smart_battery->nom_avail_capacity;
		break;

	case SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY:
		val->val1 = smart_battery->full_avail_capacity;
		break;

	case SENSOR_CHAN_GAUGE_TIME_TO_EMPTY:
		val->val1 = smart_battery->time_to_empty;
		break;

	case SENSOR_CHAN_GAUGE_TIME_TO_FULL:
		val->val1 = smart_battery->time_to_full;
		break;

	case SENSOR_CHAN_GAUGE_CYCLE_COUNT:
		val->val1 = smart_battery->cycle_count;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int smartbattery_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct smartbattery_data *smart_battery = DEV_DATA(dev);
	int status = 0;

	switch (chan) {
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		status = smart_battery_command_reg_read(
			dev, SMART_BATTERY_COMMAND_VOLTAGE, &smart_battery->voltage);
		if (status < 0) {
			LOG_ERR("Failed to read voltage");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_AVG_CURRENT:
		status = smart_battery_command_reg_read(dev,
						  SMART_BATTERY_COMMAND_AVG_CURRENT,
						  &smart_battery->avg_current);
		if (status < 0) {
			LOG_ERR("Failed to read average current ");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_TEMP:
		status = smart_battery_command_reg_read(
			dev, SMART_BATTERY_COMMAND_TEMP,
			&smart_battery->internal_temperature);
		if (status < 0) {
			LOG_ERR("Failed to read internal temperature");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
		status = smart_battery_command_reg_read(dev, SMART_BATTERY_COMMAND_ASOC,
						  &smart_battery->state_of_charge);
		if (status < 0) {
			LOG_ERR("Failed to read state of charge");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY:
		status = smart_battery_command_reg_read(
			dev, SMART_BATTERY_COMMAND_FULL_CAPACITY,
			&smart_battery->full_charge_capacity);
		if (status < 0) {
			LOG_ERR("Failed to read full charge capacity");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY:
		status = smart_battery_command_reg_read(
			dev, SMART_BATTERY_COMMAND_REM_CAPACITY,
			&smart_battery->remaining_charge_capacity);
		if (status < 0) {
			LOG_ERR("Failed to read remaining charge capacity");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY:
		status = smart_battery_command_reg_read(dev,
						  SMART_BATTERY_COMMAND_NOM_CAPACITY,
						  &smart_battery->nom_avail_capacity);
		if (status < 0) {
			LOG_ERR("Failed to read nominal available capacity");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY:
		status =
			smart_battery_command_reg_read(dev,
						 SMART_BATTERY_COMMAND_FULL_CAPACITY,
						 &smart_battery->full_avail_capacity);
		if (status < 0) {
			LOG_ERR("Failed to read full available capacity");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_TIME_TO_EMPTY:
		status = smart_battery_command_reg_read(dev, SMART_BATTERY_COMMAND_AVG_TIME2EMPTY,
						  &smart_battery->time_to_empty);

		smart_battery->time_to_empty = (smart_battery->time_to_empty) & 0x00FF;

		if (status < 0) {
			LOG_ERR("Failed to read state of health");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_TIME_TO_FULL:
		status = smart_battery_command_reg_read(dev, SMART_BATTERY_COMMAND_AVG_TIME2FULL,
						  &smart_battery->time_to_full);

		smart_battery->time_to_full = (smart_battery->time_to_full) & 0x00FF;

		if (status < 0) {
			LOG_ERR("Failed to read state of health");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_CYCLE_COUNT:
		status = smart_battery_command_reg_read(dev, SMART_BATTERY_COMMAND_CYCLE_COUNT,
						  &smart_battery->cycle_count);

		smart_battery->cycle_count = (smart_battery->cycle_count) & 0x00FF;

		if (status < 0) {
			LOG_ERR("Failed to read state of health");
			return -EIO;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief initialise the fuel gauge
 *
 * @return 0 for success
 */
static int smartbattery_gauge_init(const struct device *dev)
{
	struct smartbattery_data *smart_battery = DEV_DATA(dev);
	const struct smartbattery_config *const config = DEV_CFG(dev);

	smart_battery->i2c = device_get_binding(config->bus_name);
	if (smart_battery->i2c == NULL) {
		LOG_ERR("Could not get pointer to %s device.",
			config->bus_name);
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api smartbattery_battery_driver_api = {
	.sample_fetch = smartbattery_sample_fetch,
	.channel_get = smartbattery_channel_get,
};

#define SMART_BATTERY_INIT(index) \
	static struct smartbattery_data smartbattery_driver_##index; \
\
	static const struct smartbattery_config smartbattery_config_##index = { \
		.bus_name = DT_INST_BUS_LABEL(index), \
		.i2c_addr = DT_INST_REG_ADDR(index), \
	}; \
\
	DEVICE_DT_INST_DEFINE(index, \
			    &smartbattery_gauge_init, \
				device_pm_control_nop, \
				&smartbattery_driver_##index, \
			    &smartbattery_config_##index, POST_KERNEL, \
			    CONFIG_SENSOR_INIT_PRIORITY, \
			    &smartbattery_battery_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SMART_BATTERY_INIT)
