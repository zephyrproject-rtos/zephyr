/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Protocol-neutral precision clock operations.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_H_

#include <stdint.h>

#include <zephyr/precision_timing/precision_time.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Precision Clock
 * @defgroup precision_clock Precision Clock
 * @since 4.5
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
	PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM = BIT(1),
	/** Channel supports an exact programmable high-pulse width. */
	PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH = BIT(2),
	/** Channel can observe whether its hardware output generator is armed or running. */
	PRECISION_CLOCK_OUTPUT_CAP_HARDWARE_ACTIVE = BIT(5),
};

/**
 * @brief Capabilities and limits of a precision clock output channel.
 *
 * The period limits are meaningful only when @ref PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM
 * is advertised. The pulse-width limits are meaningful only when
 * @ref PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH is advertised.
 */
struct precision_clock_output_caps {
	/** Combination of @ref precision_clock_output_caps_flags. */
	uint32_t flags;
	/** Number of output channels exposed by the clock. */
	uint32_t channel_count;
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

/** Accepted configuration held by a scheduled clock output channel. */
union precision_clock_output_configured_config {
	/** Valid when the configured kind is @ref PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM. */
	struct precision_clock_output_waveform_config waveform;
};

/** Observable status of a precision clock output channel. */
struct precision_clock_output_status {
	/** Whether the provider currently holds a configuration for the channel. */
	bool configured;
	/** Configured output kind. Valid only when @ref configured is true. */
	enum precision_clock_output_kind kind;
	/** Accepted configuration. Valid only when @ref configured is true. */
	union precision_clock_output_configured_config config;
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

struct precision_clock;

/** Number of fractional bits in a scaled parts-per-million rate adjustment. */
#define PRECISION_CLOCK_SCALED_PPM_SHIFT 16

/** One part per million in the scaled rate-adjustment representation. */
#define PRECISION_CLOCK_SCALED_PPM_ONE (INT64_C(1) << PRECISION_CLOCK_SCALED_PPM_SHIFT)

/**
 * @brief Operations implemented by a precision clock adapter.
 *
 * The four base clock operations are mandatory. Scheduled-output operations
 * are optional and may be omitted when the clock has no output provider.
 * Callers must provide a valid initialized clock and valid arguments to the
 * base operations.
 */
struct precision_clock_api {
	/** Read the current clock value. */
	int (*read)(const struct precision_clock *precision_clk, precision_time_t *time_ns);
	/** Set the current clock value. */
	int (*set)(const struct precision_clock *precision_clk, precision_time_t time_ns);
	/** Apply a signed phase adjustment in nanoseconds. */
	int (*adjust_phase)(const struct precision_clock *precision_clk, precision_time_t phase_ns);
	/** Set the clock rate offset from its nominal frequency. */
	int (*adjust_rate)(const struct precision_clock *precision_clk, int64_t scaled_ppm);
	/** Query scheduled-output channel capabilities and limits. */
	int (*get_output_caps)(const struct precision_clock *precision_clk, uint32_t channel,
			       struct precision_clock_output_caps *caps);
	/** Start a periodic output waveform on a channel. */
	int (*output_start_waveform)(const struct precision_clock *precision_clk, uint32_t channel,
				     const struct precision_clock_output_waveform_config *config);
	/** Stop a scheduled-output channel and validate @p channel. */
	int (*output_stop)(const struct precision_clock *precision_clk, uint32_t channel);
	/** Query scheduled-output channel status and validate @p channel. */
	int (*get_output_status)(const struct precision_clock *precision_clk, uint32_t channel,
				 struct precision_clock_output_status *status);
};

/** Protocol-neutral precision clock instance. */
struct precision_clock {
	/** Adapter operations. */
	const struct precision_clock_api *api;
	/** Adapter-specific data passed to each operation. */
	void *data;
};

/**
 * @brief Read a precision clock.
 *
 * @param precision_clk Clock to read.
 * @param time_ns Destination for the clock value in nanoseconds.
 *
 * @retval 0 on success.
 * @return An adapter-specific negative error code on failure.
 */
int precision_clock_read(const struct precision_clock *precision_clk, precision_time_t *time_ns);

/**
 * @brief Set a precision clock.
 *
 * @param precision_clk Clock to set.
 * @param time_ns New clock value in nanoseconds.
 *
 * @retval 0 on success.
 * @return An adapter-specific negative error code on failure.
 */
int precision_clock_set(const struct precision_clock *precision_clk, precision_time_t time_ns);

/**
 * @brief Apply a phase adjustment to a precision clock.
 *
 * @param precision_clk Clock to adjust.
 * @param phase_ns Signed phase adjustment in nanoseconds.
 *
 * @retval 0 on success.
 * @return An adapter-specific negative error code on failure.
 */
int precision_clock_adjust_phase(const struct precision_clock *precision_clk,
				 precision_time_t phase_ns);

/**
 * @brief Convert parts per billion to scaled parts per million.
 *
 * @param ppb Signed frequency offset from nominal in parts per billion.
 * @param scaled_ppm Destination for the signed scaled parts-per-million value.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p scaled_ppm is null.
 * @retval -ERANGE if @p ppb is not finite or the result is not representable.
 */
int precision_clock_ppb_to_scaled_ppm(double ppb, int64_t *scaled_ppm);

/**
 * @brief Set the rate offset of a precision clock.
 *
 * @param precision_clk Clock to adjust.
 * @param scaled_ppm Signed frequency offset from nominal in parts per million
 *                   with a 16-bit binary fractional field. For example,
 *                   @c PRECISION_CLOCK_SCALED_PPM_ONE represents 1 ppm.
 *
 * @retval 0 on success.
 * @return An adapter-specific negative error code on failure.
 */
int precision_clock_adjust_rate(const struct precision_clock *precision_clk, int64_t scaled_ppm);

/**
 * @brief Query the capabilities and limits of a clock output channel.
 *
 * Call from thread context. The provider operation may block.
 *
 * @param precision_clk Clock whose output channel is queried.
 * @param channel Zero-based output channel index.
 * @param caps Destination for channel capabilities and limits.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument or clock API is invalid.
 * @retval -ENOTSUP if scheduled output or @p channel is unsupported.
 * @return A provider-specific negative error code on failure.
 */
int precision_clock_output_get_caps(const struct precision_clock *precision_clk, uint32_t channel,
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
 * @brief Start a periodic clock output waveform.
 *
 * The first rising edge is generated at @p config first_rising_time and
 * subsequent rising edges are separated by @p config period_ns. The channel
 * must advertise @ref PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM. When @p config
 * width_policy is @ref PRECISION_CLOCK_OUTPUT_WIDTH_EXACT the channel must also
 * advertise @ref PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH and the pulse
 * width must be positive and shorter than the period; otherwise the provider
 * default width is used and @p config pulse_width_ns is ignored. Configuration
 * values must be exactly representable at the channel resolution. Starting an
 * already configured channel is rejected; stop the channel before
 * reconfiguring it.
 *
 * A provider failure after hardware programming may conservatively retain the
 * channel configuration. Call @ref precision_clock_output_stop before retrying
 * or releasing the underlying device; a negative return does not guarantee
 * that the channel is unconfigured.
 *
 * Call from thread context. The provider operation may block.
 *
 * @param precision_clk Clock that owns the output channel.
 * @param channel Zero-based output channel index.
 * @param config First rising edge, period, and width policy.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument, width policy, or interval relationship is invalid.
 * @retval -ENOTSUP if periodic waveforms, programmable width, or @p channel is unsupported.
 * @retval -ERANGE if a value is outside channel limits or not representable.
 * @retval -ETIME if the first rising edge does not meet the minimum lead time.
 * @retval -EBUSY if the channel is already configured.
 * @return A provider-specific negative error code on failure.
 */
int precision_clock_output_start_waveform(
	const struct precision_clock *precision_clk, uint32_t channel,
	const struct precision_clock_output_waveform_config *config);

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
 * @param channel Zero-based output channel index.
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
 * when observable. The generic layer queries the channel capabilities to
 * validate @p channel and to reject a status that is inconsistent with the
 * advertised capabilities.
 *
 * Call from thread context. The provider operation may block.
 *
 * @param precision_clk Clock that owns the output channel.
 * @param channel Zero-based output channel index.
 * @param status Destination for the configuration and hardware-active state.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument or clock API is invalid, or the provider reports
 *	   a status inconsistent with its capabilities.
 * @retval -ENOTSUP if status reporting or @p channel is unsupported.
 * @return A provider-specific negative error code on failure.
 */
int precision_clock_output_get_status(const struct precision_clock *precision_clk, uint32_t channel,
				      struct precision_clock_output_status *status);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_H_ */
