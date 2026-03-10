/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Unified MUX driver API test for NXP nxp_inputmux and nxp_xbar drivers.
 *
 * The same test binary is used on two boards:
 *   mimxrt595_evk/mimxrt595s/cm33     → exercises nxp_inputmux
 *   mimxrt1050_evk/mimxrt1052/hyperflash → exercises nxp_xbar
 *
 * Per-board device topology (defined in boards/overlay):
 *
 *   alias test-mux            → the concrete MUX controller device
 *
 *   test_mux_state_node       → mux-states consumer (3-cell specifier)
 *     idx 0: active entry  (nxp_inputmux: value=0x00 ; nxp_xbar: enable=1)
 *     idx 1: second entry  (nxp_inputmux: value=0x01 ; nxp_xbar: enable=0, early-exit)
 *
 *   test_mux_ctrl_node        → mux-controls consumer (2-cell specifier)
 *     idx 0: 2-cell entry  → accepted by driver (len >= 2); runtime value
 *                             supplied via the mux_configure() state argument
 *
 * Both nxp_inputmux and nxp_xbar:
 *   - implement mux_configure() and mux_state_get()  (lock is absent)
 *   - accept any specifier with len >= 2; len < 2 → -EINVAL
 *   - use the `state` parameter as the field value (inputmux) or enable
 *     flag (xbar); for mux-states, mux_configure_default() passes cells[2]
 *     as `state`, so DT-encoded values are honoured automatically
 *
 * state_get semantics per driver:
 *   nxp_inputmux: returns FIELD_GET(mask, *reg) — the value stored in the
 *                 masked bits of the register identified by cells[0] (offset).
 *   nxp_xbar:     returns 0 if no non-zero input is routed to cells[1]
 *                 (output), 1 otherwise.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/mux.h>

/* The concrete MUX controller (inputmux or xbar1) reached via alias. */
#define TEST_MUX_NODE    DT_ALIAS(test_mux)

/* Consumer nodes defined in the board overlay. */
#define TEST_CTRL_NODE   DT_NODELABEL(test_mux_ctrl_node)
#define TEST_STATE_NODE  DT_NODELABEL(test_mux_state_node)

/* Device handle for the MUX controller under test. */
static const struct device *test_mux_dev = DEVICE_DT_GET(TEST_MUX_NODE);

/*
 * Statically define mux_control descriptors from devicetree.
 *
 * mux-states consumer (MUX_STATES_SPEC, 3-cell, default encoded in DT):
 *   idx 0 – active  entry  (inputmux: value stored; xbar: enable=1, writes reg)
 *   idx 1 – second  entry  (inputmux: value stored; xbar: enable=0, early-exit)
 *
 * mux-controls consumer (MUX_CONTROLS_SPEC, 2-cell, no embedded default):
 *   idx 0 – 2-cell specifier; passed to the driver to verify 2-cell acceptance.
 */
MUX_CONTROL_DT_SPEC_DEFINE_ALL(TEST_STATE_NODE);
MUX_CONTROL_DT_SPEC_DEFINE_ALL(TEST_CTRL_NODE);

/* Accessor macros */
#define STATE_CTRL_0  MUX_CONTROL_DT_GET_BY_IDX(TEST_STATE_NODE, 0)
#define STATE_CTRL_1  MUX_CONTROL_DT_GET_BY_IDX(TEST_STATE_NODE, 1)
#define CTRL_CTRL_0   MUX_CONTROL_DT_GET_BY_IDX(TEST_CTRL_NODE, 0)

/**
 * @brief The MUX controller device must be ready after system init.
 *
 * Covers: successful probe of nxp_inputmux / nxp_xbar (clock enable,
 * reset toggle, MMIO map).
 */
ZTEST(nxp_mux, test_device_ready)
{
	zassert_true(device_is_ready(test_mux_dev),
		     "MUX device is not ready");
}

/**
 * @brief mux_configure_default() with an active mux-states entry returns 0.
 *
 * nxp_inputmux: performs a clock-gated read-modify-write on the register.
 * nxp_xbar:     writes the input→output routing register (enable=1 path).
 */
ZTEST(nxp_mux, test_configure_default_idx0)
{
	int ret = mux_configure_default(test_mux_dev, STATE_CTRL_0);

	zassert_equal(ret, 0,
		      "mux_configure_default idx0 failed: %d", ret);
}

/**
 * @brief mux_configure_default() with the second mux-states entry returns 0.
 *
 * nxp_inputmux: writes a different value to a different register slot.
 * nxp_xbar:     exercises the enable=0 early-exit path inside
 *               nxp_xbar_configure() – no register write occurs, but 0
 *               is still returned.
 */
ZTEST(nxp_mux, test_configure_default_idx1)
{
	int ret = mux_configure_default(test_mux_dev, STATE_CTRL_1);

	zassert_equal(ret, 0,
		      "mux_configure_default idx1 failed: %d", ret);
}

/**
 * @brief mux_configure() with a mux-states specifier and explicit state=0 returns 0.
 *
 * Both drivers now use the `state` argument as the field value / enable flag.
 * Passing state=0 exercises:
 *   nxp_inputmux: writes 0 into the masked register field — succeeds.
 *   nxp_xbar:     treats state=0 as enable=0, skips the register write —
 *                 returns 0 from the early-exit path.
 */
ZTEST(nxp_mux, test_configure_via_state_spec)
{
	int ret = mux_configure(test_mux_dev, STATE_CTRL_0, 0U);

	zassert_equal(ret, 0,
		      "mux_configure via state spec failed: %d", ret);
}

/**
 * @brief mux_configure_default() on a mux-controls specifier returns -ESRCH.
 *
 * mux-controls entries carry no embedded default state (MUX_CONTROLS_SPEC).
 * The MUX API itself detects this and returns -ESRCH before reaching the
 * driver.  Applies identically to both nxp_inputmux and nxp_xbar.
 */
ZTEST(nxp_mux, test_configure_default_rejects_controls_spec)
{
	int ret = mux_configure_default(test_mux_dev, CTRL_CTRL_0);

	zassert_equal(ret, -ESRCH,
		      "expected -ESRCH for mux-controls spec, got %d", ret);
}

/**
 * @brief mux_configure() with a 2-cell mux-controls specifier now succeeds.
 *
 * Both nxp_inputmux and nxp_xbar accept len >= 2.  A mux-controls property
 * produces a 2-cell specifier (len == 2); the driver uses cells[0], cells[1]
 * for addressing and the `state` argument as the value/enable:
 *   nxp_inputmux: state=1 → writes 1 into the masked register field.
 *   nxp_xbar:     state=1 → enable=1, writes the input→output routing register.
 */
ZTEST(nxp_mux, test_configure_accepts_two_cell_spec)
{
	int ret = mux_configure(test_mux_dev, CTRL_CTRL_0, 1U);

	zassert_equal(ret, 0,
		      "expected 0 for 2-cell spec, got %d", ret);
}

/**
 * @brief mux_state_get() succeeds after mux_configure_default().
 *
 * Both nxp_inputmux and nxp_xbar now implement state_get.  After
 * configuring STATE_CTRL_0 the driver must be able to read back the
 * current register value:
 *   nxp_inputmux: FIELD_GET(mask, *reg) — value written into masked bits.
 *   nxp_xbar:     input-signal index routed to the configured output.
 *
 * The test verifies that:
 *   1. state_get returns 0 (success).
 *   2. The output parameter was actually written (differs from the sentinel).
 */
ZTEST(nxp_mux, test_state_get_after_configure)
{
	uint32_t state = 0xdeadbeefU;

	/* Ensure a known configuration is active before reading state. */
	int ret = mux_configure_default(test_mux_dev, STATE_CTRL_0);

	zassert_equal(ret, 0, "configure failed: %d", ret);

	ret = mux_state_get(test_mux_dev, STATE_CTRL_0, &state);
	zassert_equal(ret, 0, "state_get failed: %d", ret);
	zassert_not_equal(state, 0xdeadbeefU,
			  "state was not updated by state_get");
}

/**
 * @brief mux_state_get() is idempotent — two consecutive reads match.
 *
 * Reading the state twice without an intervening configure must return
 * the same value on both calls.
 */
ZTEST(nxp_mux, test_state_get_idempotent)
{
	uint32_t state1 = 0xdeadbeefU;
	uint32_t state2 = 0xdeadbeefU;

	int ret = mux_configure_default(test_mux_dev, STATE_CTRL_0);

	zassert_equal(ret, 0, "configure failed: %d", ret);

	ret = mux_state_get(test_mux_dev, STATE_CTRL_0, &state1);
	zassert_equal(ret, 0, "first state_get failed: %d", ret);

	ret = mux_state_get(test_mux_dev, STATE_CTRL_0, &state2);
	zassert_equal(ret, 0, "second state_get failed: %d", ret);

	zassert_equal(state1, state2,
		      "state_get not idempotent: %u vs %u", state1, state2);
}

/**
 * @brief mux_state_get() with a 2-cell mux-controls specifier now succeeds.
 *
 * Both nxp_inputmux and nxp_xbar accept len >= 2.  A mux-controls specifier
 * (len == 2) is valid; the driver reads the register addressed by cells[0],
 * cells[1] and writes the result into *state.
 */
ZTEST(nxp_mux, test_state_get_accepts_two_cell_spec)
{
	uint32_t state = 0xdeadbeefU;
	int ret = mux_state_get(test_mux_dev, CTRL_CTRL_0, &state);

	zassert_equal(ret, 0,
		      "expected 0 for 2-cell spec, got %d", ret);
	zassert_not_equal(state, 0xdeadbeefU,
			  "state was not updated by state_get");
}

ZTEST_SUITE(nxp_mux, NULL, NULL, NULL, NULL, NULL);
