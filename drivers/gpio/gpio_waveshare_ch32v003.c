/*
 * Copyright (c) 2026 Kim Bøndergaard <kim@fam-boendergaard.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for the Waveshare CH32V003 I2C IO expander.
 * PWM and ADC features of the chip are not supported by this driver.
 */

#define DT_DRV_COMPAT waveshare_ch32v003

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

#include "../adc/adc_waveshare_ch32v003.h"

LOG_MODULE_REGISTER(gpio_ch32v003, CONFIG_GPIO_LOG_LEVEL);

enum ch32v003_reg {
	REG_DIRECTION = 0x02,
	REG_OUTPUT = 0x03,
	REG_INPUT = 0x04,
	REG_PWM = 0x05,
	REG_ADC = 0x06,
};

/* Chip reset defaults: all pins output (see REG_DIRECTION polarity note above), output latch
 * cleared
 */
#define DIRECTION_RESET_VAL 0xffU
#define OUTPUT_RESET_VAL    0x00U

struct ch32v003_config {
	/* gpio_driver_config needs to be first */
	const struct gpio_driver_config common;
	const struct i2c_dt_spec bus;
};

struct ch32v003_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	struct k_mutex lock;
	/* the chip's direction/output registers cannot be read back over I2C;
	 * cache the last written value like the vendor driver does
	 */
	uint8_t direction;
	uint8_t output;
};

int ch32v003_get_adc_value(const struct device *dev, uint16_t *value)
{
	const struct ch32v003_config *cfg = dev->config;
	struct ch32v003_data *data = dev->data;
	uint8_t raw[2];
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = i2c_burst_read_dt(&cfg->bus, REG_ADC, raw, sizeof(raw));
	k_mutex_unlock(&data->lock);

	if (ret < 0) {
		return -EIO;
	}

	/* register holds the raw ADC sample little-endian */
	*value = (uint16_t)raw[1] << 8 | raw[0];

	return 0;
}

static int ch32v003_reg_set_masked(const struct i2c_dt_spec *bus, enum ch32v003_reg reg,
				   uint8_t *cache, uint8_t mask, uint8_t val)
{
	uint8_t reg_data = (*cache & ~mask) | (mask & val);

	LOG_DBG("set masked reg 0x%02x: mask 0x%02x, val 0x%02x, old 0x%02x, new 0x%02x", reg, mask,
		val, *cache, reg_data);

	if (i2c_reg_write_byte_dt(bus, reg, reg_data) < 0) {
		return -EIO;
	}
	*cache = reg_data;
	return 0;
}

static int ch32v003_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct ch32v003_config *cfg = dev->config;
	struct ch32v003_data *data = dev->data;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if ((flags & GPIO_SINGLE_ENDED) || (flags & GPIO_PULL_DOWN) || (flags & GPIO_PULL_UP)) {
		LOG_ERR("open-drain, open-source and pull resistors are not supported (pin %d)",
			pin);
		return -ENOTSUP;
	}

	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == GPIO_DISCONNECTED) {
		LOG_ERR("disconnected GPIO is not supported (pin %d)", pin);
		return -ENOTSUP;
	}

	if ((flags & GPIO_INPUT) && (flags & GPIO_OUTPUT)) {
		LOG_ERR("simultaneous input and output is not supported (pin %d)", pin);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	int ret = 0;

	LOG_DBG("configuring pin %d as %s", pin, (flags & GPIO_INPUT) ? "input" : "output");

	if ((flags & GPIO_OUTPUT_INIT_LOW) || (flags & GPIO_OUTPUT_INIT_HIGH)) {
		LOG_DBG("setting initial output value for pin %d to %s", pin,
			(flags & GPIO_OUTPUT_INIT_HIGH) ? "high" : "low");
		ret = ch32v003_reg_set_masked(&cfg->bus, REG_OUTPUT, &data->output, BIT(pin),
					      (flags & GPIO_OUTPUT_INIT_HIGH) ? BIT(pin) : 0);
		if (ret < 0) {
			goto out;
		}
	}

	/* direction register: 1 = output, 0 = input (verified against the vendor's
	 * esp_io_expander_set_dir(), which uses dir_out_bit_zero = 0 for this chip)
	 */
	ret = ch32v003_reg_set_masked(&cfg->bus, REG_DIRECTION, &data->direction, BIT(pin),
				      (flags & GPIO_OUTPUT) ? BIT(pin) : 0);
out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int ch32v003_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct ch32v003_config *cfg = dev->config;
	struct ch32v003_data *data = dev->data;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	int ret = 0;
	uint8_t inp_reg;

	ret = i2c_reg_read_byte_dt(&cfg->bus, REG_INPUT, &inp_reg);
	if (ret >= 0) {
		*value = inp_reg;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int ch32v003_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	const struct ch32v003_config *cfg = dev->config;
	struct ch32v003_data *data = dev->data;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	int ret = ch32v003_reg_set_masked(&cfg->bus, REG_OUTPUT, &data->output, mask, value);

	k_mutex_unlock(&data->lock);
	return ret;
}

static int ch32v003_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return ch32v003_port_set_masked_raw(dev, pins, pins);
}

static int ch32v003_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct ch32v003_config *cfg = dev->config;
	struct ch32v003_data *data = dev->data;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	int ret = ch32v003_reg_set_masked(&cfg->bus, REG_OUTPUT, &data->output, pins, 0);

	k_mutex_unlock(&data->lock);
	return ret;
}

static int ch32v003_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct ch32v003_config *cfg = dev->config;
	struct ch32v003_data *data = dev->data;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* output register is not readable back; toggle against the cached value */
	uint8_t reg_data = data->output ^ pins;
	int ret = i2c_reg_write_byte_dt(&cfg->bus, REG_OUTPUT, reg_data);

	if (ret >= 0) {
		data->output = reg_data;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int ch32v003_init(const struct device *dev)
{
	const struct ch32v003_config *cfg = dev->config;
	struct ch32v003_data *data = dev->data;
	int ret = 0;

	ret = k_mutex_init(&data->lock);
	if (ret < 0) {
		return ret;
	}

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("%s is not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	ret = i2c_reg_write_byte_dt(&cfg->bus, REG_DIRECTION, DIRECTION_RESET_VAL);
	if (ret < 0) {
		return ret;
	}
	data->direction = DIRECTION_RESET_VAL;

	ret = i2c_reg_write_byte_dt(&cfg->bus, REG_OUTPUT, OUTPUT_RESET_VAL);
	if (ret < 0) {
		return ret;
	}
	data->output = OUTPUT_RESET_VAL;

	return 0;
}

static DEVICE_API(gpio, ch32v003_api) = {
	.pin_configure = ch32v003_pin_configure,
	.port_get_raw = ch32v003_port_get_raw,
	.port_set_masked_raw = ch32v003_port_set_masked_raw,
	.port_set_bits_raw = ch32v003_port_set_bits_raw,
	.port_clear_bits_raw = ch32v003_port_clear_bits_raw,
	.port_toggle_bits = ch32v003_port_toggle_bits,
};

#define GPIO_CH32V003_INST_DEFINE(n)                                                               \
	static const struct ch32v003_config config_##n = {                                         \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),                                      \
		.bus = I2C_DT_SPEC_GET(DT_DRV_INST(n))};                                           \
	static struct ch32v003_data data_##n = {};                                                 \
	DEVICE_DT_INST_DEFINE(n, ch32v003_init, NULL, &data_##n, &config_##n, POST_KERNEL,         \
			      CONFIG_GPIO_CH32V003_INIT_PRIORITY, &ch32v003_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_CH32V003_INST_DEFINE)
