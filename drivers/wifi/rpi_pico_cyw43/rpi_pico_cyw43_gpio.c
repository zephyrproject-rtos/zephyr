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

LOG_MODULE_DECLARE(rpi_pico_cyw43_drv, CONFIG_WIFI_LOG_LEVEL);


struct cyw43_gpio_config {
	struct gpio_driver_config common;
};

static int gpio_rpi_cyw43_configure(const struct device *dev, gpio_pin_t pin,
							gpio_flags_t flags)
{
	return -ENOTSUP;
}

static int gpio_rpi_cyw43_port_get_raw(const struct device *dev,
						uint32_t *value)
{
	*value = 0;
	for (int i = 0; i < CYW43_WL_GPIO_COUNT; i++) {
		int ret;
		bool val;

		ret = cyw43_gpio_get(&cyw43_state, i, &val);
		if (ret != 0) {
			LOG_ERR("cyw43_gpio_get() pin %d: %d", i, ret);
			return ret;
		}
		if (val) {
			LOG_DBG("pin: %d ON", i);
			*value |= (1 << i);
		}
	}

	return 0;
}

static int gpio_rpi_cyw43_port_set_masked_raw(const struct device *port,
						uint32_t mask, uint32_t value)
{
	for (int i = 0; i < CYW43_WL_GPIO_COUNT; i++) {
		int ret;

		if (mask & (1 << i)) {
			ret = cyw43_gpio_set(&cyw43_state, i, value);
			if (ret) {
				LOG_ERR("cyw43_gpio_set() pin %d: %d", i, ret);
				return ret;
			}
		}
	}
	if ((mask >> CYW43_WL_GPIO_COUNT) != 0) {
		LOG_INF("Pins %#x out of cyw43 range (%d)", mask,
			CYW43_WL_GPIO_COUNT);
	}

	return 0;
}

static int gpio_rpi_cyw43_port_set_bits_raw(const struct device *port,
							uint32_t pins)
{
	for (int i = 0; i < CYW43_WL_GPIO_COUNT; i++) {
		int ret;

		if ((pins & (1 << i)) == 0) {
			continue;
		}
		ret = cyw43_gpio_set(&cyw43_state, i, 1);
		if (ret) {
			LOG_ERR("cyw43_gpio_set() pin %d: %d", i, ret);
			return ret;
		}
	}
	if ((pins >> CYW43_WL_GPIO_COUNT) != 0) {
		LOG_INF("Pins %#x out of cyw43 range (%d)", pins,
			CYW43_WL_GPIO_COUNT);
	}

	return 0;
}

static int gpio_rpi_cyw43_port_clear_bits_raw(const struct device *port,
							uint32_t pins)
{
	for (int i = 0; i < CYW43_WL_GPIO_COUNT; i++) {
		int ret;

		if ((pins & (1 << i)) == 0) {
			continue;
		}
		ret = cyw43_gpio_set(&cyw43_state, i, 0);
		if (ret) {
			LOG_ERR("cyw43_gpio_set() pin %d: %d", i, ret);
			return ret;
		}
	}
	if ((pins >> CYW43_WL_GPIO_COUNT) != 0) {
		LOG_INF("Pins %#x out of cyw43 range (%d)", pins,
			CYW43_WL_GPIO_COUNT);
	}

	return 0;
}

static int gpio_rpi_cyw43_port_toggle_bits(const struct device *port, uint32_t pins)
{
	uint32_t pin_state;
	int ret;

	ret = gpio_rpi_cyw43_port_get_raw(port, &pin_state);
	if (ret) {
		LOG_ERR("gpio_rpi_cyw43_port_get_raw(): %d", ret);
		return ret;
	}
	for (int i = 0; i < CYW43_WL_GPIO_COUNT; i++) {
		if ((pins & (1 << i)) == 0) {
			continue;
		}
		ret = cyw43_gpio_set(&cyw43_state, i, (pin_state & BIT(i)) ?
					0 : 1);
		if (ret) {
			LOG_ERR("cyw43_gpio_set() pin %d: %d", i, ret);
			return ret;
		}
	}
	return 0;
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
	return 0;
}

#define CYW43_GPIO_DEFINE(inst)								\
	static const struct cyw43_gpio_config cyw43_gpio_config_##inst = {		\
		.common = {								\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),		\
		},									\
	};										\
	DEVICE_DT_INST_DEFINE(inst, cyw43_gpio_init, NULL, NULL,			\
				&cyw43_gpio_config_##inst, POST_KERNEL,			\
				CONFIG_RPI_PICO_CYW43_GPIO_INIT_PRIORITY, &cyw43_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(CYW43_GPIO_DEFINE)
