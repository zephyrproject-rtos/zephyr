/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Signed precision time arithmetic.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_TIME_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_TIME_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Precision Timing
 * @defgroup precision_timing Precision Timing
 * @since 4.5
 * @version 0.1.0
 * @ingroup os_services
 */

/**
 * @brief Precision Time
 * @defgroup precision_time Precision Time
 * @since 4.5
 * @version 0.1.0
 * @ingroup precision_timing
 * @{
 */

/** Signed precision time value expressed in nanoseconds. */
typedef int64_t precision_time_t;

/** Maximum representable precision time value. */
#define PRECISION_TIME_MAX INT64_MAX
/** Minimum representable precision time value. */
#define PRECISION_TIME_MIN INT64_MIN

/**
 * @brief Add two precision time values with overflow checking.
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
 * @brief Subtract two precision time values with overflow checking.
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

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_TIME_H_ */
