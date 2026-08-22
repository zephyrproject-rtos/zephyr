/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Domain-qualified precision time values.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_TIME_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_TIME_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/** Semantic type of a precision time domain. */
enum precision_time_domain_type {
	/** Invalid or unspecified domain. */
	PRECISION_TIME_DOMAIN_INVALID = 0,
	/** International Atomic Time domain. */
	PRECISION_TIME_DOMAIN_TAI,
	/** Coordinated Universal Time domain. */
	PRECISION_TIME_DOMAIN_UTC,
	/** Monotonic system time domain. */
	PRECISION_TIME_DOMAIN_MONOTONIC,
	/** Precision hardware clock domain. */
	PRECISION_TIME_DOMAIN_PHC,
	/** IEEE 1588 Precision Time Protocol domain. */
	PRECISION_TIME_DOMAIN_PTP,
	/** IEEE 802.1AS generalized Precision Time Protocol domain. */
	PRECISION_TIME_DOMAIN_GPTP,
	/** Uninterpreted counter domain. */
	PRECISION_TIME_DOMAIN_RAW,
};

/** Identity of a clock or time scale. */
struct precision_time_domain {
	/** Semantic domain type. */
	enum precision_time_domain_type type;
	/** Protocol- or implementation-defined domain instance identifier. */
	uint32_t id;
};

/** Timestamp qualified by its clock domain. */
struct precision_time_point {
	/** Time value in nanoseconds. */
	precision_time_t time;
	/** Domain in which the time value is expressed. */
	struct precision_time_domain domain;
};

/** Validity flags for a precision time observation. */
enum precision_observation_flags {
	/** Source time point is valid. */
	PRECISION_OBSERVATION_SOURCE_VALID = BIT(0),
	/** Local time point is valid. */
	PRECISION_OBSERVATION_LOCAL_VALID = BIT(1),
};

/** Simultaneous observation of source and local clock domains. */
struct precision_time_observation {
	/** Observed source time. */
	struct precision_time_point source;
	/** Corresponding local time. */
	struct precision_time_point local;
	/** Estimated observation uncertainty in nanoseconds. */
	precision_time_t uncertainty_ns;
	/** Combination of @ref precision_observation_flags. */
	uint32_t flags;
};

/**
 * @brief Compare two precision time domain identities.
 *
 * @param a First domain.
 * @param b Second domain.
 *
 * @retval true if both domains are non-null and equal.
 * @retval false otherwise.
 */
static inline bool precision_time_domain_equal(const struct precision_time_domain *a,
					       const struct precision_time_domain *b)
{
	return a != NULL && b != NULL && a->type == b->type && a->id == b->id;
}

/**
 * @brief Add two signed precision time values with overflow checking.
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
 * @brief Subtract two signed precision time values with overflow checking.
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

/**
 * @brief Convert an unsigned seconds and nanoseconds pair to precision time.
 *
 * @param sec Whole seconds.
 * @param nsec Nanosecond fraction in the range 0 through 999999999.
 * @param result Destination for the converted value.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p result is null.
 * @retval -ERANGE if the input is invalid or not representable.
 */
int precision_time_from_u64_sec_nsec(uint64_t sec, uint32_t nsec, precision_time_t *result);

/**
 * @brief Convert non-negative precision time to seconds and nanoseconds.
 *
 * @param time Precision time value to convert.
 * @param sec Destination for whole seconds.
 * @param nsec Destination for the nanosecond fraction.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an output pointer is null.
 * @retval -ERANGE if @p time is negative.
 */
int precision_time_to_u64_sec_nsec(precision_time_t time, uint64_t *sec, uint32_t *nsec);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_TIME_H_ */
