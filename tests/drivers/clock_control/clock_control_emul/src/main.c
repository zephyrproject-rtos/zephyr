/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 TOKITA Hiroshi
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * A system has a single clock controller, so each build instantiates exactly
 * one emulated controller. The variant under test is selected by the overlay
 * picked in testcase.yaml; this file adapts to it through the devicetree and
 * the per-compatible hooks of clock_control_emul.h, so it holds no
 * variant-specific conditionals.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_emul.h>
#include <zephyr/ztest.h>
#include <string.h>

#define TEST_CLK DT_NODELABEL(test_clk)

/* Reaches the hooks the controller's compatible provides. */
#define TEST_CLK_HOOK(suffix) UTIL_CAT(DT_STRING_TOKEN_BY_IDX(TEST_CLK, compatible, 0), suffix)

#define NUM_CLOCKS DT_PROP_LEN(TEST_CLK, clock_initial_values)
#define NUM_CELLS  (DT_PROP_LEN(TEST_CLK, clock_ids) / NUM_CLOCKS)

/* Cell value that is deliberately absent from clock-ids. */
#define UNKNOWN_CELL 0xbeefU

static const struct device *const clk_dev = DEVICE_DT_GET(TEST_CLK);
static const uint32_t clock_id_cells[] = DT_PROP(TEST_CLK, clock_ids);
static const uint32_t initial_values[] = DT_PROP(TEST_CLK, clock_initial_values);

BUILD_ASSERT(ARRAY_SIZE(clock_id_cells) % NUM_CLOCKS == 0,
	     "clock-ids does not hold the same number of cells for every clock");

static union clock_control_emul_subsys subsys_storage[NUM_CLOCKS];
static union clock_control_emul_subsys unknown_storage;

/*
 * The leading cells are copied from a clock that does exist and only the last
 * one is replaced, so a controller that matched a selector on a prefix of its
 * cells would be caught here.
 */
static uint32_t unknown_cells[NUM_CELLS];

static clock_control_subsys_t clock_subsys(size_t idx)
{
	return TEST_CLK_HOOK(_cells_to_subsys)(&clock_id_cells[idx * NUM_CELLS],
					       &subsys_storage[idx]);
}

static clock_control_subsys_t unknown_subsys(void)
{
	return TEST_CLK_HOOK(_cells_to_subsys)(unknown_cells, &unknown_storage);
}

static clock_control_subsys_rate_t rate_arg(uint32_t rate)
{
	return (clock_control_subsys_rate_t)(uintptr_t)rate;
}

ZTEST(clock_control_emul, test_device_is_ready)
{
	zassert_true(device_is_ready(clk_dev), "%s: device not ready", clk_dev->name);
}

/* clock-initial-values must be readable back through get_rate without any setup. */
ZTEST(clock_control_emul, test_initial_rates)
{
	for (size_t i = 0; i < NUM_CLOCKS; i++) {
		uint32_t rate = 0;

		zassert_ok(clock_control_get_rate(clk_dev, clock_subsys(i), &rate),
			   "clock %zu: get_rate() failed", i);
		zassert_equal(rate, initial_values[i], "clock %zu: got %u, want %u", i, rate,
			      initial_values[i]);
	}
}

/* Clocks start out stopped, and on()/off() flip the reported status per clock. */
ZTEST(clock_control_emul, test_on_off_status)
{
	for (size_t i = 0; i < NUM_CLOCKS; i++) {
		clock_control_subsys_t sys = clock_subsys(i);

		zassert_equal(clock_control_get_status(clk_dev, sys), CLOCK_CONTROL_STATUS_OFF,
			      "clock %zu: not initially off", i);

		zassert_ok(clock_control_on(clk_dev, sys), "clock %zu: on() failed", i);
		zassert_equal(clock_control_get_status(clk_dev, sys), CLOCK_CONTROL_STATUS_ON,
			      "clock %zu: not on after on()", i);

		zassert_ok(clock_control_off(clk_dev, sys), "clock %zu: off() failed", i);
		zassert_equal(clock_control_get_status(clk_dev, sys), CLOCK_CONTROL_STATUS_OFF,
			      "clock %zu: not off after off()", i);
	}
}

/* Starting one clock must not disturb the state of its siblings. */
ZTEST(clock_control_emul, test_clocks_are_independent)
{
	if (NUM_CLOCKS < 2) {
		ztest_test_skip();
	}

	zassert_ok(clock_control_on(clk_dev, clock_subsys(0)));

	for (size_t i = 1; i < NUM_CLOCKS; i++) {
		zassert_equal(clock_control_get_status(clk_dev, clock_subsys(i)),
			      CLOCK_CONTROL_STATUS_OFF, "clock %zu: disturbed by sibling", i);
	}

	zassert_ok(clock_control_off(clk_dev, clock_subsys(0)));
}

ZTEST(clock_control_emul, test_set_rate)
{
	for (size_t i = 0; i < NUM_CLOCKS; i++) {
		clock_control_subsys_t sys = clock_subsys(i);
		const uint32_t new_rate = initial_values[i] + 1234U;
		uint32_t rate = 0;

		zassert_ok(clock_control_set_rate(clk_dev, sys, rate_arg(new_rate)),
			   "clock %zu: set_rate() failed", i);
		zassert_ok(clock_control_get_rate(clk_dev, sys, &rate));
		zassert_equal(rate, new_rate, "clock %zu: got %u, want %u", i, rate, new_rate);

		/* Restore so the ordering of tests does not matter. */
		zassert_ok(clock_control_set_rate(clk_dev, sys, rate_arg(initial_values[i])));
	}
}

/* An identifier that is not in clock-ids must be rejected by every operation. */
ZTEST(clock_control_emul, test_unknown_clock_id)
{
	clock_control_subsys_t sys = unknown_subsys();
	uint32_t rate = 0;

	for (size_t i = 0; i < NUM_CLOCKS; i++) {
		zassert_false(TEST_CLK_HOOK(_subsys_match)(sys, &clock_id_cells[i * NUM_CELLS],
							   NUM_CELLS),
			      "clock %zu: the identifier under test is not unknown", i);
	}

	zassert_equal(clock_control_on(clk_dev, sys), -EINVAL);
	zassert_equal(clock_control_off(clk_dev, sys), -EINVAL);
	zassert_equal(clock_control_get_rate(clk_dev, sys, &rate), -EINVAL);
	zassert_equal(clock_control_set_rate(clk_dev, sys, rate_arg(1U)), -EINVAL);
	zassert_equal(clock_control_get_status(clk_dev, sys), CLOCK_CONTROL_STATUS_UNKNOWN);
}

ZTEST(clock_control_emul, test_get_rate_null_output)
{
	zassert_equal(clock_control_get_rate(clk_dev, clock_subsys(0), NULL), -EINVAL);
}

ZTEST(clock_control_emul, test_null_subsys)
{
	uint32_t rate = 0;

	if (NUM_CELLS == 1) {
		/* A single-cell selector is not a pointer: NULL is just clock id 0. */
		ztest_test_skip();
	}

	zassert_equal(clock_control_get_rate(clk_dev, NULL, &rate), -EINVAL);
	zassert_equal(clock_control_get_status(clk_dev, NULL), CLOCK_CONTROL_STATUS_UNKNOWN);
}

static void *clock_control_emul_setup(void)
{
	memcpy(unknown_cells, clock_id_cells, sizeof(unknown_cells));
	unknown_cells[NUM_CELLS - 1] = UNKNOWN_CELL;

	return NULL;
}

ZTEST_SUITE(clock_control_emul, NULL, clock_control_emul_setup, NULL, NULL, NULL);
