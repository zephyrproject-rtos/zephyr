/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ESPRESSIF_COMMON_ESP_GPIO_PORT_H_
#define ZEPHYR_SOC_ESPRESSIF_COMMON_ESP_GPIO_PORT_H_

#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/toolchain.h>

/* A Zephyr GPIO port is as wide as gpio_port_pins_t, so the pads above
 * GPIO_MAX_PINS_PER_PORT are exposed through the gpio1 device. The
 * helpers are always inlined because the gpio driver calls them from
 * code that must stay in IRAM.
 */

/**
 * @brief Get the GPIO port index owning a pad.
 *
 * @param pad Absolute pad number
 * @return Port index, 0 for gpio0 and 1 for gpio1
 */
static ALWAYS_INLINE uint32_t esp_gpio_pad_port(uint32_t pad)
{
	return pad / GPIO_MAX_PINS_PER_PORT;
}

/**
 * @brief Get the pin index of a pad within its GPIO port.
 *
 * @param pad Absolute pad number
 * @return Pin index inside the owning port
 */
static ALWAYS_INLINE gpio_pin_t esp_gpio_pad_pin(uint32_t pad)
{
	return (gpio_pin_t)(pad % GPIO_MAX_PINS_PER_PORT);
}

/**
 * @brief Get the absolute pad number from a port and pin index.
 *
 * @param port Port index, 0 for gpio0 and 1 for gpio1
 * @param pin  Pin index inside the port
 * @return Absolute pad number
 */
static ALWAYS_INLINE uint32_t esp_gpio_port_pad(uint32_t port, gpio_pin_t pin)
{
	return (port * GPIO_MAX_PINS_PER_PORT) + pin;
}

#endif /* ZEPHYR_SOC_ESPRESSIF_COMMON_ESP_GPIO_PORT_H_ */
