/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POWER_ENERGY_MODEL_H_
#define ZEPHYR_INCLUDE_POWER_ENERGY_MODEL_H_

#include <stdint.h>

#include <zephyr/cpu_freq/pstate.h>
#include <zephyr/pm/state.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Power energy model helpers.
 * @defgroup subsys_power_energy_model Power Energy Model
 * @since 4.5
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

/** Input facts for evaluating one candidate P-state. */
struct power_energy_eval_input {
	/** Current P-state, if known. */
	const struct pstate *from_pstate;

	/** Candidate target p-state, the frequency point currently being evaluated. */
	const struct pstate *to_pstate;

	/** Predicted PM idle state after the workload finishes, or NULL for active. */
	const struct pm_state_info *idle_state;

	/** Estimated workload size in cycles. */
	uint64_t estimated_cycles;

	/** The duration to execute estimated_cycles under the current candidate p-state. */
	uint64_t exec_time_us;

	/** Candidate p-state transition time in microseconds. */
	uint64_t transition_time_us;

	/** The remaining idle time available before the deadline after execution and context
	 * switching are completed.
	 */
	uint64_t idle_time_us;

	/** The lowest non-zero frequency in the current set of allowed p-states. */
	uint32_t min_frequency_hz;
};

/** Comparable energy-score components for one candidate P-state. */
struct power_energy_eval_result {
	/** Active execution energy score. */
	uint64_t active_energy;

	/** P-state transition energy score. */
	uint64_t transition_energy;

	/** Predicted idle energy score. */
	uint64_t idle_energy;

	/** Sum of active, transition and idle energy scores. */
	uint64_t total_energy;
};

/**
 * @brief Evaluate the relative energy score for a P-state candidate.
 *
 * The first implementation uses relative scores. Platforms can later replace
 * the score inputs with calibrated power and transition-energy data without
 * changing CPUFreq policy structure.
 *
 * @param input Candidate facts.
 * @param result Energy-score output.
 *
 * @retval 0 If @p result was written.
 * @retval -EINVAL If required input is invalid.
 */
int power_energy_eval_pstate(const struct power_energy_eval_input *input,
			     struct power_energy_eval_result *result);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POWER_ENERGY_MODEL_H_ */
