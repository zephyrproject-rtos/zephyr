/*
 * Copyright (c) 2019-2020 Nordic Semiconductor ASA
 * Copyright (c) 2019 Piotr Mienkowski
 * Copyright (c) 2017 ARM Ltd
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for GPIO APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_H_
#error "Should only be included by zephyr/drivers/gpio.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_INTERNAL_GPIO_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_INTERNAL_GPIO_IMPL_H_

#include <errno.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/gpio/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_gpio_pin_interrupt_configure(const struct device *port,
						      gpio_pin_t pin,
						      gpio_flags_t flags)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;
	__unused const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;
	const struct gpio_driver_data *const data =
		(const struct gpio_driver_data *)port->data;
	enum gpio_int_trig trig;
	enum gpio_int_mode mode;

	if (api->pin_interrupt_configure == NULL) {
		return -ENOSYS;
	}

	__ASSERT((flags & (GPIO_INT_DISABLE | GPIO_INT_ENABLE))
		 != (GPIO_INT_DISABLE | GPIO_INT_ENABLE),
		 "Cannot both enable and disable interrupts");

	__ASSERT((flags & (GPIO_INT_DISABLE | GPIO_INT_ENABLE)) != 0U,
		 "Must either enable or disable interrupts");

	__ASSERT(((flags & GPIO_INT_ENABLE) == 0) ||
		 ((flags & GPIO_INT_EDGE) != 0) ||
		 ((flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)) !=
		  (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)),
		 "Only one of GPIO_INT_LOW_0, GPIO_INT_HIGH_1 can be "
		 "enabled for a level interrupt.");

	__ASSERT(((flags & GPIO_INT_ENABLE) == 0) ||
#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
			 ((flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)) != 0) ||
			 (flags & GPIO_INT_ENABLE_DISABLE_ONLY) != 0,
#else
			 ((flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)) != 0),
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */
		 "At least one of GPIO_INT_LOW_0, GPIO_INT_HIGH_1 has to be "
		 "enabled.");

	__ASSERT((cfg->port_pin_mask & (gpio_port_pins_t)BIT(pin)) != 0U,
		 "Unsupported pin");

	if (((flags & GPIO_INT_LEVELS_LOGICAL) != 0) &&
	    ((data->invert & (gpio_port_pins_t)BIT(pin)) != 0)) {
		/* Invert signal bits */
		flags ^= (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1);
	}

	trig = (enum gpio_int_trig)(flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1 | GPIO_INT_WAKEUP));
#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	mode = (enum gpio_int_mode)(flags & (GPIO_INT_EDGE | GPIO_INT_DISABLE | GPIO_INT_ENABLE |
					     GPIO_INT_ENABLE_DISABLE_ONLY));
#else
	mode = (enum gpio_int_mode)(flags & (GPIO_INT_EDGE | GPIO_INT_DISABLE | GPIO_INT_ENABLE));
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */

	return api->pin_interrupt_configure(port, pin, mode, trig);
}

static inline int z_impl_gpio_pin_configure(const struct device *port,
					    gpio_pin_t pin,
					    gpio_flags_t flags)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;
	__unused const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;
	struct gpio_driver_data *data =
		(struct gpio_driver_data *)port->data;

	__ASSERT((flags & GPIO_INT_MASK) == 0,
		 "Interrupt flags are not supported");

	__ASSERT((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) !=
		 (GPIO_PULL_UP | GPIO_PULL_DOWN),
		 "Pull Up and Pull Down should not be enabled simultaneously");

	__ASSERT(!((flags & GPIO_INPUT) && !(flags & GPIO_OUTPUT) && (flags & GPIO_SINGLE_ENDED)),
		 "Input cannot be enabled for 'Open Drain', 'Open Source' modes without Output");

	__ASSERT_NO_MSG((flags & GPIO_SINGLE_ENDED) != 0 ||
			(flags & GPIO_LINE_OPEN_DRAIN) == 0);

	__ASSERT((flags & (GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH)) == 0
		 || (flags & GPIO_OUTPUT) != 0,
		 "Output needs to be enabled to be initialized low or high");

	__ASSERT((flags & (GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH))
		 != (GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH),
		 "Output cannot be initialized low and high");

	if (((flags & GPIO_OUTPUT_INIT_LOGICAL) != 0)
	    && ((flags & (GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH)) != 0)
	    && ((flags & GPIO_ACTIVE_LOW) != 0)) {
		flags ^= GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH;
	}

	flags &= ~GPIO_OUTPUT_INIT_LOGICAL;

	__ASSERT((cfg->port_pin_mask & (gpio_port_pins_t)BIT(pin)) != 0U,
		 "Unsupported pin");

	if ((flags & GPIO_ACTIVE_LOW) != 0) {
		data->invert |= (gpio_port_pins_t)BIT(pin);
	} else {
		data->invert &= ~(gpio_port_pins_t)BIT(pin);
	}

	return api->pin_configure(port, pin, flags);
}

#ifdef CONFIG_GPIO_GET_DIRECTION
static inline int z_impl_gpio_port_get_direction(const struct device *port, gpio_port_pins_t map,
						 gpio_port_pins_t *inputs,
						 gpio_port_pins_t *outputs)
{
	const struct gpio_driver_api *api = (const struct gpio_driver_api *)port->api;

	if (api->port_get_direction == NULL) {
		return -ENOSYS;
	}

	return api->port_get_direction(port, map, inputs, outputs);
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

#ifdef CONFIG_GPIO_GET_CONFIG
static inline int z_impl_gpio_pin_get_config(const struct device *port,
					     gpio_pin_t pin,
					     gpio_flags_t *flags)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;

	if (api->pin_get_config == NULL)
		return -ENOSYS;

	return api->pin_get_config(port, pin, flags);
}
#endif

static inline int z_impl_gpio_port_get_raw(const struct device *port,
					   gpio_port_value_t *value)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;

	return api->port_get_raw(port, value);
}

static inline int z_impl_gpio_port_set_masked_raw(const struct device *port,
						  gpio_port_pins_t mask,
						  gpio_port_value_t value)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;

	return api->port_set_masked_raw(port, mask, value);
}

static inline int z_impl_gpio_port_set_bits_raw(const struct device *port,
						gpio_port_pins_t pins)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;

	return api->port_set_bits_raw(port, pins);
}

static inline int z_impl_gpio_port_clear_bits_raw(const struct device *port,
						  gpio_port_pins_t pins)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;

	return api->port_clear_bits_raw(port, pins);
}

static inline int z_impl_gpio_port_toggle_bits(const struct device *port,
					       gpio_port_pins_t pins)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;

	return api->port_toggle_bits(port, pins);
}

static inline int z_impl_gpio_get_pending_int(const struct device *dev)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)dev->api;

	if (api->get_pending_int == NULL) {
		return -ENOSYS;
	}

	return api->get_pending_int(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_INTERNAL_GPIO_IMPL_H_ */
