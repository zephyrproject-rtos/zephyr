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
static int cb_called;

struct gpio_simulated_level_interrupt_trigger_fixture {
	const struct gpio_dt_spec *irq_spec;
};

static void trigger_callback(const struct gpio_dt_spec *irq_spec)
{
	zassert_ok(gpio_emul_input_set(irq_spec->port, irq_spec->pin, 1),
		   "failed to set value on input pin");
	k_sleep(K_MSEC(100));
}

static void callback(const struct device *dev, struct gpio_callback *gpio_cb, uint32_t pins)
{
	cb_called = true;
	zassert_ok(gpio_emul_input_set(irq_pin.port, irq_pin.pin, 0),
		   "failed to set value on input pin");
}
static void *gpio_simulated_level_interrupt_trigger_setup(void)
{
	static struct gpio_simulated_level_interrupt_trigger_fixture fixture;

	fixture.irq_spec = &irq_pin;

	return &fixture;
}

static void gpio_simulated_level_interrupt_trigger_before(void *arg)
{
	struct gpio_simulated_level_interrupt_trigger_fixture *fixture =
		(struct gpio_simulated_level_interrupt_trigger_fixture *)arg;

	zassert_true(gpio_is_ready_dt(fixture->irq_spec), "GPIO device is not ready");

	zassert_ok(gpio_pin_configure_dt(fixture->irq_spec, GPIO_INPUT));
	zassert_ok(gpio_emul_input_set(fixture->irq_spec->port, fixture->irq_spec->pin, 0),
		   "failed to set value on input pin");
	cb_called = false;

	zassert_ok(gpio_pin_interrupt_configure_dt(fixture->irq_spec, GPIO_INT_LEVEL_HIGH));
	gpio_init_callback(&cb_data, callback, BIT(fixture->irq_spec->pin));
	zassert_ok(gpio_add_callback(fixture->irq_spec->port, &cb_data), "failed to add callback");
}

static void gpio_simulated_level_interrupt_trigger_after(void *arg)
{
	struct gpio_simulated_level_interrupt_trigger_fixture *fixture =
		(struct gpio_simulated_level_interrupt_trigger_fixture *)arg;

	zassert_ok(gpio_remove_callback(fixture->irq_spec->port, &cb_data),
		   "failed to remove callback");
}

ZTEST_F(gpio_simulated_level_interrupt_trigger, test_simulated_level_trigger)
{
	trigger_callback(fixture->irq_spec);
	zassert_true(cb_called, "callback should be executed more than once");
}

ZTEST_SUITE(gpio_simulated_level_interrupt_trigger, NULL,
	    gpio_simulated_level_interrupt_trigger_setup,
	    gpio_simulated_level_interrupt_trigger_before,
	    gpio_simulated_level_interrupt_trigger_after, NULL);
