/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/mfd/tla2528.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/mutex.h>
#include <zephyr/sys/util_macro.h>

#define DT_DRV_COMPAT ti_tla2528_gpio

LOG_MODULE_DECLARE(TLA2528, CONFIG_GPIO_LOG_LEVEL);

struct gpio_tla2528_config {
	/* gpio_driver_config needs to be first */
	const struct gpio_driver_config common;
	const struct device *parent;
};

struct gpio_tla2528_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
};

static int tla2528_pin_verify_config(gpio_flags_t flags)
{
	if ((flags & (GPIO_SINGLE_ENDED | GPIO_LINE_OPEN_DRAIN)) == GPIO_OPEN_SOURCE) {
		LOG_ERR("open-source is not supported");
		return -ENOTSUP;
	}

	if (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
		LOG_ERR("pull resistors are not supported");
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

	if (flags & GPIO_INT_WAKEUP) {
		LOG_ERR("interrupt wakeup is not supported");
		return -ENOTSUP;
	}

	return 0;
}

static int tla2528_pin_configure_locked(const struct device *dev, gpio_pin_t pin,
					gpio_flags_t flags)
{
	const struct gpio_tla2528_config *cfg = dev->config;

	int ret = 0;

	ret = tla2528_register_set_bits(cfg->parent, TLA2528_PIN_CFG, BIT(pin));
	if (ret < 0) {
		return ret;
	}

	if (flags & GPIO_INPUT) {
		ret = tla2528_register_clear_bits(cfg->parent, TLA2528_GPIO_CFG, BIT(pin));
		return ret;
	}

	/* configure as output */
	if (flags & GPIO_OUTPUT_INIT_LOW) {
		ret = tla2528_register_clear_bits(cfg->parent, TLA2528_GPO_VALUE, BIT(pin));
		if (ret < 0) {
			return ret;
		}
	} else if (flags & GPIO_OUTPUT_INIT_HIGH) {
		ret = tla2528_register_set_bits(cfg->parent, TLA2528_GPO_VALUE, BIT(pin));
		if (ret < 0) {
			return ret;
		}
	} else {
		/* no init val set */
	}

	ret = tla2528_register_set_bits(cfg->parent, TLA2528_GPIO_CFG, BIT(pin));
	if (ret < 0) {
		return ret;
	}

	if (flags & GPIO_OPEN_DRAIN) {
		ret = tla2528_register_clear_bits(cfg->parent, TLA2528_GPO_DRIVE_CFG, BIT(pin));
		if (ret < 0) {
			return ret;
		}
	} else { /* push-pull */
		ret = tla2528_register_set_bits(cfg->parent, TLA2528_GPO_DRIVE_CFG, BIT(pin));
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int tla2528_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_tla2528_config *cfg = dev->config;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	int ret = 0;

	ret = tla2528_pin_verify_config(flags);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(tla2528_get_lock(cfg->parent), K_FOREVER);

	ret = tla2528_pin_configure_locked(dev, pin, flags);

	k_mutex_unlock(tla2528_get_lock(cfg->parent));
	return ret;
}

static int tla2528_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_tla2528_config *cfg = dev->config;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(tla2528_get_lock(cfg->parent), K_FOREVER);

	*value = 0; /* ensure the upper bits are cleared (only lower 8 bits are written by read) */
	const int ret = tla2528_register_read(cfg->parent, TLA2528_GPI_VALUE, (uint8_t *)value);

	k_mutex_unlock(tla2528_get_lock(cfg->parent));
	return ret;
}

static int tla2528_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				       gpio_port_value_t value)
{
	const struct gpio_tla2528_config *cfg = dev->config;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(tla2528_get_lock(cfg->parent), K_FOREVER);

	int ret = 0;

	ret = tla2528_register_clear_bits(cfg->parent, TLA2528_GPO_VALUE, ~value & mask);
	if (ret < 0) {
		goto out;
	}

	ret = tla2528_register_set_bits(cfg->parent, TLA2528_GPO_VALUE, value & mask);
	if (ret < 0) {
		goto out;
	}

out:
	k_mutex_unlock(tla2528_get_lock(cfg->parent));
	return ret;
}

static int tla2528_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_tla2528_config *cfg = dev->config;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(tla2528_get_lock(cfg->parent), K_FOREVER);

	const int ret = tla2528_register_set_bits(cfg->parent, TLA2528_GPO_VALUE, pins);

	k_mutex_unlock(tla2528_get_lock(cfg->parent));
	return ret;
}

static int tla2528_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_tla2528_config *cfg = dev->config;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(tla2528_get_lock(cfg->parent), K_FOREVER);

	const int ret = tla2528_register_clear_bits(cfg->parent, TLA2528_GPO_VALUE, pins);

	k_mutex_unlock(tla2528_get_lock(cfg->parent));
	return ret;
}

static int tla2528_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_tla2528_config *cfg = dev->config;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(tla2528_get_lock(cfg->parent), K_FOREVER);

	int ret = 0;
	uint8_t outport;

	ret = tla2528_register_read(cfg->parent, TLA2528_GPO_VALUE, &outport);
	if (ret < 0) {
		goto out;
	}

	ret = tla2528_register_write(cfg->parent, TLA2528_GPO_VALUE, outport ^ pins);
	if (ret < 0) {
		goto out;
	}

out:
	k_mutex_unlock(tla2528_get_lock(cfg->parent));
	return ret;
}

static int tla2528_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					   enum gpio_int_mode mode, enum gpio_int_trig trigger)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(mode);
	ARG_UNUSED(trigger);

	return -ENOTSUP;
}

static int tla2528_init(const struct device *dev)
{
	const struct gpio_tla2528_config *cfg = dev->config;

	if (!device_is_ready(cfg->parent)) {
		return -ENODEV;
	}

	return 0;
}

static const DEVICE_API(gpio, gpio_tla2528_api) = {
	.pin_configure = tla2528_pin_configure,
	.port_get_raw = tla2528_port_get_raw,
	.port_set_masked_raw = tla2528_port_set_masked_raw,
	.port_set_bits_raw = tla2528_port_set_bits_raw,
	.port_clear_bits_raw = tla2528_port_clear_bits_raw,
	.port_toggle_bits = tla2528_port_toggle_bits,
	.pin_interrupt_configure = tla2528_pin_interrupt_configure,
};

#define GPIO_TLA2528_INST_DEFINE(n)                                                                \
	static const struct gpio_tla2528_config config_##n = {                                     \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),                                      \
		.parent = DEVICE_DT_GET(DT_INST_BUS(n))};                                          \
	static struct gpio_tla2528_data data_##n = {};                                             \
	DEVICE_DT_INST_DEFINE(n, tla2528_init, NULL, &data_##n, &config_##n, POST_KERNEL,          \
			      CONFIG_GPIO_TLA2528_INIT_PRIO, &gpio_tla2528_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_TLA2528_INST_DEFINE);
