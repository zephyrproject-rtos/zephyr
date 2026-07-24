/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Gerson Fernando Budke
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fast GPIO shared implementation.
 *
 * Provides the generic fallback gpio_fast_configure_generic() and
 * gpio_fast_pre/post_stream_generic().
 *
 * Vendor-specific implementations live in the respective vendor
 * driver files (gpio_nrfx.c, gpio_sam0.c, etc.).
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_fast.h>

int gpio_fast_configure_generic(struct gpio_fast_spec_generic *fast,
				const struct device *port,
				gpio_port_pins_t pin_mask, gpio_flags_t flags)
{
	int ret;

	/* Configure pins using the standard GPIO API */
	for (uint8_t pin = 0; pin < 32; pin++) {
		if (pin_mask & BIT(pin)) {
			ret = gpio_pin_configure(port, pin, flags);
			if (ret < 0) {
				return ret;
			}
		}
	}

	fast->port = port;
	fast->pin_mask = pin_mask;

	return 0;
}

int gpio_fast_pre_stream_generic(const void *spec)
{
	ARG_UNUSED(spec);
	return 0;
}

int gpio_fast_post_stream_generic(const void *spec)
{
	ARG_UNUSED(spec);
	return 0;
}
