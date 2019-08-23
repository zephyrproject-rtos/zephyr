/*
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>
#include <logging/log.h>
#include <kernel.h>

#include "tmp116.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(TMP116);

static int tmp116_reg_read(struct device *dev, u8_t reg, u16_t *val)
{
	struct tmp116_data *drv_data = dev->driver_data;
	const struct tmp116_dev_config *cfg = dev->config->config_info;

	if (i2c_burst_read(drv_data->i2c, cfg->i2c_addr, reg, (u8_t *)val, 2)
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
	u16_t value;

	if (tmp116_reg_read(dev, TMP116_REG_DEVICE_ID, &value) != 0) {
		LOG_ERR("%s: Failed to get Device ID register!",
			DT_INST_0_TI_TMP116_LABEL);
		return -EIO;
	}

	if (value != TMP116_DEVICE_ID) {
		LOG_ERR("%s: Failed to match the device IDs!",
			DT_INST_0_TI_TMP116_LABEL);
		return -EINVAL;
	}

	return 0;
}

static int tmp116_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct tmp116_data *drv_data = dev->driver_data;
	u16_t value;
	int rc;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_AMBIENT_TEMP);

	/* clear sensor values */
	drv_data->sample = 0U;

	/* Get the most recent temperature measurement */
	rc = tmp116_reg_read(dev, TMP116_REG_TEMP, &value);
	if (rc < 0) {
		LOG_ERR("%s: Failed to read from TEMP register!",
			DT_INST_0_TI_TMP116_LABEL);
		return rc;
	}

	/* store measurements to the driver */
	drv_data->sample = (s16_t)value;

	return 0;
}

static int tmp116_channel_get(struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct tmp116_data *drv_data = dev->driver_data;
	s32_t tmp;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	/*
	 * See datasheet "Temperature Results and Limits" section for more
	 * details on processing sample data.
	 */
	tmp = (s32_t)drv_data->sample * TMP116_RESOLUTION;
	val->val1 = tmp / 10000000; /* Tens of uCelsius */
	val->val2 = tmp % 10000000;

	return 0;
}

static const struct sensor_driver_api tmp116_driver_api = {
	.sample_fetch = tmp116_sample_fetch,
	.channel_get = tmp116_channel_get
};

static int tmp116_init(struct device *dev)
{
	struct tmp116_data *drv_data = dev->driver_data;
	int rc;

	/* Bind to the I2C bus that the sensor is connected */
	drv_data->i2c = device_get_binding(DT_INST_0_TI_TMP116_BUS_NAME);
	if (!drv_data->i2c) {
		LOG_ERR("Cannot bind to %s device!",
			DT_INST_0_TI_TMP116_BUS_NAME);
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
	.i2c_addr = DT_INST_0_TI_TMP116_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(hdc1080, DT_INST_0_TI_TMP116_LABEL, tmp116_init,
		    &tmp116_data, &tmp116_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &tmp116_driver_api);
