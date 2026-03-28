/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <zephyr/sys/util.h>

#define DATE_STRING_BUF_SZ  26U
#define DATE_WDAY_STRING_SZ 7U
#define DATE_MON_STRING_SZ  12U
#define DATE_TM_YEAR_BASE   1900

static char *asctime_impl(const struct tm *tp, char *buf)
{
	static const char wday_str[DATE_WDAY_STRING_SZ][3] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
	};
	static const char mon_str[DATE_MON_STRING_SZ][3] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
	};

	if ((buf == NULL) || (tp == NULL) || ((unsigned int)tp->tm_wday >= DATE_WDAY_STRING_SZ) ||
	    ((unsigned int)tp->tm_mon >= DATE_MON_STRING_SZ)) {
		return NULL;
	}

	unsigned int n = (unsigned int)snprintf(
		buf, DATE_STRING_BUF_SZ, "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n", wday_str[tp->tm_wday],
		mon_str[tp->tm_mon], tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec,
		DATE_TM_YEAR_BASE + tp->tm_year);

	if (n >= DATE_STRING_BUF_SZ) {
		return NULL;
	}

	return buf;
}

char *asctime(const struct tm *tp)
{
	static char buf[DATE_STRING_BUF_SZ];

	return asctime_impl(tp, buf);
}

#if defined(CONFIG_COMMON_LIBC_ASCTIME_R)
char *asctime_r(const struct tm *tp, char *buf)
{
	return asctime_impl(tp, buf);
}
#endif /* CONFIG_COMMON_LIBC_ASCTIME_R */
