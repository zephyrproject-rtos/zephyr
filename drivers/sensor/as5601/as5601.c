/* as5601.c - Driver for AS5601 magnetic rotary position sensor */

/*
 * Copyright ...
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ams_as5601_qdec

#include <drivers/sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "as5601.h"

LOG_MODULE_REGISTER(AS5601, CONFIG_SENSOR_LOG_LEVEL);

static int as5601_sample_fetch(const struct device *dev,
                               enum sensor_channel chan)
{
	struct as5601_data *data = dev->data;
	const struct as5601_config *config = dev->config;
	uint8_t out[3];
	int rc;

	out[0] = AS5601_REG_ANGLE;

	rc = i2c_write_read(data->i2c_master, config->i2c_slave_addr,
			    out, 1, &out[1], 2);

	if (rc < 0) {
		LOG_DBG("Failed to read sample!");
		return -EIO;
	}

	data->sample_angle = (out[0] << 8 | out[1]);

	return 0;
}

static int as5601_channel_get(const struct device *dev,
                              enum sensor_channel chan,
                              struct sensor_value *val)
{
	struct as5601_data *data = dev->data;

	if (chan == SENSOR_CHAN_ROTATION) {
		as5601_rot_convert(val, data->sample_angle);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api as5601_api_func = {
	.sample_fetch = as5601_sample_fetch,
	.channel_get = as5601_channel_get,
};

static int as5601_init(const struct device *dev)
{
	const struct as5601_config * const config = dev->config;
	struct as5601_data *data = dev->data;

	data->i2c_master = device_get_binding(config->i2c_master_dev_name);
	if (!data->i2c_master) {
		LOG_DBG("I2C master not found: %s", config->i2c_master_dev_name);
		return -EINVAL;
	}

	if (as5601_init_ic(dev) < 0) {
		LOG_DBG("Failed to init IC");
		return -EIO;
	}

	return 0;
}

static const as5601_config as5601_config = {
	.i2c_master_dev_name = DT_INST_BUS_LABEL(0),
	.i2c_slave_addr = DT_INST_REG_ADDR(0),
};

static struct as5601_data as5601_data;

DEVICE_DT_INST_DEFINE(0, as5601_init, NULL,
                      &as5601_data, &as5601_config, POST_KERNEL,
                      CONFIG_SENSOR_INIT_PRIORITY, &as5601_api_funcs);
