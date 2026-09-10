/*
 * Copyright 2020 Peter Bigot Consulting, LLC
 * Copyright 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/ztest.h>

static const struct device *const reg = DEVICE_DT_GET(DT_PATH(regulator));
static const struct gpio_dt_spec check_gpio =
	GPIO_DT_SPEC_GET(DT_PATH(resources), check_gpios);
static const uint32_t startup_delay_ms = DT_PROP(DT_PATH(regulator),
						 startup_delay_us) / 1000U;
static const uint32_t off_on_delay_ms = DT_PROP(DT_PATH(regulator),
						off_on_delay_us) / 1000U;

ZTEST(regulator_fixed, test_enable_disable)
{
	int ret;
	int64_t init;

	zassert_true(k_uptime_get() >= startup_delay_ms);

	ret = gpio_pin_get_dt(&check_gpio);
	zassert_equal(ret, 1);

	zassert_true(regulator_is_enabled(reg));

	ret = regulator_disable(reg);
	zassert_equal(ret, 0);

	ret = gpio_pin_get_dt(&check_gpio);
	zassert_equal(ret, 0);

	init = k_uptime_get();
	ret = regulator_enable(reg);
	zassert_equal(ret, 0);
	zassert_true(k_uptime_delta(&init) >= off_on_delay_ms);

	ret = gpio_pin_get_dt(&check_gpio);
	zassert_equal(ret, 1);

	ret = regulator_disable(reg);
	zassert_equal(ret, 0);

	ret = gpio_pin_get_dt(&check_gpio);
	zassert_equal(ret, 0);
}

void *setup(void)
{
	zassert_true(device_is_ready(reg));
	zassert_equal(gpio_pin_configure_dt(&check_gpio, GPIO_INPUT), 0);

	return NULL;
}

ZTEST_SUITE(regulator_fixed, NULL, setup, NULL, NULL, NULL);
