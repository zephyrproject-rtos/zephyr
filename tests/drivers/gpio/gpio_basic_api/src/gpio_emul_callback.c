/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/sys/util.h>
#include <zephyr/zephyr.h>

#include "test_gpio.h"

/*
 * When GPIO are emulated, this callback can be used to implement the
 * "wiring". E.g. in this test application, PIN_OUT is connected to
 * PIN_IN. When PIN_OUT is set high or low, PIN_IN must be set
 * correspondingly, as if a wire were connecting the two.
 */
static void gpio_emul_callback_handler(const struct device *port,
				      struct gpio_callback *cb,
				      gpio_port_pins_t pins);

struct gpio_callback gpio_emul_callback = {
	.handler = gpio_emul_callback_handler,
	.pin_mask = BIT(PIN_IN) | BIT(PIN_OUT),
};

static void gpio_emul_callback_handler(const struct device *port,
				      struct gpio_callback *cb,
				      gpio_port_pins_t pins)
{
	int r;
	int val;
	uint32_t output_flags;
	uint32_t input_flags;

	__ASSERT(pins & gpio_emul_callback.pin_mask, "invalid mask: %x", pins);

	r = gpio_emul_flags_get(port, PIN_OUT, &output_flags);
	__ASSERT(r == 0, "gpio_emul_flags_get() failed: %d", r);
	r = gpio_emul_flags_get(port, PIN_IN, &input_flags);
	__ASSERT(r == 0, "gpio_emul_flags_get() failed: %d", r);

	if ((output_flags & GPIO_OUTPUT) && (input_flags & GPIO_INPUT)) {
		r = gpio_emul_output_get(port, PIN_OUT);
		__ASSERT(r == 0 || r == 1, "gpio_emul_output_get() failed: %d", r);
		val = r;
		r = gpio_emul_input_set(port, PIN_IN, val);
		__ASSERT(r == 0, "gpio_emul_input_set() failed: %d", r);

		return;
	}

	if ((output_flags == GPIO_DISCONNECTED) && (input_flags & GPIO_INPUT)) {
		if (input_flags & GPIO_PULL_UP) {
			val = 1;
		} else {
			/* either GPIO_PULL_DOWN or no input */
			val = 0;
		}

		r = gpio_emul_input_set(port, PIN_IN, val);
		__ASSERT(r == 0, "gpio_emul_input_set() failed: %d", r);

		return;
	}
}
