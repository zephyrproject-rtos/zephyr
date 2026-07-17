/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Time types.
 * @ingroup posix
 *
 * Provides clock identifiers, the POSIX timer API, thread-safe time conversion
 * helpers, and locale-aware time formatting that extend the C standard time.h
 * interface.
 *
 * @posix_header{time.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_POSIX_TIME_H_
#define ZEPHYR_INCLUDE_POSIX_POSIX_TIME_H_

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
/**
 * @brief Identifies a clock (e.g. @c CLOCK_REALTIME, @c CLOCK_MONOTONIC).
 */
typedef unsigned long clockid_t;
/** @cond INTERNAL_HIDDEN */
#define _CLOCKID_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __clockid_t_defined
/** @endcond */
#endif

#if !defined(_TIMER_T_DECLARED) && !defined(__timer_t_defined)
/**
 * @brief Opaque handle for a POSIX interval timer created with timer_create().
 */
typedef unsigned long timer_t;
/** @cond INTERNAL_HIDDEN */
#define _TIMER_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __timer_t_defined
/** @endcond */
#endif

#if !defined(_LOCALE_T_DECLARED) && !defined(__locale_t_defined)
#ifdef CONFIG_NEWLIB_LIBC
struct __locale_t;

/**
 * @brief Opaque locale object.
 */
typedef struct __locale_t *locale_t;
#else
/**
 * @brief Opaque locale object.
 */
typedef void *locale_t;
#endif
/** @cond INTERNAL_HIDDEN */
#define _LOCALE_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __locale_t_defined
/** @endcond */
#endif

#if !defined(_PID_T_DECLARED) && !defined(__pid_t_defined)
/**
 * @brief Process ID type.
 */
typedef int pid_t;
/** @cond INTERNAL_HIDDEN */
#define _PID_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __pid_t_defined
/** @endcond */
#endif

#if defined(_POSIX_REALTIME_SIGNALS)
struct sigevent;
#endif

/* struct tm must be defined in the libc time.h */

#if __STDC_VERSION__ >= 201112L
/* struct timespec must be defined in the libc time.h */
#else
#if !defined(_TIMESPEC_DECLARED) && !defined(__timespec_defined)
/**
 * @brief Time interval in seconds and nanoseconds.
 */
struct timespec {
	/**
	 * @brief Seconds component.
	 */
	time_t tv_sec;
	/**
	 * @brief Nanoseconds component.
	 */
	long tv_nsec;
};
/** @cond INTERNAL_HIDDEN */
#define _TIMESPEC_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __timespec_defined
/** @endcond */
#endif
#endif

#if !defined(_ITIMERSPEC_DECLARED) && !defined(__itimerspec_defined)
/**
 * @brief Interval timer specification.
 */
struct itimerspec {
	/**
	 * @brief Timer period.
	 */
	struct timespec it_interval;
	/**
	 * @brief Timer expiration.
	 */
	struct timespec it_value;
};
/** @cond INTERNAL_HIDDEN */
#define _ITIMERSPEC_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __itimerspec_defined
/** @endcond */
#endif

/* NULL must be defined in the libc stddef.h */

#ifndef CLOCK_REALTIME
/**
 * @brief The identifier of the system-wide clock measuring real time.
 */
#define CLOCK_REALTIME ((clockid_t)SYS_CLOCK_REALTIME)
#endif

#ifndef CLOCKS_PER_SEC
#if defined(_XOPEN_SOURCE)
/**
 * @brief A number used to convert the value returned by the clock() function into seconds.
 */
#define CLOCKS_PER_SEC 1000000
#else
/**
 * @brief A number used to convert the value returned by the clock() function into seconds.
 */
#define CLOCKS_PER_SEC CONFIG_SYS_CLOCK_TICKS_PER_SEC
#endif
#endif

#if defined(_POSIX_CPUTIME) || defined(__DOXYGEN__)
#ifndef CLOCK_PROCESS_CPUTIME_ID
/**
 * @brief The identifier of the CPU-time clock associated with the process making a clock() or
 * timer*() function call.
 */
#define CLOCK_PROCESS_CPUTIME_ID ((clockid_t)2)
#endif
#endif

#if defined(_POSIX_THREAD_CPUTIME) || defined(__DOXYGEN__)
#ifndef CLOCK_THREAD_CPUTIME_ID
/**
 * @brief The identifier of the CPU-time clock associated with the thread making a clock() or
 * timer*() function call.
 */
#define CLOCK_THREAD_CPUTIME_ID ((clockid_t)3)
#endif
#endif

#if defined(_POSIX_MONOTONIC_CLOCK) || defined(__DOXYGEN__)
#ifndef CLOCK_MONOTONIC
/**
 * @brief The identifier for the system-wide monotonic clock, which is defined as a clock measuring
 * real time, whose value cannot be set via clock_settime() and which cannot have negative clock
 * jumps.
 */
#define CLOCK_MONOTONIC ((clockid_t)SYS_CLOCK_MONOTONIC)
#endif
#endif

#ifndef TIMER_ABSTIME
/**
 * @brief Flag indicating time is absolute.
 */
#define TIMER_ABSTIME SYS_TIMER_ABSTIME
#endif

/**
 * @brief Convert date and time to a string.
 *
 * @posix_func{asctime}
 */
/* asctime() must be declared in the libc time.h */
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(__DOXYGEN__)
/**
 * @brief Convert broken-down time to a string (thread-safe version of asctime()).
 *
 * @param tm  Broken-down time to convert.
 * @param buf Caller-supplied buffer of at least 26 bytes.
 *
 * @return @p buf on success, or NULL on failure.
 *
 * @posix_func{asctime_r}
 */
char *asctime_r(const struct tm *ZRESTRICT tm, char *ZRESTRICT buf);
#endif

/**
 * @brief Report CPU time used.
 *
 * @posix_func{clock}
 */
/* clock() must be declared in the libc time.h */
#if defined(_POSIX_CPUTIME) || defined(__DOXYGEN__)
/**
 * @brief Access a process CPU-time clock.
 *
 * @param pid      Process ID (0 means the calling process).
 * @param clock_id Output: CPU-time clock ID of the process.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{clock_getcpuclockid}
 */
int clock_getcpuclockid(pid_t pid, clockid_t *clock_id);
#endif
#if defined(_POSIX_TIMERS) || defined(__DOXYGEN__)
/**
 * @brief Get the resolution of a clock.
 *
 * @param clock_id Clock to query.
 * @param ts       Output: resolution (smallest representable interval) of the clock.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{clock_getres}
 */
int clock_getres(clockid_t clock_id, struct timespec *ts);

/**
 * @brief Get the current time of a clock.
 *
 * @param clock_id Clock to read.
 * @param ts       Output: current time of the clock.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{clock_gettime}
 */
int clock_gettime(clockid_t clock_id, struct timespec *ts);
#endif
#if defined(_POSIX_CLOCK_SELECTION) || defined(__DOXYGEN__)
/**
 * @brief High resolution sleep with specifiable clock.
 *
 * @param clock_id Clock to measure the sleep against.
 * @param flags    0 for a relative sleep, @c TIMER_ABSTIME for an absolute wakeup time.
 * @param rqtp     Requested sleep duration or absolute wakeup time.
 * @param rmtp     Output: remaining time if interrupted (only for relative sleeps), or NULL.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{clock_nanosleep}
 */
int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *rqtp,
		    struct timespec *rmtp);
#endif
#if defined(_POSIX_TIMERS) || defined(__DOXYGEN__)
/**
 * @brief Set the time of a clock.
 *
 * @param clock_id Clock to set.
 * @param ts       New time value.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{clock_settime}
 */
int clock_settime(clockid_t clock_id, const struct timespec *ts);
#endif

/**
 * @brief Convert a time value to a date and time string.
 *
 * @posix_func{ctime}
 */
/* ctime() must be declared in the libc time.h */
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(__DOXYGEN__)
/**
 * @brief Convert a time value to a date and time string (thread-safe version of ctime()).
 *
 * @param clock Pointer to the time value to convert.
 * @param buf   Caller-supplied buffer of at least 26 bytes.
 *
 * @return @p buf on success, or NULL on failure.
 *
 * @posix_func{ctime_r}
 */
char *ctime_r(const time_t *clock, char *buf);
#endif

/**
 * @brief Compute the difference between two calendar time values.
 *
 * @posix_func{difftime}
 */
/* difftime() must be declared in the libc time.h */
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
/**
 * @brief Convert user format date and time.
 *
 * @param string Date and time string; the format is given by the DATEMSK environment variable.
 *
 * @return Pointer to a broken-down time on success, or NULL on failure.
 *
 * @posix_func{getdate}
 */
struct tm *getdate(const char *string);
#endif

/**
 * @brief Convert a time value to a broken-down UTC time.
 *
 * @posix_func{gmtime}
 */
/* gmtime() must be declared in the libc time.h */
#if __STDC_VERSION__ >= 202311L
/**
 * @brief Convert a time value to a broken-down UTC time.
 *
 * @posix_func{gmtime_r}
 */
/* gmtime_r() must be declared in the libc time.h */
#else
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(__DOXYGEN__)
/**
 * @brief Convert a time value to a broken-down UTC time.
 *
 * @param timer  Pointer to the time value to convert.
 * @param result Caller-supplied storage for the broken-down time.
 *
 * @return @p result on success, or NULL on failure.
 *
 * @posix_func{gmtime_r}
 */
struct tm *gmtime_r(const time_t *ZRESTRICT timer, struct tm *ZRESTRICT result);
#endif
#endif

/**
 * @brief Convert a time value to a broken-down local time.
 *
 * @posix_func{localtime}
 */
/* localtime() must be declared in the libc time.h */
#if __STDC_VERSION__ >= 202311L
/**
 * @brief Convert a time value to a broken-down local time.
 *
 * @posix_func{localtime_r}
 */
/* localtime_r() must be declared in the libc time.h */
#else
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(__DOXYGEN__)
/**
 * @brief Convert a time value to a broken-down local time.
 *
 * @param timer  Pointer to the time value to convert.
 * @param result Caller-supplied storage for the broken-down time.
 *
 * @return @p result on success, or NULL on failure.
 *
 * @posix_func{localtime_r}
 */
struct tm *localtime_r(const time_t *ZRESTRICT timer, struct tm *ZRESTRICT result);
#endif
#endif

/**
 * @brief Convert broken-down time into time since the Epoch.
 *
 * @posix_func{mktime}
 */
/* mktime() must be declared in the libc time.h */
#if defined(_POSIX_TIMERS) || defined(__DOXYGEN__)
/**
 * @brief High resolution sleep.
 *
 * @param rqtp Requested sleep duration.
 * @param rmtp Output: remaining time if the call was interrupted, or NULL.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{nanosleep}
 */
int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);
#endif

/**
 * @brief Convert date and time to a string.
 *
 * @posix_func{strftime}
 */
/* strftime() must be declared in the libc time.h */
/**
 * @brief Format broken-down time into a string using the specified locale.
 *
 * @param s       Output buffer.
 * @param maxsize Maximum number of bytes to write, including the terminating null byte.
 * @param format  Format string (same syntax as strftime()).
 * @param timeptr Broken-down time to format.
 * @param locale  Locale to use for formatting.
 *
 * @return Number of bytes written, not including the terminating null byte, or 0 if @p maxsize
 *         would have been exceeded.
 *
 * @posix_func{strftime_l}
 */
size_t strftime_l(char *ZRESTRICT s, size_t maxsize, const char *ZRESTRICT format,
		  const struct tm *ZRESTRICT timeptr, locale_t locale);
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
/**
 * @brief Parse a date and time string according to a format.
 *
 * @param s      Input string to parse.
 * @param format Format string describing the date and time fields to parse.
 * @param tm     Output: broken-down time filled in from the parsed fields.
 *
 * @return Pointer to the first character in @p s not consumed by parsing, or NULL on failure.
 *
 * @posix_func{strptime}
 */
char *strptime(const char *ZRESTRICT s, const char *ZRESTRICT format, struct tm *ZRESTRICT tm);
#endif

/**
 * @brief Get time.
 *
 * @posix_func{time}
 */
/* time() must be declared in the libc time.h */
#if defined(_POSIX_TIMERS) || defined(__DOXYGEN__)
/**
 * @brief Create a per-process timer.
 *
 * @param clockId Clock to base the timer on.
 * @param evp     Notification specification, or NULL for @c SIGALRM delivery.
 * @param timerid Output: ID of the new timer.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{timer_create}
 */
int timer_create(clockid_t clockId, struct sigevent *ZRESTRICT evp, timer_t *ZRESTRICT timerid);

/**
 * @brief Delete a per-process timer.
 *
 * @param timerid Timer to delete.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{timer_delete}
 */
int timer_delete(timer_t timerid);

/**
 * @brief Get the number of timer overruns since the last timer expiration notification.
 *
 * @param timerid Timer to query.
 *
 * @return Overrun count on success, or -1 with errno set on failure.
 *
 * @posix_func{timer_getoverrun}
 */
int timer_getoverrun(timer_t timerid);

/**
 * @brief Get the time remaining until the next timer expiration.
 *
 * @param timerid Timer to query.
 * @param its     Output: current timer value and interval.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{timer_gettime}
 */
int timer_gettime(timer_t timerid, struct itimerspec *its);

/**
 * @brief Arm or disarm a per-process timer.
 *
 * @param timerid Timer to set.
 * @param flags   0 for a relative time, @c TIMER_ABSTIME for an absolute time.
 * @param value   New expiration time and interval; set it_value to zero to disarm.
 * @param ovalue  Output: previous timer value, or NULL.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{timer_settime}
 */
int timer_settime(timer_t timerid, int flags, const struct itimerspec *value,
		  struct itimerspec *ovalue);
#endif

#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
/**
 * @brief Daylight saving time flag.
 */
extern int daylight;

/**
 * @brief Seconds west of UTC.
 */
extern long timezone;
#endif

/**
 * @brief Timezone name strings.
 */
extern char *tzname[];

#ifdef __cplusplus
}
#endif

#endif /* defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__) */

#endif /* ZEPHYR_INCLUDE_POSIX_POSIX_TIME_H_ */
