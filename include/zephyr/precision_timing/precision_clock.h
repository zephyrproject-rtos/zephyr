/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Protocol-neutral precision clock abstraction.
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

/** Operations implemented by a precision clock adapter. */
struct precision_clock_api {
	/** Read the current clock value. */
	int (*read)(const struct precision_clock *precision_clk, struct precision_time_point *tp);
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
	/** Adapter operations. */
	const struct precision_clock_api *api;
	/** Adapter instance passed back to @ref precision_clock_api operations. */
	const void *adapter;
	/** Domain produced and consumed by this clock. */
	struct precision_time_domain domain;
};

/**
 * @brief Read a precision clock.
 *
 * @param precision_clk Clock to read.
 * @param tp Destination for the clock value and domain.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument or clock API is invalid.
 * @retval -ENOTSUP if the clock does not support reading.
 * @return An adapter-specific negative error code on failure.
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
 * @return An adapter-specific negative error code on failure.
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
 * @retval -ERANGE if @p phase_ns exceeds adapter limits.
 * @return An adapter-specific negative error code on failure.
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
 * @retval -ERANGE if @p rate_ppb exceeds adapter limits.
 * @return An adapter-specific negative error code on failure.
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
 * @return An adapter-specific negative error code on failure.
 */
int precision_clock_get_caps(const struct precision_clock *precision_clk,
			     struct precision_clock_caps *caps);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_H_ */
