/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>

/* The input register reflects the pad level, also for an output */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

static int pad(const struct gpio_dt_spec *spec)
{
	k_busy_wait(10);

	return gpio_pin_get_raw(spec->port, spec->pin);
}

ZTEST(aurix_gpio, test_output_levels)
{
	/* LED is active low: inactive drives the pad high */
	zassert_ok(gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE));
	zassert_equal(pad(&led), 1);

	zassert_ok(gpio_pin_set_raw(led.port, led.pin, 0));
	zassert_equal(pad(&led), 0);

	zassert_ok(gpio_pin_toggle_dt(&led));
	zassert_equal(pad(&led), 1);

	zassert_ok(gpio_port_set_masked_raw(led.port, BIT(led.pin), 0));
	zassert_equal(pad(&led), 0);

	zassert_ok(gpio_port_set_bits_raw(led.port, BIT(led.pin)));
	zassert_equal(pad(&led), 1);

	zassert_ok(gpio_port_clear_bits_raw(led.port, BIT(led.pin)));
	zassert_equal(pad(&led), 0);

	zassert_ok(gpio_pin_set_dt(&led, 0));
	zassert_equal(pad(&led), 1);
}

ZTEST(aurix_gpio, test_input_pull_up)
{
	/* Released button: the pull-up of the devicetree flags holds the pad high */
	zassert_ok(gpio_pin_configure_dt(&button, GPIO_INPUT));
	k_busy_wait(100);
	zassert_equal(gpio_pin_get_raw(button.port, button.pin), 1);
	zassert_equal(gpio_pin_get_dt(&button), 0);
}

ZTEST(aurix_gpio, test_unsupported_flags)
{
	zassert_equal(gpio_pin_configure(led.port, led.pin, GPIO_DISCONNECTED), -ENOTSUP);
	zassert_equal(gpio_pin_configure(led.port, led.pin, GPIO_OUTPUT | GPIO_OPEN_SOURCE),
		      -ENOTSUP);
}

static void *setup(void)
{
	zassert_true(gpio_is_ready_dt(&led));
	zassert_true(gpio_is_ready_dt(&button));

	return NULL;
}

ZTEST_SUITE(aurix_gpio, NULL, setup, NULL, NULL, NULL);
