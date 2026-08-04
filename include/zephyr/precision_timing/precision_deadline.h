/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Monotonic deadline helper for precision timing housekeeping.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_DEADLINE_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_DEADLINE_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/precision_timing/precision_time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Precision Deadline
 * @defgroup precision_deadline Precision Deadline
 * @since 4.5
 * @version 0.1.0
 * @ingroup precision_timing
 * @{
 */

/** Monotonic deadline used to rate-limit precision timing housekeeping. */
struct precision_deadline {
	/** Uptime in milliseconds at which the deadline expires. */
	int64_t expiry_ms;
	/** Whether a deadline is currently scheduled. */
	bool scheduled;
};

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

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_DEADLINE_H_ */
