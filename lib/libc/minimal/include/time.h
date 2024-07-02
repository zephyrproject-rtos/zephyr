/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_TIME_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_TIME_H_

#include <stdint.h>
#include <zephyr/toolchain.h>
#include <sys/_types.h>
#include <sys/_timespec.h>

/* Minimal time.h to fulfill the requirements of certain libraries
 * like mbedTLS and to support time APIs.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

#if !defined(__time_t_defined)
#define __time_t_defined
typedef _TIME_T_ time_t;
#endif

#if !defined(__suseconds_t_defined)
#define __suseconds_t_defined
typedef _SUSECONDS_T_ suseconds_t;
#endif

/*
 * Conversion between civil time and UNIX time.  The companion
 * localtime() and inverse mktime() are not provided here since they
 * require access to time zone information.
 */
struct tm *gmtime(const time_t *timep);
struct tm *gmtime_r(const time_t *ZRESTRICT timep,
		    struct tm *ZRESTRICT result);
char *asctime(const struct tm *timeptr);
struct tm *localtime(const time_t *timer);
struct tm *localtime_r(const time_t *ZRESTRICT timer, struct tm *ZRESTRICT result);
char *ctime(const time_t *clock);

#if defined(CONFIG_COMMON_LIBC_ASCTIME_R) || defined(__DOXYGEN__)
char *asctime_r(const struct tm *ZRESTRICT tp, char *ZRESTRICT buf);
#endif /* CONFIG_COMMON_LIBC_ASCTIME_R */

#if defined(CONFIG_COMMON_LIBC_CTIME_R) || defined(__DOXYGEN__)
char *ctime_r(const time_t *clock, char *buf);
#endif /* CONFIG_COMMON_LIBC_CTIME_R */

time_t time(time_t *tloc);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDIO_H_ */
