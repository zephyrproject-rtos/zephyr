/*
 * Copyright 2024 Navimatix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest_assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

struct drv8424_emul_fixture {
	const struct device *dev;
};

struct gpio_dt_spec en_pin = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(drv8424), en_gpios, {0});
struct gpio_dt_spec slp_pin = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(drv8424), sleep_gpios, {0});

static void *drv8424_emul_setup(void)
{
	static struct drv8424_emul_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(drv8424)),
	};

	zassert_not_null(fixture.dev);
	return &fixture;
}

static void drv8424_emul_before(void *f)
{
	struct drv8424_emul_fixture *fixture = f;
	(void)stepper_set_reference_position(fixture->dev, 0);
	(void)stepper_set_micro_step_res(fixture->dev, 1);
}

static void drv8424_emul_after(void *f)
{
	struct drv8424_emul_fixture *fixture = f;
	(void)stepper_enable(fixture->dev, false);
}

ZTEST_F(drv8424_emul, test_enable_on_gpio_pins)
{
	int value = 0;
	(void)stepper_enable(fixture->dev, true);
	/* As sleep and enable pins are optional, check if they exist*/
	if (en_pin.port != NULL) {
		value = gpio_emul_output_get(en_pin.port, en_pin.pin);
		zassert_equal(value, 1, "Enable pin should be set");
	}
	if (slp_pin.port != NULL) {
		value = !gpio_emul_output_get(slp_pin.port, slp_pin.pin);
		zassert_equal(value, 0, "Sleep pin should not be set");
	}
}

ZTEST_F(drv8424_emul, test_enable_off_gpio_pins)
{
	int value = 0;
	/* Enable first to ensure that disable works correctly and the check is not against values
	 * from initialisation or from previous tests
	 */
	(void)stepper_enable(fixture->dev, true);
	(void)stepper_enable(fixture->dev, false);
	/* As sleep and enable pins are optional, check if they exist*/
	if (en_pin.port != NULL) {
		value = gpio_emul_output_get(en_pin.port, en_pin.pin);
		zassert_equal(value, 0, "Enable pin should not be set");
	}
	if (slp_pin.port != NULL) {
		value = !gpio_emul_output_get(slp_pin.port, slp_pin.pin);
		zassert_equal(value, 1, "Sleep pin should be set");
	}
}

ZTEST_SUITE(drv8424_emul, NULL, drv8424_emul_setup, drv8424_emul_before, drv8424_emul_after, NULL);
