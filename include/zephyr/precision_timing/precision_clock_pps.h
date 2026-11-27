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

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_PPS_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_PPS_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/precision_timing/precision_clock_output.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Precision Clock PPS
 * @defgroup precision_clock_pps Precision Clock PPS
 * @since 4.6
 * @version 0.1.0
 * @ingroup precision_timing
 * @{
 */

/**
 * @brief Events reported by the autonomous PPS service.
 *
 * More than one flag may be set at once, for example a hard step that is
 * immediately followed by a successful rearm. These events do not report
 * physical pulse edges or their timestamps.
 */
enum precision_clock_pps_event {
	/** No event has been reported. */
	PRECISION_CLOCK_PPS_EVENT_NONE = 0,
	/** The output was armed or adopted for the first time since start. */
	PRECISION_CLOCK_PPS_EVENT_ARMED = BIT(0),
	/** The output was rearmed after previously being active. */
	PRECISION_CLOCK_PPS_EVENT_RECOVERED = BIT(1),
	/** A clock discontinuity beyond the configured threshold was detected. */
	PRECISION_CLOCK_PPS_EVENT_HARD_STEP = BIT(2),
	/** A previously active output was found unconfigured, mismatched, or inactive. */
	PRECISION_CLOCK_PPS_EVENT_OUTPUT_INACTIVE = BIT(3),
	/** Reading the registered clock failed. */
	PRECISION_CLOCK_PPS_EVENT_READ_ERROR = BIT(4),
	/** Reading the registered clock recovered after a prior failure. */
	PRECISION_CLOCK_PPS_EVENT_READ_RECOVERED = BIT(5),
	/** Querying the output channel status failed. */
	PRECISION_CLOCK_PPS_EVENT_STATUS_ERROR = BIT(6),
	/** Querying the output channel status recovered after a prior failure. */
	PRECISION_CLOCK_PPS_EVENT_STATUS_RECOVERED = BIT(7),
	/** The service failed to stop the channel before arming it. */
	PRECISION_CLOCK_PPS_EVENT_STOP_ERROR = BIT(8),
	/** The service failed to compute the next edge time or start the channel. */
	PRECISION_CLOCK_PPS_EVENT_START_ERROR = BIT(9),
};

struct precision_clock_pps;

/**
 * @brief Callback invoked when a PPS poll produces one or more events.
 *
 * The callback runs on the subsystem's dedicated workqueue without the instance
 * lock held. Use precision_clock_pps_state_get() to read the current state and
 * this notification's event mask, and @c pps->config.user_data to read the
 * caller's context. The mask remains stable throughout the callback, but the
 * phase can become @ref PRECISION_CLOCK_PPS_STOPPING if another thread stops the
 * instance. The callback must not block for an extended time, as it delays
 * subsequent polls on the shared workqueue.
 *
 * Calling precision_clock_pps_stop() on @p pps from this callback returns
 * @c -EDEADLK. Stopping a different instance is allowed.
 *
 * @param pps Instance that produced the notification.
 */
typedef void (*precision_clock_pps_callback_t)(struct precision_clock_pps *pps);

/**
 * @brief Configuration of a @ref precision_clock_pps instance.
 *
 * The configuration is copied into the instance by precision_clock_pps_init()
 * and never mutated afterward. The waveform period is always one second, so it
 * is not configurable. The @ref pulse_width_ns field is consulted only when
 * @ref width_policy is @ref PRECISION_CLOCK_OUTPUT_WIDTH_EXACT.
 *
 * Each instance supplies its own configuration. The Kconfig values used by
 * @ref PRECISION_CLOCK_PPS_CONFIG_DEFAULTS are optional initialization
 * defaults, not device-wide or driver configuration.
 */
struct precision_clock_pps_config {
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
	/** Positive interval between polls, representable as a finite kernel timeout. */
	uint32_t poll_interval_ms;
	/** Event callback, or NULL to accumulate events for polling instead. */
	precision_clock_pps_callback_t callback;
	/** Opaque caller context, available to the callback as @c pps->config.user_data. */
	void *user_data;
};

/**
 * @brief Initialize PPS configuration from subsystem Kconfig defaults.
 *
 * Callers may override individual fields before initializing the service.
 * Provider-default width uses zero as the unused exact-width value.
 */
#define PRECISION_CLOCK_PPS_CONFIG_DEFAULTS                                                        \
	{                                                                                          \
		.channel = CONFIG_PRECISION_CLOCK_PPS_CHANNEL,                                     \
		.width_policy = IS_ENABLED(CONFIG_PRECISION_CLOCK_PPS_WIDTH_EXACT)                 \
					? PRECISION_CLOCK_OUTPUT_WIDTH_EXACT                       \
					: PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT,           \
		.pulse_width_ns = IS_ENABLED(CONFIG_PRECISION_CLOCK_PPS_WIDTH_EXACT)               \
					  ? CONFIG_PRECISION_CLOCK_PPS_PULSE_WIDTH_NS              \
					  : 0,                                                     \
		.start_guard_ns = CONFIG_PRECISION_CLOCK_PPS_START_GUARD_NS,                       \
		.step_limit_ns = CONFIG_PRECISION_CLOCK_PPS_STEP_LIMIT_NS,                         \
		.poll_interval_ms = CONFIG_PRECISION_CLOCK_PPS_POLL_INTERVAL_MS,                   \
	}

/** Lifecycle phase of an autonomous PPS instance. */
enum precision_clock_pps_phase {
	/** The instance is initialized but not polling. */
	PRECISION_CLOCK_PPS_STOPPED = 0,
	/** Polling for initial status; a matching output may be adopted. */
	PRECISION_CLOCK_PPS_STARTING,
	/** An initial stop and arm is required. */
	PRECISION_CLOCK_PPS_ARMING,
	/** The matching output is armed or running. */
	PRECISION_CLOCK_PPS_ACTIVE,
	/** A stop and rearm is required after previously being active. */
	PRECISION_CLOCK_PPS_REARMING,
	/** A caller is synchronously stopping the service. */
	PRECISION_CLOCK_PPS_STOPPING,
};

/** Coherent snapshot of an instance's autonomous PPS output state. */
struct precision_clock_pps_state {
	/** Effective waveform configuration, valid only in @ref PRECISION_CLOCK_PPS_ACTIVE. */
	struct precision_clock_output_waveform_config config;
	/** Current lifecycle phase. */
	enum precision_clock_pps_phase phase;
	/** Combination of @ref precision_clock_pps_event flags. */
	uint32_t events;
};

/**
 * @brief Caller-owned autonomous one-pulse-per-second output instance.
 *
 * The structure is exposed so that it can be embedded by value, for example as
 * a static or a member of a larger object. Its contents are managed by the
 * precision_clock_pps_*() API. Use precision_clock_pps_state_get() to observe
 * the instance; only @c config.user_data may be read directly by the caller.
 * The storage must remain valid and must not be moved or copied. The clock must
 * remain valid from initialization until the instance is no longer used, including
 * between stopping and restarting it. When @kconfig{CONFIG_KERNEL_COHERENCE} is
 * enabled, the storage must also reside in coherent memory: the instance embeds
 * a @c struct k_work_sync used by stop(), which generally cannot be on the stack.
 * See sys_cache_is_mem_coherent().
 */
struct precision_clock_pps {
	/** Delayable poll work scheduled on the subsystem workqueue. */
	struct k_work_delayable work;
	/** Sync object for synchronous poll cancellation during stop. */
	struct k_work_sync work_sync;
	/** Per-instance lock protecting the state below. */
	struct k_mutex lock;
	/** Protocol-neutral clock that owns the output channel. */
	const struct precision_clock *precision_clk;
	/** Immutable configuration copied from the caller. */
	struct precision_clock_pps_config config;
	/** Provider's minimum lead time plus the instance's start guard. */
	precision_time_t start_lead_time_ns;
	/** Published state snapshot. */
	struct precision_clock_pps_state state;
	/** Registered clock time at the most recent successful service read. */
	precision_time_t clock_phc_ns;
	/** Monotonic time immediately before that clock read. */
	precision_time_t clock_mono_pre_ns;
	/** Monotonic time immediately after that clock read. */
	precision_time_t clock_mono_post_ns;
	/** Whether a successful clock sample is available for continuity checking. */
	bool clock_read_done;
	/** Whether the most recent clock read failed. */
	bool read_failed;
	/** Consecutive status errors, saturated at the threshold until status recovers. */
	uint8_t consecutive_status_errors;
};

/**
 * @brief Initialize a one-pulse-per-second output instance.
 *
 * Copies @p config into @p pps, validates it against the configured clock
 * output channel's capabilities, and prepares the internal poll work and lock.
 * The clock time is not read here, and the output is not changed.
 *
 * Call from thread context, before using the instance in any other PPS API.
 * Do not reinitialize an instance while it is in use. Using the instance
 * before this function succeeds is outside the API contract.
 *
 * @param pps Caller-owned instance storage to initialize.
 * @param precision_clk Protocol-neutral clock that owns the output channel.
 * @param config Runtime configuration copied into the instance.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument is null or the configuration is invalid, for
 *	   example an unknown width policy, an out-of-range exact pulse width, a
 *	   negative guard or step limit, a zero poll interval, or invalid provider
 *	   resolution or minimum lead time.
 * @retval -ENOTSUP if the requested channel is unavailable or lacks support for
 *	   periodic waveforms or the requested width policy.
 * @retval -ERANGE if the poll interval is not a finite kernel timeout, the
 *	   period or exact width is outside the channel's limits or resolution,
 *	   or adding the start guard to the minimum lead time overflows.
 * @return Another negative error code from querying the clock capabilities.
 */
int precision_clock_pps_init(struct precision_clock_pps *pps,
			     const struct precision_clock *precision_clk,
			     const struct precision_clock_pps_config *config);

/**
 * @brief Start autonomously arming and maintaining the output.
 *
 * Schedules the first poll immediately on the subsystem's dedicated workqueue.
 * Arming does not wait for or inspect any synchronization protocol;
 * it proceeds as soon as the clock can be read and its output channel
 * controlled. Multiple instances have independent configuration, state, and
 * lifetime, but their polls, poll-time provider operations, and callbacks are
 * serialized on one shared dedicated workqueue. A blocking poll-time provider
 * operation or callback for one instance can therefore delay polling of other
 * instances. The capability query in init() and the provider stop in stop()
 * run on the calling thread and may overlap another instance's poll. Providers
 * must support concurrent operations on different channels of the same clock.
 *
 * Call this function from thread context on an initialized, stopped instance.
 * The caller must grant the instance exclusive management of its configured
 * clock and output-channel pair until precision_clock_pps_stop() succeeds.
 * During that interval, no other PPS instance or direct scheduled-output API
 * caller may use the channel. A matching output that exists before the first
 * poll may be adopted only when its exclusive management has been transferred
 * to this instance.
 *
 * @param pps Initialized instance to start.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p pps is null.
 * @retval -EALREADY if the instance is not stopped.
 * @return Another negative error code if scheduling the first poll fails.
 */
int precision_clock_pps_start(struct precision_clock_pps *pps);

/**
 * @brief Copy the current autonomous PPS output state.
 *
 * When a callback is configured, the snapshot contains the most recent
 * notification mask without clearing it. Otherwise the snapshot contains the
 * accumulated events since the previous call, and copying them clears the
 * accumulated mask. A successful start clears any previously reported events
 * in either mode.
 *
 * Call from thread context, not from an ISR. The snapshot may be taken while
 * stopped, concurrently with polling, and from the instance callback.
 *
 * @param pps Initialized instance to observe.
 * @param state Destination for the coherent state snapshot.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument is null.
 */
int precision_clock_pps_state_get(struct precision_clock_pps *pps,
				  struct precision_clock_pps_state *state);

/**
 * @brief Stop an autonomous PPS output instance.
 *
 * On success, synchronously cancels any pending or running poll, stops the
 * output channel, and marks the instance stopped. Cancellation waits for a
 * running poll and its callback to finish before stopping the output, so
 * neither will use the clock or callback context after a successful stop
 * unless the instance is started again. A stopped instance may be restarted
 * without reinitialization if its clock and callback context remain valid.
 *
 * If the provider stop fails, the instance returns to its prior phase and
 * polling resumes. Do not release the clock or callback context in that case.
 *
 * Call this function from thread context, and never from the instance's own
 * @ref precision_clock_pps_callback_t; a self-stop is rejected with
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
int precision_clock_pps_stop(struct precision_clock_pps *pps);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_PPS_H_ */
