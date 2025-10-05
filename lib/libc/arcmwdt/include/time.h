/*
 * Copyright (c) 2024 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_LIBC_ARCMWDT_INCLUDE_TIME_H_
#define LIB_LIBC_ARCMWDT_INCLUDE_TIME_H_

#include_next <time.h>

#if defined(_POSIX_C_SOURCE)
/*
 * POSIX requires time.h to define pid_t and clockid_t
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/time.h.html
 */
#if !defined(_PID_T_DECLARED)
typedef long pid_t;
#define _PID_T_DECLARED
#endif

#if !defined(_CLOCKID_T_DECLARED)
typedef int clockid_t;
#define _CLOCKID_T_DECLARED
#endif
#endif

#define _TIMESPEC_DECLARED

#include <zephyr/posix/posix_time.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char *asctime_r(const struct tm *tp, char *buf);
extern char *ctime_r(const time_t *clock, char *buf);
extern struct tm *gmtime_r(const time_t *timep, struct tm *result);
extern struct tm *localtime_r(const time_t *timer, struct tm *result);

#ifdef __cplusplus
}
#endif

#endif /* LIB_LIBC_ARCMWDT_INCLUDE_TIME_H_ */
