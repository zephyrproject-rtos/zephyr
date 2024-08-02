/*
 * Copyright (c) 2023 Endor AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT issi_is31fl3216a

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/logging/log.h>

#define IS31FL3216A_REG_CONFIG		0x00
#define IS31FL3216A_REG_CTL_1		0x01
#define IS31FL3216A_REG_CTL_2		0x02
#define IS31FL3216A_REG_LIGHT_EFFECT	0x03
#define IS31FL3216A_REG_CHANNEL_CONFIG	0x04
#define IS31FL3216A_REG_GPIO_CONFIG	0x05
#define IS31FL3216A_REG_OUTPUT_PORT	0x06
#define IS31FL3216A_REG_INT_CONTROL	0x07
#define IS31FL3216A_REG_ADC_SAMPLE_RATE	0x09
#define IS31FL3216A_REG_PWM_FIRST	0x10
#define IS31FL3216A_REG_PWM_LAST	0x1F
#define IS31FL3216A_REG_UPDATE		0xB0
#define IS31FL3216A_REG_FRAME_DELAY	0xB6
#define IS31FL3216A_REG_FRAME_START	0xB7

#define IS31FL3216A_MAX_LEDS		16

LOG_MODULE_REGISTER(is31fl3216a, CONFIG_LED_LOG_LEVEL);

struct is31fl3216a_cfg {
	struct i2c_dt_spec i2c;
};

static int is31fl3216a_write_buffer(const struct i2c_dt_spec *i2c,
				    const uint8_t *buffer, uint32_t num_bytes)
{
	int status;

	status = i2c_write_dt(i2c, buffer, num_bytes);
	if (status < 0) {
		LOG_ERR("Could not write buffer: %i", status);
		return status;
	}

	return 0;
}

static int is31fl3216a_write_reg(const struct i2c_dt_spec *i2c, uint8_t reg,
				 uint8_t val)
{
	uint8_t buffer[2] = {reg, val};

	return is31fl3216a_write_buffer(i2c, buffer, sizeof(buffer));
}

static int is31fl3216a_update_pwm(const struct i2c_dt_spec *i2c)
{
	return is31fl3216a_write_reg(i2c, IS31FL3216A_REG_UPDATE, 0);
}

static uint8_t is31fl3216a_brightness_to_pwm(uint8_t brightness)
{
	return (0xFFU * brightness) / 100;
}

static int is31fl3216a_led_write_channels(const struct device *dev,
					  uint32_t start_channel,
					  uint32_t num_channels,
					  const uint8_t *buf)
{
	const struct is31fl3216a_cfg *config = dev->config;
	uint8_t i2c_buffer[IS31FL3216A_MAX_LEDS + 1];
	int status;
	int i;

	if (num_channels == 0) {
		return 0;
	}

	if (start_channel + num_channels > IS31FL3216A_MAX_LEDS) {
		return -EINVAL;
	}

	/* Invert channels, last register is channel 0 */
	i2c_buffer[0] = IS31FL3216A_REG_PWM_LAST - start_channel -
			(num_channels - 1);
	for (i = 0; i < num_channels; i++) {
		if (buf[num_channels - i - 1] > 100) {
			return -EINVAL;
		}
		i2c_buffer[i + 1] = is31fl3216a_brightness_to_pwm(
			buf[num_channels - i - 1]);
	}

	status = is31fl3216a_write_buffer(&config->i2c, i2c_buffer,
					  num_channels + 1);
	if (status < 0) {
		return status;
	}

	return is31fl3216a_update_pwm(&config->i2c);
}

static int is31fl3216a_led_set_brightness(const struct device *dev,
					  uint32_t led, uint8_t value)
{
	const struct is31fl3216a_cfg *config = dev->config;
	uint8_t pwm_reg = IS31FL3216A_REG_PWM_LAST - led;
	int status;
	uint8_t pwm_value;

	if (led > IS31FL3216A_MAX_LEDS - 1 || value > 100) {
		return -EINVAL;
	}

	pwm_value = is31fl3216a_brightness_to_pwm(value);
	status = is31fl3216a_write_reg(&config->i2c, pwm_reg, pwm_value);
	if (status < 0) {
		return status;
	}

	return is31fl3216a_update_pwm(&config->i2c);
}

static int is31fl3216a_led_on(const struct device *dev, uint32_t led)
{
	return is31fl3216a_led_set_brightness(dev, led, 100);
}

static int is31fl3216a_led_off(const struct device *dev, uint32_t led)
{
	return is31fl3216a_led_set_brightness(dev, led, 0);
}

static int is31fl3216a_init_registers(const struct i2c_dt_spec *i2c)
{
	int i;
	int status;

	status = is31fl3216a_write_reg(i2c, IS31FL3216A_REG_CTL_1, 0xFF);
	if (status < 0) {
		return status;
	}

	status = is31fl3216a_write_reg(i2c, IS31FL3216A_REG_CTL_2, 0xFF);
	if (status < 0) {
		return status;
	}

	status = is31fl3216a_write_reg(i2c, IS31FL3216A_REG_LIGHT_EFFECT, 0x00);
	if (status < 0) {
		return status;
	}

	status = is31fl3216a_write_reg(i2c, IS31FL3216A_REG_CHANNEL_CONFIG,
				       0x00);
	if (status < 0) {
		return status;
	}

	status = is31fl3216a_write_reg(i2c, IS31FL3216A_REG_GPIO_CONFIG, 0x00);
	if (status < 0) {
		return status;
	}

	status = is31fl3216a_write_reg(i2c, IS31FL3216A_REG_OUTPUT_PORT, 0x00);
	if (status < 0) {
		return status;
	}

	status = is31fl3216a_write_reg(i2c, IS31FL3216A_REG_INT_CONTROL, 0x00);
	if (status < 0) {
		return status;
	}

	status = is31fl3216a_write_reg(i2c, IS31FL3216A_REG_ADC_SAMPLE_RATE,
				       0x00);
	if (status < 0) {
		return status;
	}

	status = is31fl3216a_write_reg(i2c, IS31FL3216A_REG_FRAME_DELAY, 0x00);
	if (status < 0) {
		return status;
	}

	status = is31fl3216a_write_reg(i2c, IS31FL3216A_REG_FRAME_START, 0x00);
	if (status < 0) {
		return status;
	}

	for (i = IS31FL3216A_REG_PWM_FIRST;
	     i <= IS31FL3216A_REG_PWM_LAST;
	     i++) {
		status = is31fl3216a_write_reg(i2c, i, 0);
		if (status < 0) {
			return status;
		}
	}

	status = is31fl3216a_write_reg(i2c, IS31FL3216A_REG_UPDATE, 0);
	if (status < 0) {
		return status;
	}

	return is31fl3216a_write_reg(i2c, IS31FL3216A_REG_CONFIG, 0x00);
}

static int is31fl3216a_init(const struct device *dev)
{
	const struct is31fl3216a_cfg *config = dev->config;

	LOG_DBG("Initializing @0x%x...", config->i2c.addr);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	return is31fl3216a_init_registers(&config->i2c);
}

static const struct led_driver_api is31fl3216a_led_api = {
	.set_brightness = is31fl3216a_led_set_brightness,
	.on = is31fl3216a_led_on,
	.off = is31fl3216a_led_off,
	.write_channels = is31fl3216a_led_write_channels
};

#define IS31FL3216A_INIT(id) \
	static const struct is31fl3216a_cfg is31fl3216a_##id##_cfg = {	\
		.i2c = I2C_DT_SPEC_INST_GET(id),			\
	};								\
	DEVICE_DT_INST_DEFINE(id, &is31fl3216a_init, NULL, NULL,	\
		&is31fl3216a_##id##_cfg, POST_KERNEL,			\
		CONFIG_LED_INIT_PRIORITY, &is31fl3216a_led_api);

DT_INST_FOREACH_STATUS_OKAY(IS31FL3216A_INIT)
