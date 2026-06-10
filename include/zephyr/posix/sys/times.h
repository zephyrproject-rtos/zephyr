/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Process CPU time accounting.
 * @ingroup posix
 *
 * Defines the tms structure reporting the user and system CPU time consumed by
 * the calling process and its waited-for children, and the times() function
 * that retrieves it.
 *
 * @posix_header{sys_times.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_TIMES_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_TIMES_H_

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_POSIX_MULTI_PROCESS) || defined(__DOXYGEN__)

#if !defined(_TMS_DECLARED) && !defined(__tms_defined)
/**
 * @brief Process times.
 */
struct tms {
	/**
	 * @brief User CPU time of the process.
	 */
	clock_t tms_utime;
	/**
	 * @brief System CPU time of the process.
	 */
	clock_t tms_stime;
	/**
	 * @brief User CPU time of waited-for children.
	 */
	clock_t tms_cutime;
	/**
	 * @brief System CPU time of waited-for children.
	 */
	clock_t tms_cstime;
};
/** @cond INTERNAL_HIDDEN */
#define _TMS_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __tms_defined
/** @endcond */
#endif

/**
 * @brief Get process and waited-for child process times.
 *
 * @param buf Output: filled with CPU times for the calling process.
 *
 * @return Elapsed real time in clock ticks since an arbitrary epoch, or
 *         (clock_t)-1 with errno set on failure.
 *
 * @posix_func{times}
 */
clock_t times(struct tms *buf);

#endif /* _POSIX_MULTI_PROCESS */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_TIMES_H_ */
