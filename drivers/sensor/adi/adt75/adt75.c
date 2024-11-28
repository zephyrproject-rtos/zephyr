/*
 * Copyright (c) 2024 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adt75

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>

#include "adt75.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ADT75, CONFIG_SENSOR_LOG_LEVEL);

static int adt75_temp_reg_read(const struct device *dev, uint8_t reg, int16_t *val)
{
	const struct adt75_dev_config *cfg = dev->config;

	if (i2c_burst_read_dt(&cfg->i2c, reg, (uint8_t *)val, 2) < 0) {
		return -EIO;
	}

	*val = sys_be16_to_cpu(*val);
	return 0;
}

static int adt75_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct adt75_data *drv_data = dev->data;
	int16_t value;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

	if (adt75_temp_reg_read(dev, ADT75_REG_TEMPERATURE, &value) < 0) {
		return -EIO;
	}

	drv_data->sample = value >> 4; /* use 12-bit only */
	return 0;
}

static int adt75_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct adt75_data *drv_data = dev->data;
	int32_t value;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	value = (int32_t)drv_data->sample * ADT75_TEMP_SCALE;
	val->val1 = value / 1000000;
	val->val2 = value % 1000000;

	return 0;
}

static const struct sensor_driver_api adt75_driver_api = {
	.sample_fetch = adt75_sample_fetch,
	.channel_get = adt75_channel_get,
};

static int adt75_probe(const struct device *dev)
{
	const struct adt75_dev_config *cfg = dev->config;
	uint8_t value;
	int ret;

	ret = i2c_reg_write_byte_dt(&cfg->i2c, ADT75_REG_CONFIGURATION,
				    ADT75_DEFAULT_CONFIGURATION);
	if (ret) {
		return ret;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, ADT75_REG_CONFIGURATION, &value);
	if (ret) {
		return ret;
	}

	if (value != ADT75_DEFAULT_CONFIGURATION) {
		LOG_INF("%d", value);
		return -ENODEV;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, ADT75_REG_CONFIGURATION,
				    ADT75_CONFIG_OS_SMBUS_ALERT_MODE);

	if (ret) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, ADT75_REG_CONFIGURATION,
				    ADT75_DEFAULT_CONFIGURATION);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adt75_init(const struct device *dev)
{
	const struct adt75_dev_config *cfg = dev->config;

	LOG_INF("Initializing sensor");

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -EINVAL;
	}

	return adt75_probe(dev);
}

#define ADT75_DEFINE(inst)                                                                         \
	static struct adt75_data adt75_data_##inst;                                                \
                                                                                                   \
	static const struct adt75_dev_config adt75_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst)};                                                \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adt75_init, NULL, &adt75_data_##inst,                   \
				     &adt75_config_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &adt75_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADT75_DEFINE)
