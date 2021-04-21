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

static int tmp116_reg_read(const struct device *dev, uint8_t reg,
			   uint16_t *val)
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

static int tmp116_reg_write(const struct device *dev, uint8_t reg,
			    uint16_t val)
{
	struct tmp116_data *drv_data = dev->data;
	const struct tmp116_dev_config *cfg = dev->config;
	uint8_t tx_buf[3] = {reg, val >> 8, val & 0xFF};

	return i2c_write(drv_data->i2c, tx_buf, sizeof(tx_buf),
			cfg->i2c_addr);
}

/**
 * @brief Check the Device ID
 *
 * @param[in]   dev     Pointer to the device structure
 * @param[in]	id	Pointer to the variable for storing the device id
 *
 * @retval 0 on success
 * @retval -EIO Otherwise
 */
static inline int tmp116_device_id_check(const struct device *dev, uint16_t *id)
{
	if (tmp116_reg_read(dev, TMP116_REG_DEVICE_ID, id) != 0) {
		LOG_ERR("%s: Failed to get Device ID register!",
			dev->name);
		return -EIO;
	}

	if ((*id != TMP116_DEVICE_ID) && (*id != TMP117_DEVICE_ID)) {
		LOG_ERR("%s: Failed to match the device IDs!",
			dev->name);
		return -EINVAL;
	}

	return 0;
}

static int tmp116_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
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
			dev->name);
		return rc;
	}

	/* store measurements to the driver */
	drv_data->sample = (int16_t)value;

	return 0;
}

static int tmp116_channel_get(const struct device *dev,
			      enum sensor_channel chan,
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

static int tmp116_attr_set(const struct device *dev,
			   enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	struct tmp116_data *drv_data = dev->data;
	int16_t value;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_OFFSET:
		if (drv_data->id != TMP117_DEVICE_ID) {
			LOG_ERR("%s: Offset is only supported by TMP117",
			dev->name);
			return -EINVAL;
		}
		/*
		 * The offset is encoded into the temperature register format.
		 */
		value = (((val->val1) * 10000000) + ((val->val2) * 10))
						/ (int32_t)TMP116_RESOLUTION;

		return tmp116_reg_write(dev, TMP117_REG_TEMP_OFFSET, value);

	default:
		return -ENOTSUP;
	}
}

static const struct sensor_driver_api tmp116_driver_api = {
	.attr_set = tmp116_attr_set,
	.sample_fetch = tmp116_sample_fetch,
	.channel_get = tmp116_channel_get
};

static int tmp116_init(const struct device *dev)
{
	struct tmp116_data *drv_data = dev->data;
	const struct tmp116_dev_config *cfg = dev->config;
	int rc;
	uint16_t id;

	/* Bind to the I2C bus that the sensor is connected */
	drv_data->i2c = device_get_binding(cfg->i2c_bus_label);
	if (!drv_data->i2c) {
		LOG_ERR("Cannot bind to %s device!",
			cfg->i2c_bus_label);
		return -EINVAL;
	}

	/* Check the Device ID */
	rc = tmp116_device_id_check(dev, &id);
	if (rc < 0) {
		return rc;
	}
	LOG_DBG("Got device ID: %x", id);
	drv_data->id = id;

	return 0;
}

#define DEFINE_TMP116(_num) \
	static struct tmp116_data tmp116_data_##_num; \
	static const struct tmp116_dev_config tmp116_config_##_num = { \
		.i2c_addr = DT_INST_REG_ADDR(_num), \
		.i2c_bus_label = DT_INST_BUS_LABEL(_num) \
	}; \
	DEVICE_DT_INST_DEFINE(_num, tmp116_init, device_pm_control_nop, \
		&tmp116_data_##_num, &tmp116_config_##_num, POST_KERNEL, \
		CONFIG_SENSOR_INIT_PRIORITY, &tmp116_driver_api)

DT_INST_FOREACH_STATUS_OKAY(DEFINE_TMP116);
