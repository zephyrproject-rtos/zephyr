/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static struct gpio_dt_spec irq_pin =
	GPIO_DT_SPEC_GET(DT_INST(0, test_gpio_enable_disable_interrupt), irq_gpios);
static struct gpio_callback cb_data;
static bool cb_called;

static void callback(const struct device *dev, struct gpio_callback *gpio_cb, uint32_t pins)
{
	cb_called = true;
}

struct gpio_enable_disable_interrupt_fixture {
	const struct gpio_dt_spec *irq_spec;
};

static void *gpio_enable_disable_interrupt_setup(void)
{
	static struct gpio_enable_disable_interrupt_fixture fixture;

	fixture.irq_spec = &irq_pin;

	return &fixture;
}

static void gpio_enable_disable_interrupt_before(void *arg)
{
	struct gpio_enable_disable_interrupt_fixture *fixture =
		(struct gpio_enable_disable_interrupt_fixture *)arg;

	zassert_true(gpio_is_ready_dt(fixture->irq_spec), "GPIO device is not ready");

	zassert_ok(gpio_pin_configure_dt(fixture->irq_spec, GPIO_INPUT));
	zassert_ok(gpio_emul_input_set(fixture->irq_spec->port, fixture->irq_spec->pin, 0),
		   "failed to set value on input pin");
	cb_called = false;

	zassert_ok(gpio_pin_interrupt_configure_dt(fixture->irq_spec, GPIO_INT_DISABLE));
	gpio_init_callback(&cb_data, callback, BIT(fixture->irq_spec->pin));
	zassert_ok(gpio_add_callback(fixture->irq_spec->port, &cb_data), "failed to add callback");
}

static void gpio_enable_disable_interrupt_after(void *arg)
{
	struct gpio_enable_disable_interrupt_fixture *fixture =
		(struct gpio_enable_disable_interrupt_fixture *)arg;

	zassert_ok(gpio_remove_callback(fixture->irq_spec->port, &cb_data),
		   "failed to remove callback");
}

static void enable_interrupt(const struct gpio_dt_spec *irq_spec)
{
	zassert_ok(gpio_pin_interrupt_configure_dt(irq_spec, GPIO_INT_MODE_ENABLE_ONLY),
		   "failed to only enable interrupt");
}

static void disable_interrupt(const struct gpio_dt_spec *irq_spec)
{
	zassert_ok(gpio_pin_interrupt_configure_dt(irq_spec, GPIO_INT_MODE_DISABLE_ONLY),
		   " failed to only disable interrupt");
}

static void trigger_callback(const struct gpio_dt_spec *irq_spec)
{
	zassert_ok(gpio_emul_input_set(irq_spec->port, irq_spec->pin, 1),
		   "failed to set value on input pin");
	k_sleep(K_MSEC(100));
}

ZTEST_F(gpio_enable_disable_interrupt, test_not_configured_as_interrupt)
{
	enable_interrupt(fixture->irq_spec);
	trigger_callback(fixture->irq_spec);
	zassert_false(cb_called,
		      "callback should not be executed before configuring the interrupt");
}

ZTEST_F(gpio_enable_disable_interrupt, test_initial_enable_then_disable)
{
	zassert_ok(gpio_pin_interrupt_configure_dt(fixture->irq_spec, GPIO_INT_EDGE_RISING),
		   "failed to set interrupt with edge rising");
	disable_interrupt(fixture->irq_spec);
	trigger_callback(fixture->irq_spec);
	zassert_false(cb_called, "callback should not be executed after disabling the interrupt");
}

ZTEST_F(gpio_enable_disable_interrupt, test_disable_then_enable)
{
	zassert_ok(gpio_pin_interrupt_configure_dt(fixture->irq_spec, GPIO_INT_EDGE_RISING),
		   "failed to set interrupt with edge rising");
	disable_interrupt(fixture->irq_spec);
	trigger_callback(fixture->irq_spec);
	enable_interrupt(fixture->irq_spec);
	zassert_true(cb_called, "callback should be executed after enabling the interrupt");
}

ZTEST_SUITE(gpio_enable_disable_interrupt, NULL, gpio_enable_disable_interrupt_setup,
	    gpio_enable_disable_interrupt_before, gpio_enable_disable_interrupt_after, NULL);
