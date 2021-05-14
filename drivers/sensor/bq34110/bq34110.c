/*
 * Copyright (c) 2021 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq34110

#include <init.h>
#include <math.h>
#include <string.h>
#include <sys/util.h>
#include <drivers/i2c.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <drivers/sensor.h>

#include "bq34110.h"

/* 5ms delay between each data flash write */
#define BQ34110_DELAY	5
/*
 * @brief Read a register value
 *
 * Registers have an address and 16-bit value
 *
 * @param bq34110 Private data for the driver
 * @param reg_addr Register address to read
 * @param val Place to put the value on success
 * @return 0 if success, or negative error code from I2C_API
 */
static int bq34110_cmd_reg_read(const struct device *dev,
				uint8_t reg_addr, int16_t *val)
{
	int ret;
	uint8_t i2c_data[2];
	const struct bq34110_config *bq34110_cfg = dev->config;

	ret = i2c_write_read(bq34110_cfg->i2c_dev, bq34110_cfg->i2c_addr,
			     &reg_addr, sizeof(reg_addr), &i2c_data, 2);
	if (ret < 0) {
		LOG_ERR("Unable to read register");
		return ret;
	}

	*val = ((i2c_data[1] << 8) | i2c_data[0]);

	return 0;
}

/**
 * @brief sensor value get
 *
 * @param dev BQ34110 device to access
 * @param chan Channel number to read
 * @param val Returns sensor value read on success
 * @return 0 if successful
 * @return -ENOTSUP for unsupported channels
 */
static int bq34110_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	float int_temp;
	int32_t avg_power;
	struct bq34110_data *bq34110 = dev->data;

	switch (chan) {
	case SENSOR_CHAN_GAUGE_TEMP:
		/* Convert the temperature in kelvin to deg celcius */
		int_temp = (bq34110->internal_temperature * 0.1);
		int_temp = int_temp - 273.15;
		val->val1 = (int32_t)int_temp;
		val->val2 = (int_temp - (int32_t)int_temp) * 1000000;
		break;

	case SENSOR_CHAN_GAUGE_VOLTAGE:
		val->val1 = (bq34110->voltage / 1000);
		val->val2 = ((bq34110->voltage % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_AVG_POWER:
		avg_power = (bq34110->avg_power * 10);
		val->val1 = (avg_power / 1000);
		val->val2 = ((avg_power % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_AVG_CURRENT:
		val->val1 = (bq34110->avg_current / 1000);
		val->val2 = ((bq34110->avg_current % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_HEALTH:
		val->val1 = bq34110->state_of_health;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
		val->val1 = bq34110->state_of_charge;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_TIME_TO_FULL:
		/* Get time in minute */
		val->val1 = bq34110->time_to_full;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_TIME_TO_EMPTY:
		/* Get time in minute */
		val->val1 = bq34110->time_to_empty;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY:
		val->val1 = (bq34110->full_charge_capacity / 1000);
		val->val2 = ((bq34110->full_charge_capacity % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY:
		val->val1 = (bq34110->remaining_charge_capacity / 1000);
		val->val2 =
			((bq34110->remaining_charge_capacity % 1000) * 1000U);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int bq34110_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct bq34110_data *bq34110 = dev->data;
	struct {
		uint8_t reg_addr;
		int16_t *dest;
	} regs[] = {
		{ INTERNAL_TEMPERATURE, &bq34110->internal_temperature },
		{ VOLTAGE, &bq34110->voltage },
		{ AVERAGE_POWER, &bq34110->avg_power },
		{ CURRENT, &bq34110->avg_current },
		{ STATE_OF_HEALTH, &bq34110->state_of_health },
		{ RELATIVE_STATE_OF_CHARGE, &bq34110->state_of_charge },
		{ TIME_TO_EMPTY, &bq34110->time_to_empty },
		{ TIME_TO_FULL, &bq34110->time_to_full },
		{ FULL_CHARGE_CAPACITY, &bq34110->full_charge_capacity },
		{ REMAINING_CAPACITY, &bq34110->remaining_charge_capacity },
	};

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);
	for (size_t i = 0; i < ARRAY_SIZE(regs); i++) {
		int rc;

		rc = bq34110_cmd_reg_read(dev, regs[i].reg_addr,
					  regs[i].dest);
		if (rc != 0) {
			LOG_ERR("Failed to read channel %d", chan);
			return rc;
		}
	}

	return 0;
}

static int bq34110_ctrl_reg_read(const struct device *dev,
				 uint16_t subcommand,
				 uint16_t *val, uint8_t read_addr)
{
	int ret;
	uint8_t i2c_write_data[3], read_data[2], read_reg;
	const struct bq34110_config *bq34110_cfg = dev->config;

	memset(&i2c_write_data, 0, sizeof(i2c_write_data));
	i2c_write_data[0] = MANUFACTURER_ACCESS_CONTROL;
	i2c_write_data[1] = (uint8_t)(subcommand & 0x00FF);
	i2c_write_data[2] = (uint8_t)((subcommand >> 8) & 0x00FF);
	ret = i2c_write(bq34110_cfg->i2c_dev, i2c_write_data,
			sizeof(i2c_write_data), bq34110_cfg->i2c_addr);
	if (ret < 0) {
		LOG_ERR("Failed to write");
		return ret;
	}

	k_msleep(BQ34110_DELAY);

	read_reg = read_addr;
	ret = i2c_write_read(bq34110_cfg->i2c_dev, bq34110_cfg->i2c_addr,
			     &read_reg, sizeof(read_reg), &read_data,
			     sizeof(read_data));
	if (ret < 0) {
		LOG_ERR("Failed to read write");
		return ret;
	}

	*val = ((read_data[1] << 8) | read_data[0]);

	return 0;
}

static int update_df_parameter(const struct device *dev, uint16_t df_addr,
			       uint8_t data[], uint8_t datalength)
{
	int ret;
	uint16_t datasum;
	uint8_t i, i2c_df_addr_data[3],
		i2c_datasum_datalen[3],
		i2c_df_write_data[datalength+1];
	const struct bq34110_config *bq34110_cfg = dev->config;

	i2c_df_addr_data[0] = MANUFACTURER_ACCESS_CONTROL;
	i2c_df_addr_data[1] = (uint8_t)(df_addr & 0x00FF);
	i2c_df_addr_data[2] = (uint8_t)((df_addr >> 8) & 0x00FF);

	ret = i2c_write(bq34110_cfg->i2c_dev, i2c_df_addr_data,
			sizeof(i2c_df_addr_data), bq34110_cfg->i2c_addr);
	if (ret < 0) {
		LOG_ERR("Failed to write MAC");
		return ret;
	}

	i2c_df_write_data[0] = MAC_DATA;
	for (i = 1; i <= datalength; i++) {
		i2c_df_write_data[i] = data[i-1];
	}

	ret = i2c_write(bq34110_cfg->i2c_dev, i2c_df_write_data,
			sizeof(i2c_df_write_data), bq34110_cfg->i2c_addr);
	if (ret < 0) {
		LOG_ERR("Failed to write MAC");
		return ret;
	}

	datasum = i2c_df_addr_data[1] + i2c_df_addr_data[2];
	for (i = 1; i <= datalength; i++) {
		datasum += i2c_df_write_data[i];
	}

	i2c_datasum_datalen[0] = MAC_DATA_SUM;
	i2c_datasum_datalen[1] = (uint8_t)~datasum;
	i2c_datasum_datalen[2] = 4 + datalength;
	ret = i2c_write(bq34110_cfg->i2c_dev, i2c_datasum_datalen,
			sizeof(i2c_datasum_datalen), bq34110_cfg->i2c_addr);
	if (ret < 0) {
		LOG_ERR("Failed to write MACDatasum");
		return ret;
	}

	return 0;
}

/**
 * @brief initialise the fuel gauge
 *
 * @return 0 for success
 */
static int bq34110_gauge_init(const struct device *dev)
{
	int ret;
	uint8_t writedata[2];
	int16_t design_capacity;
	const struct bq34110_config *bq34110_cfg = dev->config;
	uint16_t design_voltage, device_type, taper_current, taper_voltage;

	if (!device_is_ready(bq34110_cfg->i2c_dev)) {
		LOG_ERR("bus %s is not ready", bq34110_cfg->i2c_dev->name);
		return -ENODEV;
	}

	ret = bq34110_ctrl_reg_read(dev, SUB_DEVICE_TYPE,
				    &device_type, MAC_DATA);
	if (ret < 0) {
		LOG_ERR("Failed to get device type");
		return -EIO;
	}

	if (device_type != BQ34110_DEVICE_ID) {
		LOG_ERR("Invalid Device");
		return -EINVAL;
	}

	memset(&writedata, 0, sizeof(writedata));
	design_voltage = bq34110_cfg->design_voltage;
	writedata[0] = (design_voltage >> 8) & 0x00FF;
	writedata[1] = (design_voltage & 0xFF);

	/* Update Design Voltage */
	ret = update_df_parameter(dev, DESIGN_VOLTAGE, writedata, sizeof(writedata));
	if (ret < 0) {
		LOG_ERR("Failed to update design voltage");
		return ret;
	}

	k_msleep(BQ34110_DELAY);

	memset(&writedata, 0, sizeof(writedata));
	design_capacity = bq34110_cfg->design_capacity;
	writedata[0] = (design_capacity >> 8) & 0x00FF;
	writedata[1] = (design_capacity & 0xFF);

	/* Update Design Capacity */
	ret = update_df_parameter(dev, DESIGN_CAPACITY_MAH, writedata,
				sizeof(writedata));
	if (ret < 0) {
		LOG_ERR("Failed to update Design Capacity");
		return ret;
	}

	k_msleep(BQ34110_DELAY);

	memset(&writedata, 0, sizeof(writedata));
	writedata[0] = 0x02;
	writedata[1] = 0x04;

	/* Select Internal Temperature Sensor */
	ret = update_df_parameter(dev, OPERATION_CONFIG_A, writedata,
				sizeof(writedata));
	if (ret < 0) {
		LOG_ERR("Failed to update Operation Config A");
		return ret;
	}

	memset(&writedata, 0, sizeof(writedata));
	taper_current = bq34110_cfg->taper_current;
	writedata[0] = (taper_current >> 8) & 0x00FF;
	writedata[1] = (taper_current & 0xFF);

	/* Update Taper Current */
	ret = update_df_parameter(dev, TAPER_CURRENT, writedata, sizeof(writedata));
	if (ret < 0) {
		LOG_ERR("Failed to update Taper Current");
		return ret;
	}

	k_msleep(BQ34110_DELAY);

	memset(&writedata, 0, sizeof(writedata));
	taper_voltage = bq34110_cfg->taper_voltage;
	writedata[0] = (taper_voltage >> 8) & 0x00FF;
	writedata[1] = (taper_voltage & 0xFF);

	/* Update Taper Voltage */
	ret = update_df_parameter(dev, TAPER_VOLTAGE, writedata, sizeof(writedata));
	if (ret < 0) {
		LOG_ERR("Failed to update Taper Voltage");
		return ret;
	}

	k_msleep(BQ34110_DELAY);

	return 0;
}

static const struct sensor_driver_api bq34110_battery_driver_api = {
	.sample_fetch = bq34110_sample_fetch,
	.channel_get = bq34110_channel_get,
};

#define BQ34110_INIT(index)						\
	static struct bq34110_data bq34110_driver_##index;		\
									\
	static const struct bq34110_config bq34110_config_##index = {	\
		.i2c_addr = DT_INST_REG_ADDR(index),			\
		.i2c_dev = DEVICE_DT_GET(DT_INST_BUS(index)),		\
		.taper_current = DT_INST_PROP(index, taper_current),	\
		.taper_voltage = DT_INST_PROP(index, taper_voltage),	\
		.design_voltage = DT_INST_PROP(index, design_voltage),	\
		.design_capacity = DT_INST_PROP(index, design_capacity),\
	};								\
									\
	DEVICE_DT_INST_DEFINE(index, &bq34110_gauge_init,		\
			      NULL,					\
			      &bq34110_driver_##index,			\
			      &bq34110_config_##index, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY,		\
			      &bq34110_battery_driver_api)

DT_INST_FOREACH_STATUS_OKAY(BQ34110_INIT);
