/*
 * Copyright (c) 2019, Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Utilities for converting between civil time and UNIX time
 */

#ifndef ZEPHYR_INCLUDE_MISC_CIVILTIME_H_
#define ZEPHYR_INCLUDE_MISC_CIVILTIME_H_

#include <zephyr/types.h>
#include <time.h>

/** @brief Convert a civil (proleptic Gregorian) date to days relative to
 * 1970-01-01.
 *
 * @param y the calendar year
 * @param m the calendar month, in the range [1, 12]
 * @param d the day of the month, in the range [1, last_day_of_month(y, m)]
 *
 * @return the signed number of days between 1970-01-01 and the
 * specified day.  Negative days are before 1970-01-01.
 */
s64_t time_days_from_civil(s64_t y,
			   unsigned int m,
			   unsigned int d);

/** @brief Convert civil time to UNIX time.
 *
 * @param tvp pointer to a civil time structure.  @c tm_year, @c
 * tm_mon, @c tm_mday, @c tm_hour, @c tm_min, and @c tm_sec must be
 * valid.  All other fields are ignored.
 *
 * @return the signed number of seconds between 1970-01-01T00:00:00
 * and the specified time, ignoring leap seconds and DST offsets.
 */
time_t time_unix_from_civil(const struct tm *tvp);

/** @brief Convert days relative to 1970-01-01 to date information
 *
 * This converts signed days relative to 1970-01-01 to the POSIX
 * standard civil time structure assuming a proleptic Gregorian
 * calendar.
 *
 * @param days the time represented as days.
 *
 * @return the time information for corresponding to the provided
 * instant.  The @c tm_year, @c tm_mon@, @c tm_mday, @c tm_wday, and
 * @c tm_yday fields are set; all other fields are zero.
 */
struct tm time_civil_from_days(s64_t days);

/** @brief Convert a UNIX time to civil time.
 *
 * This converts integral seconds since (before) 1970-01-01T00:00:00 to the
 * POSIX standard civil time representation.  Any adjustments due to time zone,
 * leap seconds, or a different epoch must be applied to @p time before
 * invoking this function.
 *
 * @note On systems where @c time_t has only 32 bits the range of
 * supported conversions is restricted to 1901-12-14T20:45:51 to
 * 2038-01-19T03:14:07.
 *
 * @param time the time represented as seconds.
 *
 * @return the time information for corresponding to the provided instant.
 */
struct tm time_civil_from_unix(time_t time);

#endif /* ZEPHYR_INCLUDE_MISC_CIVILTIME_H_ */
