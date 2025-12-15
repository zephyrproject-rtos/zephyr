/*
 * Copyright (c) 2026 Tridonic GmbH & Co KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tlc59116

/**
 * @file
 * @brief LED driver for the TLC59116 I2C LED driver
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tlc59116, CONFIG_LED_LOG_LEVEL);

/* TLC59116 max supported LED id */
#define TLC59116_MAX_LED 15

/* TLC59116 Register Addresses */
#define TLC59116_MODE1   0x00
#define TLC59116_MODE2   0x01
#define TLC59116_PWM0    0x02
#define TLC59116_GRPPWM  0x12
#define TLC59116_GRPFREQ 0x13
#define TLC59116_LEDOUT0 0x14
#define TLC59116_IREF    0x1C
#define TLC59116_EFLAG1  0x1D
#define TLC59116_EFLAG2  0x1E

#define NUM_LEDOUT_REGS    4
#define AUTO_INCREMENT_ALL 0xE0

struct tlc59116_cfg {
	struct i2c_dt_spec i2c;
};

static int tlc59116_led_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct tlc59116_cfg *config = dev->config;
	uint8_t val;

	if (led > TLC59116_MAX_LED) {
		return -EINVAL;
	}

	/* Set the LED brightness value */
	val = (value * 255U) / LED_BRIGHTNESS_MAX;
	if (i2c_reg_write_byte_dt(&config->i2c, TLC59116_PWM0 + led, val)) {
		LOG_ERR("LED 0x%x reg write failed", TLC59116_PWM0 + led);
		return -EIO;
	}

	return 0;
}

static inline int tlc59116_led_on(const struct device *dev, uint32_t led)
{
	if (led > TLC59116_MAX_LED) {
		return -EINVAL;
	}

	/* Set LED state to ON */
	return tlc59116_led_set_brightness(dev, led, 100);
}

static inline int tlc59116_led_off(const struct device *dev, uint32_t led)
{
	if (led > TLC59116_MAX_LED) {
		return -EINVAL;
	}

	/* Set LED state to OFF */
	return tlc59116_led_set_brightness(dev, led, 0);
}

static int tlc59116_led_write_channels(const struct device *dev, uint32_t start_channel,
				       uint32_t num_channels, const uint8_t *buf)
{
	const struct tlc59116_cfg *config = dev->config;
	uint32_t i2c_len = num_channels;
	uint8_t i2c_msg[TLC59116_MAX_LED + 1];

	if (num_channels > (TLC59116_MAX_LED + 1)) {
		LOG_ERR("Invalid number of channels");
		return -EINVAL;
	}

	memcpy(&i2c_msg[0], buf, i2c_len);

	return i2c_burst_write_dt(&config->i2c,
				  AUTO_INCREMENT_ALL | (TLC59116_PWM0 + start_channel), i2c_msg,
				  i2c_len);
}

static int tlc59116_led_init(const struct device *dev)
{
	const struct tlc59116_cfg *config = dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus device %s is not ready", config->i2c.bus->name);
		return -ENODEV;
	}

	/* Initialize the chip */
	/* MODE1: enable oscillator and enable all-call */
	if (i2c_reg_write_byte_dt(&config->i2c, TLC59116_MODE1, 0x01)) {
		return -EIO;
	}

	/* Switch on all channels */
	uint8_t buf[NUM_LEDOUT_REGS] = {0xff, 0xff, 0xff, 0xff};

	/* All channels to PWM control */
	if (i2c_burst_write_dt(&config->i2c, AUTO_INCREMENT_ALL | TLC59116_LEDOUT0, buf,
			       NUM_LEDOUT_REGS)) {
		return -EIO;
	}

	return 0;
}

static DEVICE_API(led, tlc59116_led_api) = {.set_brightness = tlc59116_led_set_brightness,
					    .on = tlc59116_led_on,
					    .off = tlc59116_led_off,
					    .write_channels = tlc59116_led_write_channels};

#define TLC59116_DEVICE(id)                                                                        \
	static const struct tlc59116_cfg tlc59116_##id##_cfg = {                                   \
		.i2c = I2C_DT_SPEC_INST_GET(id),                                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, &tlc59116_led_init, NULL, NULL, &tlc59116_##id##_cfg,            \
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY, &tlc59116_led_api);

DT_INST_FOREACH_STATUS_OKAY(TLC59116_DEVICE)
