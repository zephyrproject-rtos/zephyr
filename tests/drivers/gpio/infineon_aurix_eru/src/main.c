/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec eru_pin =
	GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), eru_gpios, 0);
/* A pin of the same port without an ERU input */
static const struct gpio_dt_spec plain_pin =
	GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), eru_gpios, 1);

static struct gpio_callback eru_cb;
static atomic_t eru_count;

static void eru_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);

	if ((pins & BIT(eru_pin.pin)) != 0U) {
		atomic_inc(&eru_count);
	}
}

/* Drive the pin; its input stage feeds the ERU input channel */
static void drive(int value)
{
	zassert_ok(gpio_pin_set_raw(eru_pin.port, eru_pin.pin, value));
	k_busy_wait(100);
}

static void *eru_setup(void)
{
	zassert_true(gpio_is_ready_dt(&eru_pin));
	gpio_init_callback(&eru_cb, eru_handler, BIT(eru_pin.pin));
	zassert_ok(gpio_add_callback(eru_pin.port, &eru_cb));

	return NULL;
}

static void eru_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(gpio_pin_configure(eru_pin.port, eru_pin.pin, GPIO_OUTPUT_LOW | GPIO_INPUT));
	k_busy_wait(100);
	atomic_clear(&eru_count);
}

static void eru_after(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(gpio_pin_interrupt_configure(eru_pin.port, eru_pin.pin, GPIO_INT_DISABLE));
}

ZTEST(gpio_aurix_eru, test_rising_edge)
{
	zassert_ok(gpio_pin_interrupt_configure(eru_pin.port, eru_pin.pin, GPIO_INT_EDGE_RISING));

	drive(1);
	zassert_equal(atomic_get(&eru_count), 1, "rising edge: %ld", atomic_get(&eru_count));
	drive(0);
	zassert_equal(atomic_get(&eru_count), 1, "falling edge counted");
	drive(1);
	zassert_equal(atomic_get(&eru_count), 2, "second rising edge: %ld",
		      atomic_get(&eru_count));
}

ZTEST(gpio_aurix_eru, test_falling_edge)
{
	zassert_ok(gpio_pin_interrupt_configure(eru_pin.port, eru_pin.pin, GPIO_INT_EDGE_FALLING));

	drive(1);
	zassert_equal(atomic_get(&eru_count), 0, "rising edge counted");
	drive(0);
	zassert_equal(atomic_get(&eru_count), 1, "falling edge: %ld", atomic_get(&eru_count));
}

ZTEST(gpio_aurix_eru, test_both_edges)
{
	zassert_ok(gpio_pin_interrupt_configure(eru_pin.port, eru_pin.pin, GPIO_INT_EDGE_BOTH));

	for (int i = 0; i < 4; i++) {
		drive((i % 2) == 0 ? 1 : 0);
	}
	zassert_equal(atomic_get(&eru_count), 4, "both edges: %ld", atomic_get(&eru_count));
}

ZTEST(gpio_aurix_eru, test_disable)
{
	zassert_ok(gpio_pin_interrupt_configure(eru_pin.port, eru_pin.pin, GPIO_INT_EDGE_BOTH));
	zassert_ok(gpio_pin_interrupt_configure(eru_pin.port, eru_pin.pin, GPIO_INT_DISABLE));

	drive(1);
	drive(0);
	zassert_equal(atomic_get(&eru_count), 0, "disabled pin interrupted: %ld",
		      atomic_get(&eru_count));
}

ZTEST(gpio_aurix_eru, test_unsupported)
{
	zassert_equal(gpio_pin_interrupt_configure(eru_pin.port, eru_pin.pin,
						   GPIO_INT_LEVEL_HIGH),
		      -ENOTSUP, "level interrupt accepted");
	zassert_equal(gpio_pin_interrupt_configure(plain_pin.port, plain_pin.pin,
						   GPIO_INT_EDGE_RISING),
		      -ENOTSUP, "pin without ERU input accepted");
}

ZTEST_SUITE(gpio_aurix_eru, NULL, eru_setup, eru_before, eru_after, NULL);
