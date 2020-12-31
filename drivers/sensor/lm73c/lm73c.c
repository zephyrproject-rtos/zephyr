/*
 * Copyright (c) 2020 SER Consulting LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/sensor.h>
#include <kernel.h>
#include <device.h>
#include <logging/log.h>
#include <drivers/i2c.h>
#include "lm73c.h"

#define DT_DRV_COMPAT ti_lm73c

#define LM73C_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_REGISTER(LM73C, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "LM73C driver enabled without any devices"
#endif


struct lm73c_data {
	const struct device *i2c_dev;
	uint16_t temperature;
};

/**
 * @brief fetch a sample from the sensor
 *
 * @return 0
 */
static int lm73c_sample_fetch(const struct device *dev,
			      enum sensor_channel chan)
{
	int retval;
	struct lm73c_data *lm73_data = dev->data;
	uint8_t temp[2];

	retval = i2c_burst_read(lm73_data->i2c_dev,
				DT_INST_REG_ADDR(0),
				LM73_REG_TEMP,
				temp, sizeof(temp));

	if (retval == 0) {
		lm73_data->temperature = (temp[0] << 8) | temp[1];
	} else {
		LOG_ERR("lm73c read register err");
	}

	return retval;
}

/**
 * @brief sensor value get
 *
 * @return -ENOTSUP for unsupported channels
 */
static int lm73c_channel_get(const struct device *dev,
			     enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct lm73c_data *lm73_data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}
	int32_t temp = (int32_t)lm73_data->temperature;

	if (temp > 0x7fff) {
		temp -= 0x8000;
	}
	temp = (temp >> 2) * 31250;
	val->val1 = temp / 1000000;
	val->val2 = temp % 1000000;
	return 0;
}

static const struct sensor_driver_api lm73c_api = {
	.sample_fetch = &lm73c_sample_fetch,
	.channel_get = &lm73c_channel_get,
};

/**
 * @brief initialize the sensor
 *
 * @return 0 for success
 */

static int lm73c_init(const struct device *dev)
{
	const char *name = dev->name;
	struct lm73c_data *drv_data = dev->data;

	LOG_DBG("%s: Initializing", name);

	drv_data->i2c_dev = device_get_binding(
		DT_INST_BUS_LABEL(0));

	if (!drv_data->i2c_dev) {
		LOG_DBG("%s: i2c master not found: 0", name);
		return -EINVAL;
	}

	int retval;
	uint8_t temp[2];
	uint16_t id;

	retval = i2c_burst_read(drv_data->i2c_dev,
				DT_INST_REG_ADDR(0),
				LM73_REG_ID,
				temp, sizeof(temp));
	if (retval == 0) {
		id = (temp[0] << 8) | temp[1];
		LOG_DBG("%s: id (0x0190) = %x", name, id);

		/* set resolution to .03125 deg C */
		temp[0] = LM73_TEMP_PREC;
		retval = i2c_burst_write(drv_data->i2c_dev,
					DT_INST_REG_ADDR(0),
					LM73_REG_CTRL,
					temp, 1);
		if (retval == 0) {
			/* kick off the first read */
			retval = i2c_burst_read(drv_data->i2c_dev,
						DT_INST_REG_ADDR(0),
						LM73_REG_TEMP,
						temp, sizeof(temp));
			if (retval == 0) {
				drv_data->temperature =
					(temp[0] << 8) | temp[1];
			} else {
				LOG_ERR("%s: First temp read err",
					name);
				return -EINVAL;
			}
		} else {
			LOG_ERR("%s: Set resolution err",
				name);
			return -EINVAL;
		}
	} else {
		LOG_ERR("%s: Read ID register err", name);
		return -EINVAL;
	}
	LOG_DBG("%s: Initialized", name);
	return 0;
}

static struct lm73c_data lm73_data;

DEVICE_AND_API_INIT(lm73c, DT_INST_LABEL(0), lm73c_init,
			&lm73_data, NULL, POST_KERNEL,
			CONFIG_SENSOR_INIT_PRIORITY, &lm73c_api);
