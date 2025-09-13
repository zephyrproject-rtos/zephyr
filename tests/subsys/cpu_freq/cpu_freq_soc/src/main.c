/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/cpu_freq/cpu_freq.h>

LOG_MODULE_REGISTER(cpu_freq_soc_test, LOG_LEVEL_INF);

const struct pstate *soc_pstates_dt[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(performance_states), PSTATE_DT_GET, (,))};

/*
 * Test SoC integration of CPU Freq
 */
ZTEST(cpu_freq_soc, test_soc_pstates)
{
	int ret;

	zassert_true(ARRAY_SIZE(soc_pstates_dt) > 0, "No p-states defined in devicetree");

	LOG_INF("%zu p-states defined for %s", ARRAY_SIZE(soc_pstates_dt), CONFIG_BOARD_TARGET);

	zassert_equal(cpu_freq_pstate_set(NULL), -EINVAL, "Expected -EINVAL for NULL pstate");

	for (int i = 0; i < ARRAY_SIZE(soc_pstates_dt); i++) {
		const struct pstate *state = soc_pstates_dt[i];

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
		/*
		 * Lock the scheduler to ensure that the current thread
		 * does not migrate to another CPU.
		 */
		k_sched_lock();
#endif

		/* Set performance state using pstate driver */
		ret = cpu_freq_pstate_set(state);

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
		k_sched_unlock();
#endif
		zassert_equal(ret, 0, "Failed to set p-state %d", i);
	}
}

ZTEST_SUITE(cpu_freq_soc, NULL, NULL, NULL, NULL, NULL);
