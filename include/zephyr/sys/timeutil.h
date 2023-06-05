/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
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

#include <time.h>

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

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_SYS_TIMEUTIL_H_ */
