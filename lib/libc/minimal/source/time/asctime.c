/*
 * Copyright (c) 2021 Sun Amar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <bits/restrict.h>

static char *z_impl_asctime(const struct tm *timeptr, char *buf, size_t buflen)
{
	static char wday_name[7][3] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static char mon_name[12][3] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	if (timeptr == NULL) {
		errno = EINVAL;
		return NULL;
	}

	sprintf(buf, "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n",
		wday_name[timeptr->tm_wday],
		mon_name[timeptr->tm_mon],
		timeptr->tm_mday,
		timeptr->tm_hour,
		timeptr->tm_min,
		timeptr->tm_sec,
		1900 + timeptr->tm_year);
	return buf;
}

/*
 * same as asctime but put it into user supplied buffer.
 * according to standard the buffer should be at least 26 bytes.
 */
char *asctime_r(const struct tm *timeptr, char *buf)
{
	return z_impl_asctime(timeptr, buf, MINLIBC_ASCTIME_MIN_BUF_SIZE_REQUIRED);
}

char *asctime(const struct tm *timeptr)
{
	static char result[MINLIBC_ASCTIME_MIN_BUF_SIZE_REQUIRED];

	return z_impl_asctime(timeptr, result, MINLIBC_ASCTIME_MIN_BUF_SIZE_REQUIRED);
}
