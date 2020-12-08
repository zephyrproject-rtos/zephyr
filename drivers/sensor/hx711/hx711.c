/*
 * Copyright (c) 2020 Kalyan Sriram <kalyan@coderkalyan.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT avia_hx711

#include <init.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include "hx711.h"

LOG_MODULE_REGISTER(HX711, CONFIG_SENSOR_LOG_LEVEL);

static void hx711_convert_data(struct sensor_value *val, double raw_val,
		double offset, double scale)
{
	double conv_val = (raw_val * scale) - offset;

	val->val1 = (int32_t) conv_val;
	val->val2 = (conv_val - ((int32_t) conv_val)) * 1e6;
}

static void hx711_convert_data_raw(struct sensor_value *val, double raw_val)
{
	val->val1 = (int32_t) raw_val;
	val->val2 = (raw_val - ((int32_t) raw_val)) * 1e6;
}

static int hx711_channel_get(const struct device *dev,
		enum sensor_channel chan,
		struct sensor_value *val)
{
	struct hx711_data *drv_data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_FORCE:
		hx711_convert_data(val, drv_data->data,
				drv_data->offset, drv_data->scale);
		break;
	case SENSOR_CHAN_FORCE_RAW:
		hx711_convert_data_raw(val, drv_data->data);
		break;
	default:
		hx711_convert_data(val, drv_data->data,
				drv_data->offset, drv_data->scale);
		break;
	}

	return 0;
}

static int hx711_attr_set(const struct device *dev,
		enum sensor_channel chan,
		enum sensor_attribute attr,
		const struct sensor_value *val)
{
	struct hx711_data *drv_data = dev->data;

	switch (attr) {
	case SENSOR_ATTR_OFFSET:
		drv_data->offset = sensor_value_to_double(val);
		break;
	case SENSOR_ATTR_MULTIPLIER:
		drv_data->scale = sensor_value_to_double(val);
		break;
	default:
		drv_data->offset = sensor_value_to_double(val);
		break;
	}

	return 0;
}

static int hx711_sample_fetch(const struct device *dev,
		enum sensor_channel chan)
{
	struct hx711_data *drv_data = dev->data;
	const struct hx711_config *cfg = dev->config;
	uint8_t raw[4];
	int32_t data = 0;

	/* wait until the sensor is ready */
	bool ready = false;

	do {
		int ret = gpio_pin_get(drv_data->dout, cfg->dout_pin);

		if (ret < 0) {
			LOG_ERR("Failed to read data sample.");
			return -EIO;
		}
		ready = (ret == 0);
	} while (!ready);

	/* shift out 24 bits of data */
	for (int i = 2; i >= 0; i--) {
		for (int j = 0; j < 8; j++) {
			gpio_pin_set(drv_data->sck, cfg->sck_pin, 1);
			gpio_pin_set(drv_data->sck, cfg->sck_pin, 0);

			raw[i] <<= 1;
			int ret = gpio_pin_get(drv_data->dout, cfg->dout_pin);

			if (ret < 0) {
				LOG_ERR("Failed to read data sample.");
				return -EIO;
			}
			raw[i] += ret;
		}
	}

	/* now set the gain for the next read */
	uint8_t gain;

	switch (CONFIG_HX711_GAIN) {
	case 128:
		gain = 1;
		break;
	case 64:
		gain = 3;
		break;
	case 32:
		gain = 2;
		break;
	default:
		LOG_ERR("Invalid value for sensor input gain.");
		return -EINVAL;
	}

	for (int i = 0; i < gain; i++) {
		/* we're not reading anything, just pulse the clock
		 * to indicate the gain configuration
		 */
		gpio_pin_set(drv_data->sck, cfg->sck_pin, 1);
		gpio_pin_set(drv_data->sck, cfg->sck_pin, 0);
	}

	/* pad out 32-bit signed integer */
	if (raw[2] & 0x80) {
		raw[3] = 0xFF;
	} else {
		raw[3] = 0x00;
	}

	data = (raw[3] << 24) | (raw[2] << 16) | (raw[1] << 8) | (raw[0]);
	drv_data->data = (double) data;

	return 0;
}

static const struct sensor_driver_api hx711_driver_api = {
#if CONFIG_HX711_TRIGGER
	.trigger_set = hx711_trigger_set,
#endif
	.attr_set = hx711_attr_set,
	.sample_fetch = hx711_sample_fetch,
	.channel_get = hx711_channel_get,
};

int hx711_init(const struct device *dev)
{
	struct hx711_data *drv_data = dev->data;
	const struct hx711_config *cfg = dev->config;

	drv_data->sck = device_get_binding(cfg->sck_label);
	if (drv_data->sck == NULL) {
		LOG_ERR("Failed to get pointer to %s device",
				cfg->sck_label);
		return -EINVAL;
	}

	drv_data->dout = device_get_binding(cfg->dout_label);
	if (drv_data->dout == NULL) {
		LOG_ERR("Failed to get pointer to %s device",
				cfg->dout_label);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->sck, cfg->sck_pin,
			GPIO_OUTPUT | cfg->sck_flags);

	gpio_pin_configure(drv_data->dout, cfg->dout_pin,
			GPIO_INPUT | cfg->dout_flags);

	drv_data->data = 0.0;
	drv_data->offset = 0.0;
	drv_data->scale = 1.0;

#ifdef CONFIG_HX711_TRIGGER
	if (hx711_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupts.");
		return -EIO;
	}
#endif

	/* default input selection is channel A, gain 128
	 * we need to shift out one "read" operation in 
	 * order to be able to set the gain
	 */
	hx711_sample_fetch(dev, SENSOR_CHAN_FORCE);

	return 0;
}

static struct hx711_data hx711_driver;
static const struct hx711_config hx711_cfg = {
	.sck_label = DT_INST_GPIO_LABEL(0, sck_gpios),
	.sck_pin = DT_INST_GPIO_PIN(0, sck_gpios),
	.sck_flags = DT_INST_GPIO_FLAGS(0, sck_gpios),
	.dout_label = DT_INST_GPIO_LABEL(0, dout_gpios),
	.dout_pin = DT_INST_GPIO_PIN(0, dout_gpios),
	.dout_flags = DT_INST_GPIO_FLAGS(0, dout_gpios),
};

DEVICE_AND_API_INIT(hx711, DT_INST_LABEL(0),
		hx711_init, &hx711_driver, &hx711_cfg,
		POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		&hx711_driver_api);
