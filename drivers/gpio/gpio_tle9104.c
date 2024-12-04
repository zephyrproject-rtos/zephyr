/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_tle9104_gpio

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/mfd/tle9104.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_tle9104, CONFIG_GPIO_LOG_LEVEL);

struct tle9104_gpio_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* parent MFD */
	const struct device *parent;
	bool parallel_mode_out12;
	bool parallel_mode_out34;
};

struct tle9104_gpio_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* each bit is one output channel, bit 0 = OUT1, ... */
	uint8_t state;
	/* each bit defines if the output channel is configured, see state */
	uint8_t configured;
	struct k_mutex lock;
};

static int tle9104_gpio_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct tle9104_gpio_config *config = dev->config;
	struct tle9104_gpio_data *data = dev->data;
	int result;

	/* cannot execute a bus operation in an ISR context */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (pin >= TLE9104_GPIO_COUNT) {
		LOG_ERR("invalid pin number %i", pin);
		return -EINVAL;
	}

	if ((flags & GPIO_INPUT) != 0) {
		LOG_ERR("cannot configure pin as input");
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) == 0) {
		LOG_ERR("pin must be configured as an output");
		return -ENOTSUP;
	}

	if ((flags & GPIO_SINGLE_ENDED) == 0) {
		LOG_ERR("pin must be configured as single ended");
		return -ENOTSUP;
	}

	if ((flags & GPIO_LINE_OPEN_DRAIN) == 0) {
		LOG_ERR("pin must be configured as open drain");
		return -ENOTSUP;
	}

	if ((flags & GPIO_PULL_UP) != 0) {
		LOG_ERR("pin cannot have a pull up configured");
		return -ENOTSUP;
	}

	if ((flags & GPIO_PULL_DOWN) != 0) {
		LOG_ERR("pin cannot have a pull down configured");
		return -ENOTSUP;
	}

	if (config->parallel_mode_out12 && pin == 1) {
		LOG_ERR("cannot configure OUT2 if parallel mode is enabled for OUT1 and OUT2");
		return -EINVAL;
	}

	if (config->parallel_mode_out34 && pin == 3) {
		LOG_ERR("cannot configure OUT4 if parallel mode is enabled for OUT3 and OUT4");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
		WRITE_BIT(data->state, pin, 0);
	} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
		WRITE_BIT(data->state, pin, 1);
	}

	WRITE_BIT(data->configured, pin, 1);
	result = tle9104_write_state(config->parent, data->state);
	k_mutex_unlock(&data->lock);

	return result;
}

static int tle9104_gpio_port_get_raw(const struct device *dev, uint32_t *value)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(value);

	LOG_ERR("input pins are not available");
	return -ENOTSUP;
}

static int tle9104_gpio_port_set_masked_raw(const struct device *dev, uint32_t mask, uint32_t value)
{
	const struct tle9104_gpio_config *config = dev->config;
	struct tle9104_gpio_data *data = dev->data;
	int result;

	if (config->parallel_mode_out12 && (BIT(1) & mask) != 0) {
		LOG_ERR("cannot set OUT2 if parallel mode is enabled for OUT1 and OUT2");
		return -EINVAL;
	}

	if (config->parallel_mode_out34 && (BIT(3) & mask) != 0) {
		LOG_ERR("cannot set OUT4 if parallel mode is enabled for OUT3 and OUT4");
		return -EINVAL;
	}

	/* cannot execute a bus operation in an ISR context */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->state = (data->state & ~mask) | (mask & value);
	result = tle9104_write_state(config->parent, data->state);
	k_mutex_unlock(&data->lock);

	return result;
}

static int tle9104_gpio_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	return tle9104_gpio_port_set_masked_raw(dev, mask, mask);
}

static int tle9104_gpio_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	return tle9104_gpio_port_set_masked_raw(dev, mask, 0);
}

static int tle9104_gpio_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct tle9104_gpio_config *config = dev->config;
	struct tle9104_gpio_data *data = dev->data;
	int result;

	if (config->parallel_mode_out12 && (BIT(1) & mask) != 0) {
		LOG_ERR("cannot toggle OUT2 if parallel mode is enabled for OUT1 and OUT2");
		return -EINVAL;
	}

	if (config->parallel_mode_out34 && (BIT(3) & mask) != 0) {
		LOG_ERR("cannot toggle OUT4 if parallel mode is enabled for OUT3 and OUT4");
		return -EINVAL;
	}

	/* cannot execute a bus operation in an ISR context */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->state ^= mask;
	result = tle9104_write_state(config->parent, data->state);
	k_mutex_unlock(&data->lock);

	return result;
}

static int tle9104_gpio_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(mode);
	ARG_UNUSED(trig);
	return -ENOTSUP;
}

static DEVICE_API(gpio, api_table) = {
	.pin_configure = tle9104_gpio_pin_configure,
	.port_get_raw = tle9104_gpio_port_get_raw,
	.port_set_masked_raw = tle9104_gpio_port_set_masked_raw,
	.port_set_bits_raw = tle9104_gpio_port_set_bits_raw,
	.port_clear_bits_raw = tle9104_gpio_port_clear_bits_raw,
	.port_toggle_bits = tle9104_gpio_port_toggle_bits,
	.pin_interrupt_configure = tle9104_gpio_pin_interrupt_configure,
};

static int tle9104_gpio_init(const struct device *dev)
{
	const struct tle9104_gpio_config *config = dev->config;
	struct tle9104_gpio_data *data = dev->data;
	int result;

	LOG_DBG("initialize TLE9104 GPIO instance %s", dev->name);

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

#define TLE9104_GPIO_INIT(inst)                                                                    \
	static const struct tle9104_gpio_config tle9104_gpio_##inst##_config = {                   \
		.common = {                                                                        \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),                    \
		},                                                                                 \
		.parent = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                             \
		.parallel_mode_out12 = DT_PROP(DT_PARENT(DT_DRV_INST(inst)), parallel_out12),      \
		.parallel_mode_out34 = DT_PROP(DT_PARENT(DT_DRV_INST(inst)), parallel_out34),      \
	};                                                                                         \
                                                                                                   \
	static struct tle9104_gpio_data tle9104_gpio_##inst##_drvdata;                             \
                                                                                                   \
	/* This has to be initialized after the SPI peripheral. */                                 \
	DEVICE_DT_INST_DEFINE(inst, tle9104_gpio_init, NULL, &tle9104_gpio_##inst##_drvdata,       \
			      &tle9104_gpio_##inst##_config, POST_KERNEL,                          \
			      CONFIG_GPIO_TLE9104_INIT_PRIORITY, &api_table);

DT_INST_FOREACH_STATUS_OKAY(TLE9104_GPIO_INIT)
