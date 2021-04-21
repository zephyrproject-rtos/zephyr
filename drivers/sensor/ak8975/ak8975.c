/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT asahi_kasei_ak8975

#include <device.h>
#include <drivers/i2c.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <logging/log.h>

#include "ak8975.h"

LOG_MODULE_REGISTER(AK8975, CONFIG_SENSOR_LOG_LEVEL);

static int ak8975_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct ak8975_data *drv_data = dev->data;
	uint8_t buf[6];

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (i2c_reg_write_byte(drv_data->i2c,
			       DT_INST_REG_ADDR(0),
			       AK8975_REG_CNTL, AK8975_MODE_MEASURE) < 0) {
		LOG_ERR("Failed to start measurement.");
		return -EIO;
	}

	k_busy_wait(AK8975_MEASURE_TIME_US);

	if (i2c_burst_read(drv_data->i2c,
			   DT_INST_REG_ADDR(0),
			   AK8975_REG_DATA_START, buf, 6) < 0) {
		LOG_ERR("Failed to read sample data.");
		return -EIO;
	}

	drv_data->x_sample = sys_le16_to_cpu(buf[0] | (buf[1] << 8));
	drv_data->y_sample = sys_le16_to_cpu(buf[2] | (buf[3] << 8));
	drv_data->z_sample = sys_le16_to_cpu(buf[4] | (buf[5] << 8));

	return 0;
}

static void ak8975_convert(struct sensor_value *val, int16_t sample,
			   uint8_t adjustment)
{
	int32_t conv_val;

	conv_val = sample * AK8975_MICRO_GAUSS_PER_BIT *
		   ((uint16_t)adjustment + 128) / 256;
	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

static int ak8975_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ak8975_data *drv_data = dev->data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_MAGN_XYZ ||
			chan == SENSOR_CHAN_MAGN_X ||
			chan == SENSOR_CHAN_MAGN_Y ||
			chan == SENSOR_CHAN_MAGN_Z);

	if (chan == SENSOR_CHAN_MAGN_XYZ) {
		ak8975_convert(val, drv_data->x_sample, drv_data->x_adj);
		ak8975_convert(val + 1, drv_data->y_sample, drv_data->y_adj);
		ak8975_convert(val + 2, drv_data->z_sample, drv_data->z_adj);
	} else if (chan == SENSOR_CHAN_MAGN_X) {
		ak8975_convert(val, drv_data->x_sample, drv_data->x_adj);
	} else if (chan == SENSOR_CHAN_MAGN_Y) {
		ak8975_convert(val, drv_data->y_sample, drv_data->y_adj);
	} else { /* chan == SENSOR_CHAN_MAGN_Z */
		ak8975_convert(val, drv_data->z_sample, drv_data->z_adj);
	}

	return 0;
}

static const struct sensor_driver_api ak8975_driver_api = {
	.sample_fetch = ak8975_sample_fetch,
	.channel_get = ak8975_channel_get,
};

static int ak8975_read_adjustment_data(struct ak8975_data *drv_data)
{
	uint8_t buf[3];

	if (i2c_reg_write_byte(drv_data->i2c,
			       DT_INST_REG_ADDR(0),
			       AK8975_REG_CNTL, AK8975_MODE_FUSE_ACCESS) < 0) {
		LOG_ERR("Failed to set chip in fuse access mode.");
		return -EIO;
	}

	if (i2c_burst_read(drv_data->i2c,
			   DT_INST_REG_ADDR(0),
			   AK8975_REG_ADJ_DATA_START, buf, 3) < 0) {
		LOG_ERR("Failed to read adjustment data.");
		return -EIO;
	}

	drv_data->x_adj = buf[0];
	drv_data->y_adj = buf[1];
	drv_data->z_adj = buf[2];

	return 0;
}

int ak8975_init(const struct device *dev)
{
	struct ak8975_data *drv_data = dev->data;
	uint8_t id;

	drv_data->i2c =
		device_get_binding(DT_INST_BUS_LABEL(0));
	if (drv_data->i2c == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			    DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}

#if DT_NODE_HAS_STATUS(DT_INST(0, invensense_mpu9150), okay)
	/* wake up MPU9150 chip */
	if (i2c_reg_update_byte(drv_data->i2c, MPU9150_I2C_ADDR,
				MPU9150_REG_PWR_MGMT1, MPU9150_SLEEP_EN,
				0) < 0) {
		LOG_ERR("Failed to wake up MPU9150 chip.");
		return -EIO;
	}

	/* enable MPU9150 pass-though to have access to AK8975 */
	if (i2c_reg_update_byte(drv_data->i2c, MPU9150_I2C_ADDR,
				MPU9150_REG_BYPASS_CFG, MPU9150_I2C_BYPASS_EN,
				MPU9150_I2C_BYPASS_EN) < 0) {
		LOG_ERR("Failed to enable pass-through mode for MPU9150.");
		return -EIO;
	}
#endif

	/* check chip ID */
	if (i2c_reg_read_byte(drv_data->i2c,
			      DT_INST_REG_ADDR(0),
			      AK8975_REG_CHIP_ID, &id) < 0) {
		LOG_ERR("Failed to read chip ID.");
		return -EIO;
	}

	if (id != AK8975_CHIP_ID) {
		LOG_ERR("Invalid chip ID.");
		return -EINVAL;
	}

	if (ak8975_read_adjustment_data(drv_data) < 0) {
		return -EIO;
	}

	return 0;
}

struct ak8975_data ak8975_data;

DEVICE_DT_INST_DEFINE(0, ak8975_init, device_pm_control_nop,
		    &ak8975_data,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &ak8975_driver_api);
