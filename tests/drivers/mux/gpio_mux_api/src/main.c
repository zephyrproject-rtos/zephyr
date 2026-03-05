/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mux.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <stdint.h>

#ifdef CONFIG_GPIO_EMUL
#include <zephyr/drivers/gpio/gpio_emul.h>
#endif

#define GPIO_MUX_NODE DT_NODELABEL(gpio_mux_test)
#define MUX_BITS DT_PROP_LEN(GPIO_MUX_NODE, mux_gpios)

static uint32_t mux_default_state_cells[] = {0x5U};

static struct mux_control mux_control = {
	.cells = NULL,
	.len = 0,
	.type = MUX_CONTROLS_SPEC,
};

static struct mux_control mux_default_state = {
	.cells = mux_default_state_cells,
	.len = ARRAY_SIZE(mux_default_state_cells),
	.type = MUX_STATES_SPEC,
};

static const struct device *const mux_dev = DEVICE_DT_GET(GPIO_MUX_NODE);

#define GPIO_MUX_SPEC_GET(node_id, prop, idx) GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx)
static const struct gpio_dt_spec mux_sel_gpios[] = {
	DT_FOREACH_PROP_ELEM_SEP(GPIO_MUX_NODE, mux_gpios, GPIO_MUX_SPEC_GET, (,))
};

static int mux_gpio_output_get(const struct gpio_dt_spec *spec)
{
#ifdef CONFIG_GPIO_EMUL
	return gpio_emul_output_get_dt(spec);
#else
	return gpio_pin_get_dt(spec);
#endif
}

static void assert_gpio_mux_bits(uint32_t state)
{
	for (size_t i = 0; i < ARRAY_SIZE(mux_sel_gpios); i++) {
		int value = mux_gpio_output_get(&mux_sel_gpios[i]);

		zassert_true((value == 0) || (value == 1),
			     "Invalid GPIO value for %s pin %d: %d",
			     mux_sel_gpios[i].port->name, mux_sel_gpios[i].pin, value);
		zassert_equal(value, (state >> i) & 0x1,
			      "GPIO bit %zu mismatch: exp=%u got=%d",
			      i, (state >> i) & 0x1, value);
	}
}

ZTEST(gpio_mux, test_00_initial_state)
{
	uint32_t state = UINT32_MAX;

	zassert_true(device_is_ready(mux_dev), "Mux device is not ready");
	zassert_ok(mux_state_get(mux_dev, &mux_control, &state), "mux_state_get failed");
	zassert_equal(state, 0U, "Initial state should be zero");
	assert_gpio_mux_bits(0U);
}

ZTEST(gpio_mux, test_01_configure_default_state)
{
	uint32_t state = UINT32_MAX;

	zassert_equal(mux_configure_default(mux_dev, &mux_control), -ESRCH,
		      "mux_configure_default on mux-controls should return -ESRCH");

	zassert_ok(mux_configure_default(mux_dev, &mux_default_state),
		   "mux_configure_default failed");
	zassert_ok(mux_state_get(mux_dev, &mux_control, &state), "mux_state_get failed");

	zassert_equal(state, 0x5U, "Default state mismatch");

	assert_gpio_mux_bits(0x5U);
}

ZTEST(gpio_mux, test_02_configure_explicit_state)
{
	const uint32_t states_to_set[] = {0x0, 0x1, 0x2, 0x5, 0x7, 0xF};

	for (size_t i = 0; i < ARRAY_SIZE(states_to_set); i++) {
		uint32_t expected = states_to_set[i] & BIT_MASK(MUX_BITS);
		uint32_t state = UINT32_MAX;

		zassert_ok(mux_configure(mux_dev, &mux_control, states_to_set[i]),
			   "mux_configure failed for state=0x%x", states_to_set[i]);

		assert_gpio_mux_bits(expected);

		zassert_ok(mux_state_get(mux_dev, &mux_control, &state),
			   "mux_state_get failed for state=0x%x", states_to_set[i]);
		zassert_equal(state, expected,
			      "State mismatch: exp=0x%x got=0x%x", expected, state);
	}
}

ZTEST_SUITE(gpio_mux, NULL, NULL, NULL, NULL, NULL);
