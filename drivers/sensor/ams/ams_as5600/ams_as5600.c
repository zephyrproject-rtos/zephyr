/*
 * Copyright (c) 2022, Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ams_as5600

#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ams_as5600, CONFIG_SENSOR_LOG_LEVEL);

#define AS5600_ANGLE_REGISTER_H	0x0E
#define AS5600_ANGLE_REGISTER_RAW_H	0x0C
#define AS5600_STATUS_REGISTER	0x0B
#define AS5600_FULL_ANGLE		360
#define AS5600_PULSES_PER_REV	4096
#define AS5600_MILLION_UNIT	1000000

#define AS5600_STATUS_MH_BIT	(3) /* Magnet too strong */
#define AS5600_STATUS_ML_BIT	(4) /* Magnet too weak */
#define AS5600_STATUS_MD_BIT	(5) /* Magnet detected */


struct as5600_dev_cfg {
	struct i2c_dt_spec i2c_port;
};

/* Device run time data */
struct as5600_dev_data {
	uint16_t position;
};

static int as5600_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct as5600_dev_data *dev_data = dev->data;
	const struct as5600_dev_cfg *dev_cfg = dev->config;

	uint8_t status;
	int err = i2c_reg_read_byte_dt(&dev_cfg->i2c_port,
					AS5600_STATUS_REGISTER,
					&status);

	if (err != 0) {
		LOG_ERR("Failed to read status register: %d", err);
		return err;
	}

	/* Check if the magnet is present */
	if (!(status & BIT(AS5600_STATUS_MD_BIT))) {
		LOG_WRN("Magnet not detected.");
		return -ENODATA;
	}

	/* Check if the magnet is too strong or too weak */
	if (status & BIT(AS5600_STATUS_MH_BIT)) {
		LOG_WRN("Magnet too strong.");
		return -ENODATA;
	}

	if (status & BIT(AS5600_STATUS_ML_BIT)) {
		LOG_WRN("Magnet too weak.");
		return -ENODATA;
	}

	uint8_t buffer[2] = {0, 0};

	err = i2c_burst_read_dt(&dev_cfg->i2c_port,
				AS5600_ANGLE_REGISTER_RAW_H,
				(uint8_t *) &buffer,
				sizeof(buffer));

	/* invalid readings preserves the last good value */
	if (!err) {
		dev_data->position = sys_get_be16(buffer);
	}

	return err;
}

static int as5600_get(const struct device *dev, enum sensor_channel chan,
			struct sensor_value *val)
{
	struct as5600_dev_data *dev_data = dev->data;

	if (chan == SENSOR_CHAN_ROTATION) {
		val->val1 = ((int32_t)dev_data->position * AS5600_FULL_ANGLE) /
							AS5600_PULSES_PER_REV;

		val->val2 = (((int32_t)dev_data->position * AS5600_FULL_ANGLE) %
			     AS5600_PULSES_PER_REV) * (AS5600_MILLION_UNIT / AS5600_PULSES_PER_REV);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int as5600_initialize(const struct device *dev)
{
	struct as5600_dev_data *const dev_data = dev->data;

	dev_data->position = 0;

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static DEVICE_API(sensor, as5600_driver_api) = {
	.sample_fetch = as5600_fetch,
	.channel_get = as5600_get,
};

#define AS5600_INIT(n)						\
	static struct as5600_dev_data as5600_data##n;		\
	static const struct as5600_dev_cfg as5600_cfg##n = {\
		.i2c_port = I2C_DT_SPEC_INST_GET(n)	\
	};	\
									\
	SENSOR_DEVICE_DT_INST_DEFINE(n, as5600_initialize, NULL,	\
			    &as5600_data##n, &as5600_cfg##n, \
			    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,	\
			    &as5600_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AS5600_INIT)
