/* ST Microelectronics STTS22H temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/stts22h.pdf
 */

#define DT_DRV_COMPAT st_stts22h

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "stts22h.h"

LOG_MODULE_REGISTER(STTS22H, CONFIG_SENSOR_LOG_LEVEL);

static inline int stts22h_set_odr_raw(const struct device *dev, uint8_t odr)
{
	struct stts22h_data *data = dev->data;

	switch (odr) {

	case ODR_1HZ:
		return stts22h_temp_data_rate_set(data->ctx, STTS22H_1Hz);
	case ODR_25HZ:
		return stts22h_temp_data_rate_set(data->ctx, STTS22H_25Hz);
	case ODR_50HZ:
		return stts22h_temp_data_rate_set(data->ctx, STTS22H_50Hz);
	case ODR_100HZ:
		return stts22h_temp_data_rate_set(data->ctx, STTS22H_100Hz);
	case ODR_200HZ:
		return stts22h_temp_data_rate_set(data->ctx, STTS22H_200Hz);
	default:
		return -EINVAL;
	}
}

static int stts22h_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret = 0;
	struct stts22h_data *data = dev->data;
	int16_t raw_temp;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (stts22h_temperature_raw_get(data->ctx, &raw_temp) < 0) {
		LOG_ERR("Failed to read sample");
		return -EIO;
	}

	data->sample_temp = raw_temp;

	return ret;
}

static inline void stts22h_temp_convert(struct sensor_value *val, int16_t raw_val)
{
	val->val1 = raw_val / 100;
	val->val2 = ((int32_t)raw_val % 100) * 10000;
}

static int stts22h_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct stts22h_data *data = dev->data;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		stts22h_temp_convert(val, data->sample_temp);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int stts22h_odr_set(const struct device *dev, const struct sensor_value *val)
{

	if (stts22h_set_odr_raw(dev, val->val1) < 0) {
		LOG_DBG("failed to set sampling rate");
		return -EIO;
	}

	return 0;
}

static int stts22h_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return stts22h_odr_set(dev, val);
	default:
		LOG_DBG("operation not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api stts22h_api_funcs = {
	.attr_set = stts22h_attr_set,
	.sample_fetch = stts22h_sample_fetch,
	.channel_get = stts22h_channel_get,
#if CONFIG_STTS22H_TRIGGER
	.trigger_set = stts22h_trigger_set,
#endif
};

static int stts22h_init_chip(const struct device *dev)
{
	struct stts22h_data *data = dev->data;
	uint8_t chip_id;

	if (stts22h_dev_id_get(data->ctx, &chip_id) < 0) {
		LOG_ERR("Failed reading chip id");
		return -EIO;
	}
	LOG_DBG("Sensor Chip ID: %02X\n", chip_id);

	if (stts22h_set_odr_raw(dev, CONFIG_STTS22H_SAMPLING_RATE) < 0) {
		LOG_ERR("Failed to set sampling rate");
		return -EIO;
	}

	stts22h_dev_status_t status;

	if (stts22h_dev_status_get(data->ctx, &status) == 0) {
		printk("Dev Status : %d\n", status.busy);
	}

	return 0;
}

static int stts22h_init(const struct device *dev)
{
	const struct stts22h_config *const config = dev->config;
	struct stts22h_data *data = dev->data;

	data->dev = dev;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	config->bus_init(dev);

	if (stts22h_init_chip(dev) < 0) {
		LOG_DBG("Failed to initialize chip");
		return -EIO;
	}
	LOG_DBG("Sensor Initialized...\n");
#ifdef CONFIG_STTS22H_TRIGGER
	if (config->int_gpio.port) {
		if (stts22h_init_interrupt(dev) < 0) {
			LOG_ERR("Failed to initialize interrupt.");
			return -EIO;
		}
	}
#endif

	return 0;
}

#define STTS22H_DEFINE(inst)                                                                       \
	static struct stts22h_data stts22h_data_##inst;                                            \
                                                                                                   \
	static const struct stts22h_config stts22h_config_##inst = {COND_CODE_1(                   \
		DT_INST_ON_BUS(inst, i2c),                                                         \
		(.i2c = I2C_DT_SPEC_INST_GET(inst), .bus_init = stts22h_i2c_init,),               \
		()) IF_ENABLED(CONFIG_STTS22H_TRIGGER,                                             \
			       (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, drdy_gpios, {0}),))};  \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, stts22h_init, NULL, &stts22h_data_##inst,               \
				     &stts22h_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &stts22h_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(STTS22H_DEFINE)
