/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/mutex.h>
#include <zephyr/sys/util_macro.h>

#define DT_DRV_COMPAT ti_tca9554

LOG_MODULE_REGISTER(TCA9554, CONFIG_GPIO_LOG_LEVEL);

enum tca9554_reg {
	REG_IN_PORT = 0,
	REG_OUT_PORT = 1,
	REG_POL_INV = 2,
	REG_CONFIG = 3,
};

struct tca9554_config {
	/* gpio_driver_config needs to be first */
	const struct gpio_driver_config common;
	const struct i2c_dt_spec bus;
};

struct tca9554_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	struct k_mutex lock;
};

static int tca9554_reg_set_masked(const struct i2c_dt_spec *bus, enum tca9554_reg reg, uint8_t mask,
				  uint8_t val)
{
	uint8_t reg_data;

	if (i2c_reg_read_byte_dt(bus, reg, &reg_data) < 0) {
		return -EIO;
	}
	reg_data &= ~mask;
	reg_data |= mask & val;

	if (i2c_reg_write_byte_dt(bus, reg, reg_data) < 0) {
		return -EIO;
	}
	return 0;
}

static int tca9554_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct tca9554_config *cfg = dev->config;
	struct tca9554_data *data = dev->data;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if ((flags & GPIO_SINGLE_ENDED) || (flags & GPIO_PULL_DOWN) || (flags & GPIO_PULL_UP)) {
		LOG_ERR("open-drain, open-source and pull resistors are not supported");
		return -ENOTSUP;
	}

	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == GPIO_DISCONNECTED) {
		LOG_ERR("disconnected gpio is not supported");
		return -ENOTSUP;
	}

	if ((flags & GPIO_INPUT) && (flags & GPIO_OUTPUT)) {
		LOG_ERR("simultaneous input and output is not supported");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	int ret = 0;

	if ((flags & GPIO_OUTPUT_INIT_LOW) || (flags & GPIO_OUTPUT_INIT_HIGH)) {
		ret = tca9554_reg_set_masked(&cfg->bus, REG_OUT_PORT, BIT(pin),
					     (flags & GPIO_OUTPUT_INIT_HIGH) ? BIT(pin) : 0);
		if (ret < 0) {
			goto out;
		}
	}

	ret = tca9554_reg_set_masked(&cfg->bus, REG_CONFIG, BIT(pin),
				     (flags & GPIO_INPUT) ? BIT(pin) : 0);

out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int tca9554_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct tca9554_config *cfg = dev->config;
	struct tca9554_data *data = dev->data;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	int ret = 0;
	uint8_t inp_reg;

	ret = i2c_reg_read_byte_dt(&cfg->bus, REG_IN_PORT, &inp_reg);
	if (ret >= 0) {
		*value = inp_reg;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int tca9554_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				       gpio_port_value_t value)
{
	const struct tca9554_config *cfg = dev->config;
	struct tca9554_data *data = dev->data;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	int ret = tca9554_reg_set_masked(&cfg->bus, REG_OUT_PORT, mask, value);

	k_mutex_unlock(&data->lock);
	return ret;
}

static int tca9554_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct tca9554_config *cfg = dev->config;
	struct tca9554_data *data = dev->data;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	int ret = tca9554_reg_set_masked(&cfg->bus, REG_OUT_PORT, pins, pins);

	k_mutex_unlock(&data->lock);
	return ret;
}

static int tca9554_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct tca9554_config *cfg = dev->config;
	struct tca9554_data *data = dev->data;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	int ret = tca9554_reg_set_masked(&cfg->bus, REG_OUT_PORT, pins, 0);

	k_mutex_unlock(&data->lock);
	return ret;
}

static int tca9554_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct tca9554_config *cfg = dev->config;
	struct tca9554_data *data = dev->data;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	int ret = 0;
	uint8_t reg_data;

	ret = i2c_reg_read_byte_dt(&cfg->bus, REG_OUT_PORT, &reg_data);
	if (ret < 0) {
		goto out;
	}
	reg_data ^= pins;

	ret = i2c_reg_write_byte_dt(&cfg->bus, REG_OUT_PORT, reg_data);
	if (ret < 0) {
		goto out;
	}

out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int tca9554_init(const struct device *dev)
{
	const struct tca9554_config *cfg = dev->config;
	struct tca9554_data *data = dev->data;
	int ret = 0;

	ret = k_mutex_init(&data->lock);
	if (ret < 0) {
		return ret;
	}

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->bus.bus);
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(gpio, tca9554_api) = {
	.pin_configure = tca9554_pin_configure,
	.port_get_raw = tca9554_port_get_raw,
	.port_set_masked_raw = tca9554_port_set_masked_raw,
	.port_set_bits_raw = tca9554_port_set_bits_raw,
	.port_clear_bits_raw = tca9554_port_clear_bits_raw,
	.port_toggle_bits = tca9554_port_toggle_bits,
};

#define GPIO_TCA9554_INST_DEFINE(n)                                                                \
	static const struct tca9554_config config_##n = {                                          \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),                                      \
		.bus = I2C_DT_SPEC_GET(DT_DRV_INST(n))};                                           \
	static struct tca9554_data data_##n = {};                                                  \
	DEVICE_DT_INST_DEFINE(n, tca9554_init, NULL, &data_##n, &config_##n, POST_KERNEL,          \
			      CONFIG_GPIO_TCA9554_INIT_PRIO, &tca9554_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_TCA9554_INST_DEFINE);
