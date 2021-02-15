/* mpr.c - Driver for Honeywell MPR pressure sensor series */

/*
 * Copyright (c) 2020 Sven Herrmann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT honeywell_mpr

#include <errno.h>
#include <kernel.h>
#include <sys/__assert.h>
#include <drivers/sensor.h>
#include <drivers/i2c.h>
#include <sys/byteorder.h>
#include <init.h>
#include <logging/log.h>
#include "mpr.h"
#include "mpr_configuration.h"

LOG_MODULE_REGISTER(MPR, CONFIG_SENSOR_LOG_LEVEL);

static int mpr_init(const struct device *dev)
{
	struct mpr_data *data = dev->data;
	const struct mpr_config *cfg = dev->config;

	data->i2c_master = device_get_binding(cfg->i2c_bus);
	if (!data->i2c_master) {
		LOG_ERR("mpr: i2c master not found: %s", cfg->i2c_bus);
		return -EINVAL;
	}
	return 0;
}

static int mpr_read_reg(const struct device *dev)
{
	struct mpr_data *data = dev->data;
	const struct mpr_config *cfg = dev->config;

	uint8_t write_buf[] = { MPR_OUTPUT_MEASUREMENT_COMMAND, 0x00, 0x00 };
	uint8_t read_buf[4] = { 0x0 };

	int rc = i2c_write(data->i2c_master, write_buf, sizeof(write_buf),
			   cfg->i2c_addr);

	if (rc < 0) {
		return rc;
	}

	uint8_t retries = MPR_REG_READ_MAX_RETRIES;

	for (; retries > 0; retries--) {
		k_sleep(K_MSEC(MPR_REG_READ_DATA_CONV_DELAY_MS));

		rc = i2c_read(data->i2c_master, read_buf, sizeof(read_buf),
				  cfg->i2c_addr);
		if (rc < 0) {
			return rc;
		}

		if (!(*read_buf & MPR_STATUS_MASK_POWER_ON)
			|| (*read_buf & MPR_STATUS_MASK_INTEGRITY_TEST_FAILED)
			|| (*read_buf & MPR_STATUS_MASK_MATH_SATURATION)) {
			return -EIO;
		}

		if (!(*read_buf & MPR_STATUS_MASK_BUSY)) {
			break;
		}
	}

	if (retries == 0) {
		return -EIO;
	}

	data->reg_val = (read_buf[1] << 16)
			| (read_buf[2] << 8)
			|  read_buf[3];

	return 0;
}

/*            (reg_value - out_min) * (p_max - p_min)
 * pressure = --------------------------------------- + p_min
 *                     out_max - out_min
 *
 * returns pressure [kPa] * 10^6
 */
static inline void mpr_convert_reg(const uint32_t *reg, uint64_t *value)
{
	if (*reg > MPR_OUTPUT_MIN) {
		*value = (uint64_t)(*reg - MPR_OUTPUT_MIN) * (MPR_P_MAX - MPR_P_MIN);
		*value *= MPR_CONVERSION_FACTOR;
		*value /= MPR_OUTPUT_RANGE;
		*value += MPR_P_MIN;
	} else {
		*value = MPR_P_MIN;
	}
}

static int mpr_sample_fetch(const struct device *dev,
			    enum sensor_channel chan)
{
	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_PRESS);

	return mpr_read_reg(dev);
}

static int mpr_channel_get(const struct device *dev,
			   enum sensor_channel chan,
			   struct sensor_value *val)
{
	const struct mpr_data *data = dev->data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_PRESS);

	uint64_t value;

	mpr_convert_reg(&data->reg_val, &value);

	val->val1 = value / 1000000;
	val->val2 = value % 1000000;

	return 0;
}

static const struct sensor_driver_api mpr_api_funcs = {
	.sample_fetch = mpr_sample_fetch,
	.channel_get = mpr_channel_get,
};

static struct mpr_data mpr_data;
static const struct mpr_config mpr_cfg = {
	.i2c_bus = DT_INST_BUS_LABEL(0),
	.i2c_addr = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, mpr_init, device_pm_control_nop, &mpr_data,
			&mpr_cfg, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
			&mpr_api_funcs);
