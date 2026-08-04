/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Experimental protocol-neutral precision timing APIs.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_TIMING_PRECISION_TIMING_H_
#define ZEPHYR_INCLUDE_ZEPHYR_TIMING_PRECISION_TIMING_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Precision Timing APIs
 * @defgroup precision_timing Precision Timing
 * @since 4.5
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

/** Signed precision time value expressed in nanoseconds. */
typedef int64_t precision_time_t;

/** Maximum representable precision time value. */
#define PRECISION_TIME_MAX INT64_MAX
/** Minimum representable precision time value. */
#define PRECISION_TIME_MIN INT64_MIN

/** Semantic type of a precision time domain. */
enum precision_time_domain_type {
	/** Invalid or unspecified domain. */
	PRECISION_TIME_DOMAIN_INVALID = 0,
	/** International Atomic Time domain. */
	PRECISION_TIME_DOMAIN_TAI,
	/** Coordinated Universal Time domain. */
	PRECISION_TIME_DOMAIN_UTC,
	/** Monotonic system time domain. */
	PRECISION_TIME_DOMAIN_MONOTONIC,
	/** Precision hardware clock domain. */
	PRECISION_TIME_DOMAIN_PHC,
	/** IEEE 1588 Precision Time Protocol domain. */
	PRECISION_TIME_DOMAIN_PTP,
	/** IEEE 802.1AS generalized Precision Time Protocol domain. */
	PRECISION_TIME_DOMAIN_GPTP,
	/** Uninterpreted counter domain. */
	PRECISION_TIME_DOMAIN_RAW,
};

/** Identity of a clock or time scale. */
struct precision_time_domain {
	/** Semantic domain type. */
	enum precision_time_domain_type type;
	/** Protocol- or implementation-defined domain instance identifier. */
	uint32_t id;
};

/** Timestamp qualified by its clock domain. */
struct precision_time_point {
	/** Time value in nanoseconds. */
	precision_time_t time;
	/** Domain in which the time value is expressed. */
	struct precision_time_domain domain;
};

/** Validity flags for a precision time observation. */
enum precision_observation_flags {
	/** Source time point is valid. */
	PRECISION_OBSERVATION_SOURCE_VALID = BIT(0),
	/** Local time point is valid. */
	PRECISION_OBSERVATION_LOCAL_VALID = BIT(1),
};

/** Simultaneous observation of source and local clock domains. */
struct precision_time_observation {
	/** Observed source time. */
	struct precision_time_point source;
	/** Corresponding local time. */
	struct precision_time_point local;
	/** Estimated observation uncertainty in nanoseconds. */
	precision_time_t uncertainty_ns;
	/** Combination of @ref precision_observation_flags. */
	uint32_t flags;
};

/** Affine mapping between a source domain and a local domain. */
struct precision_time_mapping {
	/** Domain used as the mapping reference. */
	struct precision_time_domain source_domain;
	/** Domain used as the mapping local clock. */
	struct precision_time_domain local_domain;
	/** Underlying affine synchronization state. */
	struct timeutil_sync_state state;
	/** Bias used to represent signed source values in @ref timeutil_sync_state. */
	precision_time_t source_bias;
	/** Bias used to represent signed local values in @ref timeutil_sync_state. */
	precision_time_t local_bias;
	/** Whether the mapping has at least one valid observation. */
	bool valid;
};

/** Precision clock capability flags. */
enum precision_clock_caps_flags {
	/** Clock can be read. */
	PRECISION_CLOCK_CAP_READ = BIT(0),
	/** Clock can be set to an absolute time. */
	PRECISION_CLOCK_CAP_SET = BIT(1),
	/** Clock supports phase adjustment. */
	PRECISION_CLOCK_CAP_ADJUST_PHASE = BIT(2),
	/** Clock supports rate adjustment. */
	PRECISION_CLOCK_CAP_ADJUST_RATE = BIT(3),
};

/** Capabilities and adjustment limits of a precision clock. */
struct precision_clock_caps {
	/** Combination of @ref precision_clock_caps_flags. */
	uint32_t flags;
	/** Smallest representable clock increment in nanoseconds. */
	precision_time_t resolution_ns;
	/** Largest supported absolute phase adjustment in nanoseconds. */
	precision_time_t max_phase_adjust_ns;
	/** Minimum supported rate adjustment in parts per billion. */
	int32_t min_rate_ppb;
	/** Maximum supported rate adjustment in parts per billion. */
	int32_t max_rate_ppb;
};

struct precision_clock;

/** Operations implemented by a precision clock provider. */
struct precision_clock_api {
	/** Read the current clock value. */
	int (*read)(const struct precision_clock *precision_clk,
		    struct precision_time_point *tp);
	/** Set the current clock value. */
	int (*set)(const struct precision_clock *precision_clk,
		   const struct precision_time_point *tp);
	/** Apply a signed phase adjustment in nanoseconds. */
	int (*adjust_phase)(const struct precision_clock *precision_clk, precision_time_t phase_ns);
	/** Apply a signed rate adjustment in parts per billion. */
	int (*adjust_rate)(const struct precision_clock *precision_clk, int32_t rate_ppb);
	/** Query clock capabilities and limits. */
	int (*get_caps)(const struct precision_clock *precision_clk,
			struct precision_clock_caps *caps);
};

/** Protocol-neutral precision clock instance. */
struct precision_clock {
	/** Provider operations. */
	const struct precision_clock_api *api;
	/** Provider-specific context passed to operations. */
	const void *user_data;
	/** Domain produced and consumed by this clock. */
	struct precision_time_domain domain;
};

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

/** Monotonic deadline used to rate-limit precision timing housekeeping. */
struct precision_deadline {
	/** Uptime in milliseconds at which the deadline expires. */
	int64_t expiry_ms;
	/** Whether a deadline is currently scheduled. */
	bool scheduled;
};

/**
 * @brief Compare two precision time domain identities.
 *
 * @param a First domain.
 * @param b Second domain.
 *
 * @retval true if both domains are non-null and equal.
 * @retval false otherwise.
 */
static inline bool precision_time_domain_equal(const struct precision_time_domain *a,
					       const struct precision_time_domain *b)
{
	return a != NULL && b != NULL && a->type == b->type && a->id == b->id;
}

/**
 * @brief Add two signed precision time values with overflow checking.
 *
 * @param a First operand in nanoseconds.
 * @param b Second operand in nanoseconds.
 * @param result Destination for the sum.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p result is null.
 * @retval -ERANGE if the sum is not representable.
 */
int precision_time_add(precision_time_t a, precision_time_t b, precision_time_t *result);

/**
 * @brief Subtract two signed precision time values with overflow checking.
 *
 * @param a Minuend in nanoseconds.
 * @param b Subtrahend in nanoseconds.
 * @param result Destination for the difference.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p result is null.
 * @retval -ERANGE if the difference is not representable.
 */
int precision_time_sub(precision_time_t a, precision_time_t b, precision_time_t *result);

/**
 * @brief Convert an unsigned seconds and nanoseconds pair to precision time.
 *
 * @param sec Whole seconds.
 * @param nsec Nanosecond fraction in the range 0 through 999999999.
 * @param result Destination for the converted value.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p result is null.
 * @retval -ERANGE if the input is invalid or not representable.
 */
int precision_time_from_u64_sec_nsec(uint64_t sec, uint32_t nsec, precision_time_t *result);

/**
 * @brief Convert non-negative precision time to seconds and nanoseconds.
 *
 * @param time Precision time value to convert.
 * @param sec Destination for whole seconds.
 * @param nsec Destination for the nanosecond fraction.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an output pointer is null.
 * @retval -ERANGE if @p time is negative.
 */
int precision_time_to_u64_sec_nsec(precision_time_t time, uint64_t *sec, uint32_t *nsec);

/**
 * @brief Initialize an empty precision time mapping.
 *
 * A null @p mapping is ignored.
 *
 * @param mapping Mapping to initialize.
 * @param source_domain Source domain accepted by the mapping.
 * @param local_domain Local domain produced by the mapping.
 */
void precision_time_mapping_init(struct precision_time_mapping *mapping,
				 struct precision_time_domain source_domain,
				 struct precision_time_domain local_domain);

/**
 * @brief Invalidate a precision time mapping while preserving its domains.
 *
 * A null @p mapping is ignored.
 *
 * @param mapping Mapping to invalidate.
 */
void precision_time_mapping_invalidate(struct precision_time_mapping *mapping);

/**
 * @brief Update a precision time mapping from a simultaneous observation.
 *
 * @param mapping Mapping to update.
 * @param observation Valid source and local observation.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument, validity flag, domain, or update is invalid.
 * @retval -ERANGE if an observation cannot be represented by the mapping.
 */
int precision_time_mapping_update(struct precision_time_mapping *mapping,
				  const struct precision_time_observation *observation);

/**
 * @brief Convert a source-domain time point to the local domain.
 *
 * @param mapping Valid mapping to use.
 * @param source Source-domain time point.
 * @param local Destination for the converted local-domain time point.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument or source domain is invalid.
 * @retval -EAGAIN if the mapping is not valid yet.
 * @retval -ERANGE if the converted value is not representable.
 */
int precision_time_mapping_source_to_local(const struct precision_time_mapping *mapping,
					   const struct precision_time_point *source,
					   struct precision_time_point *local);

/**
 * @brief Convert a local-domain time point to the source domain.
 *
 * @param mapping Valid mapping to use.
 * @param local Local-domain time point.
 * @param source Destination for the converted source-domain time point.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument or local domain is invalid.
 * @retval -EAGAIN if the mapping is not valid yet.
 * @retval -ERANGE if the converted value is not representable.
 */
int precision_time_mapping_local_to_source(const struct precision_time_mapping *mapping,
					   const struct precision_time_point *local,
					   struct precision_time_point *source);

/**
 * @brief Read a precision clock.
 *
 * @param precision_clk Clock to read.
 * @param tp Destination for the clock value and domain.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument or clock API is invalid.
 * @retval -ENOTSUP if the clock does not support reading.
 * @return A provider-specific negative error code on failure.
 */
int precision_clock_read(const struct precision_clock *precision_clk,
			 struct precision_time_point *tp);

/**
 * @brief Set a precision clock to an absolute time.
 *
 * @param precision_clk Clock to set.
 * @param tp New clock value in the clock's domain.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument, API, or time domain is invalid.
 * @retval -ENOTSUP if the clock does not support setting.
 * @return A provider-specific negative error code on failure.
 */
int precision_clock_set(const struct precision_clock *precision_clk,
			const struct precision_time_point *tp);

/**
 * @brief Apply a phase adjustment to a precision clock.
 *
 * @param precision_clk Clock to adjust.
 * @param phase_ns Signed phase adjustment in nanoseconds.
 *
 * @retval 0 on success.
 * @retval -EINVAL if the clock or API is invalid.
 * @retval -ENOTSUP if phase adjustment is unsupported.
 * @retval -ERANGE if @p phase_ns exceeds provider limits.
 * @return A provider-specific negative error code on failure.
 */
int precision_clock_adjust_phase(const struct precision_clock *precision_clk,
				 precision_time_t phase_ns);

/**
 * @brief Apply a rate adjustment to a precision clock.
 *
 * @param precision_clk Clock to adjust.
 * @param rate_ppb Signed rate adjustment in parts per billion.
 *
 * @retval 0 on success.
 * @retval -EINVAL if the clock or API is invalid.
 * @retval -ENOTSUP if rate adjustment is unsupported.
 * @retval -ERANGE if @p rate_ppb exceeds provider limits.
 * @return A provider-specific negative error code on failure.
 */
int precision_clock_adjust_rate(const struct precision_clock *precision_clk, int32_t rate_ppb);

/**
 * @brief Query precision clock capabilities and limits.
 *
 * @param precision_clk Clock to query.
 * @param caps Destination for capabilities and limits.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument or clock API is invalid.
 * @retval -ENOTSUP if capability reporting is unsupported.
 * @return A provider-specific negative error code on failure.
 */
int precision_clock_get_caps(const struct precision_clock *precision_clk,
			     struct precision_clock_caps *caps);

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

/**
 * @brief Cancel a scheduled precision deadline.
 *
 * A null @p deadline is ignored.
 *
 * @param deadline Deadline to cancel.
 */
void precision_deadline_cancel(struct precision_deadline *deadline);

/**
 * @brief Schedule a precision deadline relative to the current uptime.
 *
 * A non-positive @p delay_ns cancels the deadline. The delay is rounded up to
 * the next whole millisecond so that the deadline never expires early.
 *
 * @param deadline Deadline to schedule.
 * @param delay_ns Delay from now in nanoseconds.
 */
void precision_deadline_schedule(struct precision_deadline *deadline, precision_time_t delay_ns);

/**
 * @brief Check whether a scheduled precision deadline expired.
 *
 * An expired deadline is cancelled, so a single expiry is reported once.
 *
 * @param deadline Deadline to check.
 *
 * @retval true if the deadline was scheduled and has expired.
 * @retval false otherwise.
 */
bool precision_deadline_due(struct precision_deadline *deadline);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_TIMING_PRECISION_TIMING_H_ */
