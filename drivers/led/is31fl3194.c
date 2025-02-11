/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT issi_is31fl3194

/**
 * @file
 * @brief IS31FL3194 LED driver
 *
 * The IS31FL3194 is a 3-channel LED driver that communicates over I2C.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/dt-bindings/led/led.h>

LOG_MODULE_REGISTER(is31fl3194, CONFIG_LED_LOG_LEVEL);

#define IS31FL3194_PROD_ID_REG		0x00
#define IS31FL3194_CONF_REG		0x01
#define IS31FL3194_CURRENT_REG		0x03
#define IS31FL3194_OUT1_REG		0x10
#define IS31FL3194_OUT2_REG		0x21
#define IS31FL3194_OUT3_REG		0x32
#define IS31FL3194_UPDATE_REG		0x40

#define IS31FL3194_PROD_ID_VAL		0xce
#define IS31FL3194_CONF_ENABLE		0x01
#define IS31FL3194_UPDATE_VAL		0xc5

#define IS31FL3194_CHANNEL_COUNT 3

static const uint8_t led_channels[] = {
	IS31FL3194_OUT1_REG,
	IS31FL3194_OUT2_REG,
	IS31FL3194_OUT3_REG
};

struct is31fl3194_config {
	struct i2c_dt_spec bus;
	uint8_t num_leds;
	const struct led_info *led_infos;
	const uint8_t *current_limits;
};

static const struct led_info *is31fl3194_led_to_info(const struct is31fl3194_config *config,
						     uint32_t led)
{
	if (led < config->num_leds) {
		return &config->led_infos[led];
	}

	return NULL;
}

static int is31fl3194_get_info(const struct device *dev,
			       uint32_t led,
			       const struct led_info **info_out)
{
	const struct is31fl3194_config *config = dev->config;
	const struct led_info *info = is31fl3194_led_to_info(config, led);

	if (info == NULL) {
		return -EINVAL;
	}

	*info_out = info;
	return 0;
}

static int is31fl3194_set_color(const struct device *dev, uint32_t led, uint8_t num_colors,
				const uint8_t *color)
{
	const struct is31fl3194_config *config = dev->config;
	const struct led_info *info = is31fl3194_led_to_info(config, led);
	int ret;

	if (info == NULL) {
		return -ENODEV;
	}

	if (info->num_colors != 3) {
		return -ENOTSUP;
	}

	if (num_colors != 3) {
		return -EINVAL;
	}

	for (int i = 0; i < 3; i++) {
		uint8_t value;

		switch (info->color_mapping[i]) {
		case LED_COLOR_ID_RED:
			value = color[0];
			break;
		case LED_COLOR_ID_GREEN:
			value = color[1];
			break;
		case LED_COLOR_ID_BLUE:
			value = color[2];
			break;
		default:
			/* unreachable: mapping already tested in is31fl3194_check_config */
			continue;
		}

		ret = i2c_reg_write_byte_dt(&config->bus, led_channels[i], value);
		if (ret != 0) {
			break;
		}
	}

	if (ret == 0) {
		ret = i2c_reg_write_byte_dt(&config->bus,
					    IS31FL3194_UPDATE_REG,
					    IS31FL3194_UPDATE_VAL);
	}

	if (ret != 0) {
		LOG_ERR("%s: LED write failed: %d", dev->name, ret);
	}

	return ret;
}

static int is31fl3194_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct is31fl3194_config *config = dev->config;
	const struct led_info *info = is31fl3194_led_to_info(config, led);
	int ret = 0;

	if (info == NULL) {
		return -ENODEV;
	}

	if (info->num_colors != 1) {
		return -ENOTSUP;
	}

	if (value > 100) {
		return -EINVAL;
	}

	/* Rescale 0..100 to 0..255 */
	value = value * 255 / 100;

	ret = i2c_reg_write_byte_dt(&config->bus, led_channels[led], value);
	if (ret == 0) {
		ret = i2c_reg_write_byte_dt(&config->bus,
					    IS31FL3194_UPDATE_REG,
					    IS31FL3194_UPDATE_VAL);
	}

	if (ret != 0) {
		LOG_ERR("%s: LED write failed", dev->name);
	}

	return ret;
}

static inline int is31fl3194_led_on(const struct device *dev, uint32_t led)
{
	return is31fl3194_set_brightness(dev, led, 100);
}

static inline int is31fl3194_led_off(const struct device *dev, uint32_t led)
{
	return is31fl3194_set_brightness(dev, led, 0);
}

/*
 * Counts red, green, blue channels; returns true if color_id is valid
 * and no more than one channel maps to the same color
 */
static bool is31fl3194_count_colors(const struct device *dev,
				    uint8_t color_id, uint8_t *rgb_counts)
{
	bool ret = false;

	switch (color_id) {
	case LED_COLOR_ID_RED:
		ret = (++rgb_counts[0] == 1);
		break;
	case LED_COLOR_ID_GREEN:
		ret = (++rgb_counts[1] == 1);
		break;
	case LED_COLOR_ID_BLUE:
		ret = (++rgb_counts[2] == 1);
		break;
	}

	if (!ret) {
		LOG_ERR("%s: invalid color %d (duplicate or not RGB)",
			dev->name, color_id);
	}

	return ret;
}

static int is31fl3194_check_config(const struct device *dev)
{
	const struct is31fl3194_config *config = dev->config;
	const struct led_info *info;
	uint8_t rgb_counts[3] = { 0 };
	uint8_t i;

	switch (config->num_leds) {
	case 1:
		/* check that it is a three-channel LED */
		info = &config->led_infos[0];

		if (info->num_colors != 3) {
			LOG_ERR("%s: invalid number of colors %d "
				"(must be 3 for RGB LED)",
				dev->name, info->num_colors);
			return -EINVAL;
		}

		for (i = 0; i < 3; i++) {
			if (!is31fl3194_count_colors(dev, info->color_mapping[i], rgb_counts)) {
				return -EINVAL;
			}

		}
		break;
	case 3:
		/* check that each LED is single-color */
		for (i = 0; i < 3; i++) {
			info = &config->led_infos[i];

			if (info->num_colors != 1) {
				LOG_ERR("%s: invalid number of colors %d "
					"(must be 1 when defining multiple LEDs)",
					dev->name, info->num_colors);
				return -EINVAL;
			}

			if (!is31fl3194_count_colors(dev, info->color_mapping[0], rgb_counts)) {
				return -EINVAL;
			}
		}
		break;
	default:
		LOG_ERR("%s: invalid number of LEDs %d (must be 1 or 3)",
			dev->name, config->num_leds);
		return -EINVAL;
	}

	return 0;
}

static int is31fl3194_init(const struct device *dev)
{
	const struct is31fl3194_config *config = dev->config;
	const struct led_info *info = NULL;
	int i, ret;
	uint8_t prod_id, band;
	uint8_t current_reg = 0;

	ret = is31fl3194_check_config(dev);
	if (ret != 0) {
		return ret;
	}

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("%s: I2C device not ready", dev->name);
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->bus, IS31FL3194_PROD_ID_REG, &prod_id);
	if (ret != 0) {
		LOG_ERR("%s: failed to read product ID", dev->name);
		return ret;
	}

	if (prod_id != IS31FL3194_PROD_ID_VAL) {
		LOG_ERR("%s: invalid product ID 0x%02x (expected 0x%02x)", dev->name, prod_id,
			IS31FL3194_PROD_ID_VAL);
		return -ENODEV;
	}

	/* calc current limit register value */
	info = &config->led_infos[0];
	if (info->num_colors == IS31FL3194_CHANNEL_COUNT) {
		/* one RGB LED: set all channels to the same current limit */
		band = (config->current_limits[0] / 10) - 1;
		for (i = 0; i < IS31FL3194_CHANNEL_COUNT; i++) {
			current_reg |= band << (2 * i);
		}
	} else {
		/* single-channel LEDs: independent limits */
		for (i = 0; i < config->num_leds; i++) {
			band = (config->current_limits[i] / 10) - 1;
			current_reg |= band << (2 * i);
		}
	}

	ret = i2c_reg_write_byte_dt(&config->bus, IS31FL3194_CURRENT_REG, current_reg);
	if (ret != 0) {
		LOG_ERR("%s: failed to set current limit", dev->name);
		return ret;
	}

	/* enable device */
	return i2c_reg_write_byte_dt(&config->bus, IS31FL3194_CONF_REG, IS31FL3194_CONF_ENABLE);
}

static const struct led_driver_api is31fl3194_led_api = {
	.set_brightness = is31fl3194_set_brightness,
	.on = is31fl3194_led_on,
	.off = is31fl3194_led_off,
	.get_info = is31fl3194_get_info,
	.set_color = is31fl3194_set_color,
};

#define COLOR_MAPPING(led_node_id)						\
	static const uint8_t color_mapping_##led_node_id[] =			\
		DT_PROP(led_node_id, color_mapping);

#define LED_INFO(led_node_id)							\
	{									\
		.label = DT_PROP(led_node_id, label),				\
		.num_colors = DT_PROP_LEN(led_node_id, color_mapping),		\
		.color_mapping = color_mapping_##led_node_id,			\
	},

#define LED_CURRENT(led_node_id)						\
	DT_PROP(led_node_id, current_limit),

#define IS31FL3194_DEFINE(id)							\
										\
	DT_INST_FOREACH_CHILD(id, COLOR_MAPPING)				\
										\
	static const struct led_info is31fl3194_leds_##id[] =			\
		{ DT_INST_FOREACH_CHILD(id, LED_INFO) };			\
	static const uint8_t is31fl3194_currents_##id[] =			\
		{ DT_INST_FOREACH_CHILD(id, LED_CURRENT) };			\
	BUILD_ASSERT(ARRAY_SIZE(is31fl3194_leds_##id) > 0,			\
		     "No LEDs defined for " #id);				\
										\
	static const struct is31fl3194_config is31fl3194_config_##id = {	\
		.bus = I2C_DT_SPEC_INST_GET(id),				\
		.num_leds = ARRAY_SIZE(is31fl3194_leds_##id),			\
		.led_infos = is31fl3194_leds_##id,				\
		.current_limits = is31fl3194_currents_##id,			\
	};									\
	DEVICE_DT_INST_DEFINE(id, &is31fl3194_init, NULL, NULL,                 \
			      &is31fl3194_config_##id, POST_KERNEL,             \
			      CONFIG_LED_INIT_PRIORITY, &is31fl3194_led_api);

DT_INST_FOREACH_STATUS_OKAY(IS31FL3194_DEFINE)
