/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_TIME_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_TIME_H_

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The <time.h> header shall define the clock_t, size_t, time_t, types as described in
 * <sys/types.h>. */

#if (!defined(_CLOCK_T_DECLARED) && !defined(__clock_t_defined)) || defined(__DOXYGEN__)
typedef unsigned long clock_t;
#define _CLOCK_T_DECLARED
#define __clock_t_defined
#endif

/* size_t */
#include <stddef.h>

/* _TIME_T_, _SUSECONDS_T_*/
#include <sys/_types.h>

#if (!defined(_TIME_T_DECLARED) && !defined(__time_t_defined)) || defined(__DOXYGEN__)
typedef _TIME_T_ time_t;
#define _TIME_T_DECLARED
#define __time_t_defined
#endif

#if !defined(__suseconds_t_defined)
#define __suseconds_t_defined
typedef _SUSECONDS_T_ suseconds_t;
#endif

/* The <time.h> header shall define the clockid_t and timer_t types as described in <sys/types.h>.
 */

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)

#if (!defined(_CLOCKID_T_DECLARED) && !defined(__clockid_t_defined)) || defined(__DOXYGEN__)
typedef unsigned long clockid_t;
#define _CLOCKID_T_DECLARED
#define __clockid_t_defined
#endif

#if (!defined(_TIMER_T_DECLARED) && !defined(__timer_t_defined)) || defined(__DOXYGEN__)
typedef unsigned long timer_t;
#define _TIMER_T_DECLARED
#define __timer_t_defined
#endif

#endif

/* The <time.h> header shall define the locale_t type as described in <locale.h>. */
/* #include <locale.h> */
#if (!defined(_LOCALE_T_DECLARED) && !defined(__locale_t_defined)) || defined(__DOXYGEN__)

struct locale_s {
	char __unused: 1;
};

typedef struct locale_s locale_t;
#define _LOCALE_T_DECLARED
#define __locale_t_defined
#endif

/* The <time.h> header shall define the pid_t type as described in <sys/types.h>. */

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)

#if (!defined(_PID_T_DECLARED) && !defined(__pid_t_defined)) || defined(__DOXYGEN__)
typedef long pid_t;
#define _PID_T_DECLARED
#define __pid_t_defined
#endif

#endif

/* The tag sigevent shall be declared as naming an incomplete structure type, the contents of which
 * are described in the <signal.h> header. */

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)
struct sigevent;
#endif

/* The <time.h> header shall declare the tm structure */

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

/* The <time.h> header shall declare the timespec structure */
#include <sys/_timespec.h>

/* The <time.h> header shall also declare the itimerspec structure */

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)

#if defined(_POSIX_TIMERS) || defined(__DOXYGEN__)
struct itimer_spec {
	struct timespec it_interval; /* Timer interval */
	struct timespec it_value;    /* Timer expiration */
};
#endif

#endif

/* The <time.h> header shall define the following macros: NULL (stddef.h), CLOCKS_PER_SEC [XSI] */

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)

#define CLOCKS_PER_SEC 1000000

#endif

/* The <time.h> header shall define the following symbolic constants. The values shall have a type
 * that is assignment-compatible with clockid_t. CLOCK_MONOTONIC [MON], CLOCK_PROCESS_CPUTIME_ID
 * [CPT], CLOCK_REALTIME [CX], CLOCK_THREAD_CPUTIME_ID [TCT]
 */

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)

#include <zephyr/sys/clock.h>

#if defined(_POSIX_MONOTONIC_CLOCK) || defined(__DOXYGEN__)
#define CLOCK_MONOTONIC ((clockid_t)SYS_CLOCK_MONOTONIC)
#endif

#if defined(_POSIX_CPUTIME) || defined(__DOXYGEN__)
#define CLOCK_PROCESS_CPUTIME_ID ((clockid_t)2)
#endif

#if defined(_POSIX_TIMERS) || defined(__DOXYGEN__)
/* moved from timers to base in issue 7 (_POSIX_TIMERS part of base) */
#define CLOCK_REALTIME ((clockid_t)SYS_CLOCK_REALTIME)
#endif

#if defined(_POSIX_THREAD_CPUTIME) || defined(__DOXYGEN__)
#define CLOCK_THREAD_CPUTIME_ID ((clockid_t)3)
#endif

#endif

/* The <time.h> header shall define the following symbolic constant */

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)

#include <zephyr/sys/clock.h>

#if defined(_POSIX_TIMERS) || defined(__DOXYGEN__)
/* moved from timers to base in issue 7 (_POSIX_TIMERS part of base) */
#define TIMER_ABSTIME SYS_TIMER_ABSTIME
#endif

#endif

/* The <time.h> header shall provide a declaration or definition for getdate_err. */
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
extern int getdate_err;
#endif

/* The following shall be declared as functions and may also be defined as macros. Function
 * prototypes shall be provided. */

char *asctime(const struct tm *tp);

#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || (_POSIX_C_SOURCE >= 200809L) || defined(__DOXYGEN__)
/* moved from thread safe to base in issue 7 (_POSIX_THREAD_SAFE_FUNCTIONS part of base) */
char *asctime_r(const struct tm *ZRESTRICT tp, char *ZRESTRICT buf);
#endif

clock_t clock(void);

#if defined(_POSIX_CPUTIME) || defined(__DOXYGEN__)
int clock_getcpuclockid(pid_t pid, clockid_t *clock_id);
#endif

#if defined(_POSIX_TIMERS) || defined(__DOXYGEN__)
/* moved from timers to base in issue 7 (_POSIX_TIMERS part of base) */
int clock_getres(clockid_t clock_id, struct timespec *);
int clock_gettime(clockid_t clock_id, struct timespec *);
#endif

#if defined(_POSIX_CLOCK_SELECTION) || defined(__DOXYGEN__)
int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *req,
		    struct timespec *rem);
#endif

#if defined(_POSIX_TIMERS) || defined(__DOXYGEN__)
/* moved from timers to base in issue 7 (_POSIX_TIMERS part of base) */
int clock_settime(clockid_t clock_id, const struct timespec *tp);
#endif

char *ctime(const time_t *timer);

#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || (_POSIX_C_SOURCE >= 200809L) || defined(__DOXYGEN__)
/* moved from thread safe to base in issue 7 (_POSIX_THREAD_SAFE_FUNCTIONS part of base) */
char *ctime_r(const time_t *, char *);
#endif

double difftime(time_t, time_t);

#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
struct tm *getdate(const char *string);
#endif

struct tm *gmtime(const time_t *timer);

#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || (_POSIX_C_SOURCE >= 200809L) || defined(__DOXYGEN__)
/* also visible in C23 */
struct tm *gmtime_r(const time_t *ZRESTRICT timep, struct tm *ZRESTRICT result);
#endif

struct tm *localtime(const time_t *timer);

#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || (_POSIX_C_SOURCE >= 200809L) || defined(__DOXYGEN__)
struct tm *localtime_r(const time_t *ZRESTRICT timer, struct tm *ZRESTRICT result);
#endif

time_t mktime(struct tm *arg);

#if defined(_POSIX_TIMERS) || defined(__DOXYGEN__)
/* moved from timers to base in issue 7 (_POSIX_TIMERS part of base) */
int nanosleep(const struct timespec *req, struct timespec *rem);
#endif

size_t strftime(char *ZRESTRICT s, size_t maxsize, const char *ZRESTRICT format,
		const struct tm *ZRESTRICT timeptr);

/*
 * FIXME: needs locale.h or locale_t
 * size_t strftime_l(char *restrict, size_t, const char *restrict, const struct tm *restrict,
 * locale_t);
 */

#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
char *strptime(const char *ZRESTRICT s, const char *ZRESTRICT format, struct tm *ZRESTRICT tm);
#endif

time_t time(time_t *tloc);

#if defined(_POSIX_TIMERS) || defined(__DOXYGEN__)
/* moved from timers to base in issue 7 (_POSIX_TIMERS part of base) */
int timer_create(clockid_t clock_id, struct sigevent *ZRESTRICT evp, timer_t *ZRESTRICT timer_id);
int timer_delete(timer_t timer_id);
int timer_getoverrun(timer_t timer_id);
int timer_gettime(timer_t timer_id, struct itimerspec *ZRESTRICT value);
int timer_settime(timer_t timer_id, int flags, const struct itimerspec *ZRESTRICT value,
		  struct itimerspec *ZRESTRICT old_value);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDIO_H_ */
