/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


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

LOG_MODULE_REGISTER(is31fl319x, CONFIG_LED_LOG_LEVEL);
#define REG_NOT_DEFINED 0xff

struct is31f1319x_model {
	const uint8_t prod_id_reg;
	const uint8_t shutdown_reg;
	const uint8_t conf_reg;
	const uint8_t current_reg;
	const uint8_t update_reg;

	const uint8_t prod_id_val;
	const uint8_t shutdown_reg_val;
	const uint8_t conf_enable;
	const uint8_t update_val;

	const uint8_t led_channels[];
};


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

static const struct is31f1319x_model is31f13194_model = {
	/* register indexes */
	.prod_id_reg = IS31FL3194_PROD_ID_REG,
	.shutdown_reg = REG_NOT_DEFINED,
	.conf_reg = IS31FL3194_CONF_REG,
	.current_reg = IS31FL3194_CURRENT_REG,
	.update_reg = IS31FL3194_UPDATE_REG,

	/* values for those registers */
	.prod_id_val = IS31FL3194_PROD_ID_VAL,
	.shutdown_reg_val = 0,
	.conf_enable = IS31FL3194_CONF_ENABLE,
	.update_val = IS31FL3194_UPDATE_VAL,

	/* channel output registers */
	.led_channels = {IS31FL3194_OUT1_REG, IS31FL3194_OUT2_REG, IS31FL3194_OUT3_REG}};

struct is31fl319x_config {
	struct i2c_dt_spec bus;
	uint8_t channel_count;
	uint8_t num_leds;
	const struct led_info *led_infos;
	const uint8_t *current_limits;
	const struct is31f1319x_model *regs;
};

static const struct led_info *is31fl319x_led_to_info(const struct is31fl319x_config *config,
						     uint32_t led)
{
	if (led < config->num_leds) {
		return &config->led_infos[led];
	}

	return NULL;
}

static int is31fl319x_get_info(const struct device *dev,
			       uint32_t led,
			       const struct led_info **info_out)
{
	const struct is31fl319x_config *config = dev->config;
	const struct led_info *info = is31fl319x_led_to_info(config, led);

	if (info == NULL) {
		return -EINVAL;
	}

	*info_out = info;
	return 0;
}

static uint8_t is31fl319x_map_led_to_start_channel(const struct is31fl319x_config *config,
						   uint32_t led)
{
	/* It is assumed that led has been validated before calling this */
	const struct led_info *info = config->led_infos;
	uint8_t channel = 0;

	while (led) {
		channel += info->num_colors;
		led--;
		info++;
	}
	return channel;
}

static int is31fl319x_write_channels(const struct device *dev, uint32_t start_channel,
				     uint32_t num_channels, const uint8_t *buf)
{
	const struct is31fl319x_config *config = dev->config;
	const struct is31f1319x_model *regs = config->regs;
	int ret;
	int i;

	if ((start_channel + num_channels) > config->channel_count) {
		return -ENOTSUP;
	}

	for (i = 0; i < num_channels; i++) {
		ret = i2c_reg_write_byte_dt(&config->bus, regs->led_channels[i + start_channel],
					    buf[i]);
		if (ret != 0) {
			break;
		}
	}

	if (ret == 0) {
		ret = i2c_reg_write_byte_dt(&config->bus,
					    regs->update_reg,
					    regs->update_val);
	}

	if (ret != 0) {
		LOG_ERR("%s: LED write failed: %d", dev->name, ret);
	}

	return ret;
}

static int is31fl319x_set_color(const struct device *dev, uint32_t led, uint8_t num_colors,
				const uint8_t *color)
{
	const struct is31fl319x_config *config = dev->config;
	const struct led_info *info = is31fl319x_led_to_info(config, led);
	uint8_t channel_start;

	if (info == NULL) {
		return -ENODEV;
	}

	channel_start = is31fl319x_map_led_to_start_channel(config, led);

	if (info->num_colors > config->channel_count) {
		return -ENOTSUP;
	}

	return is31fl319x_write_channels(dev, channel_start, num_colors, color);
}

static int is31fl319x_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct is31fl319x_config *config = dev->config;
	const struct led_info *info = is31fl319x_led_to_info(config, led);
	const struct is31f1319x_model *regs = config->regs;
	uint8_t channel_start;

	int ret = 0;

	if (info == NULL) {
		return -ENODEV;
	}

	if (info->num_colors != 1) {
		return -ENOTSUP;
	}

	/* Rescale 0..100 to 0..255 */
	value = value * 255 / LED_BRIGHTNESS_MAX;

	channel_start = is31fl319x_map_led_to_start_channel(config, led);

	ret = i2c_reg_write_byte_dt(&config->bus, regs->led_channels[channel_start], value);
	if (ret == 0) {
		ret = i2c_reg_write_byte_dt(&config->bus,
					    regs->update_reg,
					    regs->update_val);
	}

	if (ret != 0) {
		LOG_ERR("%s: LED write failed", dev->name);
	}

	return ret;
}

static int is31fl319x_check_config(const struct device *dev)
{
	const struct is31fl319x_config *config = dev->config;
	const struct led_info *info;
	uint8_t rgb_count = 0;
	uint8_t i;

	/* verify that number of leds defined is not > number of channels */
	for (i = 0; i < config->num_leds; i++) {
		info = &config->led_infos[i];
		rgb_count += info->num_colors;
	}

	if (rgb_count > config->channel_count) {
		return -EINVAL;
	}

	return 0;
}

static int is31fl319x_init(const struct device *dev)
{
	const struct is31fl319x_config *config = dev->config;
	const struct led_info *info = NULL;
	const struct is31f1319x_model *regs = config->regs;
	int ret;
	uint8_t prod_id;
	uint8_t band;
	uint8_t channel;
	uint8_t current_reg = 0;
	int i;
	int j;

	ret = is31fl319x_check_config(dev);
	if (ret != 0) {
		return ret;
	}

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("%s: I2C device not ready", dev->name);
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->bus, regs->prod_id_reg, &prod_id);
	if (ret != 0) {
		LOG_ERR("%s: failed to read product ID", dev->name);
		return ret;
	}

	if (prod_id != regs->prod_id_val) {
		LOG_ERR("%s: invalid product ID 0x%02x (expected 0x%02x)", dev->name, prod_id,
			regs->prod_id_val);
		return -ENODEV;
	}

	/* calc current limit register value */
	if (regs->current_reg != REG_NOT_DEFINED) {
		channel = 0;
		for (i = 0; i < config->num_leds; i++) {
			info = &config->led_infos[i];
			band = (config->current_limits[i] / 10) - 1;

			for (j = 0; j < info->num_colors; j++) {
				current_reg |= band << (2 * channel);
				channel++;
			}
		}

		ret = i2c_reg_write_byte_dt(&config->bus, regs->current_reg, current_reg);
		if (ret != 0) {
			LOG_ERR("%s: failed to set current limit", dev->name);
			return ret;
		}
	}
	/* enable device */
	return i2c_reg_write_byte_dt(&config->bus, regs->conf_reg,
				     regs->conf_enable);
}

static DEVICE_API(led, is31fl319x_led_api) = {
	.set_brightness = is31fl319x_set_brightness,
	.get_info = is31fl319x_get_info,
	.set_color = is31fl319x_set_color,
	.write_channels = is31fl319x_write_channels,
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

#define IS31FL319X_DEVICE(n, id, nchannels, pregs)				\
										\
	DT_INST_FOREACH_CHILD(n, COLOR_MAPPING)					\
										\
	static const struct led_info is31fl319##id##_leds_##n[] =		\
		{ DT_INST_FOREACH_CHILD(n, LED_INFO) };				\
										\
	static const uint8_t is31fl319##id##_currents_##n[] =			\
		{ DT_INST_FOREACH_CHILD(n, LED_CURRENT) };			\
	BUILD_ASSERT(ARRAY_SIZE(is31fl319##id##_leds_##n) > 0,			\
		     "No LEDs defined for " #n);				\
										\
	static const struct is31fl319x_config is31fl319##id##_config_##n = {	\
		.bus = I2C_DT_SPEC_INST_GET(n),					\
		.channel_count = nchannels,					\
		.num_leds = ARRAY_SIZE(is31fl319##id##_leds_##n),		\
		.led_infos = is31fl319##id##_leds_##n,				\
		.current_limits = is31fl319##id##_currents_##n,			\
		.regs = pregs,							\
	};									\
	DEVICE_DT_INST_DEFINE(n, &is31fl319x_init, NULL, NULL,			\
			      &is31fl319##id##_config_##n, POST_KERNEL,		\
			      CONFIG_LED_INIT_PRIORITY, &is31fl319x_led_api);

#define DT_DRV_COMPAT issi_is31fl3194
DT_INST_FOREACH_STATUS_OKAY_VARGS(IS31FL319X_DEVICE, 4, IS31FL3194_CHANNEL_COUNT,
				  &is31f13194_model)
