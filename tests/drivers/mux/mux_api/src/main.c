/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * MUX subsystem API test suite.
 *
 * Configures a route with the generic mux API (mux_control_set /
 * mux_state_apply) and verifies it by reading it back with mux_state_get(),
 * exercising a real MUX controller end-to-end (set and get_state paths).
 *
 * The suite is controller-agnostic: it operates on the consumer fixture node
 * "mux_consumer" via the generic devicetree macros, so the addressing-cell
 * width and the concrete controller (XBAR, TRGMUX, ...) are opaque. Per-target
 * wiring lives in the board overlay; per-target values come from Kconfig:
 *
 *   CONFIG_TEST_MUX_SET_VALUE_A / _B  - runtime states to set and read back
 *
 * The fixed-state route (mux-states) is verified against its own DT value, so
 * no per-target constant is hardcoded here.
 *
 * The consumer fixture provides one control entry:
 *   idx 0 - the line driven by set / apply and read back by get.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mux.h>
#include <zephyr/ztest.h>

#define CONSUMER DT_NODELABEL(mux_consumer)

BUILD_ASSERT(DT_NODE_EXISTS(CONSUMER),
	     "board overlay must define a 'mux_consumer' node for this test");

MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX(CONSUMER, 0);
MUX_STATE_DT_SPEC_DEFINE(CONSUMER);

static const struct device *const mux_dev = MUX_CONTROL_DT_DEV_GET(CONSUMER);

ZTEST(mux_api, test_dev_ready)
{
	zassert_true(device_is_ready(mux_dev), "mux controller not ready");
	zassert_equal_ptr(MUX_STATE_DT_DEV_GET(CONSUMER), mux_dev,
			  "control and state should resolve to the same controller");
}

ZTEST(mux_api, test_set_then_get)
{
	const struct mux_control *ctrl = MUX_CONTROL_DT_GET_BY_IDX(CONSUMER, 0);
	uint32_t state;

	zassert_ok(mux_control_set(mux_dev, ctrl, CONFIG_TEST_MUX_SET_VALUE_A),
		   "set A failed");
	zassert_ok(mux_state_get(mux_dev, ctrl, &state), "get A failed");
	zassert_equal(state, CONFIG_TEST_MUX_SET_VALUE_A,
		      "read-back should match the value just set");

	zassert_ok(mux_control_set(mux_dev, ctrl, CONFIG_TEST_MUX_SET_VALUE_B),
		   "set B failed");
	zassert_ok(mux_state_get(mux_dev, ctrl, &state), "get B failed");
	zassert_equal(state, CONFIG_TEST_MUX_SET_VALUE_B,
		      "read-back should track the latest set");
}

ZTEST(mux_api, test_state_apply_then_get)
{
	const struct mux_state *mstate = MUX_STATE_DT_GET(CONSUMER);
	uint32_t state;

	/* mux_state_apply() programs the DT-defined routing; read it back
	 * through the state's own addressing and compare to the DT value.
	 */
	zassert_ok(mux_state_apply(mux_dev, mstate), "apply failed");
	zassert_ok(mux_state_get(mux_dev, mstate->control, &state), "get failed");
	zassert_equal(state, mstate->state,
		      "read-back should equal the applied DT state");
}

ZTEST_SUITE(mux_api, NULL, NULL, NULL, NULL, NULL);
