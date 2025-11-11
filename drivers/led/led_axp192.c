/*
 * Copyright (c) 2024 TOKITA Hiroshi
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/led.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(led_axp192, CONFIG_LED_LOG_LEVEL);

#define CHGLED_OUTPUT_HIZ        0x0
#define CHGLED_OUTPUT_SLOW_BLINK 0x1
#define CHGLED_OUTPUT_FAST_BLINK 0x2
#define CHGLED_OUTPUT_DRIVE_LOW  0x3

#define CHGLED_OUTPUT_OFFSET 4

#define CHGLED_ON          (CHGLED_OUTPUT_DRIVE_LOW << CHGLED_OUTPUT_OFFSET)
#define CHGLED_OFF         (CHGLED_OUTPUT_HIZ << CHGLED_OUTPUT_OFFSET)
#define CHGLED_BLINK_SLOW  (CHGLED_OUTPUT_SLOW_BLINK << CHGLED_OUTPUT_OFFSET)
#define CHGLED_BLINK_FAST  (CHGLED_OUTPUT_FAST_BLINK << CHGLED_OUTPUT_OFFSET)
#define CHGLED_OUTPUT_MASK (BIT_MASK(2) << CHGLED_OUTPUT_OFFSET)

#define SLOW_BLINK_DELAY_ON  (((1000 / 4) / 1) * 1)
#define SLOW_BLINK_DELAY_OFF (((1000 / 4) / 1) * 3)
#define FAST_BLINK_DELAY_ON  (((1000 / 4) / 4) * 1)
#define FAST_BLINK_DELAY_OFF (((1000 / 4) / 4) * 3)

#define CHGLED_CTRL_TYPE_A    0x0
#define CHGLED_CTRL_TYPE_B    0x1
#define CHGLED_CTRL_BY_REG    0x2
#define CHGLED_CTRL_BY_CHARGE 0x3

#define AXP192_REG_PWROFF_BATTCHK_CHGLED 0x32
#define AXP192_REG_CHGLED                AXP192_REG_PWROFF_BATTCHK_CHGLED
#define AXP192_CHGLED_CTRL_MASK          0x2
#define AXP192_CHGLED_CTRL_OFFSET        2

#define AXP2101_REG_CHGLED         0x69
#define AXP2101_CHGLED_CTRL_MASK   0x3
#define AXP2101_CHGLED_CTRL_OFFSET 1

struct led_axp192_config {
	struct i2c_dt_spec i2c;
	uint8_t addr;
	uint8_t mode;
	uint8_t mode_mask;
	uint8_t mode_offset;
};

static int led_axp192_on(const struct device *dev, uint32_t led)
{
	const struct led_axp192_config *config = dev->config;

	ARG_UNUSED(led);

	if (config->mode != CHGLED_CTRL_BY_REG) {
		return -EINVAL;
	}

	return i2c_reg_update_byte_dt(&config->i2c, config->addr, CHGLED_OUTPUT_MASK, CHGLED_ON);
}

static int led_axp192_off(const struct device *dev, uint32_t led)
{
	const struct led_axp192_config *config = dev->config;

	ARG_UNUSED(led);

	if (config->mode != CHGLED_CTRL_BY_REG) {
		return -EINVAL;
	}

	return i2c_reg_update_byte_dt(&config->i2c, config->addr, CHGLED_OUTPUT_MASK, CHGLED_OFF);
}

static int led_axp192_blink(const struct device *dev, uint32_t led, uint32_t delay_on,
			    uint32_t delay_off)
{
	const struct led_axp192_config *config = dev->config;

	ARG_UNUSED(led);

	if (config->mode != CHGLED_CTRL_BY_REG) {
		return -EINVAL;
	}

	if ((delay_on == SLOW_BLINK_DELAY_ON) && (delay_off == SLOW_BLINK_DELAY_OFF)) {
		return i2c_reg_update_byte_dt(&config->i2c, config->addr, CHGLED_OUTPUT_MASK,
					      CHGLED_BLINK_SLOW);
	} else if ((delay_on == FAST_BLINK_DELAY_ON) && (delay_off == FAST_BLINK_DELAY_OFF)) {
		return i2c_reg_update_byte_dt(&config->i2c, config->addr, CHGLED_OUTPUT_MASK,
					      CHGLED_BLINK_FAST);
	} else {
		LOG_ERR("The AXP192 blink setting can only %d/%d or %d/%d. (%d/%d)",
			SLOW_BLINK_DELAY_ON, SLOW_BLINK_DELAY_OFF, FAST_BLINK_DELAY_ON,
			FAST_BLINK_DELAY_OFF, delay_on, delay_off);
	}

	return -ENOTSUP;
}

static DEVICE_API(led, led_axp192_api) = {
	.on = led_axp192_on,
	.off = led_axp192_off,
	.blink = led_axp192_blink,
};

static int led_axp192_init(const struct device *dev)
{
	const struct led_axp192_config *config = dev->config;

	switch (config->mode) {
	case CHGLED_CTRL_TYPE_A:
	case CHGLED_CTRL_TYPE_B:
	case CHGLED_CTRL_BY_REG:
	case CHGLED_CTRL_BY_CHARGE:
		return i2c_reg_update_byte_dt(&config->i2c, config->addr,
					      config->mode_mask << config->mode_offset,
					      config->mode << config->mode_offset);
	}

	return -EINVAL;
}

#define LED_AXPXXXX_DEFINE(n, model, compat)                                                       \
	static const struct led_axp192_config led_axp_config_##model##_##n = {                     \
		.i2c = I2C_DT_SPEC_GET(DT_PARENT(n)),                                              \
		.addr = AXP##model##_REG_CHGLED,                                                   \
		.mode = UTIL_CAT(CHGLED_CTRL_, DT_STRING_UPPER_TOKEN(n, x_powers_mode)),           \
		.mode_mask = AXP##model##_CHGLED_CTRL_MASK,                                        \
		.mode_offset = AXP##model##_CHGLED_CTRL_OFFSET,                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(n, led_axp192_init, NULL, NULL, &led_axp_config_##model##_##n,            \
			 POST_KERNEL, CONFIG_LED_INIT_PRIORITY, &led_axp192_api);

#define LED_AXP192_DEFINE(n)  LED_AXPXXXX_DEFINE(n, 192, x_powers_axp192_led)
#define LED_AXP2101_DEFINE(n) LED_AXPXXXX_DEFINE(n, 2101, x_powers_axp2101_led)

DT_FOREACH_STATUS_OKAY(x_powers_axp192_led, LED_AXP192_DEFINE)
DT_FOREACH_STATUS_OKAY(x_powers_axp2101_led, LED_AXP2101_DEFINE)
