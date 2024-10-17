/*
 * Copyright (c) 2024 TOKITA Hiroshi
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT x_powers_axp2101_led

#include <zephyr/drivers/led.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/axp2101.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(led_axp2101, CONFIG_LED_LOG_LEVEL);

#define AXP2101_CHGLED_ON                                                                          \
	(AXP2101_CHGLED_OUTPUT_DRIVE_LOW << AXP2101_CHGLED_OUTPUT_OFFSET) |                        \
		(AXP2101_CHGLED_CTRL_BY_REG << AXP2101_CHGLED_CTRL_OFFSET) |                       \
		(AXP2101_CHGLED_PIN_ENABLE << AXP2101_CHGLED_PIN_OFFSET)

#define AXP2101_CHGLED_OFF                                                                         \
	(AXP2101_CHGLED_OUTPUT_HIZ << AXP2101_CHGLED_OUTPUT_OFFSET) |                              \
		(AXP2101_CHGLED_CTRL_BY_REG << AXP2101_CHGLED_CTRL_OFFSET) |                       \
		(AXP2101_CHGLED_PIN_ENABLE << AXP2101_CHGLED_PIN_OFFSET)

#define AXP2101_CHGLED_BLINK_SLOW                                                                  \
	(AXP2101_CHGLED_OUTPUT_SLOW_BLINK << AXP2101_CHGLED_OUTPUT_OFFSET) |                       \
		(AXP2101_CHGLED_CTRL_BY_REG << AXP2101_CHGLED_CTRL_OFFSET) |                       \
		(AXP2101_CHGLED_PIN_ENABLE << AXP2101_CHGLED_PIN_OFFSET)

#define AXP2101_CHGLED_BLINK_FAST                                                                  \
	(AXP2101_CHGLED_OUTPUT_FAST_BLINK << AXP2101_CHGLED_OUTPUT_OFFSET) |                       \
		(AXP2101_CHGLED_CTRL_BY_REG << AXP2101_CHGLED_CTRL_OFFSET) |                       \
		(AXP2101_CHGLED_PIN_ENABLE << AXP2101_CHGLED_PIN_OFFSET)

#define AXP2101_CHGLED_TYPE_A                                                                      \
	(AXP2101_CHGLED_CTRL_TYPE_A << AXP2101_CHGLED_CTRL_OFFSET) |                               \
		(AXP2101_CHGLED_PIN_ENABLE << AXP2101_CHGLED_PIN_OFFSET)

#define AXP2101_CHGLED_TYPE_B                                                                      \
	(AXP2101_CHGLED_CTRL_TYPE_B << AXP2101_CHGLED_CTRL_OFFSET) |                               \
		(AXP2101_CHGLED_PIN_ENABLE << AXP2101_CHGLED_PIN_OFFSET)

struct led_axp2101_config {
	struct i2c_dt_spec i2c;
	uint32_t mode;
};

static int led_axp2101_on(const struct device *dev, uint32_t led)
{
	const struct led_axp2101_config *config = dev->config;

	ARG_UNUSED(led);

	if (config->mode != AXP2101_CHGLED_CTRL_BY_REG) {
		return -EINVAL;
	}

	return i2c_reg_write_byte_dt(&config->i2c, AXP2101_REG_CHGLED, AXP2101_CHGLED_ON);
}

static int led_axp2101_off(const struct device *dev, uint32_t led)
{
	const struct led_axp2101_config *config = dev->config;

	ARG_UNUSED(led);

	if (config->mode != AXP2101_CHGLED_CTRL_BY_REG) {
		return -EINVAL;
	}

	return i2c_reg_write_byte_dt(&config->i2c, AXP2101_REG_CHGLED, AXP2101_CHGLED_OFF);
}

static int led_axp2101_blink(const struct device *dev, uint32_t led, uint32_t delay_on,
			     uint32_t delay_off)
{
	const struct led_axp2101_config *config = dev->config;

	ARG_UNUSED(led);

	if (config->mode != AXP2101_CHGLED_CTRL_BY_REG) {
		return -EINVAL;
	}

	if ((delay_on == AXP2101_SLOW_BLINK_DELAY_ON) &&
	    (delay_off == AXP2101_SLOW_BLINK_DELAY_OFF)) {
		return i2c_reg_write_byte_dt(&config->i2c, AXP2101_REG_CHGLED,
					     AXP2101_CHGLED_BLINK_SLOW);
	} else if ((delay_on == AXP2101_FAST_BLINK_DELAY_ON) &&
		   (delay_off == AXP2101_FAST_BLINK_DELAY_OFF)) {
		return i2c_reg_write_byte_dt(&config->i2c, AXP2101_REG_CHGLED,
					     AXP2101_CHGLED_BLINK_FAST);
	} else {
		LOG_ERR("The AXP2101 blink setting can only 250/750 or 62/186. (%d/%d)", delay_on,
			delay_off);
	}

	return -ENOTSUP;
}

static const struct led_driver_api led_axp2101_api = {
	.on = led_axp2101_on,
	.off = led_axp2101_off,
	.blink = led_axp2101_blink,
};

static int led_axp2101_init(const struct device *dev)
{
	const struct led_axp2101_config *config = dev->config;

	switch (config->mode) {
	case AXP2101_CHGLED_CTRL_TYPE_A:
		return i2c_reg_write_byte_dt(&config->i2c, AXP2101_REG_CHGLED,
					     AXP2101_CHGLED_TYPE_A);
	case AXP2101_CHGLED_CTRL_TYPE_B:
		return i2c_reg_write_byte_dt(&config->i2c, AXP2101_REG_CHGLED,
					     AXP2101_CHGLED_TYPE_B);
	case AXP2101_CHGLED_CTRL_BY_REG:
		return 0;
	}

	return -EINVAL;
}

#define LED_AXP2101_DEFINE(n)                                                                      \
	static const struct led_axp2101_config led_axp2101_config##n = {                           \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(n)),                                         \
		.mode = UTIL_CAT(AXP2101_CHGLED_CTRL_,                                             \
				 DT_INST_STRING_UPPER_TOKEN(n, x_powers_mode)),                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &led_axp2101_init, NULL, NULL, &led_axp2101_config##n,            \
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY, &led_axp2101_api);

DT_INST_FOREACH_STATUS_OKAY(LED_AXP2101_DEFINE)
