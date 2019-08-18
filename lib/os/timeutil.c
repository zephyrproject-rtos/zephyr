/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The time_days_from_civil function is derived directly from public
 * domain content written by Howard Hinnant and available at:
 * http://howardhinnant.github.io/date_algorithms.html#days_from_civil
 */

#include <zephyr/types.h>
#include <errno.h>
#include <sys/timeutil.h>

/** Convert a civil (proleptic Gregorian) date to days relative to
 * 1970-01-01.
 *
 * @param y the calendar year
 * @param m the calendar month, in the range [1, 12]
 * @param d the day of the month, in the range [1, last_day_of_month(y, m)]
 *
 * @return the signed number of days between the specified day and
 * 1970-01-01
 *
 * @see http://howardhinnant.github.io/date_algorithms.html#days_from_civil
 */
static s64_t time_days_from_civil(s64_t y,
				  unsigned int m,
				  unsigned int d)
{
	y -= m <= 2;

	s64_t era = (y >= 0 ? y : y - 399) / 400;
	unsigned int yoe = y - era * 400;
	unsigned int doy = (153U * (m + (m > 2 ? -3 : 9)) + 2U) / 5U + d;
	unsigned int doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;

	return era * 146097 + (time_t)doe - 719468;
}

s64_t timeutil_timegm64(const struct tm *tm)
{
	s64_t y = 1900 + (s64_t)tm->tm_year;
	unsigned int m = tm->tm_mon + 1;
	unsigned int d = tm->tm_mday - 1;
	s64_t ndays = time_days_from_civil(y, m, d);
	s64_t time = tm->tm_sec;

	time += 60LL * (tm->tm_min + 60LL * tm->tm_hour);
	time += 86400LL * ndays;

	return time;
}

time_t timeutil_timegm(const struct tm *tm)
{
	s64_t time = timeutil_timegm64(tm);
	time_t rv = (time_t)time;

	errno = 0;
	if ((sizeof(rv) == sizeof(s32_t))
	    && ((time < (s64_t)INT32_MIN)
		|| (time > (s64_t)INT32_MAX))) {
		errno = ERANGE;
		rv = -1;
	}
	return rv;
}
