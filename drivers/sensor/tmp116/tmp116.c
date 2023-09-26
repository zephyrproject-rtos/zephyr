/*
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp116

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/tmp116.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "tmp116.h"

#define  EEPROM_SIZE_REG sizeof(uint16_t)
#define  EEPROM_TMP117_RESERVED (2 * sizeof(uint16_t))
#define  EEPROM_MIN_BUSY_MS 7

LOG_MODULE_REGISTER(TMP116, CONFIG_SENSOR_LOG_LEVEL);

static int tmp116_reg_read(const struct device *dev, uint8_t reg,
			   uint16_t *val)
{
	const struct tmp116_dev_config *cfg = dev->config;

	if (i2c_burst_read_dt(&cfg->bus, reg, (uint8_t *)val, 2)
	    < 0) {
		return -EIO;
	}

	*val = sys_be16_to_cpu(*val);

	return 0;
}

static int tmp116_reg_write(const struct device *dev, uint8_t reg,
			    uint16_t val)
{
	const struct tmp116_dev_config *cfg = dev->config;
	uint8_t tx_buf[3] = {reg, val >> 8, val & 0xFF};

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static bool check_eeprom_bounds(const struct device *dev, off_t offset,
			       size_t len)
{
	struct tmp116_data *drv_data = dev->data;

	if ((offset + len) > EEPROM_TMP116_SIZE ||
	    offset % EEPROM_SIZE_REG != 0 ||
	    len % EEPROM_SIZE_REG != 0) {
		return false;
	}

	/* TMP117 uses EEPROM[2] as temperature offset register */
	if (drv_data->id == TMP117_DEVICE_ID &&
	    offset <= EEPROM_TMP117_RESERVED &&
	    (offset + len) > EEPROM_TMP117_RESERVED) {
		return false;
	}

	return true;
}

int tmp116_eeprom_write(const struct device *dev, off_t offset,
			const void *data, size_t len)
{
	uint8_t reg;
	const uint16_t *src = data;
	int res;

	if (!check_eeprom_bounds(dev, offset, len)) {
		return -EINVAL;
	}

	res = tmp116_reg_write(dev, TMP116_REG_EEPROM_UL, TMP116_EEPROM_UL_UNLOCK);
	if (res) {
		return res;
	}

	for (reg = (offset / 2); reg < offset / 2 + len / 2; reg++) {
		uint16_t val = *src;

		res = tmp116_reg_write(dev, reg + TMP116_REG_EEPROM1, val);
		if (res != 0) {
			break;
		}

		k_sleep(K_MSEC(EEPROM_MIN_BUSY_MS));

		do {
			res = tmp116_reg_read(dev, TMP116_REG_EEPROM_UL, &val);
			if (res != 0) {
				break;
			}
		} while (val & TMP116_EEPROM_UL_BUSY);
		src++;

		if (res != 0) {
			break;
		}
	}

	res = tmp116_reg_write(dev, TMP116_REG_EEPROM_UL, 0);

	return res;
}

int tmp116_eeprom_read(const struct device *dev, off_t offset, void *data,
		       size_t len)
{
	uint8_t reg;
	uint16_t *dst = data;
	int res = 0;

	if (!check_eeprom_bounds(dev, offset, len)) {
		return -EINVAL;
	}

	for (reg = (offset / 2); reg < offset / 2 + len / 2; reg++) {
		res = tmp116_reg_read(dev, reg + TMP116_REG_EEPROM1, dst);
		if (res != 0) {
			break;
		}
		dst++;
	}

	return res;
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
	uint16_t cfg_reg = 0;
	int rc;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_AMBIENT_TEMP);

	/* clear sensor values */
	drv_data->sample = 0U;

	/* Make sure that a data is available */
	rc = tmp116_reg_read(dev, TMP116_REG_CFGR, &cfg_reg);
	if (rc < 0) {
		LOG_ERR("%s, Failed to read from CFGR register",
			dev->name);
		return rc;
	}

	if ((cfg_reg & TMP116_CFGR_DATA_READY) == 0) {
		LOG_DBG("%s: no data ready", dev->name);
		return -EBUSY;
	}

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

static int tmp116_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	uint16_t data;
	int rc;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		rc = tmp116_reg_read(dev, TMP116_REG_CFGR, &data);
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

static const struct sensor_driver_api tmp116_driver_api = {
	.attr_set = tmp116_attr_set,
	.attr_get = tmp116_attr_get,
	.sample_fetch = tmp116_sample_fetch,
	.channel_get = tmp116_channel_get
};

static int tmp116_init(const struct device *dev)
{
	struct tmp116_data *drv_data = dev->data;
	const struct tmp116_dev_config *cfg = dev->config;
	int rc;
	uint16_t id;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("I2C dev %s not ready", cfg->bus.bus->name);
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
		.bus = I2C_DT_SPEC_INST_GET(_num) \
	}; \
	SENSOR_DEVICE_DT_INST_DEFINE(_num, tmp116_init, NULL, \
		&tmp116_data_##_num, &tmp116_config_##_num, POST_KERNEL, \
		CONFIG_SENSOR_INIT_PRIORITY, &tmp116_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_TMP116)
