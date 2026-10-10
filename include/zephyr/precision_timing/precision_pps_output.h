/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Autonomous one-pulse-per-second clock output service.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_PPS_OUTPUT_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_PPS_OUTPUT_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/precision_timing/precision_clock.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Precision PPS Output
 * @defgroup precision_pps_output Precision PPS Output
 * @since 4.5
 * @version 0.1.0
 * @ingroup precision_timing
 * @{
 */

/**
 * Events reported to a @ref precision_pps_output_callback_t by a single poll.
 *
 * More than one flag may be set at once, for example a hard step that is
 * immediately followed by a successful rearm.
 */
enum precision_pps_output_event {
	/** No event was produced by the poll. */
	PRECISION_PPS_OUTPUT_EVENT_NONE = 0,
	/** The output was armed for the first time on this instance. */
	PRECISION_PPS_OUTPUT_EVENT_ARMED = BIT(0),
	/** The output was rearmed after a prior loss of the waveform. */
	PRECISION_PPS_OUTPUT_EVENT_RECOVERED = BIT(1),
	/** A clock discontinuity beyond the configured threshold was detected. */
	PRECISION_PPS_OUTPUT_EVENT_HARD_STEP = BIT(2),
	/** The output channel was found unconfigured, mismatched, or inactive. */
	PRECISION_PPS_OUTPUT_EVENT_OUTPUT_INACTIVE = BIT(3),
	/** Reading the registered clock failed. */
	PRECISION_PPS_OUTPUT_EVENT_READ_ERROR = BIT(4),
	/** Reading the registered clock recovered after a prior failure. */
	PRECISION_PPS_OUTPUT_EVENT_READ_RECOVERED = BIT(5),
	/** Querying the output channel status failed. */
	PRECISION_PPS_OUTPUT_EVENT_STATUS_ERROR = BIT(6),
	/** Querying the output channel status recovered after a prior failure. */
	PRECISION_PPS_OUTPUT_EVENT_STATUS_RECOVERED = BIT(7),
	/** Stopping the output channel for a rearm failed. */
	PRECISION_PPS_OUTPUT_EVENT_STOP_ERROR = BIT(8),
	/** Starting the output channel during a rearm failed. */
	PRECISION_PPS_OUTPUT_EVENT_START_ERROR = BIT(9),
};

/**
 * @brief Runtime configuration of a @ref precision_pps_output instance.
 *
 * The configuration is copied into the instance by precision_pps_output_init()
 * and never mutated afterward. The waveform period is always one second, so it
 * is not configurable. The @ref pulse_width_ns field is consulted only when
 * @ref width_policy is @ref PRECISION_CLOCK_OUTPUT_WIDTH_EXACT.
 */
struct precision_pps_output_config {
	/** Zero-based scheduled-output channel armed by the instance. */
	uint32_t channel;
	/** High-pulse width policy for the one-second waveform. */
	enum precision_clock_output_width_policy width_policy;
	/**
	 * Exact active-high pulse width in nanoseconds. Used only when
	 * @ref width_policy is @ref PRECISION_CLOCK_OUTPUT_WIDTH_EXACT, where it
	 * must be positive and shorter than the one-second period.
	 */
	precision_time_t pulse_width_ns;
	/**
	 * Non-negative guard added to the provider's minimum lead time before
	 * aligning the first output edge to a whole-second boundary.
	 */
	precision_time_t start_guard_ns;
	/**
	 * Non-negative clock-discontinuity threshold in nanoseconds. When the
	 * absolute discrepancy between clock and monotonic elapsed time exceeds
	 * it, after allowing for both clock reads' sampling intervals, the clock
	 * is treated as having taken a hard step and the output is stopped and
	 * rearmed.
	 */
	precision_time_t step_limit_ns;
	/** Positive interval between polls of the clock and output channel. */
	uint32_t poll_interval_ms;
};

/**
 * @brief Initialize PPS configuration from subsystem Kconfig defaults.
 *
 * Callers may override individual fields before initializing the service.
 * Provider-default width uses zero as the unused exact-width value.
 */
#define PRECISION_PPS_OUTPUT_CONFIG_DEFAULTS                                                       \
	{                                                                                          \
		.channel = CONFIG_PRECISION_PPS_OUTPUT_CHANNEL,                                    \
		.width_policy = IS_ENABLED(CONFIG_PRECISION_PPS_OUTPUT_WIDTH_EXACT)                \
					? PRECISION_CLOCK_OUTPUT_WIDTH_EXACT                       \
					: PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT,           \
		.pulse_width_ns = IS_ENABLED(CONFIG_PRECISION_PPS_OUTPUT_WIDTH_EXACT)              \
					  ? CONFIG_PRECISION_PPS_OUTPUT_PULSE_WIDTH_NS             \
					  : 0,                                                     \
		.start_guard_ns = CONFIG_PRECISION_PPS_OUTPUT_START_GUARD_NS,                      \
		.step_limit_ns = CONFIG_PRECISION_PPS_OUTPUT_STEP_LIMIT_NS,                        \
		.poll_interval_ms = CONFIG_PRECISION_PPS_OUTPUT_POLL_INTERVAL_MS,                  \
	}

/** Coherent snapshot of an instance's autonomous PPS output state. */
struct precision_pps_output_state {
	/** Effective waveform configuration while @ref active is true. */
	struct precision_clock_output_waveform_config config;
	/** Registered clock's time at the most recent successful read. */
	precision_time_t phc_time_ns;
	/**
	 * Signed minimum discrepancy between clock and monotonic elapsed time
	 * after allowing for both clock reads' sampling intervals. Zero when
	 * the observations are consistent with a continuous clock.
	 */
	precision_time_t continuity_error_ns;
	/** Saturating count of successful arm and rearm operations. */
	uint32_t generation;
	/** Saturating count of rearms following a prior armed state. */
	uint32_t rearm_count;
	/** Most recent negative error code, or 0 while healthy. */
	int last_error;
	/** Whether the output channel is currently armed. */
	bool active;
	/** Whether a stop and rearm attempt is pending. */
	bool rearm_pending;
	/** Whether @ref phc_time_ns reflects a successful clock read. */
	bool phc_read_valid;
};

struct precision_pps_output;

/**
 * @brief Callback invoked when a poll produces a non-zero event mask.
 *
 * Invoked from the subsystem's dedicated workqueue thread with no instance
 * lock held. @p state points to a stack-local copy that is valid only for the
 * duration of the call; copy any fields the callback needs to retain. The
 * callback must not block for an extended time, since it delays subsequent
 * polls, and must not call precision_pps_output_stop() on @p pps itself, which
 * would deadlock; that call is rejected with @c -EDEADLK. Stopping a different
 * instance from the callback is allowed.
 *
 * @param pps Instance that produced the event.
 * @param events Combination of @ref precision_pps_output_event flags.
 * @param state Immutable snapshot of the instance state after the poll.
 * @param user_data Opaque pointer supplied to precision_pps_output_init().
 */
typedef void (*precision_pps_output_callback_t)(struct precision_pps_output *pps, uint32_t events,
						const struct precision_pps_output_state *state,
						void *user_data);

/**
 * @brief Caller-owned autonomous one-pulse-per-second output instance.
 *
 * The structure is exposed so that it can be embedded by value, for example as
 * a static or a member of a larger object. Its contents are managed entirely by
 * the precision_pps_output_*() API; treat every field as private and use
 * precision_pps_output_state_get() to observe the instance rather than reading
 * the members directly. The storage must remain valid and must not be moved or
 * copied for the lifetime of the instance, and the owning clock must remain
 * valid until precision_pps_output_stop() returns successfully.
 */
struct precision_pps_output {
	/** Delayable poll work scheduled on the subsystem workqueue. */
	struct k_work_delayable work;
	/** Per-instance lock protecting the state below. */
	struct k_mutex lock;
	/** Protocol-neutral clock that owns the output channel. */
	const struct precision_clock *precision_clk;
	/** Event callback. */
	precision_pps_output_callback_t callback;
	/** Opaque pointer passed back to @ref callback. */
	void *user_data;
	/** Immutable runtime configuration. */
	struct precision_pps_output_config config;
	/** Effective exact pulse width, or 0 for the provider-default policy. */
	precision_time_t effective_pulse_width_ns;
	/** Cached output-channel capabilities queried at start. */
	struct precision_clock_output_caps caps;
	/** Published state snapshot. */
	struct precision_pps_output_state state;
	/** Registered clock time at the previous successful read. */
	precision_time_t previous_phc_ns;
	/** Monotonic time immediately before the previous successful clock read. */
	precision_time_t previous_monotonic_ns;
	/** Monotonic time immediately after the previous successful clock read. */
	precision_time_t previous_monotonic_end_ns;
	/** Whether a previous read is available for continuity checking. */
	bool have_previous;
	/** Whether the instance has armed the output at least once. */
	bool ever_armed;
	/** Whether the next poll must stop and rearm the channel. */
	bool force_rearm;
	/** Whether the previous clock read failed. */
	bool read_failed;
	/** Whether the previous status query failed. */
	bool status_failed;
	/** Number of consecutive output-status query failures. */
	uint8_t consecutive_status_errors;
	/** Whether the instance is currently started and polling. */
	bool started;
	/** Whether a stop is in progress. */
	bool stopping;
};

/**
 * @brief Initialize a one-pulse-per-second output instance.
 *
 * Copies @p config into @p pps, validates it against the fixed one-second
 * period, and prepares the internal poll work and lock. The clock is not
 * accessed here; the compiled-in output capabilities are checked when the
 * instance is started. Call this once on fresh storage before any other
 * operation and never while the instance is started. Calling
 * precision_pps_output_start(), precision_pps_output_state_get(), or
 * precision_pps_output_stop() before this function succeeds is outside the API
 * contract because the instance contains kernel objects initialized here.
 *
 * @param pps Caller-owned instance storage to initialize.
 * @param precision_clk Protocol-neutral clock that owns the output channel.
 * @param config Runtime configuration copied into the instance.
 * @param callback Function invoked from the workqueue on each event.
 * @param user_data Opaque pointer passed back to @p callback.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument is null or the configuration is invalid, for
 *	   example an unknown width policy, an out-of-range exact pulse width, a
 *	   negative guard or step limit, or a zero poll interval.
 */
int precision_pps_output_init(struct precision_pps_output *pps,
			      const struct precision_clock *precision_clk,
			      const struct precision_pps_output_config *config,
			      precision_pps_output_callback_t callback, void *user_data);

/**
 * @brief Start autonomously arming and maintaining the output.
 *
 * Validates the configuration against the clock's reported output capabilities,
 * then schedules the first poll immediately on the subsystem's dedicated
 * workqueue. Arming does not wait for or inspect any synchronization protocol;
 * it proceeds as soon as the clock can be read and its output channel
 * controlled. Multiple instances have independent configuration, state, and
 * lifetime, but their polls, poll-time provider operations, and callbacks are
 * serialized on one shared dedicated workqueue. A blocking poll-time provider
 * operation or callback for one instance can therefore delay polling of other
 * instances. The capability query in start() and the provider stop in stop()
 * run on the calling thread and may overlap another instance's poll. Providers
 * must support concurrent operations on different channels of the same clock.
 *
 * Call this function from thread context on an initialized, stopped instance.
 * The caller must grant the instance exclusive management of its configured
 * clock and output-channel pair until precision_pps_output_stop() succeeds.
 * During that interval, no other PPS instance or direct scheduled-output API
 * caller may use the channel. A matching output that exists before the first
 * poll may be adopted only when its exclusive management has been transferred
 * to this instance.
 *
 * @param pps Initialized instance to start.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p pps is null.
 * @retval -EALREADY if the instance is already started.
 * @retval -ENOTSUP if the clock or its output channel lacks a required
 *	   capability, such as periodic waveforms or programmable width for the
 *	   exact width policy.
 * @retval -ERANGE if the fixed period or exact pulse width is outside the
 *	   channel's reported limits or resolution.
 * @return A provider-specific negative error code if the initial capability
 *	   query fails.
 */
int precision_pps_output_start(struct precision_pps_output *pps);

/**
 * @brief Copy the current autonomous PPS output state.
 *
 * Thread-safe; may be called concurrently with polling and with the instance
 * callback.
 *
 * @param pps Started instance to observe.
 * @param state Destination for the coherent state snapshot.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument is null or the instance is not started.
 */
int precision_pps_output_state_get(struct precision_pps_output *pps,
				   struct precision_pps_output_state *state);

/**
 * @brief Stop an autonomous PPS output instance.
 *
 * On success, synchronously cancels any pending or running poll, stops the
 * output channel, and marks the instance stopped so that no later callback
 * invocation or state access can occur. The owning clock may be released by the
 * caller only after this function returns 0. A stopped instance may be started
 * again without reinitialization.
 *
 * If stopping the output channel fails, the instance is left started and
 * polling resumes so that the caller cannot release the clock while the channel
 * might still be driven. Retry the call, for example after the next poll, or
 * investigate the provider error.
 *
 * Call this function from thread context, and never from the instance's own
 * @ref precision_pps_output_callback_t; a self-stop is rejected with
 * @c -EDEADLK. Stopping a different instance from a callback is allowed.
 *
 * @param pps Instance to stop.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p pps is null.
 * @retval -EALREADY if the instance is not started.
 * @retval -EBUSY if another stop call is already in progress.
 * @retval -EDEADLK if called from @p pps's own poll or callback context.
 * @return A provider-specific negative error code if stopping the output
 *	   channel fails; the instance remains started in that case.
 */
int precision_pps_output_stop(struct precision_pps_output *pps);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_PPS_OUTPUT_H_ */
