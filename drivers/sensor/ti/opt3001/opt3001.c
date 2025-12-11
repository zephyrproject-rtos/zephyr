/*
 * Copyright (c) 2019 Actinius
 * Copyright (c) 2025 Bang & Olufsen A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_opt3001

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "opt3001.h"

LOG_MODULE_REGISTER(opt3001, CONFIG_SENSOR_LOG_LEVEL);

int opt3001_reg_read(const struct device *dev, uint8_t reg,
			    uint16_t *val)
{
	const struct opt3001_config *config = dev->config;
	uint8_t value[2];

	if (i2c_burst_read_dt(&config->i2c, reg, value, 2) != 0) {
		return -EIO;
	}

	*val = ((uint16_t)value[0] << 8) + value[1];

	return 0;
}

static int opt3001_reg_write(const struct device *dev, uint8_t reg,
			     uint16_t val)
{
	const struct opt3001_config *config = dev->config;
	uint8_t new_value[2];

	new_value[0] = val >> 8;
	new_value[1] = val & 0xff;

	uint8_t tx_buf[3] = { reg, new_value[0], new_value[1] };

	return i2c_write_dt(&config->i2c, tx_buf, sizeof(tx_buf));
}

int opt3001_reg_update(const struct device *dev, uint8_t reg,
			      uint16_t mask, uint16_t val)
{
	uint16_t old_val;
	uint16_t new_val;

	if (opt3001_reg_read(dev, reg, &old_val) != 0) {
		return -EIO;
	}

	new_val = old_val & ~mask;
	new_val |= val & mask;

	return opt3001_reg_write(dev, reg, new_val);
}

static int opt3001_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct opt3001_data *drv_data = dev->data;
	uint16_t value;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT);

	drv_data->sample = 0U;

	if (opt3001_reg_read(dev, OPT3001_REG_RESULT, &value) != 0) {
		return -EIO;
	}

	drv_data->sample = value;

	return 0;
}

static int opt3001_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct opt3001_data *drv_data = dev->data;
	uint16_t sample = drv_data->sample;
	int32_t uval;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	/**
	 * sample consists of 4 bits of exponent and 12 bits of mantissa
	 * bits 15 to 12 are exponent bits
	 * bits 11 to 0 are the mantissa bits
	 *
	 * lux is the integer obtained using the following formula:
	 * (2^(exponent value)) * 0.01 * mantissa value
	 */
	uval = (1 << (sample >> OPT3001_SAMPLE_EXPONENT_SHIFT)) * (sample & OPT3001_MANTISSA_MASK);
	val->val1 = uval / 100;
	val->val2 = (uval % 100) * 10000;

	return 0;
}

static DEVICE_API(sensor, opt3001_driver_api) = {
#ifdef CONFIG_OPT3001_TRIGGER
	.trigger_set = opt3001_trigger_set,
#endif
	.sample_fetch = opt3001_sample_fetch,
	.channel_get = opt3001_channel_get,
};

static int opt3001_chip_init(const struct device *dev)
{
	const struct opt3001_config *config = dev->config;
	uint16_t value;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	if (opt3001_reg_read(dev, OPT3001_REG_MANUFACTURER_ID, &value) != 0) {
		return -EIO;
	}

	if (value != OPT3001_MANUFACTURER_ID_VALUE) {
		LOG_ERR("Bad manufacturer id 0x%x", value);
		return -ENOTSUP;
	}

	if (opt3001_reg_read(dev, OPT3001_REG_DEVICE_ID, &value) != 0) {
		return -EIO;
	}

	if (value != OPT3001_DEVICE_ID_VALUE) {
		LOG_ERR("Bad device id 0x%x", value);
		return -ENOTSUP;
	}

	if (opt3001_reg_update(dev, OPT3001_REG_CONFIG,
			       OPT3001_CONVERSION_MODE_MASK,
			       OPT3001_CONVERSION_MODE_CONTINUOUS) != 0) {
		LOG_ERR("Failed to set mode to continuous conversion");
		return -EIO;
	}

	return 0;
}

int opt3001_init(const struct device *dev)
{
	if (opt3001_chip_init(dev) < 0) {
		return -EINVAL;
	}

#ifdef CONFIG_OPT3001_TRIGGER
	if (opt3001_init_interrupt(dev)) {
		LOG_ERR("Failed to initialize interrupt");
		return -EIO;
	}
#endif

	return 0;
}

#define OPT3001_DEFINE(inst)									\
	static struct opt3001_data opt3001_data_##inst;						\
												\
	static const struct opt3001_config opt3001_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		IF_ENABLED(CONFIG_OPT3001_TRIGGER,						\
			(.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),))		\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, opt3001_init, NULL,					\
			      &opt3001_data_##inst, &opt3001_config_##inst, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY, &opt3001_driver_api);		\

DT_INST_FOREACH_STATUS_OKAY(OPT3001_DEFINE)
