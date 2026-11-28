/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/ztest.h>

#define TEST_NODE DT_NODELABEL(test_device)

PINCTRL_DT_DEFINE(TEST_NODE);

static const struct pinctrl_dev_config *pcfg = PINCTRL_DT_DEV_CONFIG_GET(TEST_NODE);

ZTEST(aurix_pinctrl, test_state_from_devicetree)
{
	const struct pinctrl_state *state;

	zassert_ok(pinctrl_lookup_state(pcfg, PINCTRL_STATE_DEFAULT, &state));
	zassert_equal(state->pin_cnt, 2);

	zassert_equal(AURIX_PINMUX_PORT(state->pins[0].pinmux), 14);
	zassert_equal(AURIX_PINMUX_PIN(state->pins[0].pinmux), 0);
	zassert_equal(AURIX_PINMUX_FUNC(state->pins[0].pinmux), 2);
	zassert_true(state->pins[0].output);
	zassert_false(state->pins[0].open_drain);

	zassert_equal(AURIX_PINMUX_PORT(state->pins[1].pinmux), 14);
	zassert_equal(AURIX_PINMUX_PIN(state->pins[1].pinmux), 1);
	zassert_false(state->pins[1].output);
	zassert_true(state->pins[1].pull_up);
	zassert_false(state->pins[1].pull_down);
}

ZTEST(aurix_pinctrl, test_apply_state)
{
	zassert_ok(pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT));
}

ZTEST(aurix_pinctrl, test_port_out_of_range)
{
	const pinctrl_soc_pin_t pin = {.pinmux = AURIX_PINMUX(255, 0, 0)};

	zassert_equal(pinctrl_configure_pins(&pin, 1, PINCTRL_REG_NONE), -EINVAL);
}

ZTEST(aurix_pinctrl, test_tc3x_alternate_range)
{
	const pinctrl_soc_pin_t pin = {.pinmux = AURIX_PINMUX(14, 0, 8), .output = 1};

	Z_TEST_SKIP_IFNDEF(CONFIG_PINCTRL_INFINEON_AURIX_TC3X);

	/* TC3x IOCR codes alternate outputs 1 to 7 only */
	zassert_equal(pinctrl_configure_pins(&pin, 1, PINCTRL_REG_NONE), -EINVAL);
}

ZTEST_SUITE(aurix_pinctrl, NULL, NULL, NULL, NULL, NULL);
