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

struct drv84xx_emul_fixture {
	const struct device *dev;
};

struct gpio_dt_spec en_pin = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(drv84xx), en_gpios, {0});
struct gpio_dt_spec slp_pin = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(drv84xx), sleep_gpios, {0});
struct gpio_dt_spec m0_pin = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(drv84xx), m0_gpios, {0});
struct gpio_dt_spec m1_pin = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(drv84xx), m1_gpios, {0});

static void *drv84xx_emul_setup(void)
{
	static struct drv84xx_emul_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(drv84xx)),
	};

	zassert_not_null(fixture.dev);
	return &fixture;
}

ZTEST_F(drv84xx_emul, test_enable_gpio_pins)
{
	int value = 0;
	int err;

	err = stepper_enable(fixture->dev);
	if (err == -ENOTSUP) {
		ztest_test_skip();
	}
	/* As sleep and enable pins are optional, check if they exist*/
	if (en_pin.port != NULL) {
		value = gpio_emul_output_get(en_pin.port, en_pin.pin);
		zassert_equal(value, 1, "Enable pin should be set");
	}
	if (slp_pin.port != NULL) {
		value = !gpio_emul_output_get(slp_pin.port, slp_pin.pin);
		zassert_equal(value, 0, "Sleep pin should not be set");
	}

	/* As enable is supported, disable must also be supported */
	zassert_ok(stepper_disable(fixture->dev));

	if (en_pin.port != NULL) {
		value = gpio_emul_output_get(en_pin.port, en_pin.pin);
		zassert_equal(value, 0, "Enable pin should not be set");
	}
	if (slp_pin.port != NULL) {
		value = !gpio_emul_output_get(slp_pin.port, slp_pin.pin);
		zassert_equal(value, 1, "Sleep pin should be set");
	}
}

ZTEST_F(drv84xx_emul, test_micro_step_res_set)
{
	enum stepper_micro_step_resolution res;
	int value = 0;

	zassert_ok(stepper_set_micro_step_res(fixture->dev, 4));

	if (m0_pin.port == NULL || m1_pin.port == NULL) {
		ztest_test_skip();
	}

	value = gpio_emul_output_get(m0_pin.port, m0_pin.pin);
	zassert_equal(value, 0, "M0 pin should be 0");

	value = gpio_emul_output_get(m1_pin.port, m1_pin.pin);
	zassert_equal(value, 1, "M1 pin should be 1");

	zassert_ok(stepper_get_micro_step_res(fixture->dev, &res));
	zassert_equal(res, 4, "Micro step resolution not set correctly, should be %d but is %d", 4,
		      res);
}

ZTEST_SUITE(drv84xx_emul, NULL, drv84xx_emul_setup, NULL, NULL, NULL);
