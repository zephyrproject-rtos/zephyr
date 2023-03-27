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
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ams_as5600, CONFIG_SENSOR_LOG_LEVEL);

#define AS5600_ANGLE_REGISTER_H 0x0E
#define AS5600_FULL_ANGLE		360
#define AS5600_PULSES_PER_REV	4096

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

	uint8_t read_data[2] = {0, 0};
	uint8_t angle_reg = AS5600_ANGLE_REGISTER_H;

	int err = i2c_write_read_dt(&dev_cfg->i2c_port,
						&angle_reg,
						1,
						&read_data,
						sizeof(read_data));

	/* invalid readings preserves the last good value */
	if (!err) {
		dev_data->position = ((uint16_t)read_data[0] << 8) | read_data[1];
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

		val->val2 = ((int32_t)dev_data->position * AS5600_FULL_ANGLE) -
					(val->val1 * AS5600_PULSES_PER_REV);
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

static const struct sensor_driver_api as5600_driver_api = {
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
