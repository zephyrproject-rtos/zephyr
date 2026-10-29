/*
 * Copyright (c) 2026 Kim Bøndergaard <kim@fam-boendergaard.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for the Waveshare CH32V003 I2C IO expander.
 */

#define DT_DRV_COMPAT waveshare_ch32v003_gpio

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/mfd/waveshare_ch32v003.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(gpio_ch32v003, CONFIG_GPIO_LOG_LEVEL);

#define DIRECTION_RESET_VAL 0xffU
#define OUTPUT_RESET_VAL    0x00U

struct ch32v003_gpio_config {
	struct gpio_driver_config common;
	const struct device *parent;
};

struct ch32v003_gpio_data {
	struct gpio_driver_data common;
	struct k_mutex *lock;
	uint8_t direction;
	uint8_t output;
};

static int ch32v003_reg_set_masked_locked(const struct device *parent,
					  enum ch32v003_reg reg, uint8_t *cache,
					  uint8_t mask, uint8_t val)
{
	uint8_t reg_data = (*cache & ~mask) | (mask & val);

	if (ch32v003_write_reg8(parent, reg, reg_data) < 0) {
		return -EIO;
	}
	*cache = reg_data;
	return 0;
}

static int ch32v003_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct ch32v003_gpio_config *cfg = dev->config;
	struct ch32v003_gpio_data *data = dev->data;
	int ret = 0;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if ((flags & GPIO_SINGLE_ENDED) || (flags & GPIO_PULL_DOWN) || (flags & GPIO_PULL_UP)) {
		LOG_ERR("open-drain, open-source and pull resistors are not supported (pin %d)", pin);
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

	k_mutex_lock(data->lock, K_FOREVER);

	if ((flags & GPIO_OUTPUT_INIT_LOW) || (flags & GPIO_OUTPUT_INIT_HIGH)) {
		ret = ch32v003_reg_set_masked_locked(cfg->parent, CH32V003_REG_OUTPUT,
						     &data->output, BIT(pin),
						     (flags & GPIO_OUTPUT_INIT_HIGH) ? BIT(pin) : 0);
		if (ret < 0) {
			goto out;
		}
	}

	ret = ch32v003_reg_set_masked_locked(cfg->parent, CH32V003_REG_DIRECTION,
					     &data->direction, BIT(pin),
					     (flags & GPIO_OUTPUT) ? BIT(pin) : 0);
out:
	k_mutex_unlock(data->lock);
	return ret;
}

static int ch32v003_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct ch32v003_gpio_config *cfg = dev->config;
	struct ch32v003_gpio_data *data = dev->data;
	int ret = 0;
	uint8_t inp_reg;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(data->lock, K_FOREVER);
	ret = ch32v003_read_reg8(cfg->parent, CH32V003_REG_INPUT, &inp_reg);
	if (ret == 0) {
		*value = inp_reg;
	}
	k_mutex_unlock(data->lock);
	return ret;
}

static int ch32v003_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	const struct ch32v003_gpio_config *cfg = dev->config;
	struct ch32v003_gpio_data *data = dev->data;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(data->lock, K_FOREVER);
	ret = ch32v003_reg_set_masked_locked(cfg->parent, CH32V003_REG_OUTPUT, &data->output,
					     mask, value);
	k_mutex_unlock(data->lock);
	return ret;
}

static int ch32v003_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return ch32v003_port_set_masked_raw(dev, pins, pins);
}

static int ch32v003_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct ch32v003_gpio_config *cfg = dev->config;
	struct ch32v003_gpio_data *data = dev->data;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(data->lock, K_FOREVER);
	ret = ch32v003_reg_set_masked_locked(cfg->parent, CH32V003_REG_OUTPUT, &data->output,
					     pins, 0);
	k_mutex_unlock(data->lock);
	return ret;
}

static int ch32v003_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct ch32v003_gpio_config *cfg = dev->config;
	struct ch32v003_gpio_data *data = dev->data;
	uint8_t reg_data;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(data->lock, K_FOREVER);
	reg_data = data->output ^ pins;
	ret = ch32v003_write_reg8(cfg->parent, CH32V003_REG_OUTPUT, reg_data);
	if (ret == 0) {
		data->output = reg_data;
	}
	k_mutex_unlock(data->lock);
	return ret;
}

static int ch32v003_gpio_init(const struct device *dev)
{
	const struct ch32v003_gpio_config *cfg = dev->config;
	struct ch32v003_gpio_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->parent)) {
		LOG_ERR("Parent CH32V003 MFD device is not ready");
		return -ENODEV;
	}

	data->lock = ch32v003_get_lock(cfg->parent);

	ret = ch32v003_write_reg8(cfg->parent, CH32V003_REG_DIRECTION, DIRECTION_RESET_VAL);
	if (ret < 0) {
		return ret;
	}
	data->direction = DIRECTION_RESET_VAL;

	ret = ch32v003_write_reg8(cfg->parent, CH32V003_REG_OUTPUT, OUTPUT_RESET_VAL);
	if (ret < 0) {
		return ret;
	}
	data->output = OUTPUT_RESET_VAL;

	return 0;
}

static DEVICE_API(gpio, ch32v003_gpio_api) = {
	.pin_configure = ch32v003_pin_configure,
	.port_get_raw = ch32v003_port_get_raw,
	.port_set_masked_raw = ch32v003_port_set_masked_raw,
	.port_set_bits_raw = ch32v003_port_set_bits_raw,
	.port_clear_bits_raw = ch32v003_port_clear_bits_raw,
	.port_toggle_bits = ch32v003_port_toggle_bits,
};

#define GPIO_CH32V003_INST_DEFINE(n)                                                               \
	static const struct ch32v003_gpio_config config_##n = {                                    \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),                                       \
		.parent = DEVICE_DT_GET(DT_INST_BUS(n)),                                            \
	};                                                                                         \
	static struct ch32v003_gpio_data data_##n = {};                                             \
	DEVICE_DT_INST_DEFINE(n, ch32v003_gpio_init, NULL, &data_##n, &config_##n, POST_KERNEL,    \
			      CONFIG_GPIO_CH32V003_INIT_PRIORITY, &ch32v003_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_CH32V003_INST_DEFINE)
