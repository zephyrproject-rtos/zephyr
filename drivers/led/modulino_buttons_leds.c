/*
 * Copyright 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arduino_modulino_buttons_leds

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(modulino_buttons_leds, CONFIG_LED_LOG_LEVEL);

#define MODULINO_BUTTONS_NUM_LEDS 3

struct modulino_buttons_leds_config {
	struct i2c_dt_spec bus;
};

struct modulino_buttons_leds_data {
	uint8_t buf[MODULINO_BUTTONS_NUM_LEDS];
};

static int modulino_buttons_leds_set(const struct device *dev,
				     uint32_t led, bool value)
{
	const struct modulino_buttons_leds_config *cfg = dev->config;
	struct modulino_buttons_leds_data *data = dev->data;
	int ret;

	if (led >= MODULINO_BUTTONS_NUM_LEDS) {
		return -EINVAL;
	}

	data->buf[led] = value ? 1 : 0;

	ret = i2c_write_dt(&cfg->bus, data->buf, sizeof(data->buf));
	if (ret < 0) {
		LOG_ERR("i2c write error: %d", ret);
		return ret;
	}

	return 0;
}

static int modulino_buttons_leds_on(const struct device *dev, uint32_t led)
{
	return modulino_buttons_leds_set(dev, led, true);
}

static int modulino_buttons_leds_off(const struct device *dev, uint32_t led)
{
	return modulino_buttons_leds_set(dev, led, false);
}

static int modulino_buttons_leds_set_brightness(const struct device *dev,
						uint32_t led, uint8_t value)
{
	return modulino_buttons_leds_set(dev, led, value > 0);
}

static int modulino_buttons_leds_init(const struct device *dev)
{
	const struct modulino_buttons_leds_config *cfg = dev->config;
	struct modulino_buttons_leds_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	/* Reset to all LEDs off */
	ret = i2c_write_dt(&cfg->bus, data->buf, sizeof(data->buf));
	if (ret < 0) {
		LOG_ERR("i2c write error: %d", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(led, modulino_buttons_leds_api) = {
	.on = modulino_buttons_leds_on,
	.off = modulino_buttons_leds_off,
	.set_brightness = modulino_buttons_leds_set_brightness,
};

#define MODULINO_BUTTONS_INIT(inst)							\
	static const struct modulino_buttons_leds_config				\
	modulino_buttons_leds_cfg_##inst = {						\
		.bus = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),				\
	};										\
											\
	static struct modulino_buttons_leds_data modulino_buttons_leds_data_##inst;	\
											\
	DEVICE_DT_INST_DEFINE(inst, modulino_buttons_leds_init, NULL,			\
			      &modulino_buttons_leds_data_##inst,			\
			      &modulino_buttons_leds_cfg_##inst,			\
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,			\
			      &modulino_buttons_leds_api);


DT_INST_FOREACH_STATUS_OKAY(MODULINO_BUTTONS_INIT)
