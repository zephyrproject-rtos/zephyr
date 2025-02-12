/*
 * Copyright (c) 2024 Fredrik Gihl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp114

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(TMP114, CONFIG_SENSOR_LOG_LEVEL);

#define TMP114_REG_TEMP		0x0
#define TMP114_REG_ALERT	0x2
#define TMP114_REG_CFGR		0x3
#define TMP114_REG_DEVICE_ID	0xb

#define TMP114_RESOLUTION	78125       /* in tens of uCelsius*/
#define TMP114_RESOLUTION_DIV	10000000

#define TMP114_DEVICE_ID	0x1114

#define TMP114_ALERT_DATA_READY  BIT(0)

struct tmp114_data {
	uint16_t sample;
	uint16_t id;
};

struct tmp114_dev_config {
	struct i2c_dt_spec bus;
};

static int tmp114_reg_read(const struct device *dev, uint8_t reg,
			   uint16_t *val)
{
	const struct tmp114_dev_config *cfg = dev->config;

	if (i2c_burst_read_dt(&cfg->bus, reg, (uint8_t *)val, 2)
	    < 0) {
		return -EIO;
	}

	*val = sys_be16_to_cpu(*val);

	return 0;
}

static inline int tmp114_device_id_check(const struct device *dev, uint16_t *id)
{
	if (tmp114_reg_read(dev, TMP114_REG_DEVICE_ID, id) != 0) {
		LOG_ERR("%s: Failed to get Device ID register!", dev->name);
		return -EIO;
	}

	if (*id != TMP114_DEVICE_ID) {
		LOG_ERR("%s: Failed to match the device ID!", dev->name);
		return -EINVAL;
	}

	return 0;
}

static int tmp114_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct tmp114_data *drv_data = dev->data;
	uint16_t value;
	uint16_t cfg_reg = 0;
	int rc;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_AMBIENT_TEMP);

	/* clear sensor values */
	drv_data->sample = 0U;

	/* Check alert register to make sure that a data is available */
	rc = tmp114_reg_read(dev, TMP114_REG_ALERT, &cfg_reg);
	if (rc < 0) {
		LOG_ERR("%s, Failed to read from CFGR register", dev->name);
		return rc;
	}

	if ((cfg_reg & TMP114_ALERT_DATA_READY) == 0) {
		LOG_DBG("%s: no data ready", dev->name);
		return -EBUSY;
	}

	/* Get the most recent temperature measurement */
	rc = tmp114_reg_read(dev, TMP114_REG_TEMP, &value);
	if (rc < 0) {
		LOG_ERR("%s: Failed to read from TEMP register!", dev->name);
		return rc;
	}

	/* store measurements to the driver */
	drv_data->sample = (int16_t)value;

	return 0;
}

static int tmp114_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct tmp114_data *drv_data = dev->data;
	int32_t tmp;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	/*
	 * See datasheet for tmp114, section 'Temp_Result Register' section
	 * for more details on processing sample data.
	 */
	tmp = ((int16_t)drv_data->sample * (int32_t)TMP114_RESOLUTION) / 10;
	val->val1 = tmp / 1000000; /* uCelsius */
	val->val2 = tmp % 1000000;

	return 0;
}

static int tmp114_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	uint16_t data;
	int rc;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		rc = tmp114_reg_read(dev, TMP114_REG_CFGR, &data);
		if (rc < 0) {
			return rc;
		}
		break;
	default:
		return -ENOTSUP;
	}

	val->val1 = data;
	val->val2 = 0;

	return 0;
}

static const struct sensor_driver_api tmp114_driver_api = {
	.attr_get = tmp114_attr_get,
	.sample_fetch = tmp114_sample_fetch,
	.channel_get = tmp114_channel_get
};

static int tmp114_init(const struct device *dev)
{
	struct tmp114_data *drv_data = dev->data;
	const struct tmp114_dev_config *cfg = dev->config;
	int rc;
	uint16_t id;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C dev %s not ready", cfg->bus.bus->name);
		return -EINVAL;
	}

	/* Check the Device ID */
	rc = tmp114_device_id_check(dev, &id);
	if (rc < 0) {
		return rc;
	}
	LOG_INF("Got device ID: %x", id);
	drv_data->id = id;

	return 0;
}

#define DEFINE_TMP114(_num) \
	static struct tmp114_data tmp114_data_##_num; \
	static const struct tmp114_dev_config tmp114_config_##_num = { \
		.bus = I2C_DT_SPEC_INST_GET(_num) \
	}; \
	SENSOR_DEVICE_DT_INST_DEFINE(_num, tmp114_init, NULL, \
		&tmp114_data_##_num, &tmp114_config_##_num, POST_KERNEL, \
		CONFIG_SENSOR_INIT_PRIORITY, &tmp114_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_TMP114)
