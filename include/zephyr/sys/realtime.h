/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_REALTIME_H_
#define ZEPHYR_INCLUDE_SYS_REALTIME_H_

#include <zephyr/kernel.h>

struct sys_datetime {
	int tm_sec;	/**< Seconds [0, 59] */
	int tm_min;	/**< Minutes [0, 59] */
	int tm_hour;	/**< Hours [0, 23] */
	int tm_mday;	/**< Day of the month [1, 31] */
	int tm_mon;	/**< Month [0, 11] */
	int tm_year;	/**< Year - 1900 */
	int tm_wday;	/**< Day of the week [0, 6] (Sunday = 0) (Unknown = -1) */
	int tm_yday;	/**< Day of the year [0, 365] (Unknown = -1) */
	int tm_isdst;	/**< Daylight saving time flag [-1] (Unknown = -1) */
	int tm_nsec;	/**< Nanoseconds [0, 999999999] (Unknown = 0) */
};

/** Get universal coordinated time unix timestamp in milliseconds */
__syscall int sys_realtime_get_timestamp(int64_t *timestamp_ms);
/** Set universal coordinated time unix timestamp in milliseconds */
__syscall int sys_realtime_set_timestamp(const int64_t *timestamp_ms);

/** Get universal coordinated time datetime */
__syscall int sys_realtime_get_datetime(struct sys_datetime *datetime);
/** Set universal coordinated time datetime */
__syscall int sys_realtime_set_datetime(const struct sys_datetime *datetime);

/** Convert universal coordinated time datetime to unix timestamp in milliseconds */
int sys_realtime_datetime_to_timestamp(int64_t *timestamp_ms, const struct sys_datetime *datetime);
/** Convert universal coordinated time unix timestamp in milliseconds to datetime */
int sys_realtime_timestamp_to_datetime(struct sys_datetime *datetime, const int64_t *timestamp_ms);

#include <zephyr/syscalls/realtime.h>

#endif /* ZEPHYR_INCLUDE_SYS_REALTIME_H_ */
