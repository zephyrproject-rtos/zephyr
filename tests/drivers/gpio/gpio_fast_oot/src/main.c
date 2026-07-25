/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Tests that the OOT fast GPIO backend include mechanism works.
 *
 * Verifies that:
 *  1. The OOT stub shadow pulled in the mock backend header.
 *  2. The dispatch macros resolve to the OOT backend's types and
 *     symbols via token-paste on the zephyr,gpio-emul compatible.
 *  3. Dispatch resolves to OOT backend.
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_fast.h>

#define TEST_NODE DT_NODELABEL(test_fast_gpio_oot)
#define TEST_BACKEND GPIO_FAST_COMPAT(TEST_NODE, gpios)

ZTEST(gpio_fast_oot, test_oot_backend_included)
{
#ifdef GPIO_FAST_OOT_BACKEND_ACTIVE
	zassert_true(true, "OOT backend header was included");
#else
	zassert_true(false,
		     "GPIO_FAST_OOT_BACKEND_ACTIVE not defined;"
		     " OOT stub shadow did not work");
#endif
}

ZTEST(gpio_fast_oot, test_dispatch_resolves_to_oot)
{
	zassert_equal(
		GPIO_FAST_DISPATCH_VAL(TEST_BACKEND, GPIO_FAST_SET_CYCLES),
		GPIO_FAST_SET_CYCLES_zephyr_gpio_emul,
		"Dispatch should resolve to OOT backend");
}

ZTEST(gpio_fast_oot, test_dispatch_type_resolves)
{
	struct GPIO_FAST_DISPATCH_TYPE(TEST_BACKEND) spec;

	zassert_true(sizeof(spec) > 0,
		     "Dispatch type should resolve to a real struct");
}

ZTEST(gpio_fast_oot, test_dispatch_call_compiles)
{
	struct GPIO_FAST_DISPATCH_TYPE(TEST_BACKEND) spec = { 0 };

	GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_set, &spec);
	GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_clear, &spec);
	GPIO_FAST_DISPATCH_CALL(TEST_BACKEND, gpio_fast_toggle, &spec);

	zassert_true(true, "Dispatch calls compiled and linked");
}

ZTEST_SUITE(gpio_fast_oot, NULL, NULL, NULL, NULL, NULL);
