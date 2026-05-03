/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/cpu_freq/cpu_freq.h>
#include <zephyr/cpu_freq/policy.h>
#include <zephyr/cpu_workload/cpu_workload.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/state.h>
#include <zephyr/power/energy_model.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>

LOG_MODULE_REGISTER(cpu_freq_policy_energy_aware, CONFIG_CPU_FREQ_LOG_LEVEL);

const struct pstate *soc_pstates[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(performance_states), PSTATE_DT_GET, (,))
};

const size_t soc_pstates_count = ARRAY_SIZE(soc_pstates);

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1) && \
	!defined(CONFIG_CPU_FREQ_PER_CPU_SCALING)
#define CPU_FREQ_IPI_TRACKING

static struct k_spinlock lock;
static const struct pstate *pstate_best;
static unsigned int num_unprocessed_cpus;
#endif /* CONFIG_SMP && CONFIG_MP_MAX_NUM_CPUS > 1 && !CONFIG_CPU_FREQ_PER_CPU_SCALING */

static int current_cpu_id(void)
{
#if defined(CONFIG_SMP)
	return arch_curr_cpu()->id;
#else
	return 0;
#endif /* CONFIG_SMP */
}

static uint32_t lowest_pstate_frequency_hz(void)
{
	uint32_t lowest_hz = UINT32_MAX;

	for (size_t i = 0U; i < soc_pstates_count; i++) {
		const struct pstate *state = soc_pstates[i];

		if (state->frequency_hz == 0U) {
			continue;
		}

		lowest_hz = MIN(lowest_hz, state->frequency_hz);
	}

	return lowest_hz;
}

static uint64_t cycles_to_us_ceil(uint64_t cycles, uint32_t frequency_hz)
{
	uint64_t seconds;
	uint64_t remainder;
	uint64_t us;

	if (frequency_hz == 0U) {
		return UINT64_MAX;
	}

	seconds = cycles / frequency_hz;
	remainder = cycles % frequency_hz;

	if (seconds > (UINT64_MAX / USEC_PER_SEC)) {
		return UINT64_MAX;
	}

	us = seconds * USEC_PER_SEC;
	return us + DIV_ROUND_UP(remainder * USEC_PER_SEC, (uint64_t)frequency_hz);
}

static uint64_t pstate_exec_time_us(const struct pstate *state, uint64_t estimated_cycles)
{
	return cycles_to_us_ceil(estimated_cycles, state->frequency_hz);
}

static uint64_t pstate_transition_time_us(const struct pstate *current_state,
						  const struct pstate *target_state)
{
	if (current_state == target_state) {
		return 0U;
	}

	return target_state->transition_latency_us;
}

static uint64_t pstate_finish_time_us(uint64_t exec_us, uint64_t transition_us)
{
	if (exec_us > (UINT64_MAX - transition_us)) {
		return UINT64_MAX;
	}

	return exec_us + transition_us;
}

static uint64_t fallback_deadline_us(void)
{
	if (CONFIG_CPU_FREQ_POLICY_ENERGY_AWARE_DEADLINE_US > 0) {
		return CONFIG_CPU_FREQ_POLICY_ENERGY_AWARE_DEADLINE_US;
	}

	return (uint64_t)CONFIG_CPU_FREQ_INTERVAL_MS * USEC_PER_MSEC;
}

static uint64_t policy_deadline_us(void)
{
	uint64_t deadline_us = fallback_deadline_us();
	int64_t next_event_ticks = pm_policy_next_event_ticks();

	if (next_event_ticks >= 0) {
		uint64_t next_event_us = k_ticks_to_us_ceil64((uint64_t)next_event_ticks);

		deadline_us = MIN(deadline_us, next_event_us);
	}

	return deadline_us;
}

static uint64_t candidate_energy_score(const struct pstate *current_state,
					       const struct pstate *target_state,
					       uint64_t estimated_cycles, uint64_t exec_us,
					       uint64_t transition_us, uint64_t idle_us,
					       const struct pm_state_info *idle_state,
					       uint32_t lowest_hz)
{
	struct power_energy_eval_result result;
	struct power_energy_eval_input input = {
		.from_pstate = current_state,
		.to_pstate = target_state,
		.idle_state = idle_state,
		.estimated_cycles = estimated_cycles,
		.exec_time_us = exec_us,
		.transition_time_us = transition_us,
		.idle_time_us = idle_us,
		.min_frequency_hz = lowest_hz,
	};
	int ret;

	ret = power_energy_eval_pstate(&input, &result);
	if (ret != 0) {
		return UINT64_MAX;
	}

	return result.total_energy;
}

static bool pstate_has_more_capacity(const struct pstate *left, const struct pstate *right)
{
	return left->frequency_hz > right->frequency_hz;
}

static bool pstate_has_lower_capacity(const struct pstate *left, const struct pstate *right)
{
	return left->frequency_hz < right->frequency_hz;
}

int cpu_freq_policy_select_pstate(const struct pstate **pstate_out)
{
	struct cpu_workload_estimate estimate;
	const struct pstate *best_state = NULL;
	const struct pstate *current_state;
	const struct pstate *fastest_state = NULL;
	uint64_t best_score = UINT64_MAX;
	uint64_t fastest_finish_us = UINT64_MAX;
	uint64_t deadline_us;
	uint32_t lowest_hz;
	int cpu_id = current_cpu_id();
	int ret;

	if (pstate_out == NULL) {
		LOG_ERR("Energy-aware Policy: pstate_out is NULL");
		return -EINVAL;
	}

	ret = cpu_workload_estimate_get(cpu_id, &estimate);
	if (ret != 0) {
		LOG_ERR("CPU%d workload estimate unavailable: %d", cpu_id, ret);
		return ret;
	}

	deadline_us = policy_deadline_us();
	lowest_hz = lowest_pstate_frequency_hz();
	current_state = cpu_freq_pstate_current_get();

	if ((deadline_us == 0U) || (lowest_hz == UINT32_MAX)) {
		*pstate_out = soc_pstates[0];
		return 0;
	}

	for (size_t i = 0U; i < soc_pstates_count; i++) {
		const struct pstate *state = soc_pstates[i];
		uint64_t exec_us = pstate_exec_time_us(state, estimate.estimated_cycles);
		uint64_t transition_us = pstate_transition_time_us(current_state, state);
		uint64_t finish_us = pstate_finish_time_us(exec_us, transition_us);
		uint64_t score;

		if ((finish_us < fastest_finish_us) ||
		    ((finish_us == fastest_finish_us) &&
		     ((fastest_state == NULL) || pstate_has_more_capacity(state, fastest_state)))) {
			fastest_state = state;
			fastest_finish_us = finish_us;
		}

		if (finish_us > deadline_us) {
			LOG_DBG("CPU%d reject P-state %zu: finish=%llu us deadline=%llu us",
				cpu_id, i, (unsigned long long)finish_us,
				(unsigned long long)deadline_us);
			continue;
		}

		uint64_t idle_us = deadline_us - finish_us;
		const struct pm_state_info *idle_state =
			pm_policy_state_for_residency((uint8_t)cpu_id, idle_us);

		score = candidate_energy_score(current_state, state, estimate.estimated_cycles,
					       exec_us, transition_us, idle_us, idle_state, lowest_hz);
		if ((best_state == NULL) || (score < best_score) ||
		    ((score == best_score) && pstate_has_lower_capacity(state, best_state))) {
			best_state = state;
			best_score = score;
		}

		LOG_DBG("CPU%d candidate P-state %zu: cycles=%llu finish=%llu us idle=%llu us "
			"score=%llu confidence=%u", cpu_id, i,
			(unsigned long long)estimate.estimated_cycles, (unsigned long long)finish_us,
			(unsigned long long)(deadline_us - finish_us), (unsigned long long)score,
			estimate.confidence);
	}

	if (best_state == NULL) {
		best_state = fastest_state;
		LOG_DBG("CPU%d no legal P-state for deadline=%llu us; selecting fastest finish=%llu us",
			cpu_id, (unsigned long long)deadline_us, (unsigned long long)fastest_finish_us);
	}

	*pstate_out = best_state;
	LOG_DBG("CPU%d Energy-aware Policy: selected frequency=%u Hz latency=%u us",
		cpu_id, best_state->frequency_hz, best_state->transition_latency_us);

	return 0;
}

void cpu_freq_policy_reset(void)
{
#ifdef CPU_FREQ_IPI_TRACKING
	k_spinlock_key_t key = k_spin_lock(&lock);

	pstate_best = NULL;
	num_unprocessed_cpus = arch_num_cpus();

	k_spin_unlock(&lock, key);
#endif /* CPU_FREQ_IPI_TRACKING */
}

const struct pstate *cpu_freq_policy_pstate_set(const struct pstate *state)
{
	int ret;

#ifdef CPU_FREQ_IPI_TRACKING
	k_spinlock_key_t key = k_spin_lock(&lock);

	if ((pstate_best == NULL) || pstate_has_more_capacity(state, pstate_best)) {
		pstate_best = state;
	}

	__ASSERT(num_unprocessed_cpus != 0U, "cpu_freq: Out of sync");

	num_unprocessed_cpus--;
	if (num_unprocessed_cpus > 0U) {
		k_spin_unlock(&lock, key);
		return NULL;
	}
	state = pstate_best;
	k_spin_unlock(&lock, key);
#endif /* CPU_FREQ_IPI_TRACKING */

	ret = cpu_freq_pstate_set(state);
	if (ret != 0) {
		LOG_ERR("Failed to set P-state: %d", ret);
		return NULL;
	}

	return state;
}
