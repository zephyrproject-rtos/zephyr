/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <power/power.h>

/* Last state has not declared a minimum residency, so it should be
 * set the default 0 value
 */
static struct pm_state_info infos[] = {{PM_STATE_SUSPEND_TO_IDLE, 1},
			{PM_STATE_SUSPEND_TO_RAM, 5}, {PM_STATE_STANDBY, 0}};
static enum pm_state states[] = {PM_STATE_SUSPEND_TO_IDLE,
				PM_STATE_SUSPEND_TO_RAM, PM_STATE_STANDBY};
static enum pm_state wrong_states[] = {PM_STATE_SUSPEND_TO_DISK,
			PM_STATE_SUSPEND_TO_RAM, PM_STATE_SUSPEND_TO_RAM};

void test_power_states(void)
{
	enum pm_state dts_states[] =
		PM_STATE_DT_ITEMS_LIST(DT_NODELABEL(power_states));
	struct pm_state_info dts_infos[] =
		PM_STATE_INFO_DT_ITEMS_LIST(DT_NODELABEL(power_states));
	uint32_t dts_states_len =
		PM_STATE_DT_ITEMS_LEN(DT_NODELABEL(power_states));

	zassert_true(ARRAY_SIZE(states) == dts_states_len,
		     "Invalid number of pm states");
	zassert_true(memcmp(infos, dts_infos, sizeof(dts_infos)) == 0,
		     "Invalid pm_state_info array");
	zassert_true(memcmp(states, dts_states, sizeof(dts_states)) == 0,
		     "Invalid pm-states array");

	zassert_false(memcmp(wrong_states, dts_states, sizeof(dts_states)) == 0,
		     "Invalid pm-states array");
}

void test_main(void)
{
	ztest_test_suite(power_states_test,
			 ztest_1cpu_unit_test(test_power_states));
	ztest_run_test_suite(power_states_test);
}
