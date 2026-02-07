/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/drivers/timer/system_timer.h>

ZTEST(sys_clock_hw_cycles_update, test_update_no_change_is_noop)
{
	uint32_t old_hz = sys_clock_hw_cycles_per_sec();

	z_sys_clock_hw_cycles_per_sec_update(old_hz);
	zassert_equal(sys_clock_hw_cycles_per_sec(), old_hz, "frequency changed unexpectedly");
}

ZTEST(sys_clock_hw_cycles_update, test_update_zero_is_ignored)
{
	uint32_t old_hz = sys_clock_hw_cycles_per_sec();

	z_sys_clock_hw_cycles_per_sec_update(0U);
	zassert_equal(sys_clock_hw_cycles_per_sec(), old_hz, "frequency changed unexpectedly");
}

ZTEST(sys_clock_hw_cycles_update, test_update_changes_value_is_visible_via_getter)
{
	uint32_t old_hz = sys_clock_hw_cycles_per_sec();
	uint32_t new_hz = (old_hz == 1000000U) ? 1000001U : 1000000U;

	z_sys_clock_hw_cycles_per_sec_update(new_hz);
	zassert_equal(sys_clock_hw_cycles_per_sec(), new_hz, "frequency not updated");
}

ZTEST_SUITE(sys_clock_hw_cycles_update, NULL, NULL, NULL, NULL, NULL);
