/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Time types.
 * @ingroup posix
 *
 * Provides the timeval structure and the gettimeofday() function for obtaining
 * the current time with microsecond resolution.
 *
 * @posix_header{sys_time.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_TIME_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_TIME_H_

#ifdef CONFIG_NEWLIB_LIBC
/* Kludge to support outdated newlib version as used in SDK 0.10 for Xtensa */
#include <newlib.h>

#ifdef __NEWLIB__
#include <sys/_timeval.h>
#else
#include <sys/types.h>

/**
 * @brief Time interval in seconds and microseconds.
 */
struct timeval {
	/**
	 * @brief Seconds component.
	 */
	time_t tv_sec;
	/**
	 * @brief Microseconds component.
	 */
	suseconds_t tv_usec;
};
#endif

#else
#include <sys/types.h>
#include <sys/_timeval.h>
#endif /* CONFIG_NEWLIB_LIBC */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the date and time.
 *
 * @param tv Output: current time; @c tv_sec is seconds and @c tv_usec is microseconds
 *           since the Epoch.
 * @param tz Deprecated, must be NULL.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{gettimeofday}
 */
int gettimeofday(struct timeval *tv, void *tz);

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_SYS_TIME_H_ */
