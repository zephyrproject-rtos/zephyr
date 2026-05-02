/*
 * Copyright (c) 2025 Analog Devices, Inc.
 * Copyright (c) 2025 Sean Kyer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/cpu_freq/policy.h>
#include <zephyr/cpu_freq/cpu_freq.h>

#define CPU_FREQ_POLICY_PRESSURE_THRESHOLD                                                         \
	((CONFIG_CPU_FREQ_POLICY_PRESSURE_LOWEST_PRIO <= K_LOWEST_THREAD_PRIO)                     \
		 ? CONFIG_CPU_FREQ_POLICY_PRESSURE_LOWEST_PRIO                                     \
		 : K_LOWEST_THREAD_PRIO)

LOG_MODULE_REGISTER(cpu_freq_policy_pressure, CONFIG_CPU_FREQ_LOG_LEVEL);

const struct pstate *soc_pstates[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(performance_states), PSTATE_DT_GET, (,))};

const size_t soc_pstates_count = ARRAY_SIZE(soc_pstates);

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1) && !defined(CONFIG_CPU_FREQ_PER_CPU_SCALING)

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

struct pressure_stats {
	int pressure_acum;
	int max_pressure;
};

static bool is_runnable(const struct k_thread *thread)
{
	bool rv = false;

	if ((thread->base.thread_state & _THREAD_QUEUED) != 0) {
		return true;
	}

#ifdef CONFIG_SMP
	uint8_t cpu = thread->base.cpu;

	rv = (_kernel.cpus[cpu].current == thread);
#endif /* CONFIG_SMP */

	return rv;
}

static void thread_eval_cb(const struct k_thread *thread, void *user_data)
{
	struct pressure_stats *pressure = user_data;

	LOG_DBG("Evaluating thread: %p with prio: %d status: %d", thread, thread->base.prio,
		thread->base.thread_state);

	int weight = CPU_FREQ_POLICY_PRESSURE_THRESHOLD - thread->base.prio + 1;

	if (weight < 0) {
		return;
	}

	pressure->max_pressure += weight;

	if (is_runnable(thread)) {
		pressure->pressure_acum += weight;
	}
}

static int get_normalized_sys_pressure(void)
{
	struct pressure_stats sys_pressure = {.max_pressure = 0, .pressure_acum = 0};

#ifdef CONFIG_CPU_FREQ_PER_CPU_SCALING
	k_thread_foreach_filter_by_cpu(_kernel.cpus[cpu].current, thread_eval_cb, &sys_pressure);
#else
	k_thread_foreach(thread_eval_cb, &sys_pressure);
#endif /* CONFIG_CPU_FREQ_PER_CPU_SCALING */

	int normalized_pressure = (sys_pressure.pressure_acum * 100) / sys_pressure.max_pressure;

	LOG_DBG("System pressure is: %d%% (raw: %d / max: %d)", normalized_pressure,
		sys_pressure.pressure_acum, sys_pressure.max_pressure);

	return normalized_pressure;
}

/*
 * The pressure policy iterates through the threads currently sitting in the ready queue at the time
 * of evaluation and accumulates the sum of their priorities, normalizing them around
 * CPU_FREQ_POLICY_PRESSURE_THRESHOLD, configured by the user. The policy then iterates through the
 * list of available P-states and selects the first P-state where the current normalized system
 * pressure is greater than or equal to the load threshold of the P-state. If the calculated
 * pressure is below all available P-state thresholds, then the last P-state in the array will be
 * selected. P-states must be defined in decreasing threshold order.
 */
int cpu_freq_policy_select_pstate(const struct pstate **pstate_out)
{
	int sys_pressure = 0;
	int cpu_id = 0;

	if (NULL == pstate_out) {
		LOG_ERR("On-Demand Policy: pstate_out is NULL");
		return -EINVAL;
	}

#if defined(CONFIG_SMP)
	/* The caller has already ensured that the CPU is fixed */
	cpu_id = arch_curr_cpu()->id;
#endif

	sys_pressure = get_normalized_sys_pressure();

	if (sys_pressure < 0) {
		LOG_ERR("Unable to retrieve system pressure");
		return sys_pressure;
	}

	LOG_DBG("CPU%d Pressure: %d%%", cpu_id, sys_pressure);

	for (int i = 0; i < soc_pstates_count; i++) {
		const struct pstate *state = soc_pstates[i];

		if (sys_pressure >= state->load_threshold) {
			*pstate_out = state;
			LOG_DBG("Pressure Policy: Selected P-state "
				"%d with load_threshold=%d%%",
				i, state->load_threshold);
			return 0;
		}
	}

	/* No threshold matched: select the last P-state (lowest performance) */
	*pstate_out = soc_pstates[soc_pstates_count - 1];
	LOG_DBG("Pressure Policy: No threshold matched for CPU load %d%%;"
		"selecting last P-state (load_threshold=%d%%)",
		sys_pressure, soc_pstates[soc_pstates_count - 1]->load_threshold);

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
	int ret;

#ifdef CPU_FREQ_IPI_TRACKING
	k_spinlock_key_t key = k_spin_lock(&lock);

	if ((pstate_best == NULL) || (state->load_threshold > pstate_best->load_threshold)) {
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

	ret = cpu_freq_pstate_set(state);
	if (ret != 0) {
		LOG_ERR("Failed to set P-state: %d", ret);
		return NULL;
	}

	return state;
}
