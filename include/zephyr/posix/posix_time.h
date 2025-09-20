/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_POSIX_POSIX_TIME_H_
#define ZEPHYR_INCLUDE_ZEPHYR_POSIX_POSIX_TIME_H_

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)

#include <stddef.h>

#include <zephyr/sys/clock.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/* clock_t must be defined in the libc time.h */
/* size_t must be defined in the libc stddef.h */
/* time_t must be defined in the libc time.h */

#if !defined(_CLOCKID_T_DECLARED) && !defined(__clockid_t_defined)
typedef unsigned long clockid_t;
#define _CLOCKID_T_DECLARED
#define __clockid_t_defined
#endif

#if !defined(_TIMER_T_DECLARED) && !defined(__timer_t_defined)
typedef unsigned long timer_t;
#define _TIMER_T_DECLARED
#define __timer_t_defined
#endif

#if !defined(_LOCALE_T_DECLARED) && !defined(__locale_t_defined)
#ifdef CONFIG_NEWLIB_LIBC
struct __locale_t;
typedef struct __locale_t *locale_t;
#else
typedef void *locale_t;
#endif
#define _LOCALE_T_DECLARED
#define __locale_t_defined
#endif

#if !defined(_PID_T_DECLARED) && !defined(__pid_t_defined)
typedef int pid_t;
#define _PID_T_DECLARED
#define __pid_t_defined
#endif

#if defined(_POSIX_REALTIME_SIGNALS)
struct sigevent;
#endif

/* struct tm must be defined in the libc time.h */

#if __STDC_VERSION__ >= 201112L
/* struct timespec must be defined in the libc time.h */
#else
#if !defined(_TIMESPEC_DECLARED) && !defined(__timespec_defined)
typedef struct {
	time_t tv_sec;
	long tv_nsec;
} timespec_t;
#define _TIMESPEC_DECLARED
#define __timespec_defined
#endif
#endif

#if !defined(_ITIMERSPEC_DECLARED) && !defined(__itimerspec_defined)
struct itimerspec {
	struct timespec it_interval;
	struct timespec it_value;
};
#define _ITIMERSPEC_DECLARED
#define __itimerspec_defined
#endif

/* NULL must be defined in the libc stddef.h */

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME ((clockid_t)SYS_CLOCK_REALTIME)
#endif

#ifndef CLOCKS_PER_SEC
#if defined(_XOPEN_SOURCE)
#define CLOCKS_PER_SEC 1000000
#else
#define CLOCKS_PER_SEC CONFIG_SYS_CLOCK_TICKS_PER_SEC
#endif
#endif

#if defined(_POSIX_CPUTIME) || defined(__DOXYGEN__)
#ifndef CLOCK_PROCESS_CPUTIME_ID
#define CLOCK_PROCESS_CPUTIME_ID ((clockid_t)2)
#endif
#endif

#if defined(_POSIX_THREAD_CPUTIME) || defined(__DOXYGEN__)
#ifndef CLOCK_THREAD_CPUTIME_ID
#define CLOCK_THREAD_CPUTIME_ID ((clockid_t)3)
#endif
#endif

#if defined(_POSIX_MONOTONIC_CLOCK) || defined(__DOXYGEN__)
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC ((clockid_t)SYS_CLOCK_MONOTONIC)
#endif
#endif

#ifndef TIMER_ABSTIME
#define TIMER_ABSTIME SYS_TIMER_ABSTIME
#endif

/* asctime() must be declared in the libc time.h */
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(__DOXYGEN__)
char *asctime_r(const struct tm *ZRESTRICT tm, char *ZRESTRICT buf);
#endif
/* clock() must be declared in the libc time.h */
#if defined(_POSIX_CPUTIME) || defined(__DOXYGEN__)
int clock_getcpuclockid(pid_t pid, clockid_t *clock_id);
#endif
#if defined(_POSIX_TIMERS) || defined(__DOXYGEN__)
int clock_getres(clockid_t clock_id, struct timespec *ts);
int clock_gettime(clockid_t clock_id, struct timespec *ts);
#endif
#if defined(_POSIX_CLOCK_SELECTION) || defined(__DOXYGEN__)
int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *rqtp,
		    struct timespec *rmtp);
#endif
#if defined(_POSIX_TIMERS) || defined(__DOXYGEN__)
int clock_settime(clockid_t clock_id, const struct timespec *ts);
#endif
/* ctime() must be declared in the libc time.h */
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(__DOXYGEN__)
char *ctime_r(const time_t *clock, char *buf);
#endif
/* difftime() must be declared in the libc time.h */
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
struct tm *getdate(const char *string);
#endif
/* gmtime() must be declared in the libc time.h */
#if __STDC_VERSION__ >= 202311L
/* gmtime_r() must be declared in the libc time.h */
#else
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(__DOXYGEN__)
struct tm *gmtime_r(const time_t *ZRESTRICT timer, struct tm *ZRESTRICT result);
#endif
#endif
/* localtime() must be declared in the libc time.h */
#if __STDC_VERSION__ >= 202311L
/* localtime_r() must be declared in the libc time.h */
#else
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(__DOXYGEN__)
struct tm *localtime_r(const time_t *ZRESTRICT timer, struct tm *ZRESTRICT result);
#endif
#endif
/* mktime() must be declared in the libc time.h */
#if defined(_POSIX_TIMERS) || defined(__DOXYGEN__)
int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);
#endif
/* strftime() must be declared in the libc time.h */
size_t strftime_l(char *ZRESTRICT s, size_t maxsize, const char *ZRESTRICT format,
		  const struct tm *ZRESTRICT timeptr, locale_t locale);
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
char *strptime(const char *ZRESTRICT s, const char *ZRESTRICT format, struct tm *ZRESTRICT tm);
#endif
/* time() must be declared in the libc time.h */
#if defined(_POSIX_TIMERS) || defined(__DOXYGEN__)
int timer_create(clockid_t clockId, struct sigevent *ZRESTRICT evp, timer_t *ZRESTRICT timerid);
int timer_delete(timer_t timerid);
int timer_getoverrun(timer_t timerid);
int timer_gettime(timer_t timerid, struct itimerspec *its);
int timer_settime(timer_t timerid, int flags, const struct itimerspec *value,
		  struct itimerspec *ovalue);
#endif

#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
extern int daylight;
extern long timezone;
#endif

extern char *tzname[];

#ifdef __cplusplus
}
#endif

#endif /* defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__) */

#endif /* ZEPHYR_INCLUDE_ZEPHYR_POSIX_POSIX_TIME_H_ */
