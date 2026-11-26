/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Protocol-neutral scheduled precision clock outputs.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_OUTPUT_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_OUTPUT_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/precision_timing/precision_clock.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Scheduled precision clock outputs
 * @defgroup precision_clock_output Precision Clock Output
 * @since 4.6
 * @version 0.1.0
 * @ingroup precision_timing
 * @{
 */

/** Precision clock output high-pulse width policy for periodic waveforms. */
enum precision_clock_output_width_policy {
	/** Use the provider's native default high-pulse width. */
	PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT = 0,
	/** Use the exact high-pulse width supplied in the configuration. */
	PRECISION_CLOCK_OUTPUT_WIDTH_EXACT = 1,
};

/** Kind of scheduled output configured on a precision clock output channel. */
enum precision_clock_output_kind {
	/** Periodic waveform. */
	PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM = 1,
};

/** Precision clock output capability flags. */
enum precision_clock_output_caps_flags {
	/** Channel can generate periodic output waveforms. */
	PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM = BIT(0),
	/** Channel supports an exact programmable high-pulse width. */
	PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH = BIT(1),
	/** Channel can observe whether its hardware output generator is armed or running. */
	PRECISION_CLOCK_OUTPUT_CAP_HARDWARE_ACTIVE = BIT(2),
};

/**
 * @brief Capabilities and limits of a precision clock output channel.
 *
 * The period limits are meaningful only when @ref PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM
 * is advertised. The pulse-width limits are meaningful only when
 * @ref PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH is advertised. Providers
 * must report a positive resolution, non-negative lead time, and positive,
 * ordered limits for the advertised capabilities.
 */
struct precision_clock_output_channel_caps {
	/** Combination of @ref precision_clock_output_caps_flags. */
	uint32_t flags;
	/** Smallest representable output time increment in nanoseconds. */
	precision_time_t resolution_ns;
	/** Minimum required delay from the current time to the first edge. */
	precision_time_t min_lead_time_ns;
	/** Minimum supported waveform period in nanoseconds. */
	precision_time_t min_period_ns;
	/** Maximum supported waveform period in nanoseconds. */
	precision_time_t max_period_ns;
	/** Minimum supported programmable high-pulse width in nanoseconds. */
	precision_time_t min_pulse_width_ns;
	/** Maximum supported programmable high-pulse width in nanoseconds. */
	precision_time_t max_pulse_width_ns;
};

/**
 * @brief Capabilities of all scheduled-output channels on a precision clock.
 *
 * The provider owns the channel table. Its address and contents remain valid
 * and unchanged for the lifetime of the initialized clock. Callers must not
 * modify or free it. Channel indices are provider-defined and zero-based.
 */
struct precision_clock_output_caps {
	/** Number of entries in @ref channels; zero when no channels are exposed. */
	uint32_t channel_count;
	/** Read-only channel table; may be null only when @ref channel_count is zero. */
	const struct precision_clock_output_channel_caps *channels;
};

/** Configuration of a periodic clock output waveform. */
struct precision_clock_output_waveform_config {
	/** Absolute time of the first rising edge in the owning clock's timescale. */
	precision_time_t first_rising_time;
	/** Interval between rising edges in nanoseconds. */
	precision_time_t period_ns;
	/** High-pulse width policy applied to each period. */
	enum precision_clock_output_width_policy width_policy;
	/**
	 * High-pulse width in nanoseconds. Used only when @ref width_policy is
	 * @ref PRECISION_CLOCK_OUTPUT_WIDTH_EXACT.
	 */
	precision_time_t pulse_width_ns;
};

/** Requested or accepted configuration of a scheduled clock output channel. */
struct precision_clock_output_config {
	/** Kind of output selected by the configuration. */
	enum precision_clock_output_kind kind;
	/** Parameters for the selected output kind. */
	union {
		/** Valid when @ref kind is @ref PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM. */
		struct precision_clock_output_waveform_config waveform;
	};
};

/** Observable status of a precision clock output channel. */
struct precision_clock_output_status {
	/** Whether the provider currently holds a configuration for the channel. */
	bool configured;
	/** Accepted configuration. Valid only when @ref configured is true. */
	struct precision_clock_output_config config;
	/**
	 * Whether @ref hardware_active reflects an observed hardware generator state.
	 * False when that state cannot be observed; configuration remains the
	 * source of truth and @ref hardware_active must be ignored.
	 */
	bool hardware_active_valid;
	/**
	 * Whether the hardware output generator is armed or running for the
	 * accepted configuration. Meaningful only when @ref hardware_active_valid
	 * is true. An armed waveform is active even before its first scheduled
	 * edge and during the low portion of each period. This is not the
	 * instantaneous pin level or an indication that an edge is occurring.
	 */
	bool hardware_active;
};

/**
 * @brief Query capabilities of all scheduled-output channels.
 *
 * The returned channel table is provider-owned and remains valid and unchanged
 * for the lifetime of the initialized clock. No caller allocation is required.
 * The provider is responsible for returning valid capabilities.
 *
 * Call from thread context. The provider operation may block.
 *
 * @param precision_clk Clock whose outputs are queried.
 * @param caps Destination for the channel count and read-only channel table.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument or clock API is invalid.
 * @retval -ENOTSUP if scheduled output is unsupported.
 * @return A provider-specific negative error code on failure.
 */
int precision_clock_output_get_caps(const struct precision_clock *precision_clk,
				    struct precision_clock_output_caps *caps);

/**
 * @brief Calculate the next aligned clock-output start time.
 *
 * Finds the earliest integer multiple of @p period_ns at or after the sum of
 * @p now and @p min_lead_time_ns. Negative clock values are supported.
 *
 * @param now Current time of the output clock.
 * @param period_ns Positive output period in nanoseconds.
 * @param min_lead_time_ns Non-negative time required before the first edge.
 * @param start_time Destination for the aligned start time.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument, period, or lead time is invalid.
 * @retval -ERANGE if calculating the aligned start time overflows.
 */
int precision_clock_output_next_start_time(precision_time_t now, precision_time_t period_ns,
					   precision_time_t min_lead_time_ns,
					   precision_time_t *start_time);

/**
 * @brief Start a scheduled clock output.
 *
 * Set @p config kind to @ref PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM to generate
 * a periodic waveform described by @p config waveform. The first rising edge
 * occurs at first_rising_time and subsequent edges are separated by period_ns.
 * The channel must advertise @ref PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM. When
 * width_policy is @ref PRECISION_CLOCK_OUTPUT_WIDTH_EXACT the channel must also
 * advertise @ref PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH and the pulse
 * width must be positive and shorter than the period; otherwise the provider
 * default width is used and pulse_width_ns is ignored. Configuration values
 * must be exactly representable at the channel resolution. Starting an
 * already configured channel is rejected; stop the channel before
 * reconfiguring it.
 *
 * Only a return of 0 or @c -EINPROGRESS gives the caller ownership of the
 * channel. @c -EINPROGRESS means programming failed and the provider retained
 * the configuration because it could not safely release the channel. In
 * that case, the caller must stop the output successfully before retrying or
 * releasing the underlying device. No other failure gives the caller
 * ownership or authorizes it to stop the channel.
 *
 * Call from thread context. The provider operation may block.
 *
 * @param precision_clk Clock that owns the output channel.
 * @param channel Provider-defined, zero-based output channel index.
 * @param config Output kind and its configuration.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument, output kind, width policy, or interval relationship is invalid,
 *                 or the provider reports a non-positive resolution for the selected channel.
 * @retval -ENOTSUP if periodic waveforms, programmable width, or @p channel is unsupported.
 * @retval -ERANGE if a value is outside channel limits or not representable.
 * @retval -ETIME if the first rising edge does not meet the minimum lead time;
 *                the caller may retry with a new start time without stopping the channel.
 * @retval -EBUSY if the channel is already configured for output, is in use by
 *                another device function, or is reserved by the driver.
 * @retval -EINPROGRESS if programming failed and this attempt retained the channel.
 * @retval -EIO if a capability query or clock read failed with @c -ETIME or
 *              @c -EINPROGRESS, which have distinct meanings for start.
 * @return A provider-specific negative error code on failure.
 */
int precision_clock_output_start(const struct precision_clock *precision_clk, uint32_t channel,
				 const struct precision_clock_output_config *config);

/**
 * @brief Stop a scheduled clock output.
 *
 * Stopping a channel that is already unconfigured is idempotent.
 * The operation is delegated directly to the provider so a capability-query
 * failure cannot prevent an active output from being stopped.
 *
 * Call from thread context. The provider operation may block.
 *
 * @param precision_clk Clock that owns the output channel.
 * @param channel Provider-defined, zero-based output channel index.
 *
 * @retval 0 on success or if the channel is already unconfigured.
 * @retval -EINVAL if the clock or clock API is invalid.
 * @retval -ENOTSUP if scheduled output or @p channel is unsupported.
 * @return A provider-specific negative error code on failure.
 */
int precision_clock_output_stop(const struct precision_clock *precision_clk, uint32_t channel);

/**
 * @brief Query scheduled clock output status.
 *
 * Reports whether the provider holds a configuration for the channel, the
 * configured kind and accepted configuration, and the physical output state
 * when observable. The provider validates @p channel and returns a consistent
 * status. This operation does not query capabilities or revalidate the accepted
 * configuration against the current time.
 *
 * Call from thread context. The provider operation may block.
 *
 * @param precision_clk Clock that owns the output channel.
 * @param channel Provider-defined, zero-based output channel index.
 * @param status Destination for the configuration and hardware-active state.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument or clock API is invalid.
 * @retval -ENOTSUP if status reporting or @p channel is unsupported.
 * @return A provider-specific negative error code on failure.
 */
int precision_clock_output_get_status(const struct precision_clock *precision_clk, uint32_t channel,
				      struct precision_clock_output_status *status);

/** @} */

/**
 * @brief Precision clock output provider extension
 * @defgroup precision_clock_output_provider Precision clock output provider
 * @since 4.6
 * @version 0.1.0
 * @ingroup precision_clock_output
 * @{
 *
 * Driver-facing contract that lets a device expose scheduled clock outputs to
 * a protocol-neutral precision clock adapter. All times are raw values in the
 * owning clock's timescale, so the extension carries no protocol-specific
 * prefix. An adapter such as the PTP clock adapter bridges it onto the public
 * API in @ref precision_clock_output.
 *
 * Providers share the public capability, configuration, and status types from
 * @ref precision_clock_output. No timescale conversion is needed at this boundary.
 * The configuration kind selects the output operation; periodic waveforms use
 * the limits and width policies supported by the channel.
 *
 * Channel indices are provider-defined and zero-based. A channel need not be
 * dedicated to output: it may share hardware with another device function,
 * such as input capture, or be reserved by the driver.
 *
 * Hardware-active status reports whether the generator is armed or running
 * for the accepted configuration, including while waiting for a future first
 * edge. Providers must not report the pin level or absence of an edge as an
 * inactive generator. If this state cannot be observed, leave
 * precision_clock_output_status.hardware_active_valid false rather than
 * inferring activity from the stored configuration.
 *
 * Provider operations are called only from thread context and may block.
 * Operations on different channels of the same device may run concurrently,
 * including during a device-wide capability query. The driver must protect
 * shared registers and state against concurrent access. A provider may
 * document additional, more restrictive context requirements.
 */

struct device;

/**
 * @brief Query capabilities of all output channels.
 *
 * The provider must return valid capabilities and a read-only channel table
 * whose address and contents remain unchanged for the initialized device's
 * lifetime. See @ref precision_clock_output_caps.
 *
 * @param dev Device that owns the output channels.
 * @param caps Destination for the channel count and read-only channel table.
 *
 * @return 0 on success or a negative error code on failure.
 */
typedef int (*precision_clock_output_get_caps_t)(const struct device *dev,
						 struct precision_clock_output_caps *caps);

/**
 * @brief Start a scheduled output in the device clock's timescale.
 *
 * The provider must validate hardware-specific constraints and arbitrate
 * channel ownership before programming the output. Generic capability checks
 * do not replace this validation: the clock can advance or be stepped between
 * a subsystem check and the provider operation. In particular, the provider
 * must check that the first edge still meets its programming lead time.
 *
 * If programming fails and the provider cannot safely release the channel,
 * it must retain the attempted configuration, report it as configured, and
 * return @c -EINPROGRESS. This is a failure requiring a successful stop, not
 * asynchronous completion. All other errors leave the caller without
 * ownership and must not alter another user's configuration.
 *
 * @param dev Device that owns the output channel.
 * @param channel Zero-based output channel index.
 * @param config Output kind and its configuration.
 *
 * @retval 0 on success.
 * @retval -EBUSY if the channel is already configured for output, is in use by
 *                another device function, or is reserved by the driver.
 * @retval -ETIME if the first edge no longer meets the programming lead time
 *                and this attempt did not retain the channel.
 * @retval -EINPROGRESS if programming failed and this attempt retained the channel.
 * @return A provider-specific negative error code on failure.
 */
typedef int (*precision_clock_output_start_t)(const struct device *dev, uint32_t channel,
					      const struct precision_clock_output_config *config);

/**
 * @brief Stop a scheduled output channel.
 *
 * Stopping a channel that is already unconfigured is idempotent.
 * The provider must validate the channel index.
 *
 * @param dev Device that owns the output channel.
 * @param channel Zero-based output channel index.
 *
 * @return 0 on success or a negative error code on failure.
 */
typedef int (*precision_clock_output_stop_t)(const struct device *dev, uint32_t channel);

/**
 * @brief Query output channel status.
 *
 * The provider must validate the channel index and return a status consistent
 * with its capabilities. The accepted configuration describes the original
 * request; its first rising edge may already be in the past.
 *
 * @param dev Device that owns the output channel.
 * @param channel Zero-based output channel index.
 * @param status Destination for the channel status.
 *
 * @return 0 on success or a negative error code on failure.
 */
typedef int (*precision_clock_output_get_status_t)(const struct device *dev, uint32_t channel,
						   struct precision_clock_output_status *status);

/**
 * @brief Scheduled clock output provider extension.
 *
 * A device advertises scheduled output support by referencing an instance of
 * this structure from its driver API. The @ref get_caps, @ref start,
 * @ref stop, and @ref get_status callbacks are required. Protocol adapters
 * dispatch operations independently and return @c -ENOTSUP for missing callbacks.
 */
struct precision_clock_output_provider {
	/** Query capabilities of all output channels. */
	precision_clock_output_get_caps_t get_caps;
	/** Start a scheduled output channel. */
	precision_clock_output_start_t start;
	/** Stop a scheduled output channel. */
	precision_clock_output_stop_t stop;
	/** Query channel status. */
	precision_clock_output_get_status_t get_status;
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_OUTPUT_H_ */
