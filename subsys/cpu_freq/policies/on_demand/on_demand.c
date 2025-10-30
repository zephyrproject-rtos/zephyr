/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/cpu_freq/policy.h>
#include <zephyr/cpu_freq/cpu_freq.h>
#include <zephyr/sys/cpu_load.h>

LOG_MODULE_REGISTER(cpu_freq_policy_on_demand, CONFIG_CPU_FREQ_LOG_LEVEL);

const struct pstate *soc_pstates[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(performance_states), PSTATE_DT_GET, (,))
};

const size_t soc_pstates_count = ARRAY_SIZE(soc_pstates);

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1) && \
	!defined(CONFIG_CPU_FREQ_PER_CPU_SCALING)

/*
 * IPI tracking is needed on SMP systems where all CPUs share the same
 * frequency. The last CPU to call cpu_freq_best_pstate() sets the best
 * P-state for all CPUs.
 */

#define CPU_FREQ_IPI_TRACKING

static struct k_spinlock lock;
static const struct pstate *pstate_best;
static unsigned int num_unprocessed_cpus;

#endif /* CONFIG_SMP && (CONFIG_MP_MAX_NUM_CPUS > 1) && !CONFIG_CPU_FREQ_PER_CPU_SCALING */

/*
 * On-demand policy scans the list of P-states from the devicetree and selects the first
 * P-state where the cpu_load is greater than or equal to the trigger threshold of the
 * P-state. If no P-state matches (i.e., the load is below all thresholds), the policy
 * will select the last P-state in the array (the lowest performance state). This is
 * intrinsic behavior: P-states must be defined in increasing threshold order.
 */
int cpu_freq_policy_select_pstate(const struct pstate **pstate_out)
{
	int cpu_load;
	int cpu_id = 0;

	if (pstate_out == NULL) {
		LOG_ERR("On-Demand Policy: pstate_out is NULL");
		return -EINVAL;
	}

#if defined(CONFIG_SMP)
	/* The caller has already ensured that the CPU is fixed */
	cpu_id = arch_curr_cpu()->id;
#endif

	cpu_load = cpu_load_metric_get(cpu_id);
	if (cpu_load < 0) {
		LOG_ERR("Unable to retrieve CPU load");
		return cpu_load;
	}

	LOG_DBG("CPU%d Load: %d%%", cpu_id, cpu_load);

	for (int i = 0; i < soc_pstates_count; i++) {
		const struct pstate *state = soc_pstates[i];

		if (cpu_load >= state->load_threshold) {
			*pstate_out = state;
			LOG_DBG("On-Demand Policy: Selected P-state "
				"%d with load_threshold=%d%%", i,
				state->load_threshold);
			return 0;
		}
	}

	/* No threshold matched: select the last P-state (lowest performance) */
	*pstate_out = soc_pstates[soc_pstates_count - 1];
	LOG_DBG("On-Demand Policy: No threshold matched for CPU load %d%%;"
		"selecting last P-state (load_threshold=%d%%)",
		cpu_load, soc_pstates[soc_pstates_count - 1]->load_threshold);

	return 0;
}

void cpu_freq_policy_reset(void)
{
#ifdef CPU_FREQ_IPI_TRACKING
	k_spinlock_key_t key = k_spin_lock(&lock);

	pstate_best = NULL;
	num_unprocessed_cpus = arch_num_cpus();

	k_spin_unlock(&lock, key);
#endif
}

const struct pstate *cpu_freq_policy_pstate_set(const struct pstate *state)
{
	int rv;


#ifdef CPU_FREQ_IPI_TRACKING
	k_spinlock_key_t key = k_spin_lock(&lock);

	if ((pstate_best == NULL) ||
	    (state->load_threshold > pstate_best->load_threshold)) {
		pstate_best = state;
	}

	__ASSERT(num_unprocessed_cpus != 0U, "cpu_freq: Out of sync");

	num_unprocessed_cpus--;
	if (num_unprocessed_cpus > 0) {
		k_spin_unlock(&lock, key);
		return NULL;
	}
	state = pstate_best;
	k_spin_unlock(&lock, key);
#endif

	rv = cpu_freq_pstate_set(state);
	if (rv != 0) {
		LOG_ERR("Failed to set P-state: %d", rv);
		return NULL;
	}

	return state;
}
