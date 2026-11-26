/*
 * Copyright (c) 2026 Igalia S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#include <hardware/gpio.h>
#include "cyw43.h"

#define DT_DRV_COMPAT infineon_cyw43_gpio

LOG_MODULE_REGISTER(gpio_cyw43_rpi_pico, CONFIG_GPIO_LOG_LEVEL);

struct cyw43_gpio_config {
	struct gpio_driver_config common;
};

struct cyw43_gpio_data {
	struct k_mutex lock;
};


static int gpio_rpi_cyw43_port_get_raw(const struct device *dev,
						uint32_t *value)
{
	struct cyw43_gpio_data *data = dev->data;
	int ret = 0;

	*value = 0;
	k_mutex_lock(&data->lock, K_FOREVER);
	for (int i = 0; i < CYW43_WL_GPIO_COUNT; i++) {
		bool val;

		ret = cyw43_gpio_get(&cyw43_state, i, &val);
		if (ret != 0) {
			LOG_ERR("cyw43_gpio_get() pin %d: %d", i, ret);
			goto unlock;
		}
		if (val) {
			LOG_DBG("pin: %d ON", i);
			*value |= BIT(i);
		}
	}
unlock:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int gpio_rpi_cyw43_port_set_masked_raw(const struct device *dev,
						uint32_t mask, uint32_t value)
{
	struct cyw43_gpio_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);
	for (int i = 0; i < CYW43_WL_GPIO_COUNT; i++) {
		if (mask & BIT(i)) {
			ret = cyw43_gpio_set(&cyw43_state, i, value & BIT(i));
			if (ret) {
				LOG_ERR("cyw43_gpio_set() pin %d: %d", i, ret);
				goto unlock;
			}
		}
	}
	if ((mask >> CYW43_WL_GPIO_COUNT) != 0) {
		LOG_INF("Pins %#x out of cyw43 range (%d)", mask,
			CYW43_WL_GPIO_COUNT);
	}
unlock:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int gpio_rpi_cyw43_port_set_bits_raw(const struct device *dev,
							uint32_t pins)
{
	struct cyw43_gpio_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);
	for (int i = 0; i < CYW43_WL_GPIO_COUNT; i++) {
		if ((pins & BIT(i)) == 0) {
			continue;
		}
		ret = cyw43_gpio_set(&cyw43_state, i, 1);
		if (ret) {
			LOG_ERR("cyw43_gpio_set() pin %d: %d", i, ret);
			goto unlock;
		}
	}
	if ((pins >> CYW43_WL_GPIO_COUNT) != 0) {
		LOG_INF("Pins %#x out of cyw43 range (%d)", pins,
			CYW43_WL_GPIO_COUNT);
	}
unlock:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int gpio_rpi_cyw43_port_clear_bits_raw(const struct device *dev,
							uint32_t pins)
{
	struct cyw43_gpio_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);
	for (int i = 0; i < CYW43_WL_GPIO_COUNT; i++) {
		if ((pins & BIT(i)) == 0) {
			continue;
		}
		ret = cyw43_gpio_set(&cyw43_state, i, 0);
		if (ret) {
			LOG_ERR("cyw43_gpio_set() pin %d: %d", i, ret);
			goto unlock;
		}
	}
	if ((pins >> CYW43_WL_GPIO_COUNT) != 0) {
		LOG_INF("Pins %#x out of cyw43 range (%d)", pins,
			CYW43_WL_GPIO_COUNT);
	}
unlock:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int gpio_rpi_cyw43_port_toggle_bits(const struct device *dev,
							uint32_t pins)
{
	struct cyw43_gpio_data *data = dev->data;
	uint32_t pin_state;
	int ret = 0;

	ret = gpio_rpi_cyw43_port_get_raw(dev, &pin_state);
	if (ret) {
		LOG_ERR("gpio_rpi_cyw43_port_get_raw(): %d", ret);
		return ret;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	for (int i = 0; i < CYW43_WL_GPIO_COUNT; i++) {
		if ((pins & BIT(i)) == 0) {
			continue;
		}
		ret = cyw43_gpio_set(&cyw43_state, i,
					(pin_state & BIT(i)) ? 0 : 1);
		if (ret) {
			LOG_ERR("cyw43_gpio_set() pin %d: %d", i, ret);
			break;
		}
	}
	k_mutex_unlock(&data->lock);

	return 0;
}

static int gpio_rpi_cyw43_configure(const struct device *dev, gpio_pin_t pin,
							gpio_flags_t flags)
{
	if (pin >= CYW43_WL_GPIO_COUNT) {
		return -EINVAL;
	}

	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		return gpio_rpi_cyw43_port_set_bits_raw(dev, BIT(pin));
	} else if (flags & GPIO_OUTPUT_INIT_LOW) {
		return gpio_rpi_cyw43_port_clear_bits_raw(dev, BIT(pin));
	}

	/* Other configurations not supported */
	return -ENOTSUP;
}

static DEVICE_API(gpio, cyw43_gpio_api) = {
	.pin_configure = gpio_rpi_cyw43_configure,
	.port_get_raw = gpio_rpi_cyw43_port_get_raw,
	.port_set_masked_raw = gpio_rpi_cyw43_port_set_masked_raw,
	.port_set_bits_raw = gpio_rpi_cyw43_port_set_bits_raw,
	.port_clear_bits_raw = gpio_rpi_cyw43_port_clear_bits_raw,
	.port_toggle_bits = gpio_rpi_cyw43_port_toggle_bits,
};

static int cyw43_gpio_init(const struct device *dev)
{
	struct cyw43_gpio_data *data = dev->data;

	k_mutex_init(&data->lock);
	return 0;
}

#define CYW43_GPIO_DEFINE(inst)								\
	static struct cyw43_gpio_data cyw43_gpio_data_##inst;				\
	static const struct cyw43_gpio_config cyw43_gpio_config_##inst = {		\
		.common = {								\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),		\
		},									\
	};										\
	DEVICE_DT_INST_DEFINE(inst, cyw43_gpio_init, NULL, &cyw43_gpio_data_##inst,	\
				&cyw43_gpio_config_##inst, POST_KERNEL,			\
				CONFIG_GPIO_CYW43_RPI_PICO_INIT_PRIORITY, &cyw43_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(CYW43_GPIO_DEFINE)
