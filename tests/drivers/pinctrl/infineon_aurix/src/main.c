/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/sys_io.h>
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

/* Port 14 register block and the pad codes of the test state */
#define PORT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(infineon_aurix_pinctrl)
#if defined(CONFIG_PINCTRL_INFINEON_AURIX_TC3X)
#define P14_BASE (DT_REG_ADDR(PORT_NODE) + 14 * 0x100)
#else
#define P14_BASE (DT_REG_ADDR(PORT_NODE) + 14 * 0x400)
#endif

ZTEST(aurix_pinctrl, test_register_coding)
{
	Z_TEST_SKIP_IFDEF(CONFIG_QEMU_TARGET);

	zassert_ok(pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT));

#if defined(CONFIG_PINCTRL_INFINEON_AURIX_TC3X)
	/* IOCR0: PC0 = push-pull alternate 2, PC1 = input pull-up */
	uint32_t iocr0 = sys_read32(P14_BASE + 0x10);

	zassert_equal((iocr0 >> 3) & 0x1F, 0x12, "IOCR0 0x%08x", iocr0);
	zassert_equal((iocr0 >> 11) & 0x1F, 0x02, "IOCR0 0x%08x", iocr0);
#else
	/* DRVCFG: DIR, OD and MODE; input pad with PD and PL at 0 */
	uint32_t pad0 = sys_read32(P14_BASE + 0x304);
	uint32_t pad1 = sys_read32(P14_BASE + 0x314);

	zassert_equal(pad0 & 0xF3, 0x21, "P14.0 DRVCFG 0x%08x", pad0);
	zassert_equal(pad1 & 0x77F3, 0x20, "P14.1 DRVCFG 0x%08x", pad1);
#endif
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
