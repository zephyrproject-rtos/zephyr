/*
 * Copyright (c) 2023 NXP Semiconductors
 * Copyright (c) 2023 Cognipilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT onnn_ncp5623

/**
 * @file
 * @brief NCP5623 LED driver
 *
 * The NCP5623 is a 3-channel LED driver that communicates over I2C.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ncp5623, CONFIG_LED_LOG_LEVEL);

#define NCP5623_LED_CURRENT 0x20
#define NCP5623_LED_PWM0    0x40
#define NCP5623_LED_PWM1    0x60
#define NCP5623_LED_PWM2    0x80

#define NCP5623_CHANNEL_COUNT 3

/* Brightness limits */
#define NCP5623_MIN_BRIGHTNESS 0
#define NCP5623_MAX_BRIGHTNESS 0x1f

static const uint8_t led_channels[] = {NCP5623_LED_PWM0, NCP5623_LED_PWM1, NCP5623_LED_PWM2};

struct ncp5623_config {
	struct i2c_dt_spec bus;
	uint8_t num_leds;
	const struct led_info *leds_info;
};

static const struct led_info *ncp5623_led_to_info(const struct ncp5623_config *config, uint32_t led)
{
	if (led < config->num_leds) {
		return &config->leds_info[led];
	}

	return NULL;
}

static int ncp5623_get_info(const struct device *dev, uint32_t led, const struct led_info **info)
{
	const struct ncp5623_config *config = dev->config;
	const struct led_info *led_info = ncp5623_led_to_info(config, led);

	if (!led_info) {
		return -EINVAL;
	}

	*info = led_info;

	return 0;
}

static int ncp5623_set_color(const struct device *dev, uint32_t led, uint8_t num_colors,
			     const uint8_t *color)
{
	const struct ncp5623_config *config = dev->config;
	const struct led_info *led_info = ncp5623_led_to_info(config, led);
	uint8_t buf[6] = {0x70, NCP5623_LED_PWM0, 0x70, NCP5623_LED_PWM1, 0x70, NCP5623_LED_PWM2};

	if (!led_info) {
		return -ENODEV;
	}

	if (led_info->num_colors != 3) {
		return -ENOTSUP;
	}
	if (num_colors != 3) {
		return -EINVAL;
	}

	buf[1] = buf[1] | color[0] / 8;
	buf[3] = buf[3] | color[1] / 8;
	buf[5] = buf[5] | color[2] / 8;

	return i2c_burst_write_dt(&config->bus, NCP5623_LED_CURRENT | NCP5623_MAX_BRIGHTNESS, buf,
				  sizeof(buf));
}

static int ncp5623_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct ncp5623_config *config = dev->config;
	const struct led_info *led_info = ncp5623_led_to_info(config, led);
	int ret = 0;

	if (!led_info) {
		return -ENODEV;
	}

	if (value > 100) {
		return -EINVAL;
	}

	if (led_info->num_colors != 1) {
		return -ENOTSUP;
	}

	/* Rescale 0..100 to 0..31 */
	value = value * NCP5623_MAX_BRIGHTNESS / 100;

	ret = i2c_reg_write_byte_dt(&config->bus, led_channels[led] | value, 0x70);

	if (ret < 0) {
		LOG_ERR("%s: LED write failed", dev->name);
	}

	return ret;
}

static inline int ncp5623_led_on(const struct device *dev, uint32_t led)
{
	return ncp5623_set_brightness(dev, led, 100);
}

static inline int ncp5623_led_off(const struct device *dev, uint32_t led)
{
	return ncp5623_set_brightness(dev, led, 0);
}

static int ncp5623_led_init(const struct device *dev)
{
	const struct ncp5623_config *config = dev->config;
	const struct led_info *led_info = NULL;
	int i;
	uint8_t buf[6] = {0x70, NCP5623_LED_PWM0, 0x70, NCP5623_LED_PWM1, 0x70, NCP5623_LED_PWM2};

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("%s: I2C device not ready", dev->name);
		return -ENODEV;
	}

	if (config->num_leds == 1) { /* one three-channel (RGB) LED */
		led_info = ncp5623_led_to_info(config, 0);

		if (!led_info) {
			return -ENODEV;
		}

		if (led_info->num_colors != NCP5623_CHANNEL_COUNT) {
			LOG_ERR("%s: invalid number of colors %d (must be %d with a single LED)",
				dev->name, led_info->num_colors, NCP5623_CHANNEL_COUNT);
			return -EINVAL;
		}
	} else if (config->num_leds <= 3) { /* three single-channel LEDs */
		for (i = 0; i < config->num_leds; i++) {
			led_info = ncp5623_led_to_info(config, i);

			if (!led_info) {
				return -ENODEV;
			}

			if (led_info->num_colors > 1) {
				LOG_ERR("%s: invalid number of colors %d (must be 1 when defining "
					"multiple leds)",
					dev->name, led_info->num_colors);
				return -EINVAL;
			}
		}
	} else {
		LOG_ERR("%s: invalid number of leds %d (max %d)", dev->name, config->num_leds,
			NCP5623_CHANNEL_COUNT);
		return -EINVAL;
	}

	if (i2c_burst_write_dt(&config->bus, NCP5623_LED_CURRENT | NCP5623_MAX_BRIGHTNESS, buf,
			       6)) {
		LOG_ERR("%s: LED write failed", dev->name);
		return -EIO;
	}

	return 0;
}

static const struct led_driver_api ncp5623_led_api = {
	.set_brightness = ncp5623_set_brightness,
	.on = ncp5623_led_on,
	.off = ncp5623_led_off,
	.get_info = ncp5623_get_info,
	.set_color = ncp5623_set_color,
};

#define COLOR_MAPPING(led_node_id)                                                                 \
	static const uint8_t color_mapping_##led_node_id[] = DT_PROP(led_node_id, color_mapping);

#define LED_INFO(led_node_id)                                                                      \
	{                                                                                          \
		.label = DT_PROP(led_node_id, label),                                              \
		.index = DT_PROP(led_node_id, index),                                              \
		.num_colors = DT_PROP_LEN(led_node_id, color_mapping),                             \
		.color_mapping = color_mapping_##led_node_id,                                      \
	},

#define NCP5623_DEFINE(id)                                                                         \
                                                                                                   \
	DT_INST_FOREACH_CHILD(id, COLOR_MAPPING)                                                   \
                                                                                                   \
	static const struct led_info ncp5623_leds_##id[] = {DT_INST_FOREACH_CHILD(id, LED_INFO)};  \
                                                                                                   \
	static const struct ncp5623_config ncp5623_config_##id = {                                 \
		.bus = I2C_DT_SPEC_INST_GET(id),                                                   \
		.num_leds = ARRAY_SIZE(ncp5623_leds_##id),                                         \
		.leds_info = ncp5623_leds_##id,                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, &ncp5623_led_init, NULL, NULL, &ncp5623_config_##id,             \
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY, &ncp5623_led_api);

DT_INST_FOREACH_STATUS_OKAY(NCP5623_DEFINE)
