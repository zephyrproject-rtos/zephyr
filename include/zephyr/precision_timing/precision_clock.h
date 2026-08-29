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

/** Operations implemented by a precision clock adapter. */
struct precision_clock_api {
	/** Read the current clock value. */
	int (*read)(const struct precision_clock *precision_clk, precision_time_t *time_ns);
	/** Set the current clock value. */
	int (*set)(const struct precision_clock *precision_clk, precision_time_t time_ns);
	/** Apply a signed phase adjustment in nanoseconds. */
	int (*adjust_phase)(const struct precision_clock *precision_clk, precision_time_t phase_ns);
	/** Set the clock rate ratio relative to its nominal frequency. */
	int (*adjust_rate)(const struct precision_clock *precision_clk, double rate_ratio);
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
 * @retval -EINVAL if an argument or clock API is invalid.
 * @retval -ENOTSUP if reading is unsupported.
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
 * @retval -EINVAL if the clock API is invalid.
 * @retval -ENOTSUP if setting is unsupported.
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
 * @retval -EINVAL if the clock API is invalid.
 * @retval -ENOTSUP if phase adjustment is unsupported.
 * @return An adapter-specific negative error code on failure.
 */
int precision_clock_adjust_phase(const struct precision_clock *precision_clk,
				 precision_time_t phase_ns);

/**
 * @brief Set the rate ratio of a precision clock.
 *
 * @param precision_clk Clock to adjust.
 * @param rate_ratio Rate ratio relative to the nominal frequency.
 *
 * @retval 0 on success.
 * @retval -EINVAL if the clock API is invalid.
 * @retval -ENOTSUP if rate adjustment is unsupported.
 * @return An adapter-specific negative error code on failure.
 */
int precision_clock_adjust_rate(const struct precision_clock *precision_clk, double rate_ratio);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_H_ */
