/*
 * Copyright (c) 2025 Tridonic GmbH & Co KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tlc59116

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tlc59116, CONFIG_LED_STRIP_LOG_LEVEL);

/* TLC59116 Register Addresses */
#define TLC59116_MODE1   0x00
#define TLC59116_MODE2   0x01
#define TLC59116_PWM0    0x02
#define TLC59116_GRPPWM  0x12
#define TLC59116_GRPFREQ 0x13
#define TLC59116_LEDOUT0 0x14
#define TLC59116_IREF    0x1C

#define NUM_LEDOUT_REGS    4
#define AUTO_INCREMENT_ALL 0xE0

struct tlc59116_config {
	struct i2c_dt_spec i2c;
	const uint8_t *color_mapping;
	size_t length;
};

static int tlc59116_update_leds(const struct device *dev, struct led_rgb *pixels, size_t num_pixels)
{
	/*
	 * The TLC59116 is a 16-channel mono driver.
	 * This driver assumes a simple mapping of pixels to channels.
	 * Although the driver uses LED_COLOR_ID_WHITE, the value from the red/r channel are used.
	 */
	const struct tlc59116_config *config = dev->config;

	uint8_t buf[16];

	for (size_t i = 0; i < num_pixels && i < 16; i++) {
		buf[i] = pixels[i].r;
	}

	int ret = i2c_burst_write_dt(&config->i2c, AUTO_INCREMENT_ALL | TLC59116_PWM0, buf,
					 num_pixels);

	return ret;
}

static int tlc59116_init(const struct device *dev)
{
	int ret;

	const struct tlc59116_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	/* Initialize the chip */
	ret = i2c_reg_write_byte_dt(&config->i2c, TLC59116_MODE1, 0x01); /* ALLCALL */
	if (ret) {
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, TLC59116_MODE2, 0x00); /* Nothing set */
	if (ret) {
		return ret;
	}

	/* Switch on all channels */
	uint8_t buf[NUM_LEDOUT_REGS] = {0xff, 0xff, 0xff, 0xff};

	ret = i2c_burst_write_dt(&config->i2c, AUTO_INCREMENT_ALL | TLC59116_LEDOUT0, buf,
				 NUM_LEDOUT_REGS); /* All channels to PWM control */
	if (ret) {
		return ret;
	}

	return 0;
}

static size_t tlc59116_strip_length(const struct device *dev)
{
	const struct tlc59116_config *config = dev->config;

	return config->length;
}

static DEVICE_API(led_strip, tlc59116_driver_api) = {
	.update_rgb = tlc59116_update_leds,
	.length = tlc59116_strip_length,
};

#define TLC59116_INIT(inst)                                         \
	static const uint8_t tlc59116_config_##inst##_color_mapping[] = \
		DT_INST_PROP(inst, color_mapping);                          \
	static const struct tlc59116_config tlc59116_config_##inst = {  \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                          \
		.color_mapping = tlc59116_config_##inst##_color_mapping,    \
		.length = DT_INST_PROP(inst, chain_length),                 \
	};                                                              \
	DEVICE_DT_INST_DEFINE(                                          \
		inst, tlc59116_init, NULL, NULL, &tlc59116_config_##inst,   \
		POST_KERNEL, CONFIG_LED_STRIP_INIT_PRIORITY,                \
		&tlc59116_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TLC59116_INIT)
