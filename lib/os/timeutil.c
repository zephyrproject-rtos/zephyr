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

/** Convert civil time to UNIX time.
 *
 * @param tvp pointer to a civil time structure.  `tm_year`, `tm_mon`,
 * `tm_mday`, `tm_hour`, `tm_min`, and `tm_sec` must be valid.  All
 * other fields are ignored.
 *
 * @return the signed number of seconds between 1970-01-01T00:00:00
 * and the specified time ignoring leap seconds and DST offsets.
 */
time_t timeutil_timegm(struct tm *tm)
{
	s64_t y = 1900 + (s64_t)tm->tm_year;
	unsigned int m = tm->tm_mon + 1;
	unsigned int d = tm->tm_mday - 1;
	s64_t ndays = time_days_from_civil(y, m, d);

	return (time_t)tm->tm_sec
	       + 60 * (tm->tm_min + 60 * tm->tm_hour)
	       + 86400 * ndays;
}
