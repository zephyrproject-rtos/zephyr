/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Protocol-neutral scheduled clock output provider extension.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PRECISION_CLOCK_OUTPUT_H_
#define ZEPHYR_INCLUDE_DRIVERS_PRECISION_CLOCK_OUTPUT_H_

#include <zephyr/device.h>
#include <zephyr/precision_timing/precision_clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Precision clock output provider extension
 * @defgroup precision_clock_output_provider Precision clock output provider
 * @since 4.6
 * @version 0.1.0
 * @ingroup precision_timing
 * @{
 *
 * Driver-facing contract that lets a device expose scheduled clock outputs to
 * a protocol-neutral precision clock adapter. All times are raw values in the
 * owning clock's timescale, so the extension carries no protocol-specific
 * prefix. An adapter such as the PTP clock adapter bridges it onto the public
 * API in @ref precision_clock.
 *
 * Providers share the public capability, waveform configuration, and status
 * types from @ref precision_clock. No timescale conversion is needed at this
 * boundary. Providers expose periodic waveforms with the limits and width
 * policies supported by their hardware.
 *
 * Hardware-active status reports whether the generator is armed or running
 * for the accepted configuration, including while waiting for a future first
 * edge. Providers must not report the pin level or absence of an edge as an
 * inactive generator. If this state cannot be observed, leave
 * precision_clock_output_status.hardware_active_valid false rather than
 * inferring activity from the stored configuration.
 *
 * Provider callbacks are invoked only from thread context and may block. A
 * provider may document additional, more restrictive context requirements.
 */

/**
 * @brief Query output channel capabilities and limits.
 *
 * @param dev Device that owns the output channel.
 * @param channel Zero-based output channel index.
 * @param caps Destination for the channel capabilities and limits.
 *
 * @return 0 on success or a negative error code on failure.
 */
typedef int (*precision_clock_output_get_caps_t)(const struct device *dev, uint32_t channel,
						 struct precision_clock_output_caps *caps);

/**
 * @brief Start a periodic output waveform in the device clock's timescale.
 *
 * A failure after hardware programming may leave the channel conservatively
 * configured. The caller must stop the channel before retrying or releasing
 * the owning device; a negative return does not guarantee an unconfigured
 * channel.
 *
 * @param dev Device that owns the output channel.
 * @param channel Zero-based output channel index.
 * @param config First rising edge, period, and width policy.
 *
 * @return 0 on success or a negative error code on failure.
 */
typedef int (*precision_clock_output_start_waveform_t)(
	const struct device *dev, uint32_t channel,
	const struct precision_clock_output_waveform_config *config);

/**
 * @brief Stop a scheduled output channel.
 *
 * Stopping a channel that is already unconfigured is idempotent.
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
 * @param dev Device that owns the output channel.
 * @param channel Zero-based output channel index.
 * @param status Destination for the channel status.
 *
 * @return 0 on success or a negative error code on failure.
 */
typedef int (*precision_clock_output_get_status_t)(
	const struct device *dev, uint32_t channel,
	struct precision_clock_output_status *status);

/**
 * @brief Scheduled clock output provider extension.
 *
 * A device advertises scheduled output support by referencing an instance of
 * this structure from its driver API. The @ref get_caps, @ref start_waveform,
 * @ref stop, and @ref get_status callbacks are required. Protocol adapters
 * dispatch operations independently and return @c -ENOTSUP for missing callbacks.
 */
struct precision_clock_output_provider {
	/** Query channel capabilities and limits. */
	precision_clock_output_get_caps_t get_caps;
	/** Start a periodic output waveform. */
	precision_clock_output_start_waveform_t start_waveform;
	/** Stop a scheduled output channel. */
	precision_clock_output_stop_t stop;
	/** Query channel status. */
	precision_clock_output_get_status_t get_status;
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PRECISION_CLOCK_OUTPUT_H_ */
