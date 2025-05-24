/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Utilities supporting operation on time data structures.
 *
 * POSIX defines gmtime() to convert from time_t to struct tm, but all
 * inverse transformations are non-standard or require access to time
 * zone information.  timeutil_timegm() implements the functionality
 * of the GNU extension timegm() function, but changes the error value
 * as @c EOVERFLOW is not a standard C error identifier.
 *
 * timeutil_timegm64() is provided to support full precision
 * conversion on platforms where @c time_t is limited to 32 bits.
 */

#ifndef ZEPHYR_INCLUDE_SYS_TIMEUTIL_H_
#define ZEPHYR_INCLUDE_SYS_TIMEUTIL_H_

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include <zephyr/sys_clock.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup timeutil_apis Time Utility APIs
 * @ingroup utilities
 * @defgroup timeutil_repr_apis Time Representation APIs
 * @ingroup timeutil_apis
 * @{
 */

/* Base Year value use in calculations in "timeutil_timegm64" API */
#define TIME_UTILS_BASE_YEAR 1900

/**
 * @brief Convert broken-down time to a POSIX epoch offset in seconds.
 *
 * @param tm pointer to broken down time.
 *
 * @return the corresponding time in the POSIX epoch time scale.
 *
 * @see http://man7.org/linux/man-pages/man3/timegm.3.html
 */
int64_t timeutil_timegm64(const struct tm *tm);

/**
 * @brief Convert broken-down time to a POSIX epoch offset in seconds.
 *
 * @param tm pointer to broken down time.
 *
 * @return the corresponding time in the POSIX epoch time scale.  If
 * the time cannot be represented then @c (time_t)-1 is returned and
 * @c errno is set to @c ERANGE`.
 *
 * @see http://man7.org/linux/man-pages/man3/timegm.3.html
 */
time_t timeutil_timegm(const struct tm *tm);

/**
 * @}
 * @defgroup timeutil_sync_apis Time Synchronization APIs
 * @ingroup timeutil_apis
 * @{
 */

/**
 * @brief Immutable state for synchronizing two clocks.
 *
 * Values required to convert durations between two time scales.
 *
 * @note The accuracy of the translation and calculated skew between sources
 * depends on the resolution of these frequencies.  A reference frequency with
 * microsecond or nanosecond resolution would produce the most accurate
 * tracking when the local reference is the Zephyr tick counter.  A reference
 * source like an RTC chip with 1 Hz resolution requires a much larger
 * interval between sampled instants to detect relative clock drift.
 */
struct timeutil_sync_config {
	/** The nominal instance counter rate in Hz.
	 *
	 * This value is assumed to be precise, but may drift depending on
	 * the reference clock source.
	 *
	 * The value must be positive.
	 */
	uint32_t ref_Hz;

	/** The nominal local counter rate in Hz.
	 *
	 * This value is assumed to be inaccurate but reasonably stable.  For
	 * a local clock driven by a crystal oscillator an error of 25 ppm is
	 * common; for an RC oscillator larger errors should be expected.  The
	 * timeutil_sync infrastructure can calculate the skew between the
	 * local and reference clocks and apply it when converting between
	 * time scales.
	 *
	 * The value must be positive.
	 */
	uint32_t local_Hz;
};

/**
 * @brief Representation of an instant in two time scales.
 *
 * Capturing the same instant in two time scales provides a
 * registration point that can be used to convert between those time
 * scales.
 */
struct timeutil_sync_instant {
	/** An instant in the reference time scale.
	 *
	 * This must never be zero in an initialized timeutil_sync_instant
	 * object.
	 */
	uint64_t ref;

	/** The corresponding instance in the local time scale.
	 *
	 * This may be zero in a valid timeutil_sync_instant object.
	 */
	uint64_t local;
};

/**
 * @brief State required to convert instants between time scales.
 *
 * This state in conjunction with functions that manipulate it capture
 * the offset information necessary to convert between two timescales
 * along with information that corrects for skew due to inaccuracies
 * in clock rates.
 *
 * State objects should be zero-initialized before use.
 */
struct timeutil_sync_state {
	/** Pointer to reference and local rate information. */
	const struct timeutil_sync_config *cfg;

	/** The base instant in both time scales. */
	struct timeutil_sync_instant base;

	/** The most recent instant in both time scales.
	 *
	 * This is captured here to provide data for skew calculation.
	 */
	struct timeutil_sync_instant latest;

	/** The scale factor used to correct for clock skew.
	 *
	 * The nominal rate for the local counter is assumed to be
	 * inaccurate but stable, i.e. it will generally be some
	 * parts-per-million faster or slower than specified.
	 *
	 * A duration in observed local clock ticks must be multiplied by
	 * this value to produce a duration in ticks of a clock operating at
	 * the nominal local rate.
	 *
	 * A zero value indicates that the skew has not been initialized.
	 * If the value is zero when #base is initialized the skew will be
	 * set to 1.  Otherwise the skew is assigned through
	 * timeutil_sync_state_set_skew().
	 */
	float skew;
};

/**
 * @brief Record a new instant in the time synchronization state.
 *
 * Note that this updates only the latest persisted instant.  The skew
 * is not adjusted automatically.
 *
 * @param tsp pointer to a timeutil_sync_state object.
 *
 * @param inst the new instant to be recorded.  This becomes the base
 * instant if there is no base instant, otherwise the value must be
 * strictly after the base instant in both the reference and local
 * time scales.
 *
 * @retval 0 if installation succeeded in providing a new base
 * @retval 1 if installation provided a new latest instant
 * @retval -EINVAL if the new instant is not compatible with the base instant
 */
int timeutil_sync_state_update(struct timeutil_sync_state *tsp,
			       const struct timeutil_sync_instant *inst);

/**
 * @brief Update the state with a new skew and possibly base value.
 *
 * Set the skew from a value retrieved from persistent storage, or
 * calculated based on recent skew estimations including from
 * timeutil_sync_estimate_skew().
 *
 * Optionally update the base timestamp.  If the base is replaced the
 * latest instant will be cleared until timeutil_sync_state_update() is
 * invoked.
 *
 * @param tsp pointer to a time synchronization state.
 *
 * @param skew the skew to be used.  The value must be positive and
 * shouldn't be too far away from 1.
 *
 * @param base optional new base to be set.  If provided this becomes
 * the base timestamp that will be used along with skew to convert
 * between reference and local timescale instants.  Setting the base
 * clears the captured latest value.
 *
 * @return 0 if skew was updated
 * @return -EINVAL if skew was not valid
 */
int timeutil_sync_state_set_skew(struct timeutil_sync_state *tsp, float skew,
				 const struct timeutil_sync_instant *base);

/**
 * @brief Estimate the skew based on current state.
 *
 * Using the base and latest syncpoints from the state determine the
 * skew of the local clock relative to the reference clock.  See
 * timeutil_sync_state::skew.
 *
 * @param tsp pointer to a time synchronization state.  The base and latest
 * syncpoints must be present and the latest syncpoint must be after
 * the base point in the local time scale.
 *
 * @return the estimated skew, or zero if skew could not be estimated.
 */
float timeutil_sync_estimate_skew(const struct timeutil_sync_state *tsp);

/**
 * @brief Interpolate a reference timescale instant from a local
 * instant.
 *
 * @param tsp pointer to a time synchronization state.  This must have a base
 * and a skew installed.
 *
 * @param local an instant measured in the local timescale.  This may
 * be before or after the base instant.
 *
 * @param refp where the corresponding instant in the reference
 * timescale should be stored.  A negative interpolated reference time
 * produces an error.  If interpolation fails the referenced object is
 * not modified.
 *
 * @retval 0 if interpolated using a skew of 1
 * @retval 1 if interpolated using a skew not equal to 1
 * @retval -EINVAL
 *   * the times synchronization state is not adequately initialized
 *   * @p refp is null
 * @retval -ERANGE the interpolated reference time would be negative
 */
int timeutil_sync_ref_from_local(const struct timeutil_sync_state *tsp,
				 uint64_t local, uint64_t *refp);

/**
 * @brief Interpolate a local timescale instant from a reference
 * instant.
 *
 * @param tsp pointer to a time synchronization state.  This must have a base
 * and a skew installed.
 *
 * @param ref an instant measured in the reference timescale.  This
 * may be before or after the base instant.
 *
 * @param localp where the corresponding instant in the local
 * timescale should be stored.  An interpolated value before local
 * time 0 is provided without error.  If interpolation fails the
 * referenced object is not modified.
 *
 * @retval 0 if successful with a skew of 1
 * @retval 1 if successful with a skew not equal to 1
 * @retval -EINVAL
 *   * the time synchronization state is not adequately initialized
 *   * @p refp is null
 */
int timeutil_sync_local_from_ref(const struct timeutil_sync_state *tsp,
				 uint64_t ref, int64_t *localp);

/**
 * @brief Convert from a skew to an error in parts-per-billion.
 *
 * A skew of 1.0 has zero error.  A skew less than 1 has a positive
 * error (clock is faster than it should be).  A skew greater than one
 * has a negative error (clock is slower than it should be).
 *
 * Note that due to the limited precision of @c float compared with @c
 * double the smallest error that can be represented is about 120 ppb.
 * A "precise" time source may have error on the order of 2000 ppb.
 *
 * A skew greater than 3.14748 may underflow the 32-bit
 * representation; this represents a clock running at less than 1/3
 * its nominal rate.
 *
 * @return skew error represented as parts-per-billion, or INT32_MIN
 * if the skew cannot be represented in the return type.
 */
int32_t timeutil_sync_skew_to_ppb(float skew);

/**
 * @}
 */

/**
 * @defgroup timeutil_timespec_apis Timespec Utility APIs
 * @ingroup timeutil_apis
 * @{
 */

/**
 * @brief Check if a timespec is valid.
 *
 * Check if a timespec is valid (i.e. normalized) by ensuring that the `tv_nsec` field is in the
 * range `[0, NSEC_PER_SEC-1]`.
 *
 * @note @p ts must not be `NULL`.
 *
 * @param ts the timespec to check
 *
 * @return `true` if the timespec is valid, otherwise `false`.
 */
static inline bool timespec_is_valid(const struct timespec *ts)
{
	__ASSERT_NO_MSG(ts != NULL);

	return (ts->tv_nsec >= 0) && (ts->tv_nsec < NSEC_PER_SEC);
}

/**
 * @brief Normalize a timespec so that the `tv_nsec` field is in valid range.
 *
 * Normalize a timespec by adjusting the `tv_sec` and `tv_nsec` fields so that the `tv_nsec` field
 * is in the range `[0, NSEC_PER_SEC-1]`. This is achieved by converting nanoseconds to seconds and
 * accumulating seconds in either the positive direction when `tv_nsec` > `NSEC_PER_SEC`, or in the
 * negative direction when `tv_nsec` < 0.
 *
 * In pseudocode, normalization can be done as follows:
 * ```python
 * if ts.tv_nsec >= NSEC_PER_SEC:
 *     sec = ts.tv_nsec / NSEC_PER_SEC
 *     ts.tv_sec += sec
 *     ts.tv_nsec -= sec * NSEC_PER_SEC
 * elif ts.tv_nsec < 0:
 *      # div_round_up(abs(ts->tv_nsec), NSEC_PER_SEC)
 *	    sec = (NSEC_PER_SEC - ts.tv_nsec - 1) / NSEC_PER_SEC
 *	    ts.tv_sec -= sec;
 *	    ts.tv_nsec += sec * NSEC_PER_SEC;
 * ```
 *
 * @note There are two cases where the normalization can result in integer overflow. These can
 * be extrapolated to not simply overflowing the `tv_sec` field by one second, but also by any
 * realizable multiple of `NSEC_PER_SEC`.
 *
 * 1. When `tv_nsec` is negative and `tv_sec` is already most negative.
 * 2. When `tv_nsec` is greater-or-equal to `NSEC_PER_SEC` and `tv_sec` is already most positive.
 *
 * If the operation would result in integer overflow, return value is `false`.
 *
 * @note @p ts must be non-`NULL`.
 *
 * @param ts the timespec to be normalized
 *
 * @return `true` if the operation completes successfully, otherwise `false`.
 */
static inline bool timespec_normalize(struct timespec *ts)
{
	__ASSERT_NO_MSG(ts != NULL);

#if defined(CONFIG_SPEED_OPTIMIZATIONS) && HAS_BUILTIN(__builtin_add_overflow)

	int sign = (ts->tv_nsec >= 0) - (ts->tv_nsec < 0);
	int64_t sec = (ts->tv_nsec >= (long)NSEC_PER_SEC) * (ts->tv_nsec / (long)NSEC_PER_SEC) +
		      ((ts->tv_nsec < 0) && (ts->tv_nsec != LONG_MIN)) *
			      DIV_ROUND_UP((unsigned long)-ts->tv_nsec, (long)NSEC_PER_SEC) +
		      (ts->tv_nsec == LONG_MIN) * ((LONG_MAX / NSEC_PER_SEC) + 1);
	bool overflow = __builtin_add_overflow(ts->tv_sec, sign * sec, &ts->tv_sec);

	ts->tv_nsec -= sign * (long)NSEC_PER_SEC * sec;

	if (!overflow) {
		__ASSERT_NO_MSG(timespec_is_valid(ts));
	}

	return !overflow;

#else

	long sec;

	if (ts->tv_nsec >= (long)NSEC_PER_SEC) {
		sec = ts->tv_nsec / (long)NSEC_PER_SEC;
	} else if (ts->tv_nsec < 0) {
		if ((sizeof(ts->tv_nsec) == sizeof(uint32_t)) && (ts->tv_nsec == LONG_MIN)) {
			sec = DIV_ROUND_UP(LONG_MAX / NSEC_PER_USEC, USEC_PER_SEC);
		} else {
			sec = DIV_ROUND_UP((unsigned long)-ts->tv_nsec, NSEC_PER_SEC);
		}
	} else {
		sec = 0;
	}

	if ((ts->tv_nsec < 0) && (ts->tv_sec < 0) && (ts->tv_sec - INT64_MIN < sec)) {
		/*
		 * When `tv_nsec` is negative and `tv_sec` is already most negative,
		 * further subtraction would cause integer overflow.
		 */
		return false;
	}

	if ((ts->tv_nsec >= (long)NSEC_PER_SEC) && (ts->tv_sec > 0) &&
	    (INT64_MAX - ts->tv_sec < sec)) {
		/*
		 * When `tv_nsec` is >= `NSEC_PER_SEC` and `tv_sec` is already most
		 * positive, further addition would cause integer overflow.
		 */
		return false;
	}

	if (ts->tv_nsec >= (long)NSEC_PER_SEC) {
		ts->tv_sec += sec;
		ts->tv_nsec -= sec * (long)NSEC_PER_SEC;
	} else if (ts->tv_nsec < 0) {
		ts->tv_sec -= sec;
		ts->tv_nsec += sec * (long)NSEC_PER_SEC;
	} else {
		/* no change: SonarQube was complaining */
	}

	__ASSERT_NO_MSG(timespec_is_valid(ts));

	return true;

#endif
}

/**
 * @brief Add one timespec to another
 *
 * This function sums the two timespecs pointed to by @p a and @p b and stores the result in the
 * timespce pointed to by @p a.
 *
 * If the operation would result in integer overflow, return value is `false`.
 *
 * @note @p a and @p b must be non-`NULL` and normalized.
 *
 * @param a the timespec which is added to
 * @param b the timespec to be added
 *
 * @return `true` if the operation was successful, otherwise `false`.
 */
static inline bool timespec_add(struct timespec *a, const struct timespec *b)
{
	__ASSERT_NO_MSG((a != NULL) && timespec_is_valid(a));
	__ASSERT_NO_MSG((b != NULL) && timespec_is_valid(b));

#if defined(CONFIG_SPEED_OPTIMIZATIONS) && HAS_BUILTIN(__builtin_add_overflow)

	return !__builtin_add_overflow(a->tv_sec, b->tv_sec, &a->tv_sec) &&
	       !__builtin_add_overflow(a->tv_nsec, b->tv_nsec, &a->tv_nsec) &&
	       timespec_normalize(a);

#else

	if ((a->tv_sec < 0) && (b->tv_sec < 0) && (INT64_MIN - a->tv_sec > b->tv_sec)) {
		/* negative integer overflow would occur */
		return false;
	}

	if ((a->tv_sec > 0) && (b->tv_sec > 0) && (INT64_MAX - a->tv_sec < b->tv_sec)) {
		/* positive integer overflow would occur */
		return false;
	}

	a->tv_sec += b->tv_sec;
	a->tv_nsec += b->tv_nsec;

	return timespec_normalize(a);

#endif
}

/**
 * @brief Negate a timespec object
 *
 * Negate the timespec object pointed to by @p ts and store the result in the same
 * memory location.
 *
 * If the operation would result in integer overflow, return value is `false`.
 *
 * @param ts The timespec object to negate.
 *
 * @return `true` of the operation is successful, otherwise `false`.
 */
static inline bool timespec_negate(struct timespec *ts)
{
	__ASSERT_NO_MSG((ts != NULL) && timespec_is_valid(ts));

#if defined(CONFIG_SPEED_OPTIMIZATIONS) && HAS_BUILTIN(__builtin_sub_overflow)

	return !__builtin_sub_overflow(0LL, ts->tv_sec, &ts->tv_sec) &&
	       !__builtin_sub_overflow(0L, ts->tv_nsec, &ts->tv_nsec) && timespec_normalize(ts);

#else

	/* note: must check for 32-bit size here until #90029 is resolved */
	if (((sizeof(ts->tv_sec) == sizeof(int32_t)) && (ts->tv_sec == INT32_MIN)) ||
	    ((sizeof(ts->tv_sec) == sizeof(int64_t)) && (ts->tv_sec == INT64_MIN))) {
		/* -INT64_MIN > INT64_MAX, so +ve integer overflow would occur */
		return false;
	}

	ts->tv_sec = -ts->tv_sec;
	ts->tv_nsec = -ts->tv_nsec;

	return timespec_normalize(ts);

#endif
}

/**
 * @brief Subtract one timespec from another
 *
 * This function subtracts the timespec pointed to by @p b from the timespec pointed to by @p a and
 * stores the result in the timespce pointed to by @p a.
 *
 * If the operation would result in integer overflow, return value is `false`.
 *
 * @note @p a and @p b must be non-`NULL`.
 *
 * @param a the timespec which is subtracted from
 * @param b the timespec to be subtracted
 *
 * @return `true` if the operation is successful, otherwise `false`.
 */
static inline bool timespec_sub(struct timespec *a, const struct timespec *b)
{
	__ASSERT_NO_MSG(a != NULL);
	__ASSERT_NO_MSG(b != NULL);

	struct timespec neg = *b;

	return timespec_negate(&neg) && timespec_add(a, &neg);
}

/**
 * @brief Compare two timespec objects
 *
 * This function compares two timespec objects pointed to by @p a and @p b.
 *
 * @note @p a and @p b must be non-`NULL` and normalized.
 *
 * @param a the first timespec to compare
 * @param b the second timespec to compare
 *
 * @return -1, 0, or +1 if @a a is less than, equal to, or greater than @a b, respectively.
 */
static inline int timespec_compare(const struct timespec *a, const struct timespec *b)
{
	__ASSERT_NO_MSG((a != NULL) && timespec_is_valid(a));
	__ASSERT_NO_MSG((b != NULL) && timespec_is_valid(b));

	return (((a->tv_sec == b->tv_sec) && (a->tv_nsec < b->tv_nsec)) * -1) +
	       (((a->tv_sec == b->tv_sec) && (a->tv_nsec > b->tv_nsec)) * 1) +
	       ((a->tv_sec < b->tv_sec) * -1) + ((a->tv_sec > b->tv_sec));
}

/**
 * @brief Check if two timespec objects are equal
 *
 * This function checks if the two timespec objects pointed to by @p a and @p b are equal.
 *
 * @note @p a and @p b must be non-`NULL` are not required to be normalized.
 *
 * @param a the first timespec to compare
 * @param b the second timespec to compare
 *
 * @return true if the two timespec objects are equal, otherwise false.
 */
static inline bool timespec_equal(const struct timespec *a, const struct timespec *b)
{
	__ASSERT_NO_MSG(a != NULL);
	__ASSERT_NO_MSG(b != NULL);

	return (a->tv_sec == b->tv_sec) && (a->tv_nsec == b->tv_nsec);
}

/**
 * @}
 */

/**
 * @ingroup timeutil_repr_apis
 * @{
 */

/**
 * @brief Convert a kernel timeout to a timespec
 *
 * This function converts time durations expressed as Zephyr @ref k_timeout_t
 * objects to `struct timespec` objects.
 *
 * @param timeout the kernel timeout to convert
 * @param[out] ts the timespec to store the result
 */
static inline void timespec_from_timeout(k_timeout_t timeout, struct timespec *ts)
{
	__ASSERT_NO_MSG(ts != NULL);

#if defined(CONFIG_SPEED_OPTIMIZATIONS)

	uint64_t ns = k_ticks_to_ns_ceil64(timeout.ticks);

	*ts = (struct timespec){
		.tv_sec = (timeout.ticks == K_TICKS_FOREVER) * INT64_MAX +
			  (timeout.ticks != K_TICKS_FOREVER) * (ns / NSEC_PER_SEC),
		.tv_nsec = (timeout.ticks == K_TICKS_FOREVER) * (NSEC_PER_SEC - 1) +
			   (timeout.ticks != K_TICKS_FOREVER) * (ns % NSEC_PER_SEC),
	};

#else

	if (timeout.ticks == 0) {
		/* This is equivalent to K_NO_WAIT, but without including <zephyr/kernel.h> */
		ts->tv_sec = 0;
		ts->tv_nsec = 0;
	} else if (timeout.ticks == K_TICKS_FOREVER) {
		/* This is roughly equivalent to K_FOREVER, but not including <zephyr/kernel.h> */
		ts->tv_sec = (time_t)INT64_MAX;
		ts->tv_nsec = NSEC_PER_SEC - 1;
	} else {
		uint64_t ns = k_ticks_to_ns_ceil64(timeout.ticks);

		ts->tv_sec = ns / NSEC_PER_SEC;
		ts->tv_nsec = ns - ts->tv_sec * NSEC_PER_SEC;
	}

#endif

	__ASSERT_NO_MSG(timespec_is_valid(ts));
}

/**
 * @brief Convert a timespec to a kernel timeout
 *
 * This function converts durations expressed as a `struct timespec` to Zephyr @ref k_timeout_t
 * objects.
 *
 * Given that the range of a `struct timespec` is much larger than the range of @ref k_timeout_t,
 * and also given that the functions are only intended to be used to convert time durations
 * (which are always positive), the function will saturate to @ref K_NO_WAIT if the `tv_sec` field
 * of @a ts is negative.
 *
 * Similarly, if the duration is too large to fit in @ref k_timeout_t, the function will
 * saturate to @ref K_FOREVER.
 *
 * @param ts the timespec to convert
 * @return the kernel timeout
 */
static inline k_timeout_t timespec_to_timeout(const struct timespec *ts)
{
	__ASSERT_NO_MSG((ts != NULL) && timespec_is_valid(ts));

#if defined(CONFIG_SPEED_OPTIMIZATIONS)

	return (k_timeout_t){
		/* note: must check for 32-bit size here until #90029 is resolved */
		.ticks = ((sizeof(ts->tv_sec) == sizeof(int32_t) && (ts->tv_sec == INT32_MAX) &&
			   (ts->tv_nsec == NSEC_PER_SEC - 1)) ||
			  ((sizeof(ts->tv_sec) == sizeof(int64_t)) && (ts->tv_sec == INT64_MAX) &&
			   (ts->tv_nsec == NSEC_PER_SEC - 1))) *
				 K_TICKS_FOREVER +
			 ((sizeof(ts->tv_sec) == sizeof(int32_t) && (ts->tv_sec == INT32_MAX) &&
			   (ts->tv_nsec == NSEC_PER_SEC - 1)) ||
			  ((sizeof(ts->tv_sec) == sizeof(int64_t)) && (ts->tv_sec != INT64_MAX) &&
			   (ts->tv_sec >= 0))) *
				 (IS_ENABLED(CONFIG_TIMEOUT_64BIT)
					  ? (int64_t)(CLAMP(
						    k_sec_to_ticks_floor64(ts->tv_sec) +
							    k_ns_to_ticks_floor64(ts->tv_nsec),
						    0, (uint64_t)INT64_MAX))
					  : (uint32_t)(CLAMP(
						    k_sec_to_ticks_floor64(ts->tv_sec) +
							    k_ns_to_ticks_floor64(ts->tv_nsec),
						    0, (uint64_t)UINT32_MAX)))};

#else

	if ((ts->tv_sec < 0) || (ts->tv_sec == 0 && ts->tv_nsec == 0)) {
		/* This is equivalent to K_NO_WAIT, but without including <zephyr/kernel.h> */
		return (k_timeout_t){
			.ticks = 0,
		};
		/* note: must check for 32-bit size here until #90029 is resolved */
	} else if (((sizeof(ts->tv_sec) == sizeof(int32_t)) && (ts->tv_sec == INT32_MAX) &&
		    (ts->tv_nsec == NSEC_PER_SEC - 1)) ||
		   ((sizeof(ts->tv_sec) == sizeof(int64_t)) && (ts->tv_sec == INT64_MAX) &&
		    (ts->tv_nsec == NSEC_PER_SEC - 1))) {
		/* This is equivalent to K_FOREVER, but not including <zephyr/kernel.h> */
		return (k_timeout_t){
			.ticks = K_TICKS_FOREVER,
		};
	} else {
		if (IS_ENABLED(CONFIG_TIMEOUT_64BIT)) {
			return (k_timeout_t){
				.ticks = (int64_t)CLAMP(k_sec_to_ticks_floor64(ts->tv_sec) +
								k_ns_to_ticks_floor64(ts->tv_nsec),
							0, (uint64_t)INT64_MAX),
			};
		} else {
			return (k_timeout_t){
				.ticks = (uint32_t)CLAMP(k_sec_to_ticks_floor64(ts->tv_sec) +
								 k_ns_to_ticks_floor64(ts->tv_nsec),
							 0, (uint64_t)UINT32_MAX),
			};
		}
	}

#endif
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_TIMEUTIL_H_ */
