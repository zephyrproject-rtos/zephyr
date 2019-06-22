/*
 * Copyright (c) 2019, Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/civiltime.h>

/* http://howardhinnant.github.io/date_algorithms.html#days_from_civil */
s64_t time_days_from_civil(s64_t y,
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

time_t time_unix_from_civil(const struct tm *tvp)
{
	s64_t y = 1900 + (s64_t)tvp->tm_year;
	unsigned int m = tvp->tm_mon + 1;
	unsigned int d = tvp->tm_mday - 1;
	s64_t ndays = time_days_from_civil(y, m, d);

	return (time_t)tvp->tm_sec
	       + 60 * (tvp->tm_min + 60 * tvp->tm_hour)
	       + 86400 * ndays;
}

/* http://howardhinnant.github.io/date_algorithms.html#civil_from_days */
struct tm time_civil_from_days(s64_t z)
{
	struct tm tv = { 0 };

	tv.tm_wday = (z >= -4) ? ((z + 4) % 7) : ((z + 5) % 7 + 6);
	z += 719468;

	s64_t era = ((z >= 0) ? z : (z - 146096)) / 146097;
	unsigned int doe = (z - era * (s64_t)146097);
	unsigned int yoe = (doe - doe / 1460U + doe / 36524U
			    - doe / 146096U) / 365U;
	s64_t y = (time_t)yoe + era * 400;
	unsigned int doy = doe - (365U * yoe + yoe / 4U - yoe / 100U);
	unsigned int mp = (5U * doy + 2U) / 153U;
	unsigned int d = doy - (153U * mp + 2U) / 5U + 1U;
	unsigned int m = mp + ((mp < 10) ? 3 : -9);

	tv.tm_year = y + (m <= 2) - 1900;
	tv.tm_mon = m - 1;
	tv.tm_mday = d;

	/* Everything above is explained on the date algorithms page,
	 * but doy is relative to --03-01 and we need it relative to
	 * --01-01.
	 *
	 * doy=306 corresponds to --01-01, doy=364 to --02-28, and
	 * doy=365 to --02-29.  So we can just subtract 306 to handle
	 * January and February.
	 *
	 * For doy<306 we have to add the number of days before
	 * --03-01, which is 59 in a common year and 60 in a leap
	 * year.  Note that the first year in the era is a leap
	 * year.
	 */
	if (doy >= 306U) {
		tv.tm_yday = doy - 306U;
	} else {
		tv.tm_yday = doy + 59U +
			     (((yoe % 4U == 0U) && (yoe % 100U != 0U))
			      || (yoe == 0U));
	}

	return tv;
}

struct tm time_civil_from_unix(time_t z)
{
	s64_t days = (z >= 0 ? z : z - 86399) / 86400;
	struct tm tv = time_civil_from_days(days);
	unsigned int rem = z - days * 86400;

	tv.tm_hour = rem / 60U / 60U;
	rem -= tv.tm_hour * 60 * 60;
	tv.tm_min = rem / 60U;
	tv.tm_sec = rem - tv.tm_min * 60;

	return tv;
}
