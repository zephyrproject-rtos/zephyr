/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief PI clock discipline for precision timing.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_PI_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_PI_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/precision_timing/precision_time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Precision PI Discipline
 * @defgroup precision_pi Precision PI Discipline
 * @since 4.5
 * @version 0.1.0
 * @ingroup precision_timing
 * @{
 */

/**
 * @brief Denominator shared by the PI gain numerators.
 *
 * Gains are expressed in thousandths, so a numerator of 700 configures a gain
 * of 0.7. @kconfig{CONFIG_PRECISION_TIMING_PI_KP} and
 * @kconfig{CONFIG_PRECISION_TIMING_PI_KI} use the same scale.
 */
#define PRECISION_PI_GAIN_DEN 1000U

/** Synchronization state of a precision discipline instance. */
enum precision_sync_state {
	/** No usable synchronization state is available. */
	PRECISION_SYNC_UNSYNCED = 0,
	/** Valid observations are being acquired. */
	PRECISION_SYNC_ACQUIRING,
	/** The configured lock criteria have been met. */
	PRECISION_SYNC_LOCKED,
	/** Source observations are stale and the previous correction is retained. */
	PRECISION_SYNC_HOLDOVER,
	/** A clock operation failed and explicit reset is required. */
	PRECISION_SYNC_FAULT,
};

/** Control action selected by the discipline engine. */
enum precision_discipline_action {
	/** Do not modify the controlled clock. */
	PRECISION_DISCIPLINE_IGNORE = 0,
	/** Step the controlled clock by the returned phase correction. */
	PRECISION_DISCIPLINE_STEP,
	/** Apply the returned rate correction. */
	PRECISION_DISCIPLINE_ADJUST_RATE,
	/** Reset protocol and clock discipline state. */
	PRECISION_DISCIPLINE_RESET,
};

/** Configuration of a PI precision clock discipline. */
struct precision_pi_config {
	/** Required source observation domain. */
	struct precision_time_domain source_domain;
	/** Required local observation domain. */
	struct precision_time_domain local_domain;
	/** Absolute offset above which a clock step is requested. */
	precision_time_t step_threshold_ns;
	/** Maximum absolute offset considered locked. */
	precision_time_t lock_threshold_ns;
	/** Absolute offset treated as an outlier while locked. */
	precision_time_t outlier_threshold_ns;
	/** Maximum accepted observation uncertainty, or zero to disable the limit. */
	precision_time_t max_uncertainty_ns;
	/** Maximum source age before entering holdover, or zero to disable timeout. */
	precision_time_t source_timeout_ns;
	/** Holdover duration before resetting, or zero for indefinite holdover. */
	precision_time_t holdover_ns;
	/** Consecutive in-threshold samples required for lock, or zero for immediate lock. */
	uint8_t lock_sample_count;
	/** Consecutive outliers required to reset the discipline. */
	uint8_t outlier_sample_count;
	/** Minimum output rate correction in parts per billion. */
	int32_t min_rate_ppb;
	/** Maximum output rate correction in parts per billion. */
	int32_t max_rate_ppb;
	/** Proportional gain numerator. */
	int32_t kp_num;
	/** Integral gain numerator. */
	int32_t ki_num;
	/** Shared proportional and integral gain denominator. */
	uint32_t gain_den;
};

/** Mutable state and diagnostics of a PI precision clock discipline.
 *
 * The structure is exposed so that it can be embedded by value. Use
 * precision_pi_get_status(), precision_pi_get_config(), and the
 * precision_pi_set_*() helpers instead of accessing the members directly.
 */
struct precision_pi_discipline {
	/** Discipline configuration retained across resets. */
	struct precision_pi_config config;
	/** Current synchronization state. */
	enum precision_sync_state state;
	/** Accumulated integral correction in parts per billion. */
	int64_t drift_ppb;
	/** Most recently accepted source-minus-local offset in nanoseconds. */
	int64_t last_offset_ns;
	/** Current clamped rate correction in parts per billion. */
	int32_t frequency_correction_ppb;
	/** Number of rejected observations since the last reset. */
	uint32_t rejected_observations;
	/** Local timestamp of the last accepted observation. */
	precision_time_t last_update_ns;
	/** Current number of consecutive lock samples. */
	uint8_t lock_samples;
	/** Current number of consecutive locked-state outliers. */
	uint8_t outlier_samples;
	/** Whether the last-update timestamp is valid. */
	bool has_last_update;
};

/** Result returned by a precision discipline operation. */
struct precision_discipline_result {
	/** Selected clock control action. */
	enum precision_discipline_action action;
	/** Synchronization state after processing. */
	enum precision_sync_state state;
	/** Source-minus-local observation offset in nanoseconds. */
	precision_time_t offset_ns;
	/** Signed phase correction for @ref PRECISION_DISCIPLINE_STEP. */
	precision_time_t phase_correction_ns;
	/** Signed rate correction in parts per billion. */
	int32_t rate_ppb;
	/** Number of rejected observations since the last reset. */
	uint32_t rejected_observations;
};

/** Observable status of a PI precision clock discipline. */
struct precision_pi_status {
	/** Current synchronization state. */
	enum precision_sync_state state;
	/** Most recently accepted source-minus-local offset in nanoseconds. */
	precision_time_t last_offset_ns;
	/** Local timestamp of the last accepted observation. */
	precision_time_t last_update_ns;
	/** Current clamped rate correction in parts per billion. */
	int32_t frequency_correction_ppb;
	/** Number of rejected observations since the last reset. */
	uint32_t rejected_observations;
	/** Current number of consecutive lock samples. */
	uint8_t lock_samples;
	/** Current number of consecutive locked-state outliers. */
	uint8_t outlier_samples;
	/** Whether at least one observation has been accepted since the last reset. */
	bool has_observation;
};

/**
 * @brief Initialize a PI precision clock discipline.
 *
 * @param discipline Discipline instance to initialize.
 * @param config Configuration copied into the discipline.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument or configuration value is invalid.
 */
int precision_pi_init(struct precision_pi_discipline *discipline,
		      const struct precision_pi_config *config);

/**
 * @brief Reset a discipline to the unsynchronized state.
 *
 * The configured policy is preserved. A null @p discipline is ignored.
 *
 * @param discipline Discipline to reset.
 */
void precision_pi_reset(struct precision_pi_discipline *discipline);

/**
 * @brief Put a discipline into the sticky fault state.
 *
 * Clock control remains blocked until @ref precision_pi_reset is called. The last
 * observation diagnostics are preserved. A null @p discipline is ignored.
 *
 * @param discipline Discipline that encountered a clock-operation failure.
 */
void precision_pi_fault(struct precision_pi_discipline *discipline);

/**
 * @brief Process a source/local time observation with a PI discipline.
 *
 * @param discipline Initialized discipline instance.
 * @param observation Observation to process.
 * @param result Optional destination for the selected action and diagnostics.
 *
 * @retval 0 if the observation produced a discipline decision.
 * @retval -EINVAL if the observation, domains, or timestamp ordering is invalid.
 * @retval -ESTALE if the observation uncertainty exceeds the configured limit.
 * @retval -ERANGE if the source/local offset is not representable.
 * @retval -EIO if the discipline is faulted and requires explicit reset.
 */
int precision_pi_process(struct precision_pi_discipline *discipline,
			 const struct precision_time_observation *observation,
			 struct precision_discipline_result *result);

/**
 * @brief Update holdover state from the current local time.
 *
 * The result action is @ref PRECISION_DISCIPLINE_IGNORE while the source is in
 * holdover and @ref PRECISION_DISCIPLINE_RESET when the holdover interval expires.
 *
 * @param discipline Initialized discipline instance.
 * @param now_local_ns Current time in the configured local domain.
 * @param result Optional destination for state and diagnostics.
 *
 * @retval 0 if the source has not timed out.
 * @retval -EAGAIN if no observation or source timeout is available.
 * @retval -ESTALE if the source is in holdover or holdover expired.
 * @retval -ERANGE if source age cannot be represented.
 * @retval -EIO if the discipline is faulted and requires explicit reset.
 */
int precision_pi_check_source_timeout(struct precision_pi_discipline *discipline,
				      precision_time_t now_local_ns,
				      struct precision_discipline_result *result);

/**
 * @brief Get a copy of the configuration of a discipline.
 *
 * @param discipline Initialized discipline instance.
 * @param config Destination for the configuration.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument is null.
 */
int precision_pi_get_config(const struct precision_pi_discipline *discipline,
			    struct precision_pi_config *config);

/**
 * @brief Get the current state and diagnostics of a discipline.
 *
 * @param discipline Initialized discipline instance.
 * @param status Destination for the status.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument is null.
 */
int precision_pi_get_status(const struct precision_pi_discipline *discipline,
			    struct precision_pi_status *status);

/**
 * @brief Update the output rate limits of a discipline.
 *
 * The accumulated integral term is preserved. The limits take effect on the
 * next processed observation.
 *
 * @param discipline Initialized discipline instance.
 * @param min_rate_ppb Minimum output rate correction in parts per billion.
 * @param max_rate_ppb Maximum output rate correction in parts per billion.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p discipline is null or the limits are inconsistent.
 */
int precision_pi_set_rate_limits(struct precision_pi_discipline *discipline, int32_t min_rate_ppb,
				 int32_t max_rate_ppb);

/**
 * @brief Update the source timeout and holdover durations of a discipline.
 *
 * @param discipline Initialized discipline instance.
 * @param source_timeout_ns Maximum source age before holdover, or zero to disable.
 * @param holdover_ns Holdover duration before reset, or zero for indefinite holdover.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p discipline is null or a duration is negative.
 */
int precision_pi_set_source_timeout(struct precision_pi_discipline *discipline,
				    precision_time_t source_timeout_ns,
				    precision_time_t holdover_ns);

/**
 * @brief Change the local domain accepted by a discipline.
 *
 * The discipline is reset because previously accepted observations belong to
 * the old local domain.
 *
 * @param discipline Initialized discipline instance.
 * @param local_domain New local domain.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p discipline is null or @p local_domain is invalid.
 */
int precision_pi_set_local_domain(struct precision_pi_discipline *discipline,
				  struct precision_time_domain local_domain);

/**
 * @brief Get the time left before the source timeout state changes.
 *
 * Returns the delay after which precision_pi_check_source_timeout() would move
 * the discipline to holdover, or out of holdover when the discipline is already
 * stale. Callers use it to poll the source timeout on a deadline instead of on
 * every protocol event.
 *
 * @param discipline Initialized discipline instance.
 * @param now_local_ns Current local clock reading in nanoseconds.
 * @param[out] remaining_ns Delay until the next source timeout transition.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p discipline or @p remaining_ns is null.
 * @retval -EAGAIN if no observation was accepted yet, the source timeout is
 *	   disabled, or holdover already expired. Nothing has to be scheduled.
 * @retval -ERANGE if the local clock readings are not representable.
 */
int precision_pi_time_to_expiry(const struct precision_pi_discipline *discipline,
				precision_time_t now_local_ns, precision_time_t *remaining_ns);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_PI_H_ */
