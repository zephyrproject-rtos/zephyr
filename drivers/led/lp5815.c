/*
 * Copyright (c) 2026 Kristina Kisiuk, Aliensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LP5815 LED driver
 *
 * The LP5815 is a 3-channel I2C RGB LED driver from Texas Instruments with
 * autonomous animation engine and instant blinking support. Each channel
 * supports 8-bit PWM dimming and individual 8-bit dot current control.
 * The maximum output current per channel is selectable between 25.5mA and
 * 51mA via the MAX_CURRENT bit.
 *
 * This driver operates the device in manual control mode (autonomous
 * animation disabled). DEV_CONFIG3 defaults to 0x00 after reset, which
 * selects manual mode with linear dimming for all channels.
 */

#define DT_DRV_COMPAT ti_lp5815

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lp5815, CONFIG_LED_LOG_LEVEL);

/* Register addresses */
#define LP5815_REG_CHIP_EN		0x00
#define LP5815_REG_DEV_CONFIG0		0x01
#define LP5815_REG_DEV_CONFIG1		0x02
#define LP5815_REG_UPDATE_CMD		0x0F
#define LP5815_REG_FLAG_CLR		0x13
#define LP5815_REG_OUT0_DC		0x14
#define LP5815_REG_OUT1_DC		0x15
#define LP5815_REG_OUT2_DC		0x16
#define LP5815_REG_OUT0_MANUAL_PWM	0x18
#define LP5815_REG_OUT1_MANUAL_PWM	0x19
#define LP5815_REG_OUT2_MANUAL_PWM	0x1A

/* CHIP_EN register bits */
#define LP5815_CHIP_EN			BIT(0)
#define LP5815_INSTABLINK_DIS		BIT(1)

/* DEV_CONFIG0 register bits */
#define LP5815_MAX_CURRENT		BIT(0)

/* DEV_CONFIG1 register bits */
#define LP5815_OUT0_EN			BIT(0)
#define LP5815_OUT1_EN			BIT(1)
#define LP5815_OUT2_EN			BIT(2)

/* FLAG_CLR register bits */
#define LP5815_POR_CLR			BIT(0)

/* Command values */
#define LP5815_UPDATE_CMD_VAL		0x55

#define LP5815_NUM_CHANNELS		3

struct lp5815_config {
	struct i2c_dt_spec bus;
	uint8_t num_leds;
	const struct led_info *leds_info;
	bool max_current;
	uint8_t out_dot_current[LP5815_NUM_CHANNELS];
};

static const struct led_info *
lp5815_led_to_info(const struct lp5815_config *config, uint32_t led)
{
	if (led < config->num_leds) {
		return &config->leds_info[led];
	}

	return NULL;
}

static int lp5815_get_info(const struct device *dev, uint32_t led,
			   const struct led_info **info)
{
	const struct lp5815_config *config = dev->config;
	const struct led_info *led_info = lp5815_led_to_info(config, led);

	if (!led_info) {
		return -ENODEV;
	}

	*info = led_info;

	return 0;
}

static int lp5815_set_brightness(const struct device *dev, uint32_t led,
				 uint8_t value)
{
	const struct lp5815_config *config = dev->config;
	const struct led_info *led_info = lp5815_led_to_info(config, led);
	uint8_t reg;
	uint8_t val;

	if (!led_info) {
		return -ENODEV;
	}

	if (led_info->index >= LP5815_NUM_CHANNELS) {
		return -EINVAL;
	}

	reg = LP5815_REG_OUT0_MANUAL_PWM + led_info->index;
	val = (value * 0xff) / LED_BRIGHTNESS_MAX;

	return i2c_reg_write_byte_dt(&config->bus, reg, val);
}

static int lp5815_on(const struct device *dev, uint32_t led)
{
	return lp5815_set_brightness(dev, led, LED_BRIGHTNESS_MAX);
}

static int lp5815_off(const struct device *dev, uint32_t led)
{
	return lp5815_set_brightness(dev, led, 0);
}

static int lp5815_set_color(const struct device *dev, uint32_t led,
			    uint8_t num_colors, const uint8_t *color)
{
	const struct lp5815_config *config = dev->config;
	const struct led_info *led_info = lp5815_led_to_info(config, led);
	uint8_t buf[LP5815_NUM_CHANNELS + 1];
	uint8_t i;

	if (!led_info) {
		return -ENODEV;
	}

	if (num_colors != led_info->num_colors) {
		LOG_ERR("Invalid number of colors: got %d, expected %d",
			num_colors, led_info->num_colors);
		return -EINVAL;
	}

	buf[0] = LP5815_REG_OUT0_MANUAL_PWM +
		 LP5815_NUM_CHANNELS * led_info->index;

	for (i = 0; i < num_colors; i++) {
		buf[1 + i] = color[i];
	}

	return i2c_write_dt(&config->bus, buf, num_colors + 1);
}

static int lp5815_init(const struct device *dev)
{
	const struct lp5815_config *config = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* Enable chip and disable instant blinking for I2C control */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_CHIP_EN,
				    LP5815_CHIP_EN | LP5815_INSTABLINK_DIS);
	if (ret < 0) {
		return ret;
	}

	/* Clear POR flag (required after power-on reset) */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_FLAG_CLR,
				    LP5815_POR_CLR);
	if (ret < 0) {
		return ret;
	}

	/* Set maximum current range */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_DEV_CONFIG0,
				    config->max_current ? LP5815_MAX_CURRENT : 0);
	if (ret < 0) {
		return ret;
	}

	/* Enable all 3 output channels */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_DEV_CONFIG1,
				    LP5815_OUT0_EN | LP5815_OUT1_EN |
				    LP5815_OUT2_EN);
	if (ret < 0) {
		return ret;
	}

	/* Apply DEV_CONFIG changes with update command */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_UPDATE_CMD,
				    LP5815_UPDATE_CMD_VAL);
	if (ret < 0) {
		return ret;
	}

	/* Set per-channel dot currents */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_OUT0_DC,
				    config->out_dot_current[0]);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_OUT1_DC,
				    config->out_dot_current[1]);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_OUT2_DC,
				    config->out_dot_current[2]);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static DEVICE_API(led, lp5815_led_api) = {
	.get_info	= lp5815_get_info,
	.on		= lp5815_on,
	.off		= lp5815_off,
	.set_brightness	= lp5815_set_brightness,
	.set_color	= lp5815_set_color,
};

#define LP5815_COLOR_MAPPING(led_node_id)				\
	static const uint8_t color_mapping_##led_node_id[] =		\
		DT_PROP(led_node_id, color_mapping);

#define LP5815_LED_INFO(led_node_id)					\
	{								\
		.label		= DT_PROP(led_node_id, label),		\
		.index		= DT_PROP(led_node_id, index),		\
		.num_colors	= DT_PROP_LEN(led_node_id,		\
					       color_mapping),		\
		.color_mapping	= color_mapping_##led_node_id,		\
	},

#define LP5815_DEFINE(n)						\
	DT_INST_FOREACH_CHILD(n, LP5815_COLOR_MAPPING)			\
									\
	static const struct led_info lp5815_leds_##n[] = {		\
		DT_INST_FOREACH_CHILD(n, LP5815_LED_INFO)		\
	};								\
									\
	static const struct lp5815_config lp5815_config_##n = {		\
		.bus		= I2C_DT_SPEC_INST_GET(n),		\
		.num_leds	= ARRAY_SIZE(lp5815_leds_##n),		\
		.leds_info	= lp5815_leds_##n,			\
		.max_current	= DT_INST_PROP(n, max_current_51mA),	\
		.out_dot_current = {					\
			DT_INST_PROP(n, out0_dot_current),		\
			DT_INST_PROP(n, out1_dot_current),		\
			DT_INST_PROP(n, out2_dot_current),		\
		},							\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, lp5815_init, NULL,			\
			      NULL, &lp5815_config_##n,			\
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,	\
			      &lp5815_led_api);

DT_INST_FOREACH_STATUS_OKAY(LP5815_DEFINE)
