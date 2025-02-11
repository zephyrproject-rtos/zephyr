/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_driver.h"

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/pm/pm.h>

/* Last state has not declared a minimum residency, so it should be
 * set the default 0 value
 */
static struct pm_state_info infos[] = {{PM_STATE_SUSPEND_TO_IDLE, 0, 10000, 100},
	       {PM_STATE_STANDBY, 0, 20000, 200}, {PM_STATE_SUSPEND_TO_RAM, 0, 50000, 500}};
static enum pm_state states[] = {PM_STATE_SUSPEND_TO_IDLE,
			PM_STATE_STANDBY, PM_STATE_SUSPEND_TO_RAM};
static enum pm_state wrong_states[] = {PM_STATE_SUSPEND_TO_DISK,
		PM_STATE_SUSPEND_TO_RAM, PM_STATE_SUSPEND_TO_RAM};

static uint8_t suspend_to_ram_count;

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	if (state == PM_STATE_SUSPEND_TO_RAM) {
		suspend_to_ram_count++;
	}

	k_cpu_idle();
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	irq_unlock(0);
}

ZTEST(power_states_1cpu, test_power_states)
{
	uint8_t ret;
	const struct pm_state_info *cpu_states;
	enum pm_state dts_states[] =
		PM_STATE_LIST_FROM_DT_CPU(DT_NODELABEL(cpu0));
	struct pm_state_info dts_infos[] =
		PM_STATE_INFO_LIST_FROM_DT_CPU(DT_NODELABEL(cpu0));
	uint32_t dts_states_len =
		DT_NUM_CPU_POWER_STATES(DT_NODELABEL(cpu0));

	zassert_true(ARRAY_SIZE(states) == dts_states_len,
		     "Invalid number of pm states");
	zassert_true(memcmp(infos, dts_infos, ARRAY_SIZE(dts_infos)) == 0,
		     "Invalid pm_state_info array");
	zassert_true(memcmp(states, dts_states, ARRAY_SIZE(dts_states)) == 0,
		     "Invalid pm-states array");

	zassert_false(memcmp(wrong_states, dts_states, ARRAY_SIZE(dts_states)) == 0,
		     "Invalid pm-states array");

	ret = pm_state_cpu_get_all(CONFIG_MP_MAX_NUM_CPUS + 1, &cpu_states);
	zassert_true(ret == 0, "Invalid pm_state_cpu_get_all return");

	ret = pm_state_cpu_get_all(0U, &cpu_states);
	zassert_true(ret == dts_states_len,
		     "Invalid number of pm states");
	zassert_true(memcmp(cpu_states, dts_infos, ARRAY_SIZE(dts_infos)) == 0,
		     "Invalid pm_state_info array");
}

ZTEST(power_states_1cpu, test_device_power_state_constraints)
{
	static const struct device *const dev =
		DEVICE_DT_GET(DT_NODELABEL(test_dev));
	suspend_to_ram_count = 0;

	test_driver_async_operation(dev);

	/** Lets sleep long enough to suspend the CPU with `suspend to ram`
	 *  power state. If everything works well the cpu should not use this
	 *  state due the constraint set by `test_dev`.
	 */
	k_sleep(K_USEC(60000));

	zassert_true(suspend_to_ram_count == 0, "Invalid suspend to ram count");

	/** Now lets check ensure that if there is no ongoing work
	 *  the cpu will suspend to ram.
	 */
	k_sleep(K_MSEC(600));

	zassert(suspend_to_ram_count != 0, "Not suspended to ram");
}

ZTEST_SUITE(power_states_1cpu, NULL, NULL, ztest_simple_1cpu_before,
			ztest_simple_1cpu_after, NULL);
