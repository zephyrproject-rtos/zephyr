/*
 * Copyright 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arduino_modulino_smartleds

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(modulino_smartleds, CONFIG_LED_STRIP_LOG_LEVEL);

#define MODULINO_SMARTLEDS_NUM_LEDS 8

/* This is a strip of LC8822 driven by the microcontroller on the Modulino
 * board, the start frame is sent automatically, the rest uses the LC8822
 * protocol:
 * - 4x "1" marker bits
 * - 5x brightness bits
 * - 3x bytes for B, G, R
 */

#define MODULINO_SMARTLEDS_MARKER (0xe0 << 24)
#define MODULINO_SMARTLEDS_FULL_BRIGHTNESS (0x1f << 24)

struct modulino_smartleds_config {
	struct i2c_dt_spec bus;
};

struct modulino_smartleds_data {
	uint32_t buf[MODULINO_SMARTLEDS_NUM_LEDS];
};

static int modulino_smartleds_update_rgb(const struct device *dev,
					 struct led_rgb *pixels,
					 size_t count)
{
	const struct modulino_smartleds_config *cfg = dev->config;
	struct modulino_smartleds_data *data = dev->data;
	int ret;

	if (count > MODULINO_SMARTLEDS_NUM_LEDS) {
		return -EINVAL;
	}

	for (uint8_t i = 0; i < count; i++) {
		data->buf[i] = sys_cpu_to_be32(
				MODULINO_SMARTLEDS_MARKER |
				MODULINO_SMARTLEDS_FULL_BRIGHTNESS |
				(pixels[i].b << 16) |
				(pixels[i].g << 8) |
				pixels[i].r);
	}

	ret = i2c_write_dt(&cfg->bus, (uint8_t *)data->buf, sizeof(data->buf));
	if (ret < 0) {
		LOG_ERR("i2c write error: %d", ret);
		return ret;
	}

	return 0;
}

static size_t modulino_smartleds_length(const struct device *dev)
{
	return MODULINO_SMARTLEDS_NUM_LEDS;
}

static int modulino_smartleds_init(const struct device *dev)
{
	const struct modulino_smartleds_config *cfg = dev->config;
	struct modulino_smartleds_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(data->buf); i++) {
		data->buf[i] = sys_cpu_to_be32(MODULINO_SMARTLEDS_MARKER);
	}

	/* Reset to all LEDs off */
	ret = i2c_write_dt(&cfg->bus, (uint8_t *)data->buf, sizeof(data->buf));
	if (ret < 0) {
		LOG_ERR("i2c write error: %d", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(led_strip, modulino_smartleds_api) = {
	.update_rgb = modulino_smartleds_update_rgb,
	.length = modulino_smartleds_length,
};

#define MODULINO_SMARTLEDS_INIT(inst)						\
	static const struct modulino_smartleds_config				\
	modulino_smartleds_cfg_##inst = {					\
		.bus = I2C_DT_SPEC_INST_GET(inst),				\
	};									\
										\
	static struct modulino_smartleds_data modulino_smartleds_data_##inst;	\
										\
	DEVICE_DT_INST_DEFINE(inst, modulino_smartleds_init, NULL,		\
			      &modulino_smartleds_data_##inst,			\
			      &modulino_smartleds_cfg_##inst,			\
			      POST_KERNEL, CONFIG_LED_STRIP_INIT_PRIORITY,	\
			      &modulino_smartleds_api);


DT_INST_FOREACH_STATUS_OKAY(MODULINO_SMARTLEDS_INIT)
