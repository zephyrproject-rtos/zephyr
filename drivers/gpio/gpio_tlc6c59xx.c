/*
 * Copyright (c) 2026 iomico
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tlc6c59xx_gpio

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/tlc6c59xx.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_tlc6c59xx, CONFIG_GPIO_LOG_LEVEL);

BUILD_ASSERT(CONFIG_GPIO_TLC6C59XX_INIT_PRIORITY > CONFIG_MFD_INIT_PRIORITY,
	     "TLC6C59XX GPIO initialization must follow the MFD parent");

struct tlc6c59xx_gpio_config {
	/* gpio_driver_config must be first. */
	struct gpio_driver_config common;
	/* Parent MFD device. */
	const struct device *parent;
	uint8_t pos;
	uint8_t gpio_count;
};

struct tlc6c59xx_gpio_data {
	/* gpio_driver_data must be first. */
	struct gpio_driver_data common;
	/* Each bit represents one output channel. */
	uint16_t state;
	struct k_mutex lock;
};

static int tlc6c59xx_gpio_update(const struct device *dev, uint32_t mask, uint32_t value,
			       uint32_t toggle)
{
	const struct tlc6c59xx_gpio_config *config = dev->config;
	struct tlc6c59xx_gpio_data *data = dev->data;
	uint16_t next_state;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (((mask | toggle) & ~config->common.port_pin_mask) != 0U) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	next_state = ((data->state & ~mask) | (value & mask)) ^ toggle;
	ret = tlc6c59xx_write_state(config->parent, config->pos, next_state);
	if (ret == 0) {
		data->state = next_state;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static int tlc6c59xx_gpio_pin_configure(const struct device *dev, gpio_pin_t pin,
				      gpio_flags_t flags)
{
	const struct tlc6c59xx_gpio_config *config = dev->config;
	uint32_t mask = 0U;
	uint32_t value = 0U;

	/* Bus operations are not allowed in interrupt context. */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (pin >= config->gpio_count || (BIT(pin) & config->common.port_pin_mask) == 0U) {
		LOG_ERR("invalid pin number %u", pin);
		return -EINVAL;
	}

	if ((flags & GPIO_INPUT) != 0U) {
		LOG_ERR("cannot configure pin as input");
		return -ENOTSUP;
	}

	if ((flags & GPIO_PULL_UP) != 0U) {
		LOG_ERR("cannot configure pin as pullup");
		return -ENOTSUP;
	}

	if ((flags & GPIO_PULL_DOWN) != 0U) {
		LOG_ERR("cannot configure pin as pulldown");
		return -ENOTSUP;
	}

	/* Polarity and logical initialization are handled by the GPIO core. */
	if ((flags & ~(GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH |
		       GPIO_ACTIVE_LOW)) != 0U) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) == 0U) {
		LOG_ERR("pin must be configured as an output");
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
		mask = BIT(pin);
	} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
		mask = BIT(pin);
		value = mask;
	}

	return tlc6c59xx_gpio_update(dev, mask, value, 0U);
}

static int tlc6c59xx_gpio_port_get_raw(const struct device *dev, uint32_t *value)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(value);

	LOG_ERR("input pins are not available");
	return -ENOTSUP;
}

static int tlc6c59xx_gpio_port_set_masked_raw(const struct device *dev, uint32_t mask,
					   uint32_t value)
{
	return tlc6c59xx_gpio_update(dev, mask, value, 0U);
}

static int tlc6c59xx_gpio_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	return tlc6c59xx_gpio_port_set_masked_raw(dev, mask, mask);
}

static int tlc6c59xx_gpio_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	return tlc6c59xx_gpio_port_set_masked_raw(dev, mask, 0U);
}

static int tlc6c59xx_gpio_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	return tlc6c59xx_gpio_update(dev, 0U, 0U, mask);
}

static int tlc6c59xx_gpio_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						   enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(mode);
	ARG_UNUSED(trig);
	return -ENOTSUP;
}

static DEVICE_API(gpio, api_table) = {
	.pin_configure = tlc6c59xx_gpio_pin_configure,
	.port_get_raw = tlc6c59xx_gpio_port_get_raw,
	.port_set_masked_raw = tlc6c59xx_gpio_port_set_masked_raw,
	.port_set_bits_raw = tlc6c59xx_gpio_port_set_bits_raw,
	.port_clear_bits_raw = tlc6c59xx_gpio_port_clear_bits_raw,
	.port_toggle_bits = tlc6c59xx_gpio_port_toggle_bits,
	.pin_interrupt_configure = tlc6c59xx_gpio_pin_interrupt_configure,
};

static int tlc6c59xx_gpio_init(const struct device *dev)
{
	const struct tlc6c59xx_gpio_config *config = dev->config;
	struct tlc6c59xx_gpio_data *data = dev->data;
	int result;

	LOG_DBG("initialize TLC6C59XX GPIO instance %s", dev->name);

	if (!device_is_ready(config->parent)) {
		LOG_ERR("%s: parent MFD is not ready", dev->name);
		return -EINVAL;
	}

	result = k_mutex_init(&data->lock);
	if (result != 0) {
		LOG_ERR("unable to initialize mutex");
		return result;
	}

	return 0;
}

#define TLC6C59XX_GPIO_INIT(inst)                                                              \
	static const struct tlc6c59xx_gpio_config tlc6c59xx_gpio_##inst##_config = {           \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                               \
		.parent = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                         \
		.pos = DT_INST_REG_ADDR(inst),                                                 \
		.gpio_count = DT_INST_PROP(inst, ngpios),                                      \
	};                                                                                     \
                                                                                               \
	static struct tlc6c59xx_gpio_data tlc6c59xx_gpio_##inst##_drvdata;                     \
                                                                                               \
	/* This has to be initialized after the parent MFD instance. */                        \
	DEVICE_DT_INST_DEFINE(inst, tlc6c59xx_gpio_init, NULL, &tlc6c59xx_gpio_##inst##_drvdata, \
			      &tlc6c59xx_gpio_##inst##_config, POST_KERNEL,                    \
			      CONFIG_GPIO_TLC6C59XX_INIT_PRIORITY, &api_table);

DT_INST_FOREACH_STATUS_OKAY(TLC6C59XX_GPIO_INIT)
