/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_FAKE_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VALUE_FUNC(int,
			gpio_fake_pin_configure,
			const struct device *,
			gpio_pin_t,
			gpio_flags_t);

#ifdef CONFIG_GPIO_GET_CONFIG
DECLARE_FAKE_VALUE_FUNC(int,
			gpio_fake_pin_get_config,
			const struct device *,
			gpio_pin_t,
			gpio_flags_t *);
#endif

DECLARE_FAKE_VALUE_FUNC(int,
			gpio_fake_port_get_raw,
			const struct device *,
			gpio_port_value_t *);

DECLARE_FAKE_VALUE_FUNC(int,
			gpio_fake_port_set_masked_raw,
			const struct device *,
			gpio_port_pins_t,
			gpio_port_value_t);

DECLARE_FAKE_VALUE_FUNC(int,
			gpio_fake_port_set_bits_raw,
			const struct device *,
			gpio_port_pins_t);

DECLARE_FAKE_VALUE_FUNC(int,
			gpio_fake_port_clear_bits_raw,
			const struct device *,
			gpio_port_pins_t);

DECLARE_FAKE_VALUE_FUNC(int,
			gpio_fake_port_toggle_bits,
			const struct device *,
			gpio_port_pins_t);

DECLARE_FAKE_VALUE_FUNC(int,
			gpio_fake_pin_interrupt_configure,
			const struct device *,
			gpio_pin_t,
			enum gpio_int_mode,
			enum gpio_int_trig);

DECLARE_FAKE_VALUE_FUNC(int,
			gpio_fake_manage_callback,
			const struct device *,
			struct gpio_callback *,
			bool);

DECLARE_FAKE_VALUE_FUNC(uint32_t,
			gpio_fake_get_pending_int,
			const struct device *);

#ifdef CONFIG_GPIO_GET_DIRECTION
DECLARE_FAKE_VALUE_FUNC(int,
			gpio_fake_port_get_direction,
			const struct device *,
			gpio_port_pins_t,
			gpio_port_pins_t *,
			gpio_port_pins_t *);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_FAKE_H_ */
