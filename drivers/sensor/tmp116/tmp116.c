/*
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp116

#include <device.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include <kernel.h>

#include "tmp116.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(TMP116);

static int tmp116_reg_read(struct device *dev, uint8_t reg, uint16_t *val)
{
	struct tmp116_data *drv_data = dev->data;
	const struct tmp116_dev_config *cfg = dev->config;

	if (i2c_burst_read(drv_data->i2c, cfg->i2c_addr, reg, (uint8_t *)val, 2)
	    < 0) {
		return -EIO;
	}

	*val = sys_be16_to_cpu(*val);

	return 0;
}

/**
 * @brief Check the Device ID
 *
 * @param[in]   dev     Pointer to the device structure
 *
 * @retval 0 On success
 * @retval -EIO Otherwise
 */
static inline int tmp116_device_id_check(struct device *dev)
{
	uint16_t value;

	if (tmp116_reg_read(dev, TMP116_REG_DEVICE_ID, &value) != 0) {
		LOG_ERR("%s: Failed to get Device ID register!",
			DT_INST_LABEL(0));
		return -EIO;
	}

	if ((value != TMP116_DEVICE_ID) && (value != TMP117_DEVICE_ID)) {
		LOG_ERR("%s: Failed to match the device IDs!",
			DT_INST_LABEL(0));
		return -EINVAL;
	}

	return 0;
}

static int tmp116_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct tmp116_data *drv_data = dev->data;
	uint16_t value;
	int rc;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_AMBIENT_TEMP);

	/* clear sensor values */
	drv_data->sample = 0U;

	/* Get the most recent temperature measurement */
	rc = tmp116_reg_read(dev, TMP116_REG_TEMP, &value);
	if (rc < 0) {
		LOG_ERR("%s: Failed to read from TEMP register!",
			DT_INST_LABEL(0));
		return rc;
	}

	/* store measurements to the driver */
	drv_data->sample = (int16_t)value;

	return 0;
}

static int tmp116_channel_get(struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct tmp116_data *drv_data = dev->data;
	int32_t tmp;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	/*
	 * See datasheet "Temperature Results and Limits" section for more
	 * details on processing sample data.
	 */
	tmp = ((int16_t)drv_data->sample * (int32_t)TMP116_RESOLUTION) / 10;
	val->val1 = tmp / 1000000; /* uCelsius */
	val->val2 = tmp % 1000000;

	return 0;
}

static const struct sensor_driver_api tmp116_driver_api = {
	.sample_fetch = tmp116_sample_fetch,
	.channel_get = tmp116_channel_get
};

static int tmp116_init(struct device *dev)
{
	struct tmp116_data *drv_data = dev->data;
	int rc;

	/* Bind to the I2C bus that the sensor is connected */
	drv_data->i2c = device_get_binding(DT_INST_BUS_LABEL(0));
	if (!drv_data->i2c) {
		LOG_ERR("Cannot bind to %s device!",
			DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}

	/* Check the Device ID */
	rc = tmp116_device_id_check(dev);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

static struct tmp116_data tmp116_data;

static const struct tmp116_dev_config tmp116_config = {
	.i2c_addr = DT_INST_REG_ADDR(0),
};

DEVICE_AND_API_INIT(hdc1080, DT_INST_LABEL(0), tmp116_init,
		    &tmp116_data, &tmp116_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &tmp116_driver_api);
