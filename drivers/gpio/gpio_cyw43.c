/*
 * Copyright (c) 2025 John Lin
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPIO driver for the Infineon CYW43 WiFi chip.
 *
 * Exposes WL_GPIO0-2 on the CYW43439 as standard Zephyr GPIO pins.
 * Pin 0 is the onboard LED on Raspberry Pi Pico W and Pico 2 W.
 * GPIO control is performed via the WHD gpioout iovar.
 */

#define DT_DRV_COMPAT infineon_cyw43_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <whd.h>

LOG_MODULE_REGISTER(gpio_cyw43, CONFIG_GPIO_LOG_LEVEL);

#define CYW43_GPIO_PINS 3U

/* Provided by the AIROC WiFi driver */
extern whd_interface_t airoc_wifi_get_whd_interface(void);

/* WHD internal API for setting iovar values */
extern uint32_t whd_wifi_set_iovar_value(whd_interface_t ifp, const char *iovar, uint32_t value);

struct gpio_cyw43_config {
	struct gpio_driver_config common;
};

struct gpio_cyw43_data {
	struct gpio_driver_data common;
	struct k_mutex lock;
	uint32_t pin_state;
};

static int gpio_cyw43_write_state(const struct device *dev)
{
	struct gpio_cyw43_data *data = dev->data;
	whd_interface_t ifp = airoc_wifi_get_whd_interface();
	uint32_t ret;

	if (ifp == NULL) {
		/* WiFi not initialized yet, state will be applied later */
		return 0;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = whd_wifi_set_iovar_value(ifp, "gpioout", data->pin_state);
	k_mutex_unlock(&data->lock);

	return (ret == 0) ? 0 : -EIO;
}

static int gpio_cyw43_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	struct gpio_cyw43_data *data = dev->data;

	if (pin >= CYW43_GPIO_PINS) {
		return -EINVAL;
	}

	if ((flags & GPIO_INPUT) != 0U) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) == 0U) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
		data->pin_state |= BIT(pin);
	} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
		data->pin_state &= ~BIT(pin);
	}

	return gpio_cyw43_write_state(dev);
}

static int gpio_cyw43_port_get_raw(const struct device *dev, uint32_t *value)
{
	struct gpio_cyw43_data *data = dev->data;

	*value = data->pin_state;
	return 0;
}

static int gpio_cyw43_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	struct gpio_cyw43_data *data = dev->data;

	data->pin_state = (data->pin_state & ~mask) | (value & mask);
	return gpio_cyw43_write_state(dev);
}

static int gpio_cyw43_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_cyw43_port_set_masked_raw(dev, pins, pins);
}

static int gpio_cyw43_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_cyw43_port_set_masked_raw(dev, pins, 0);
}

static int gpio_cyw43_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	struct gpio_cyw43_data *data = dev->data;

	data->pin_state ^= pins;
	return gpio_cyw43_write_state(dev);
}

static DEVICE_API(gpio, gpio_cyw43_api) = {
	.pin_configure = gpio_cyw43_configure,
	.port_get_raw = gpio_cyw43_port_get_raw,
	.port_set_masked_raw = gpio_cyw43_port_set_masked_raw,
	.port_set_bits_raw = gpio_cyw43_port_set_bits_raw,
	.port_clear_bits_raw = gpio_cyw43_port_clear_bits_raw,
	.port_toggle_bits = gpio_cyw43_port_toggle_bits,
};

static int gpio_cyw43_init(const struct device *dev)
{
	struct gpio_cyw43_data *data = dev->data;

	k_mutex_init(&data->lock);
	return 0;
}

#define GPIO_CYW43_DEFINE(inst)                                                                    \
	static struct gpio_cyw43_data gpio_cyw43_data_##inst;                                      \
	static const struct gpio_cyw43_config gpio_cyw43_config_##inst = {                         \
		.common = {                                                                        \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),                    \
		},                                                                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, gpio_cyw43_init, NULL, &gpio_cyw43_data_##inst,                \
			      &gpio_cyw43_config_##inst, POST_KERNEL,                              \
			      CONFIG_GPIO_CYW43_INIT_PRIORITY, &gpio_cyw43_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_CYW43_DEFINE)
