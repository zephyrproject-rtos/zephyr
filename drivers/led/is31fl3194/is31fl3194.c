/*
 * Copyright (c) 2022 Benjamin Björnsson <benjamin.bjornsson@gmail.com>.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT issi_is31fl3194

/**
 * @file
 * @brief LED driver for the IS31FL3194 I2C LED driver
 */

#include <string.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/logging/log.h>
#include <zephyr/zephyr.h>
LOG_MODULE_REGISTER(is31fl3194, CONFIG_LED_LOG_LEVEL);

#include "is31fl3194_reg.h"

#define IS31FL3194_INDEX_MAX 2

struct is31fl3194_config {
	struct is31fl3194_ctx ctx;
	const struct i2c_dt_spec i2c;
	uint8_t num_leds;
	const struct led_info *leds_info;
	const uint8_t *curr_band_max;
};

struct is31fl3194_data {
	struct is31fl3194_regs regs;
};

static int is31fl3194_i2c_read(const struct i2c_dt_spec *i2c_spec, uint8_t reg_addr, uint8_t *value,
			       uint8_t len)
{
	return i2c_burst_read_dt(i2c_spec, reg_addr, value, len);
}

static int is31fl3194_i2c_write(const struct i2c_dt_spec *i2c_spec, uint8_t reg_addr,
				uint8_t *value, uint8_t len)
{
	return i2c_burst_write_dt(i2c_spec, reg_addr, value, len);
}

static int is31fl3194_set_brightness(const struct device *dev, uint32_t led, uint8_t value);

static int is31fl3194_on(const struct device *dev, uint32_t led)
{
	return is31fl3194_set_brightness(dev, led, 100);
}

static int is31fl3194_off(const struct device *dev, uint32_t led)
{
	return is31fl3194_set_brightness(dev, led, 0);
}

static const struct led_info *is31fl3194_led_to_info(const struct is31fl3194_config *config,
						     uint32_t led)
{
	for (int i = 0; i < config->num_leds; i++) {
		if (config->leds_info[i].index == led) {
			return &config->leds_info[i];
		}
	}

	return NULL;
}

static int is31fl3194_get_info(const struct device *dev, uint32_t led, const struct led_info **info)
{
	const struct is31fl3194_config *config = dev->config;
	const struct led_info *leds_info;

	leds_info = is31fl3194_led_to_info(config, led);
	if (!leds_info) {
		*info = NULL;
		return -EINVAL;
	}

	*info = leds_info;

	return 0;
}

static int is31fl3194_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct is31fl3194_config *config = dev->config;
	const struct is31fl3194_ctx *ctx = (const struct is31fl3194_ctx *)&config->ctx;
	struct is31fl3194_outx_cl outx_cl = { .cl = ((value * 255) / 100) };
	int res;

	if (value > 100) {
		return -EINVAL;
	}

	res = is31fl3194_outx_cl_set(ctx, led, outx_cl);
	if (res) {
		return res;
	}

	return is31fl3194_color_update(ctx);
}

static int is31fl3194_write_channels(const struct device *dev, uint32_t start_channel,
				     uint32_t num_channels, const uint8_t *buf)
{
	int res;

	if ((start_channel + num_channels) > IS31FL3194_INDEX_MAX) {
		return -EINVAL;
	}

	for (int i = 0; i < num_channels; i++) {
		res = is31fl3194_set_brightness(dev, start_channel + i, buf[i]);
		if (res) {
			return res;
		}
	}

	return 0;
}

static const struct led_driver_api is31fl3194_api = {
	/* Mandatory callbacks. */
	.on = is31fl3194_on,
	.off = is31fl3194_off,
	/* Optional callbacks. */
	.get_info = is31fl3194_get_info,
	.set_brightness = is31fl3194_set_brightness,
	.write_channels = is31fl3194_write_channels,
};

static int is31fl3194_led_init(const struct device *dev)
{
	const struct is31fl3194_config *config = dev->config;
	const struct is31fl3194_ctx *ctx = (const struct is31fl3194_ctx *)&config->ctx;
	int res;
	uint8_t val;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	res = is31fl3194_product_id_get(ctx, &val);
	if (res) {
		LOG_ERR("failed to read Product ID: %d", res);
		return res;
	}

	res = is31fl3194_reset(ctx);
	if (res) {
		LOG_ERR("failed to reset device: %d", res);
		return res;
	}

	for (int i = 0; i < config->num_leds; i++) {
		enum is31fl3194_current_band current_band;

		if (config->curr_band_max[i] == 10) {
			current_band = IS31FL3194_CURRENT_BAND_1;
		} else if (config->curr_band_max[i] == 20) {
			current_band = IS31FL3194_CURRENT_BAND_2;
		} else if (config->curr_band_max[i] == 30) {
			current_band = IS31FL3194_CURRENT_BAND_3;
		} else {
			current_band = IS31FL3194_CURRENT_BAND_4;
		}

		res = is31fl3194_current_band_set(ctx, config->leds_info[i].index, current_band);
		if (res) {
			LOG_ERR("failed to set current band: %d", res);
			return res;
		}
	}

	res = is31fl3194_ops_ssd_set(ctx, 1);
	if (res) {
		LOG_ERR("failed to set normal mode: %d", res);
		return res;
	}

	for (int led = 0; led < IS31FL3194_NUM_LEDS_MAX; led++) {
		res = is31fl3194_out_en_set(ctx, led, 1);
		if (res) {
			LOG_ERR("failed to enable led %d: %d", led, res);
			return res;
		}
	}

	return 0;
}

#define COLOR_MAPPING(led_node_id)								\
	const uint8_t color_mapping_##led_node_id[] = DT_PROP(led_node_id, color_mapping);

#define LED_INFO(led_node_id)							\
	{									\
		.label = DT_LABEL(led_node_id),					\
		.index = DT_PROP(led_node_id, index),				\
		.num_colors = DT_PROP_LEN(led_node_id, color_mapping),		\
		.color_mapping = color_mapping_##led_node_id,			\
	},

#define IS31FL3194_CURR_BAND_MAX(inst) DT_PROP_OR(inst, curr_band_max, 20),

#define IS31FL3194_CONFIG(inst)									\
	{											\
		.ctx = {									\
			.read_reg = (is31fl3194_read_ptr) is31fl3194_i2c_read,			\
			.write_reg = (is31fl3194_write_ptr) is31fl3194_i2c_write,		\
			.regs = (struct is31fl3194_regs *)&is31fl3194_data_##inst.regs,		\
			.handle = (void *)&is31fl3194_config_##inst.i2c,			\
		},										\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		.num_leds = ARRAY_SIZE(is31fl3194_leds_##inst),					\
		.leds_info = is31fl3194_leds_##inst,						\
		.curr_band_max = is31fl3194_curr_band_max_##inst,				\
	}

#define IS31FL3194_DEFINE(inst)									\
	DT_INST_FOREACH_CHILD(inst, COLOR_MAPPING)						\
												\
	const struct led_info is31fl3194_leds_##inst[] = {					\
		DT_INST_FOREACH_CHILD(inst, LED_INFO)						\
	};											\
												\
	const uint8_t is31fl3194_curr_band_max_##inst[] = {					\
		DT_INST_FOREACH_CHILD(inst, IS31FL3194_CURR_BAND_MAX)				\
	};											\
												\
	static struct is31fl3194_data is31fl3194_data_##inst;					\
												\
	const struct is31fl3194_config is31fl3194_config_##inst = IS31FL3194_CONFIG(inst);	\
												\
	DEVICE_DT_INST_DEFINE(inst, &is31fl3194_led_init, NULL,					\
			      &is31fl3194_data_##inst, &is31fl3194_config_##inst, POST_KERNEL,	\
			      CONFIG_LED_INIT_PRIORITY, &is31fl3194_api);			\

DT_INST_FOREACH_STATUS_OKAY(IS31FL3194_DEFINE)
