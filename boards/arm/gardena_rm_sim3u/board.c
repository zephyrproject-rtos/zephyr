/*
 * Copyright (c) 2023 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>

static const struct gpio_dt_spec diversity_mode_gpio =
	GPIO_DT_SPEC_GET(DT_NODELABEL(gardena_antenna), diversity_mode_gpios);
static const struct gpio_dt_spec diversity_state_gpio =
	GPIO_DT_SPEC_GET(DT_NODELABEL(gardena_antenna), diversity_state_gpios);
static const struct gpio_dt_spec antenna_control_gpio =
	GPIO_DT_SPEC_GET(DT_NODELABEL(gardena_antenna), antenna_control_gpios);
static const struct gpio_dt_spec antenna_input_gpio =
	GPIO_DT_SPEC_GET(DT_NODELABEL(gardena_antenna), antenna_input_gpios);

struct led_mirror {
	struct gpio_dt_spec source;
	struct gpio_dt_spec destination;
	struct gpio_callback callback;
};

static struct led_mirror led_mirrors[] = {
	{
		.source = GPIO_DT_SPEC_GET(DT_NODELABEL(gardena_antenna), led_red_input_gpios),
		.destination = GPIO_DT_SPEC_GET(DT_NODELABEL(led_red), gpios),
	},
	{
		.source = GPIO_DT_SPEC_GET(DT_NODELABEL(gardena_antenna), led_green_input_gpios),
		.destination = GPIO_DT_SPEC_GET(DT_NODELABEL(led_green), gpios),
	},
	{
		.source = GPIO_DT_SPEC_GET(DT_NODELABEL(gardena_antenna), led_blue_input_gpios),
		.destination = GPIO_DT_SPEC_GET(DT_NODELABEL(led_blue), gpios),
	},
};

static void led_mirror_callback_handler(const struct device *port, struct gpio_callback *cb,
					gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	struct led_mirror *led_mirror = CONTAINER_OF(cb, struct led_mirror, callback);

	gpio_pin_set_dt(&led_mirror->destination, gpio_pin_get_dt(&led_mirror->source));
}

static int board_init(void)
{
	if (!device_is_ready(diversity_mode_gpio.port)) {
		return -ENODEV;
	}
	if (!device_is_ready(diversity_state_gpio.port)) {
		return -ENODEV;
	}
	if (!device_is_ready(antenna_control_gpio.port)) {
		return -ENODEV;
	}
	if (!device_is_ready(antenna_input_gpio.port)) {
		return -ENODEV;
	}

	/* inactive: manual, active: auto */
	gpio_pin_configure_dt(&diversity_mode_gpio, GPIO_OUTPUT_INACTIVE);

	/* In manual mode: diversity pin state */
	gpio_pin_configure_dt(&diversity_state_gpio, GPIO_OUTPUT_INACTIVE);

	/* inactive: external, active: internal */
	gpio_pin_configure_dt(&antenna_control_gpio, GPIO_OUTPUT_ACTIVE);

	/* Main MCU requests specific antenna. Ignored for now
	 * inactive: external, active: internal
	 */
	gpio_pin_configure_dt(&antenna_input_gpio, GPIO_INPUT);

	for (size_t index = 0; index < ARRAY_SIZE(led_mirrors); index += 1) {
		struct led_mirror *led_mirror = &led_mirrors[index];

		if (!device_is_ready(led_mirror->source.port)) {
			continue;
		}
		if (!device_is_ready(led_mirror->destination.port)) {
			continue;
		}

		gpio_pin_configure_dt(&led_mirror->source, GPIO_INPUT);
		gpio_pin_configure_dt(&led_mirror->destination, GPIO_OUTPUT);

		gpio_init_callback(&led_mirror->callback, led_mirror_callback_handler,
				   BIT(led_mirror->source.pin));
		gpio_add_callback(led_mirror->source.port, &led_mirror->callback);

		gpio_pin_interrupt_configure_dt(&led_mirror->source, GPIO_INT_EDGE_BOTH);

		/* Initially apply mirror just in case */
		led_mirror_callback_handler(NULL, &led_mirror->callback, 0);
	}

	return 0;
}

SYS_INIT(board_init, PRE_KERNEL_2, 0);
