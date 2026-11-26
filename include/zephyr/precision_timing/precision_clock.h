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

struct precision_clock;
struct precision_clock_output_caps;
struct precision_clock_output_config;
struct precision_clock_output_status;

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
	/** Query capabilities of all scheduled-output channels. */
	int (*output_get_caps)(const struct precision_clock *precision_clk,
			       struct precision_clock_output_caps *caps);
	/**
	 * Start a scheduled output on a channel.
	 *
	 * Use the return and ownership contract of @ref precision_clock_output_start.
	 */
	int (*output_start)(const struct precision_clock *precision_clk, uint32_t channel,
			    const struct precision_clock_output_config *config);
	/** Stop a scheduled-output channel and validate @p channel. */
	int (*output_stop)(const struct precision_clock *precision_clk, uint32_t channel);
	/** Query scheduled-output channel status and validate @p channel. */
	int (*output_get_status)(const struct precision_clock *precision_clk, uint32_t channel,
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

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_H_ */
