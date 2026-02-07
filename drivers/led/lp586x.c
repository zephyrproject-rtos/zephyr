/*
 * Copyright (c) 2026 ChargePoint, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lp586x, CONFIG_LED_LOG_LEVEL);

#define LP586X_T_POR_US     500
#define LP586X_T_CHIP_EN_US 100

#define LP586X_MAX_CHANNELS_PER_LED 4

#define LP586X_REG_CHIP_EN     0x000
#define LP586X_REG_DEV_INITIAL 0x001
#define LP586X_REG_DEV_CONFIG1 0x002
#define LP586X_REG_DEV_CONFIG2 0x003
#define LP586X_REG_DEV_CONFIG3 0x004

#define LP586X_REG_GLOBAL_BRI 0x005
#define LP586X_REG_GROUP1_BRI 0x006
#define LP586X_REG_GROUP2_BRI 0x007
#define LP586X_REG_GROUP3_BRI 0x008

#define LP586X_REG_R_CURRENT_SET 0x009
#define LP586X_REG_G_CURRENT_SET 0x00A
#define LP586X_REG_B_CURRENT_SET 0x00B

#define LP586X_REG_RESET 0x0A9

#define LP586X_REG_DC 0x100

#define LP586X_REG_PWM_BRI 0x200

#define LP586X_PWM_BRI_MAX 0xFFu

#define LP586X_REG_DEV_CONFIG3_MC_MASK 0x0E

struct lp586x_config {
	const struct i2c_dt_spec i2c;
	const struct gpio_dt_spec enable;
	uint8_t data_ref_mode;
	uint8_t num_lines;
	uint8_t num_channels;
	uint8_t channels_per_led;
	uint8_t max_current;
};

static int lp586x_read_regs(const struct device *dev, uint16_t reg, uint8_t *val,
			    uint32_t num_bytes)
{
	const struct lp586x_config *config = dev->config;
	struct i2c_dt_spec i2c = {
		.addr = config->i2c.addr | ((reg >> 8) & 0x3),
		.bus = config->i2c.bus,
	};

	return i2c_burst_read_dt(&i2c, reg & 0xFF, val, num_bytes);
}

static int lp586x_write_regs(const struct device *dev, uint16_t reg, const uint8_t *val,
			     uint32_t num_bytes)
{
	const struct lp586x_config *config = dev->config;
	struct i2c_dt_spec i2c = {
		.addr = config->i2c.addr | ((reg >> 8) & 0x3),
		.bus = config->i2c.bus,
	};

	return i2c_burst_write_dt(&i2c, reg & 0xFF, val, num_bytes);
}

static int lp586x_reset(const struct device *dev)
{
	uint8_t reset = 0xFF;

	return lp586x_write_regs(dev, LP586X_REG_RESET, &reset, 1);
}

static int lp586x_enable(const struct device *dev, bool enable)
{
	uint8_t chip_en = enable ? 0x01 : 0x00;
	int ret = lp586x_write_regs(dev, LP586X_REG_CHIP_EN, &chip_en, 1);

	if (ret) {
		return ret;
	}

	k_usleep(LP586X_T_CHIP_EN_US);
	return 0;
}

static int lp586x_led_pwm_bri(const struct device *dev, uint32_t led, const uint8_t *values)
{
	const struct lp586x_config *config = dev->config;
	uint16_t start_channel = LP586X_REG_PWM_BRI + led * config->channels_per_led;
	int ret = lp586x_write_regs(dev, start_channel, values, config->channels_per_led);

	if (ret) {
		LOG_ERR("write pwm_bri failed");
	}

	return ret;
}

static int lp586x_led_single_pwm_bri(const struct device *dev, uint32_t led, uint8_t brightness)
{
	const struct lp586x_config *config = dev->config;
	uint8_t buf[LP586X_MAX_CHANNELS_PER_LED];

	memset(buf, brightness, config->channels_per_led);

	if (led >= config->num_lines * config->num_channels / config->channels_per_led) {
		LOG_ERR("invalid LED index");
		return -EINVAL;
	}

	return lp586x_led_pwm_bri(dev, led, buf);
}

static int lp586x_led_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	uint8_t brightness = value * LP586X_PWM_BRI_MAX / LED_BRIGHTNESS_MAX;

	return lp586x_led_single_pwm_bri(dev, led, brightness);
}

static int lp586x_led_set_color(const struct device *dev, uint32_t led, uint8_t num_colors,
				const uint8_t *color)
{
	const struct lp586x_config *config = dev->config;

	if (led >= config->num_lines * config->num_channels / config->channels_per_led) {
		LOG_ERR("invalid LED index");
		return -EINVAL;
	}

	if (num_colors != config->channels_per_led) {
		LOG_ERR("invalid number of colors");
		return -EINVAL;
	}

	return lp586x_led_pwm_bri(dev, led, color);
}

static int lp586x_led_write_channels(const struct device *dev, uint32_t start_channel,
				     uint32_t num_channels, const uint8_t *buf)
{
	const struct lp586x_config *config = dev->config;

	if (start_channel + num_channels > config->num_lines * config->num_channels) {
		LOG_ERR("invalid channel range");
		return -EINVAL;
	}

	return lp586x_write_regs(dev, LP586X_REG_PWM_BRI + start_channel, buf, num_channels);
}

static int lp586x_led_init(const struct device *dev)
{
	const struct lp586x_config *config = dev->config;
	int ret;
	uint8_t val;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("%s is not ready", config->i2c.bus->name);
		return -ENODEV;
	}

	if (config->enable.port) {
		if (!gpio_is_ready_dt(&config->enable)) {
			LOG_ERR("%s is not ready", config->enable.port->name);
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->enable, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("failed to configure enable");
			return ret;
		}
		k_usleep(LP586X_T_POR_US);
	}

	ret = lp586x_reset(dev);
	if (ret < 0) {
		LOG_ERR("reset failed");
		return ret;
	}

	ret = lp586x_enable(dev, true);
	if (ret < 0) {
		LOG_ERR("chip_en failed");
		return ret;
	}

	val = (config->num_lines << 3) | (config->data_ref_mode << 1);
	ret = lp586x_write_regs(dev, LP586X_REG_DEV_INITIAL, &val, 1);
	if (ret < 0) {
		LOG_ERR("dev_initial failed");
		return ret;
	}

	ret = lp586x_read_regs(dev, LP586X_REG_DEV_CONFIG3, &val, 1);
	if (ret < 0) {
		LOG_ERR("read dev_config3 failed");
		return ret;
	}

	val = (val & ~LP586X_REG_DEV_CONFIG3_MC_MASK) | (config->max_current << 1);
	ret = lp586x_write_regs(dev, LP586X_REG_DEV_CONFIG3, &val, 1);
	if (ret < 0) {
		LOG_ERR("write dev_config3 failed");
		return ret;
	}

	return 0;
}

static DEVICE_API(led, lp586x_led_api) = {
	.set_brightness = lp586x_led_set_brightness,
	.set_color = lp586x_led_set_color,
	.write_channels = lp586x_led_write_channels,
};

/* From Table 7-1 in the LP586x family datasheet. */
#define LP586X_MAX_CURRENT_UA_CONVERT(ua)                                                          \
	(((ua) == 7500)    ? 0                                                                     \
	 : ((ua) == 12500) ? 1                                                                     \
	 : ((ua) == 25000) ? 2                                                                     \
	 : ((ua) == 37500) ? 3                                                                     \
	 : ((ua) == 50000) ? 4                                                                     \
	 : ((ua) == 75000) ? 5                                                                     \
			   : 6)

#define LP586X_DEVICE(n, id)                                                                       \
	BUILD_ASSERT((DT_REG_ADDR(DT_DRV_INST(n)) & 0x3) == 0,                                     \
		     "I2C address must have lower 2 bits zero to properly address registers");     \
	BUILD_ASSERT(DT_INST_PROP(n, channels_per_led) > 0,                                        \
		     "channels_per_led must be greater than 0");                                   \
	BUILD_ASSERT(DT_INST_PROP(n, channels_per_led) <= LP586X_MAX_CHANNELS_PER_LED,             \
		     "channels_per_led must be less than or equal to "                             \
		     STRINGIFY(LP586X_MAX_CHANNELS_PER_LED));                                      \
                                                                                                   \
	static const struct lp586x_config lp##id##_config_##n = {                                  \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.enable = GPIO_DT_SPEC_INST_GET_OR(n, enable_gpios, {0}),                          \
		.data_ref_mode = DT_INST_PROP_OR(n, data_ref_mode, 1) - 1,                         \
		.num_lines = DT_INST_PROP(n, num_lines),                                           \
		.num_channels = DT_INST_PROP(n, num_channels),                                     \
		.channels_per_led = DT_INST_PROP(n, channels_per_led),                             \
		.max_current = LP586X_MAX_CURRENT_UA_CONVERT(DT_INST_PROP(n, max_current_ua)),     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, lp586x_led_init, NULL, NULL, &lp##id##_config_##n, POST_KERNEL,   \
			      CONFIG_LED_INIT_PRIORITY, &lp586x_led_api);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5860
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP586X_DEVICE, 5860)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5866
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP586X_DEVICE, 5866)
