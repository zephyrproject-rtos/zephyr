/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Axonne Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_PINCTRL_HOGS),
	     "CONFIG_PINCTRL_HOGS should be enabled by devicetree");

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, pinctrl_configure_pins, const pinctrl_soc_pin_t *, uint8_t, uintptr_t);

ZTEST(pinctrl_hogs, test_pinctrl_hog_count)
{
	const uint32_t call_count = pinctrl_configure_pins_fake.call_count;

	zassert_equal(call_count, 2, "Only the two enabled controllers should be configured");
}

ZTEST(pinctrl_hogs, test_pinctrl_hog_default_state)
{
	const pinctrl_soc_pin_t *pins = pinctrl_configure_pins_fake.arg0_history[0];
	const uint8_t pin_cnt = pinctrl_configure_pins_fake.arg1_history[0];
	const uintptr_t reg = pinctrl_configure_pins_fake.arg2_history[0];

	zassert_equal(pin_cnt, 5, "Default state should include both referenced configurations");
	zassert_not_null(pins, "Default state pins are missing");
	zassert_equal(reg, PINCTRL_REG_NONE);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		zassert_equal(pins[i], i + 2, "Unexpected pin at index %u", i);
	}
}

ZTEST(pinctrl_hogs, test_pinctrl_hog_second_controller)
{
	const pinctrl_soc_pin_t *pins = pinctrl_configure_pins_fake.arg0_history[1];
	const uint8_t pin_cnt = pinctrl_configure_pins_fake.arg1_history[1];
	const uintptr_t reg = pinctrl_configure_pins_fake.arg2_history[1];

	zassert_equal(pin_cnt, 1);
	zassert_not_null(pins);
	zassert_equal(pins[0], 0xa);

	zassert_equal(reg, PINCTRL_REG_NONE);
}

ZTEST_SUITE(pinctrl_hogs, NULL, NULL, NULL, NULL, NULL);
