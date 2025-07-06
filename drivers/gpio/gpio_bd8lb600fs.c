/*
 * Copyright (c) 2022 SILA Embedded Solutions GmbH
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rohm_bd8lb600fs_gpio

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/mfd/bd8lb600fs.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_bd8lb600fs, CONFIG_GPIO_LOG_LEVEL);

struct bd8lb600fs_gpio_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	const struct device *parent_dev;
	int gpios_count;
};

struct bd8lb600fs_gpio_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data data;
	/* each bit is one output channel, bit 0 = channel 1, ... */
	uint32_t state;
	/* each bit defines if the output channel is configured, see state */
	uint32_t configured;
	struct k_mutex lock;
};

static int bd8lb600fs_gpio_pin_configure(const struct device *dev, gpio_pin_t pin,
					 gpio_flags_t flags)
{
	const struct bd8lb600fs_gpio_config *config = dev->config;
	struct bd8lb600fs_gpio_data *data = dev->data;

	/* cannot execute a bus operation in an ISR context */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (pin >= config->gpios_count) {
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

	k_mutex_lock(&data->lock, K_FOREVER);

	if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
		WRITE_BIT(data->state, pin, 0);
	} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
		WRITE_BIT(data->state, pin, 1);
	}

	WRITE_BIT(data->configured, pin, 1);

	int result = mfd_bd8lb600fs_set_outputs(config->parent_dev, data->state);

	k_mutex_unlock(&data->lock);

	return result;
}

static int bd8lb600fs_gpio_port_get_raw(const struct device *dev, uint32_t *value)
{
	LOG_ERR("input pins are not available");
	return -ENOTSUP;
}

static int bd8lb600fs_gpio_port_set_masked_raw(const struct device *dev, uint32_t mask,
					       uint32_t value)
{
	const struct bd8lb600fs_gpio_config *config = dev->config;
	struct bd8lb600fs_gpio_data *data = dev->data;

	/* cannot execute a bus operation in an ISR context */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->state = (data->state & ~mask) | (mask & value);

	int result = mfd_bd8lb600fs_set_outputs(config->parent_dev, data->state);

	k_mutex_unlock(&data->lock);

	return result;
}

static int bd8lb600fs_gpio_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	return bd8lb600fs_gpio_port_set_masked_raw(dev, mask, mask);
}

static int bd8lb600fs_gpio_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	return bd8lb600fs_gpio_port_set_masked_raw(dev, mask, 0);
}

static int bd8lb600fs_gpio_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct bd8lb600fs_gpio_config *config = dev->config;
	struct bd8lb600fs_gpio_data *data = dev->data;

	/* cannot execute a bus operation in an ISR context */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->state ^= mask;

	int result = mfd_bd8lb600fs_set_outputs(config->parent_dev, data->state);

	k_mutex_unlock(&data->lock);

	return result;
}

static DEVICE_API(gpio, api_table) = {
	.pin_configure = bd8lb600fs_gpio_pin_configure,
	.port_get_raw = bd8lb600fs_gpio_port_get_raw,
	.port_set_masked_raw = bd8lb600fs_gpio_port_set_masked_raw,
	.port_set_bits_raw = bd8lb600fs_gpio_port_set_bits_raw,
	.port_clear_bits_raw = bd8lb600fs_gpio_port_clear_bits_raw,
	.port_toggle_bits = bd8lb600fs_gpio_port_toggle_bits,
};

static int bd8lb600fs_gpio_init(const struct device *dev)
{
	const struct bd8lb600fs_gpio_config *config = dev->config;
	struct bd8lb600fs_gpio_data *data = dev->data;

	if (!device_is_ready(config->parent_dev)) {
		LOG_ERR("MFD parent is not ready");
		return -ENODEV;
	}

	int result = k_mutex_init(&data->lock);

	if (result != 0) {
		LOG_ERR("unable to initialize mutex");
		return result;
	}

	return 0;
}

#define BD8LB600FS_GPIO_INIT(inst)                                                                 \
	static const struct bd8lb600fs_gpio_config bd8lb600fs_##inst##_config = {                  \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),            \
			},                                                                         \
		.parent_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                 \
		.gpios_count = DT_INST_PROP(inst, ngpios),                                         \
	};                                                                                         \
                                                                                                   \
	static struct bd8lb600fs_gpio_data bd8lb600fs_##inst##_data = {                            \
		.state = 0x00,                                                                     \
		.configured = 0x00,                                                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, bd8lb600fs_gpio_init, NULL, &bd8lb600fs_##inst##_data,         \
			      &bd8lb600fs_##inst##_config, POST_KERNEL,                            \
			      CONFIG_GPIO_BD8LB600FS_INIT_PRIORITY, &api_table);

DT_INST_FOREACH_STATUS_OKAY(BD8LB600FS_GPIO_INIT)
