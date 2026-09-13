/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 8: Configuration — exercises i3c_config_get, i3c_configure,
 * i3c_device_find, i3c_bus_has_sec_controller.
 */

#include "test_common.h"

#include <zephyr/sys/sys_io.h>

/* The controller's own devicetree node, so the tests can check what the
 * driver reports against what it was configured with.
 */
#define I3C_DUAL_BOARD_LOOPBACK_CTRL_NODE DT_ALIAS(test_i3c)

ZTEST(dual_board_loopback, test_config_get_returns_scl_frequencies)
{
	struct i3c_config_controller cfg = {0};
	int rc = i3c_config_get(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);

	zassert_ok(rc, "config_get rc=%d", rc);
	zassert_equal(cfg.scl.i3c, DT_PROP(I3C_DUAL_BOARD_LOOPBACK_CTRL_NODE, i3c_scl_hz),
		      "i3c SCL freq = %u, devicetree asked for %u", cfg.scl.i3c,
		      DT_PROP(I3C_DUAL_BOARD_LOOPBACK_CTRL_NODE, i3c_scl_hz));
	zassert_false(cfg.is_secondary, "board A reports itself as a secondary controller");
}

/*
 * i3c_config_get(I3C_CONFIG_CONTROLLER) returns the driver's software
 * shadow of the requested configuration, not a decode of the SCL timing
 * registers, so the readbacks below prove bookkeeping only.  The achieved
 * bus rate is not observable from this suite; the private write at the end
 * is what shows the IP still transfers after the timing registers were
 * rewritten.
 */
ZTEST(dual_board_loopback, test_configure_runtime_scl_change)
{
	struct i3c_config_controller cfg = {0};
	struct i3c_config_controller readback = {0};
	uint8_t probe = 0x5AU;
	struct i3c_msg msg = {
		.buf = &probe,
		.len = 1U,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};
	uint32_t saved;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	rc = i3c_config_get(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
	zassert_ok(rc, "config_get rc=%d", rc);
	saved = cfg.scl.i3c;

	cfg.scl.i3c = saved / 2U;
	rc = i3c_configure(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
	zassert_ok(rc, "configure to %u Hz rc=%d", saved / 2U, rc);

	rc = i3c_config_get(i3c_dev, I3C_CONFIG_CONTROLLER, &readback);
	zassert_ok(rc, "readback config_get rc=%d", rc);
	zassert_equal(readback.scl.i3c, saved / 2U, "SCL shadow is %u, asked for %u",
		      readback.scl.i3c, saved / 2U);

	rc = i3c_transfer_retry(&target_b_desc, &msg, 1U);
	zassert_ok(rc, "private write after SCL change failed (%d)", rc);

	cfg.scl.i3c = saved;
	rc = i3c_configure(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
	zassert_ok(rc, "restore to %u Hz rc=%d", saved, rc);

	rc = i3c_config_get(i3c_dev, I3C_CONFIG_CONTROLLER, &readback);
	zassert_ok(rc, "restore readback config_get rc=%d", rc);
	zassert_equal(readback.scl.i3c, saved, "SCL left at %u, expected %u restored",
		      readback.scl.i3c, saved);
}

ZTEST(dual_board_loopback, test_device_find_by_pid)
{
	struct i3c_device_id id = I3C_DEVICE_ID(target_b_desc.pid);
	struct i3c_device_id absent_id = I3C_DEVICE_ID(target_b_desc.pid ^ 1ULL);
	struct i3c_device_desc *desc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA();

	desc = i3c_device_find(i3c_dev, &id);
	zassert_not_null(desc, "device_find returned NULL for attached target");
	zassert_equal_ptr(desc, &target_b_desc,
			  "device_find returned a different descriptor for target_b's PID");

	/* Without this, a lookup that ignored the PID and returned the first
	 * attached descriptor would satisfy every check above.
	 */
	desc = i3c_device_find(i3c_dev, &absent_id);
	zassert_is_null(desc, "device_find matched a PID no device on the bus has");
}

ZTEST(dual_board_loopback, test_bus_has_sec_controller)
{
	bool has = i3c_bus_has_sec_controller(i3c_dev);

	/* i3c_bus_has_sec_controller() reports whether any attached device
	 * advertises the controller role in BCR[7:6].  Every device this
	 * suite attaches leaves that field at 0 (TARGET_BCR is 0x27).
	 */
	zassert_false(has, "expected no secondary controller on bus");
}

/* The SCL push-pull timing register is the only place the requested I3C rate
 * is observable: i3c_config_get() replays the driver's shadow of the last
 * request and never decodes the hardware.
 */
#define SCL_I3C_PP_TIMING_OFF 0xb8U
#define SCL_I3C_TIMING_CNT_MIN 5U
#define SCL_I3C_TIMING_CNT_MAX 255U

#define PP_LCNT(x)   ((x) & 0xFFU)
#define PP_HCNT(x)   (((x) >> 16) & 0xFFU)
#define PP_PERIOD(x) (PP_HCNT(x) + PP_LCNT(x))

/* The driver derives lcnt as DIV_ROUND_UP(core, scl) - hcnt in unsigned
 * arithmetic, so a core clock below 5 * i3c-scl-hz underflows and clamps to
 * 255, collapsing SCL to core/260 while i3c_configure() still returns 0.
 * A count parked on either rail means the devicetree rate was not achievable
 * at the configured core clock, not that the bus is running at it.
 */
ZTEST(dual_board_loopback, test_devicetree_scl_rate_is_achievable)
{
	const mem_addr_t regs = (mem_addr_t)DT_REG_ADDR(I3C_DUAL_BOARD_LOOPBACK_CTRL_NODE);
	uint32_t pp = sys_read32(regs + SCL_I3C_PP_TIMING_OFF);
	uint32_t lcnt = pp & 0xFFU;
	uint32_t hcnt = (pp >> 16) & 0xFFU;

	zassert_not_equal(lcnt, SCL_I3C_TIMING_CNT_MAX,
			  "PP 0x%08x: lcnt clamped high, %u Hz unreachable from this core clock",
			  pp, DT_PROP(I3C_DUAL_BOARD_LOOPBACK_CTRL_NODE, i3c_scl_hz));
	zassert_not_equal(lcnt, SCL_I3C_TIMING_CNT_MIN,
			  "PP 0x%08x: lcnt clamped low, %u Hz unreachable from this core clock",
			  pp, DT_PROP(I3C_DUAL_BOARD_LOOPBACK_CTRL_NODE, i3c_scl_hz));
	zassert_between_inclusive(hcnt, SCL_I3C_TIMING_CNT_MIN, SCL_I3C_TIMING_CNT_MAX,
				  "PP 0x%08x: hcnt %u out of range", pp, hcnt);
}

/* Guards a driver defect where the push-pull lcnt was computed from the
 * previous configuration instead of the one being applied, which made the
 * register lag one i3c_configure() call behind.  Both rates below must sit
 * inside the clamp so each produces a distinct register value.
 */
ZTEST(dual_board_loopback, test_configure_scl_applies_requested_rate)
{
	const mem_addr_t regs = (mem_addr_t)DT_REG_ADDR(I3C_DUAL_BOARD_LOOPBACK_CTRL_NODE);
	struct i3c_config_controller cfg = {0};
	uint32_t baseline;
	uint32_t pp_slow;
	uint32_t pp_fast;
	uint32_t pp_slow_again;
	uint32_t pp_restored;
	uint32_t saved;
	int rc_slow;
	int rc_fast;
	int rc_slow_again;
	int rc_restore;
	int rc;

	rc = i3c_config_get(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
	zassert_ok(rc, "config_get rc=%d", rc);
	saved = cfg.scl.i3c;
	baseline = sys_read32(regs + SCL_I3C_PP_TIMING_OFF);

	/* Collect everything before asserting so a failure still leaves the
	 * bus on the rate the rest of the suite expects.
	 */
	cfg.scl.i3c = 400000U;
	rc_slow = i3c_configure(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
	pp_slow = sys_read32(regs + SCL_I3C_PP_TIMING_OFF);

	cfg.scl.i3c = 625000U;
	rc_fast = i3c_configure(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
	pp_fast = sys_read32(regs + SCL_I3C_PP_TIMING_OFF);

	cfg.scl.i3c = 400000U;
	rc_slow_again = i3c_configure(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
	pp_slow_again = sys_read32(regs + SCL_I3C_PP_TIMING_OFF);

	cfg.scl.i3c = saved;
	rc_restore = i3c_configure(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
	pp_restored = sys_read32(regs + SCL_I3C_PP_TIMING_OFF);

	zassert_ok(rc_slow, "configure to 400 kHz rc=%d", rc_slow);
	zassert_ok(rc_fast, "configure to 625 kHz rc=%d", rc_fast);
	zassert_ok(rc_slow_again, "configure back to 400 kHz rc=%d", rc_slow_again);
	zassert_ok(rc_restore, "restore to %u Hz rc=%d", saved, rc_restore);

	zassert_not_equal(pp_slow, pp_fast,
			  "400 kHz and 625 kHz produced the same PP timing 0x%08x", pp_slow);
	zassert_equal(pp_slow_again, pp_slow,
		      "400 kHz gave PP 0x%08x first and 0x%08x after a 625 kHz call", pp_slow,
		      pp_slow_again);
	zassert_equal(pp_restored, baseline, "PP left at 0x%08x, expected 0x%08x restored",
		      pp_restored, baseline);
}

/* Negative/edge coverage for SCL rate assignment */
ZTEST(dual_board_loopback, test_configure_scl_rate_is_monotonic)
{
	static const uint32_t rates[] = {400000U,  1000000U,  2000000U,  3125000U,
					 6250000U, 12500000U, 25000000U, 50000000U};
	const mem_addr_t regs = (mem_addr_t)DT_REG_ADDR(I3C_DUAL_BOARD_LOOPBACK_CTRL_NODE);
	struct i3c_config_controller cfg = {0};
	uint32_t pp[ARRAY_SIZE(rates)];
	int rc_cfg[ARRAY_SIZE(rates)];
	uint32_t baseline;
	uint32_t pp_restored;
	uint32_t saved;
	int rc_restore;
	int rc;

	rc = i3c_config_get(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
	zassert_ok(rc, "config_get rc=%d", rc);
	saved = cfg.scl.i3c;
	baseline = sys_read32(regs + SCL_I3C_PP_TIMING_OFF);

	/* Collect everything before asserting so a failure still leaves the
	 * bus on the rate the rest of the suite expects.
	 */
	for (size_t i = 0U; i < ARRAY_SIZE(rates); i++) {
		cfg.scl.i3c = rates[i];
		rc_cfg[i] = i3c_configure(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
		pp[i] = sys_read32(regs + SCL_I3C_PP_TIMING_OFF);
	}

	cfg.scl.i3c = saved;
	rc_restore = i3c_configure(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
	pp_restored = sys_read32(regs + SCL_I3C_PP_TIMING_OFF);

	zassert_ok(rc_restore, "restore to %u Hz rc=%d", saved, rc_restore);
	zassert_equal(pp_restored, baseline, "PP left at 0x%08x, expected 0x%08x restored",
		      pp_restored, baseline);

	for (size_t i = 0U; i < ARRAY_SIZE(rates); i++) {
		zassert_ok(rc_cfg[i], "configure to %u Hz rc=%d", rates[i], rc_cfg[i]);
	}

	for (size_t i = 1U; i < ARRAY_SIZE(rates); i++) {
		zassert_true(PP_PERIOD(pp[i]) <= PP_PERIOD(pp[i - 1U]),
			     "%u Hz gave period %u (PP 0x%08x) but the slower %u Hz gave %u "
			     "(PP 0x%08x)",
			     rates[i], PP_PERIOD(pp[i]), pp[i], rates[i - 1U],
			     PP_PERIOD(pp[i - 1U]), pp[i - 1U]);
	}
}

/* Ensure an impossible SCL clock rate request gives us the highest possible
 * SCL rate
 */
ZTEST(dual_board_loopback, test_configure_unachievable_scl_saturates_fastest)
{
	const mem_addr_t regs = (mem_addr_t)DT_REG_ADDR(I3C_DUAL_BOARD_LOOPBACK_CTRL_NODE);
	struct i3c_config_controller cfg = {0};
	uint32_t baseline;
	uint32_t pp_fast;
	uint32_t pp_restored;
	uint32_t saved;
	int rc_fast;
	int rc_restore;
	int rc;

	rc = i3c_config_get(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
	zassert_ok(rc, "config_get rc=%d", rc);
	saved = cfg.scl.i3c;
	baseline = sys_read32(regs + SCL_I3C_PP_TIMING_OFF);

	cfg.scl.i3c = 50000000U;
	rc_fast = i3c_configure(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
	pp_fast = sys_read32(regs + SCL_I3C_PP_TIMING_OFF);

	cfg.scl.i3c = saved;
	rc_restore = i3c_configure(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
	pp_restored = sys_read32(regs + SCL_I3C_PP_TIMING_OFF);

	zassert_ok(rc_fast, "configure to 50 MHz rc=%d", rc_fast);
	zassert_ok(rc_restore, "restore to %u Hz rc=%d", saved, rc_restore);
	zassert_equal(pp_restored, baseline, "PP left at 0x%08x, expected 0x%08x restored",
		      pp_restored, baseline);

	zassert_not_equal(PP_LCNT(pp_fast), SCL_I3C_TIMING_CNT_MAX,
			  "PP 0x%08x: unreachable rate saturated to the slowest bus", pp_fast);
	zassert_equal(PP_LCNT(pp_fast), SCL_I3C_TIMING_CNT_MIN,
		      "PP 0x%08x: unreachable rate gave lcnt %u, expected the fast rail %u",
		      pp_fast, PP_LCNT(pp_fast), SCL_I3C_TIMING_CNT_MIN);
}

/* Ensure 0 is rejected to avoid a div-by-zero scenario */
ZTEST(dual_board_loopback, test_configure_zero_scl_is_rejected)
{
	const mem_addr_t regs = (mem_addr_t)DT_REG_ADDR(I3C_DUAL_BOARD_LOOPBACK_CTRL_NODE);
	struct i3c_config_controller cfg = {0};
	uint32_t baseline;
	uint32_t pp_after;
	uint32_t saved;
	int rc_zero;
	int rc;

	rc = i3c_config_get(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
	zassert_ok(rc, "config_get rc=%d", rc);
	saved = cfg.scl.i3c;
	baseline = sys_read32(regs + SCL_I3C_PP_TIMING_OFF);

	cfg.scl.i3c = 0U;
	rc_zero = i3c_configure(i3c_dev, I3C_CONFIG_CONTROLLER, &cfg);
	pp_after = sys_read32(regs + SCL_I3C_PP_TIMING_OFF);

	cfg.scl.i3c = saved;

	zassert_equal(rc_zero, -EINVAL, "configure to 0 Hz returned %d, expected -EINVAL",
		      rc_zero);
	zassert_equal(pp_after, baseline, "0 Hz altered PP to 0x%08x from 0x%08x", pp_after,
		      baseline);
}
