/*
 * Copyright (c) 2024 Fredrik Gihl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp114

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/tmp114.h>
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

#define TMP114_ALERT_DATA_READY BIT(0)
#define TMP114_CFGR_AVG		BIT(7)
#define TMP114_AVG		BIT(7)
#define TMP114_CFGR_CONV	(BIT(0) | BIT(1) | BIT(2))

struct tmp114_data {
	uint16_t sample;
	uint16_t id;
};

struct tmp114_dev_config {
	struct i2c_dt_spec bus;
	uint16_t odr;
	bool oversampling;
};

static int tmp114_reg_read(const struct device *dev, uint8_t reg, uint16_t *val)
{
	const struct tmp114_dev_config *cfg = dev->config;

	if (i2c_burst_read_dt(&cfg->bus, reg, (uint8_t *)val, 2) < 0) {
		return -EIO;
	}

	*val = sys_be16_to_cpu(*val);

	return 0;
}

static int tmp114_reg_write(const struct device *dev, uint8_t reg, uint16_t val)
{
	const struct tmp114_dev_config *cfg = dev->config;
	uint8_t tx_buf[3] = {reg, val >> 8, val & 0xFF};

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static int tmp114_write_config(const struct device *dev, uint16_t mask, uint16_t conf)
{
	uint16_t config = 0;
	int result;

	result = tmp114_reg_read(dev, TMP114_REG_CFGR, &config);

	if (result < 0) {
		return result;
	}

	config &= ~mask;
	config |= conf;

	result = tmp114_reg_write(dev, TMP114_REG_CFGR, config);

	if (result < 0) {
		return result;
	}

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

static int tmp114_channel_get(const struct device *dev, enum sensor_channel chan,
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

static int tmp114_odr_value(const struct sensor_value *frequency, uint16_t *config_value)
{
	const uint32_t freq_micro = sensor_value_to_micro(frequency);

	if (freq_micro <= 500000) {
		*config_value = TMP114_DT_ODR_2000_MS; /* 2 s */
	} else if (freq_micro <= 1000000) {
		*config_value = TMP114_DT_ODR_1000_MS; /* 1 s */
	} else if (freq_micro <= 2000000) {
		*config_value = TMP114_DT_ODR_500_MS; /* 500 ms */
	} else if (freq_micro <= 4000000) {
		*config_value = TMP114_DT_ODR_250_MS; /* 250 ms */
	} else if (freq_micro <= 8000000) {
		*config_value = TMP114_DT_ODR_125_MS; /* 125 ms */
	} else if (freq_micro <= 16000000) {
		*config_value = TMP114_DT_ODR_62_5_MS; /* 62.5 ms */
	} else if (freq_micro <= 32000000) {
		*config_value = TMP114_DT_ODR_31_25_MS; /* 32.25 ms */
	} else if (freq_micro <= 156250000) {
		*config_value = TMP114_DT_ODR_6_4_MS; /* 6.4 ms */
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int tmp114_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	uint16_t value = 0;
	int rc = 0;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_OVERSAMPLING:
		/* Enable the AVG in tmp114. The chip will do 8 avg of 8 samples
		 * to get a more accurate value.
		 */

		if (val->val1) {
			value = TMP114_AVG;
		}

		return tmp114_write_config(dev, TMP114_CFGR_AVG, value);
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		/* Set the output data rate tmp114. */
		rc = tmp114_odr_value(val, &value);
		if (rc < 0) {
			return rc;
		}

		return tmp114_write_config(dev, TMP114_CFGR_CONV, value);
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(sensor, tmp114_driver_api) = {
	.attr_get = tmp114_attr_get,
	.attr_set = tmp114_attr_set,
	.sample_fetch = tmp114_sample_fetch,
	.channel_get = tmp114_channel_get
};

static int tmp114_init(const struct device *dev)
{
	struct tmp114_data *drv_data = dev->data;
	const struct tmp114_dev_config *cfg = dev->config;
	int rc;
	uint16_t id;
	struct sensor_value val;

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

	rc = tmp114_write_config(dev, TMP114_CFGR_CONV, cfg->odr);
	if (rc < 0) {
		return rc;
	}

	val.val1 = cfg->oversampling ? 1 : 0;
	val.val2 = 0;

	rc = tmp114_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_OVERSAMPLING, &val);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

#define DEFINE_TMP114(_num) \
	static struct tmp114_data tmp114_data_##_num; \
	static const struct tmp114_dev_config tmp114_config_##_num = { \
		.bus = I2C_DT_SPEC_INST_GET(_num), \
		.odr = DT_INST_PROP(_num, odr), \
		.oversampling = DT_INST_PROP(_num, oversampling), \
	}; \
	SENSOR_DEVICE_DT_INST_DEFINE(_num, tmp114_init, NULL, \
		&tmp114_data_##_num, &tmp114_config_##_num, POST_KERNEL, \
		CONFIG_SENSOR_INIT_PRIORITY, &tmp114_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_TMP114)
