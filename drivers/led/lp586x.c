/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Draeger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LP586x LED matrix controller
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lp586x, CONFIG_LED_LOG_LEVEL);

#define LP586X_DISABLE_DELAY_US 3
#define LP586X_ENABLE_DELAY_US  500
#define LP586X_CHIP_EN_DELAY_US 100

/* Base registers */
#define LP586X_CHIP_EN       0x00
#define LP586X_DEV_INITIAL   0x01
#define LP586X_DEV_CONFIG1   0x02
#define LP586X_DEV_CONFIG2   0x03
#define LP586X_DEV_CONFIG3   0x04
#define LP586X_GLOBAL_BRI    0x05
#define LP586X_GROUP0_BRI    0x06
#define LP586X_GROUP1_BRI    0x07
#define LP586X_GROUP2_BRI    0x08
#define LP586X_R_CURRENT_SET 0x09
#define LP586X_G_CURRENT_SET 0x0A
#define LP586X_B_CURRENT_SET 0x0B
#define LP586X_DOT_GRP_SEL   0x0C
#define LP586X_DOT_ONOFF     0x43
#define LP586X_FAULT_STATE   0x64
#define LP586X_DOT_LOD       0x65
#define LP586X_DOT_LSD       0x86
#define LP586X_LOD_CLEAR     0xA7
#define LP586X_LSD_CLEAR     0xA8
#define LP586X_RESET         0xA9

/* Register values */
#define CHIP_EN                       BIT(0)
#define DEV_INITIAL_PWM_LOW_FREQ_EN   BIT(0)
#define DEV_INITIAL_VSYNC_EN          BIT(1)
#define DEV_INITIAL_16BIT_PWM_EN      BIT(2)
#define DEV_INITIAL_RESERVED          (BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(7))
#define DEV_CONFIG1_PHASE_SHIFT_EN    BIT(1)
#define DEV_CONFIG1_EXP_SCALE_EN      BIT(2)
#define DEV_CONFIG3_MAX_CURRENT_3_MA  (0x41 | 0x00 << 1)
#define DEV_CONFIG3_MAX_CURRENT_5_MA  (0x41 | 0x01 << 1)
#define DEV_CONFIG3_MAX_CURRENT_10_MA (0x41 | 0x02 << 1)
#define DEV_CONFIG3_MAX_CURRENT_15_MA (0x41 | 0x03 << 1)
#define DEV_CONFIG3_MAX_CURRENT_20_MA (0x41 | 0x04 << 1)
#define DEV_CONFIG3_MAX_CURRENT_30_MA (0x41 | 0x05 << 1)
#define DEV_CONFIG3_MAX_CURRENT_40_MA (0x41 | 0x06 << 1)
#define DEV_CONFIG3_MAX_CURRENT_50_MA (0x41 | 0x07 << 1)
#define RESET_SW                      0xFF

/* I2C chip addresses for dot CC and PWM control */
#define LP586X_DOT_CC_ADDR(addr)  ((addr) + 1)
#define LP586X_DOT_PWM_ADDR(addr) ((addr) + 2)

/*
 * The LP586x embeds the upper 2 bits of the 10-bit register address into the
 * 7-bit I2C device address. Compute the effective I2C address for a register.
 */
#define LP586X_I2C_EFF_ADDR(base, reg) ((uint8_t)(((base) & ~0x03U) | (((reg) >> 8) & 0x03U)))

struct lp586x_config {
	struct i2c_dt_spec bus;
	const struct gpio_dt_spec gpio_enable;
	uint8_t max_channels;
	uint8_t num_leds;
	bool vsync_en;
	bool phase_shift_en;
	bool exp_scale_en;
	const struct led_info *leds_info;
};

static const struct led_info *lp586x_led_to_info(const struct lp586x_config *config, uint32_t led)
{
	if (led < config->num_leds) {
		return &config->leds_info[led];
	}

	return NULL;
}

static int lp586x_get_info(const struct device *dev, uint32_t led, const struct led_info **info)
{
	const struct lp586x_config *config = dev->config;
	const struct led_info *led_info = lp586x_led_to_info(config, led);

	if (!led_info) {
		return -EINVAL;
	}

	*info = led_info;

	return 0;
}

static int lp586x_write_channels(const struct device *dev, uint32_t start_channel,
				 uint32_t num_channels, const uint8_t *channels)
{
	const struct lp586x_config *config = dev->config;
	uint8_t buf[num_channels + 1];

	if (start_channel + num_channels > config->max_channels) {
		return -EINVAL;
	}

	buf[0] = start_channel;
	memcpy(buf + 1, channels, num_channels);

	return i2c_write(config->bus.bus, buf, num_channels + 1,
			 LP586X_DOT_PWM_ADDR(config->bus.addr));
}

static int lp586x_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct lp586x_config *config = dev->config;
	const struct led_info *led_info = lp586x_led_to_info(config, led);

	if (!led_info) {
		return -ENODEV;
	}

	if (led_info->num_colors != 1) {
		LOG_ERR("Set_brightness called for a multi-color LED (num_colors=%d)",
			led_info->num_colors);
		return -EINVAL;
	}

	return i2c_reg_write_byte(config->bus.bus, LP586X_DOT_PWM_ADDR(config->bus.addr),
				  led_info->index, (value * 0xFF) / LED_BRIGHTNESS_MAX);
}

static int lp586x_set_color(const struct device *dev, uint32_t led, uint8_t num_colors,
			    const uint8_t *color)
{
	const struct lp586x_config *config = dev->config;
	const struct led_info *led_info = lp586x_led_to_info(config, led);

	if (!led_info) {
		return -ENODEV;
	}

	if (num_colors != led_info->num_colors) {
		LOG_ERR("Invalid number of colors: got=%d, expected=%d", num_colors,
			led_info->num_colors);
		return -EINVAL;
	}

	int err = lp586x_write_channels(dev, led_info->index, led_info->num_colors, color);

	if (err) {
		LOG_ERR("Failed to set color (error %d)", err);
		return err;
	}

	return 0;
}

static int lp586x_hw_enable(const struct device *dev, bool enable)
{
	const struct lp586x_config *config = dev->config;
	int err;

	if (config->gpio_enable.port == NULL) {
		return 0;
	}

	err = gpio_pin_set_dt(&config->gpio_enable, enable);
	if (err < 0) {
		LOG_ERR("Failed to set enable gpio (error %d)", err);
		return err;
	}

	k_usleep(enable ? LP586X_ENABLE_DELAY_US : LP586X_DISABLE_DELAY_US);

	return 0;
}

static int lp586x_reset(const struct device *dev)
{
	const struct lp586x_config *config = dev->config;
	uint8_t buf[2];
	int err;

	buf[0] = LP586X_RESET;
	buf[1] = RESET_SW;
	err = i2c_write_dt(&config->bus, buf, sizeof(buf));
	if (err) {
		LOG_ERR("Failed to write RESET register (error %d)", err);
		return err;
	}

	return 0;
}

static int lp586x_enable(const struct device *dev, bool enable)
{
	const struct lp586x_config *config = dev->config;
	uint8_t value = enable ? CHIP_EN : 0;

	int err = i2c_reg_update_byte_dt(&config->bus, LP586X_CHIP_EN, CHIP_EN, value);

	if (err) {
		LOG_ERR("Failed to set EN Bit in CHIP_EN register (error %d)", err);
		return err;
	}

	k_usleep(LP586X_CHIP_EN_DELAY_US);
	return 0;
}

static int lp586x_initialize(const struct device *dev)
{
	const struct lp586x_config *config = dev->config;
	uint8_t buf[2];
	int err;

	buf[0] = LP586X_DEV_INITIAL;
	buf[1] = DEV_INITIAL_RESERVED;
	if (config->vsync_en) {
		buf[1] |= DEV_INITIAL_VSYNC_EN;
	}
	err = i2c_write_dt(&config->bus, buf, sizeof(buf));
	if (err) {
		LOG_ERR("Failed to write DEV_INITIAL register (error %d)", err);
		return err;
	}

	buf[0] = LP586X_DEV_CONFIG1;
	buf[1] = 0;
	if (config->phase_shift_en) {
		buf[1] |= DEV_CONFIG1_PHASE_SHIFT_EN;
	}
	if (config->exp_scale_en) {
		buf[1] |= DEV_CONFIG1_EXP_SCALE_EN;
	}
	err = i2c_write_dt(&config->bus, buf, sizeof(buf));
	if (err) {
		LOG_ERR("Failed to write DEV_CONFIG1 register (error %d)", err);
		return err;
	}

	buf[0] = LP586X_DEV_CONFIG2;
	buf[1] = 0;
	err = i2c_write_dt(&config->bus, buf, sizeof(buf));
	if (err) {
		LOG_ERR("Failed to clear DEV_CONFIG2 register (error %d)", err);
		return err;
	}

	buf[0] = LP586X_DEV_CONFIG3;
	buf[1] = DEV_CONFIG3_MAX_CURRENT_15_MA;
	err = i2c_write_dt(&config->bus, buf, sizeof(buf));
	if (err) {
		LOG_ERR("Failed to write DEV_CONFIG3 register (error %d)", err);
		return err;
	}

	return 0;
}

static int lp586x_init(const struct device *dev)
{
	const struct lp586x_config *config = dev->config;
	int err;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR_DEVICE_NOT_READY(config->bus.bus);
		return -ENODEV;
	}

	for (uint8_t led = 0; led < config->num_leds; led++) {
		const struct led_info *led_info = lp586x_led_to_info(config, led);
		int channel = led_info->index + led_info->num_colors - 1;

		if (channel > config->max_channels) {
			LOG_ERR("Invalid LED channel %d (max %d)", channel, config->max_channels);
			return -EINVAL;
		}
	}

	if (config->gpio_enable.port != NULL) {
		if (!gpio_is_ready_dt(&config->gpio_enable)) {
			LOG_ERR_DEVICE_NOT_READY(config->gpio_enable.port);
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->gpio_enable, GPIO_OUTPUT_INACTIVE);
		if (err < 0) {
			LOG_ERR("Failed to initialize enable gpio (error %d)", err);
			return err;
		}
	}

	err = lp586x_hw_enable(dev, true);
	if (err < 0) {
		LOG_ERR("Failed to enable hardware (error %d)", err);
		return err;
	}

	err = lp586x_reset(dev);
	if (err < 0) {
		LOG_ERR("Failed to reset (error %d)", err);
		return err;
	}

	err = lp586x_enable(dev, true);
	if (err < 0) {
		LOG_ERR("Failed to enable (error %d)", err);
		return err;
	}

	err = lp586x_initialize(dev);
	if (err < 0) {
		LOG_ERR("Failed to initialize (error %d)", err);
		return err;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int lp586x_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		return lp586x_enable(dev, false);
	case PM_DEVICE_ACTION_RESUME:
		return lp586x_enable(dev, true);
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(led, lp586x_led_api) = {
	.get_info = lp586x_get_info,
	.set_brightness = lp586x_set_brightness,
	.set_color = lp586x_set_color,
	.write_channels = lp586x_write_channels,
};

#define COLOR_MAPPING(led_node_id)                                                                 \
	const uint8_t color_mapping_##led_node_id[] = DT_PROP(led_node_id, color_mapping);

#define LED_INFO(led_node_id)                                                                      \
	{                                                                                          \
		.label = DT_PROP(led_node_id, label),                                              \
		.index = DT_PROP(led_node_id, index),                                              \
		.num_colors = DT_PROP_LEN(led_node_id, color_mapping),                             \
		.color_mapping = color_mapping_##led_node_id,                                      \
	},

#define SINKS_PER_LINE 18

#define LP586X_DEVICE(n, id, nlines)                                                               \
	DT_INST_FOREACH_CHILD(n, COLOR_MAPPING)                                                    \
                                                                                                   \
	static const struct led_info lp##id##_leds_##n[] = {DT_INST_FOREACH_CHILD(n, LED_INFO)};   \
                                                                                                   \
	static const struct lp586x_config lp##id##_config_##n = {                                  \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
		.gpio_enable = GPIO_DT_SPEC_INST_GET_OR(n, enable_gpios, {0}),                     \
		.max_channels = (SINKS_PER_LINE * nlines),                                         \
		.num_leds = ARRAY_SIZE(lp##id##_leds_##n),                                         \
		.vsync_en = DT_INST_PROP(n, vsync_en),                                             \
		.phase_shift_en = DT_INST_PROP(n, phase_shift_en),                                 \
		.exp_scale_en = DT_INST_PROP(n, exp_scale_en),                                     \
		.leds_info = lp##id##_leds_##n,                                                    \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, lp586x_pm_action);                                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, lp586x_init, PM_DEVICE_DT_INST_GET(n), NULL,                      \
			      &lp##id##_config_##n, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,         \
			      &lp586x_led_api);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5860
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP586X_DEVICE, 5860, 11)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5861
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP586X_DEVICE, 5861, 1)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5862
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP586X_DEVICE, 5862, 2)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5864
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP586X_DEVICE, 5864, 4)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5866
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP586X_DEVICE, 5866, 6)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5868
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP586X_DEVICE, 5868, 8)
