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

/**
 * There are two deadline sources, and the earlier one is chosen.
 *
 * The first source is the fallback deadline, if ENERGY_AWARE_DEADLINE_US is
 * configured, it uses ENERGY_AWARE_DEADLINE_US; otherwise, it uses CPU_FREQ_INTERVAL_MS.
 *
 * The second source is the PM's next event. That is, from the system power management
 * perspective, how long until the next event occurs.
 */
static uint64_t policy_deadline_us(void)
{
	uint64_t deadline_us = (uint64_t)CONFIG_CPU_FREQ_INTERVAL_MS * USEC_PER_MSEC;
	int64_t next_event_ticks = pm_policy_next_event_ticks();

	if (CONFIG_CPU_FREQ_POLICY_ENERGY_AWARE_DEADLINE_US > 0) {
		deadline_us = CONFIG_CPU_FREQ_POLICY_ENERGY_AWARE_DEADLINE_US;
	}

	if (next_event_ticks >= 0) {
		/* Round up to avoid calculating the deadline too early. */
		uint64_t next_event_us = k_ticks_to_us_ceil64((uint64_t)next_event_ticks);

		deadline_us = MIN(deadline_us, next_event_us);
	}

	return deadline_us;
}

/**
 * Find the lowest non-zero frequency among all P-states. This frequency value serves
 * as the baseline for the energy model. We need to know what the lowest frequency is,
 * and then compute the scale by taking the ratio of the candidate frequency to the
 * lowest frequency.
 */
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

/**
 * Convert 'how many cycles to run into 'how many microseconds' at a certain frequency.
 * Avoid overflow during calculation and round up to make the deadline judgment more
 * conservative.
 */
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

/**
 * Perform a small simulation on each P-state:
 * - Can this frequency finish the work before the deadline?
 * - How long can it idle after finishing?
 * - How deep an idle state can it enter?
 * - What is the total score of active + transition + idle?
 *
 * Then select the frequency that meets the deadline with the lowest energy score.
 * If none of them meet the deadline, select the fastest one.
 */
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
	int cpu_id = CPU_ID;
	int ret;

	if (pstate_out == NULL) {
		LOG_ERR("Energy-aware Policy: pstate_out is NULL");
		return -EINVAL;
	}

	/* Get the workload estimate for the current CPU. */
	ret = cpu_workload_estimate_get(cpu_id, &estimate);
	if (ret != 0) {
		LOG_ERR("CPU%d workload estimate unavailable: %d", cpu_id, ret);
		return ret;
	}

	deadline_us = policy_deadline_us();
	lowest_hz = lowest_pstate_frequency_hz();

	/* Without a valid deadline, or if no valid minimum frequency can be found,
	 * the policy cannot perform a complete energy-aware evaluation and therefore
	 * falls back to the first P-state.
	 */
	if ((deadline_us == 0U) || (lowest_hz == UINT32_MAX)) {
		*pstate_out = soc_pstates[0];
		return 0;
	}

	current_state = cpu_freq_pstate_current_get();

	/* Treat each P-state as a candidate and perform a trial calculation for it. */
	for (size_t i = 0U; i < soc_pstates_count; i++) {
		const struct pstate *state = soc_pstates[i];
		/* Calculate how long it will take to switch to the target state. */
		uint64_t exec_us = cycles_to_us_ceil(estimate.estimated_cycles, state->frequency_hz);
		/* Calculate how long it takes to switch to the target state. */
		uint64_t transition_us = (current_state == state) ? 0U : state->transition_latency_us;
		/* Calculate the total time to finish the workload in the target state. */
		uint64_t finish_us = (exec_us > (UINT64_MAX - transition_us)) ? UINT64_MAX :
				      exec_us + transition_us;
		uint64_t score;

		if ((finish_us < fastest_finish_us) ||
		    ((finish_us == fastest_finish_us) && ((fastest_state == NULL) ||
		      (state->frequency_hz > fastest_state->frequency_hz)))) {
			fastest_state = state;
			fastest_finish_us = finish_us;
		}

		/* If the candidate P-state cannot finish execution before deadline_us,
		 * discard it first.
		 */
		if (finish_us > deadline_us) {
			LOG_DBG("CPU%d reject P-state %zu: finish=%llu us deadline=%llu us",
				cpu_id, i, (unsigned long long)finish_us,
				(unsigned long long)deadline_us);
			continue;
		}

		/* If the task finishes on time, the remaining idle time is calculated.
		 * The faster it finishes, the more idle time is left. The more idle time remaining,
		 * the more likely it is to enter a deeper low-power state.
		 
		 * Then it asks the PM: which idle state can we approximately enter for this idle duration.
		 */
		uint64_t idle_us = deadline_us - finish_us;
		const struct pm_state_info *idle_state =
			pm_policy_state_for_residency((uint8_t)cpu_id, idle_us);

		/* Calculate the energy score of this candidate. */
		score = candidate_energy_score(current_state, state, estimate.estimated_cycles,
					       exec_us, transition_us, idle_us, idle_state, lowest_hz);

		/* best_state is the one with the lower energy score. If the energy scores are the same,
		 * the state with the lower frequency wins.
		 */
		if ((best_state == NULL) || (score < best_score) ||
		    ((score == best_score) && (state->frequency_hz < best_state->frequency_hz))) {
			best_state = state;
			best_score = score;
		}

		LOG_DBG("CPU%d candidate P-state %zu: cycles=%llu finish=%llu us idle=%llu us "
			"score=%llu confidence=%u", cpu_id, i,
			(unsigned long long)estimate.estimated_cycles, (unsigned long long)finish_us,
			(unsigned long long)(deadline_us - finish_us), (unsigned long long)score,
			estimate.confidence);
	}

	/* If no candidate can finish before the deadline, select the
	 * fastest one to at least meet the performance requirement.
	 */
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
	return;
}

const struct pstate *cpu_freq_policy_pstate_set(const struct pstate *state)
{
	int ret;

	ret = cpu_freq_pstate_set(state);
	if (ret != 0) {
		LOG_ERR("Failed to set P-state: %d", ret);
		return NULL;
	}

	return state;
}
