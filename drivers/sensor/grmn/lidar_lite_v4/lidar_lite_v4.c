/*
 * Copyright (c) 2025 Alex Aylward
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT grmn_lidar_lite_v4

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/time_units.h>

#define MAX_TIMEOUT_MS 100

#define LIDAR_LITE_V4_REG_MEASURE       0x00
#define LIDAR_LITE_V4_REG_STATUS        0x01
#define LIDAR_LITE_V4_REG_DISTANCE_LOW  0x10

#define LIDAR_LITE_V4_CMD_MEASURE 0x04
#define LIDAR_LITE_V4_STATUS_BUSY BIT(0)

struct lidar_lite_v4_config {
	struct i2c_dt_spec i2c;
};

struct lidar_lite_v4_data {
	uint16_t distance;
};

int lidar_lite_v4_init(const struct device *dev)
{
	const struct lidar_lite_v4_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	return 0;
}

static int lidar_lite_v4_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct lidar_lite_v4_data *data = dev->data;
	const struct lidar_lite_v4_config *config = dev->config;
	uint8_t status;
	uint8_t distance_bytes[2];
	int ret;

	if (chan != SENSOR_CHAN_DISTANCE && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	/* Write command to trigger measurement */
	ret = i2c_reg_write_byte_dt(&config->i2c, LIDAR_LITE_V4_REG_MEASURE,
				    LIDAR_LITE_V4_CMD_MEASURE);
	if (ret < 0) {
		return ret;
	}

	/* Read status register and wait until measurement completes (timeout based on elapsed time)
	 */
	k_timepoint_t timeout = sys_timepoint_calc(K_MSEC(MAX_TIMEOUT_MS));
	
	while (1) {
		ret = i2c_reg_read_byte_dt(&config->i2c, LIDAR_LITE_V4_REG_STATUS, &status);
		if (ret < 0) {
			return ret;
		}

		if ((status & LIDAR_LITE_V4_STATUS_BUSY) == 0) {
			break;
		}

		if (sys_timepoint_expired(timeout)) {
			return -ETIMEDOUT;
		}

		k_msleep(1);
	}

	/* Read distance data (low byte at 0x10, high byte at 0x11) */
	ret = i2c_burst_read_dt(&config->i2c, LIDAR_LITE_V4_REG_DISTANCE_LOW, distance_bytes, 2);
	if (ret < 0) {
		return ret;
	}

	/* Combine low and high bytes (little-endian) to get 16-bit distance in centimeters */
	data->distance = sys_get_le16(distance_bytes);

	return 0;
}

static int lidar_lite_v4_channel_get(const struct device *dev, enum sensor_channel chan,
				     struct sensor_value *val)
{
	struct lidar_lite_v4_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_DISTANCE:
		return sensor_value_from_milli(val, (uint32_t)data->distance * 10);
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(sensor, lidar_lite_v4_driver_api) = {
	.sample_fetch = lidar_lite_v4_sample_fetch,
	.channel_get = lidar_lite_v4_channel_get,
};

#define LIDAR_LITE_V4_DEFINE(inst)                                                                 \
	static struct lidar_lite_v4_data lidar_lite_v4_data_##inst;                                \
	static const struct lidar_lite_v4_config lidar_lite_v4_config_##inst = {                   \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lidar_lite_v4_init, NULL, &lidar_lite_v4_data_##inst,   \
				     &lidar_lite_v4_config_##inst, POST_KERNEL,                    \
				     CONFIG_SENSOR_INIT_PRIORITY, &lidar_lite_v4_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LIDAR_LITE_V4_DEFINE)
