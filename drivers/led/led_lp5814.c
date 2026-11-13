/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_lp5814

#include <string.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#define LP5814_CHANNELS        4U
#define LP5814_CHIP_EN         0x00U
#define LP5814_DEV_CONFIG0     0x01U
#define LP5814_RESET_CMD       0x0EU
#define LP5814_UPDATE_CMD      0x0FU
#define LP5814_OUT0_DC         0x14U
#define LP5814_OUT0_MANUAL_PWM 0x18U
#define LP5814_RESET           0xCCU
#define LP5814_UPDATE          0x55U

enum lp5814_setting {
	LP5814_SETTING_MAX_CURRENT,
	LP5814_SETTING_OUTPUT_ENABLE,
	LP5814_SETTING_FADE,
	LP5814_SETTING_DIMMING_MODE,
	LP5814_SETTING_ENGINE_ASSIGNMENT,
	LP5814_SETTING_COUNT,
};

struct lp5814_led {
	uint32_t max_microamp;
	uint8_t channel;
};

struct lp5814_config {
	struct i2c_dt_spec bus;
	const struct lp5814_led *leds;
	size_t num_leds;
};

static int lp5814_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct lp5814_config *config = dev->config;

	if (led >= config->num_leds || value > LED_BRIGHTNESS_MAX) {
		return -EINVAL;
	}

	return i2c_reg_write_byte_dt(&config->bus,
				    LP5814_OUT0_MANUAL_PWM + config->leds[led].channel,
				    value * UINT8_MAX / LED_BRIGHTNESS_MAX);
}

static int lp5814_write_channels(const struct device *dev, uint32_t start_channel,
				 uint32_t num_channels, const uint8_t *channels)
{
	const struct lp5814_config *config = dev->config;
	uint8_t buffer[1U + LP5814_CHANNELS];

	if (start_channel >= LP5814_CHANNELS || num_channels == 0U ||
	    num_channels > LP5814_CHANNELS - start_channel || channels == NULL) {
		return -EINVAL;
	}

	buffer[0] = LP5814_OUT0_MANUAL_PWM + start_channel;
	memcpy(&buffer[1], channels, num_channels);

	return i2c_write_dt(&config->bus, buffer, num_channels + 1U);
}

static int lp5814_init(const struct device *dev)
{
	const struct lp5814_config *config = dev->config;
	uint8_t current[LP5814_CHANNELS] = {0};
	uint8_t settings[LP5814_SETTING_COUNT] = {0};
	uint32_t current_step = 100U;
	int ret;

	if (!i2c_is_ready_dt(&config->bus)) {
		return -ENODEV;
	}

	for (size_t i = 0U; i < config->num_leds; i++) {
		uint8_t mask = BIT(config->leds[i].channel);

		if ((settings[LP5814_SETTING_OUTPUT_ENABLE] & mask) != 0U) {
			return -EINVAL;
		}
		settings[LP5814_SETTING_OUTPUT_ENABLE] |= mask;
		if (config->leds[i].max_microamp > 25500U) {
			settings[LP5814_SETTING_MAX_CURRENT] = 1U;
			current_step = 200U;
		}
	}
	for (size_t i = 0U; i < config->num_leds; i++) {
		current[config->leds[i].channel] = config->leds[i].max_microamp / current_step;
	}

	/* Reset also clears PWM values, leaving every LED off during setup. */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5814_RESET_CMD, LP5814_RESET);
	if (ret != 0) {
		return ret;
	}
	k_sleep(K_MSEC(1));

	ret = i2c_reg_write_byte_dt(&config->bus, LP5814_CHIP_EN, 1U);
	if (ret != 0) {
		return ret;
	}
	ret = i2c_burst_write_dt(&config->bus, LP5814_OUT0_DC, current, sizeof(current));
	if (ret != 0) {
		return ret;
	}
	ret = i2c_burst_write_dt(&config->bus, LP5814_DEV_CONFIG0, settings, sizeof(settings));
	if (ret != 0) {
		return ret;
	}

	return i2c_reg_write_byte_dt(&config->bus, LP5814_UPDATE_CMD, LP5814_UPDATE);
}

#ifdef CONFIG_PM_DEVICE
static int lp5814_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct lp5814_config *config = dev->config;
	uint8_t enable;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		enable = 0U;
		break;
	case PM_DEVICE_ACTION_RESUME:
		enable = 1U;
		break;
	default:
		return -ENOTSUP;
	}
	if (!k_can_yield()) {
		return -EWOULDBLOCK;
	}

	/* Standby disables the outputs without resetting current or PWM settings. */
	return i2c_reg_write_byte_dt(&config->bus, LP5814_CHIP_EN, enable);
}
#endif

static DEVICE_API(led, lp5814_api) = {
	.set_brightness = lp5814_set_brightness,
	.write_channels = lp5814_write_channels,
};

#define LP5814_LED_CHECK(node)                                                                   \
	BUILD_ASSERT(DT_PROP(node, led_max_microamp) >= 0 &&                                     \
		     DT_PROP(node, led_max_microamp) <= 51000, "Invalid LED current limit");

#define LP5814_LED_CONFIG(node)                                                                  \
	{                                                                                        \
		.channel = DT_PROP(node, index),                                                 \
		.max_microamp = DT_PROP(node, led_max_microamp),                                 \
	},

#define LP5814_DEFINE(inst)                                                                      \
	DT_INST_FOREACH_CHILD(inst, LP5814_LED_CHECK)                                            \
	BUILD_ASSERT(DT_INST_CHILD_NUM(inst) > 0 &&                                              \
		     DT_INST_CHILD_NUM(inst) <= LP5814_CHANNELS, "Invalid LED count");           \
	static const struct lp5814_led lp5814_leds_##inst[] = {                                  \
		DT_INST_FOREACH_CHILD(inst, LP5814_LED_CONFIG)                                   \
	};                                                                                       \
	static const struct lp5814_config lp5814_config_##inst = {                               \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                               \
		.leds = lp5814_leds_##inst,                                                      \
		.num_leds = ARRAY_SIZE(lp5814_leds_##inst),                                      \
	};                                                                                       \
	PM_DEVICE_DT_INST_DEFINE(inst, lp5814_pm_action);                                        \
	DEVICE_DT_INST_DEFINE(inst, lp5814_init, PM_DEVICE_DT_INST_GET(inst), NULL,              \
			      &lp5814_config_##inst, \
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY, &lp5814_api);

DT_INST_FOREACH_STATUS_OKAY(LP5814_DEFINE)
