/*
 * Copyright (c) 2018-2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Execution scheduling.
 * @ingroup posix
 *
 * Provides the scheduling policies and the sched_* functions used to query
 * and control process and thread scheduling parameters.
 *
 * @posix_header{sched.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SCHED_H_
#define ZEPHYR_INCLUDE_POSIX_SCHED_H_

#include <zephyr/kernel.h>
#include <zephyr/posix/posix_types.h>

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Other mandatory scheduling policy. Must be numerically distinct. May
 * execute identically to SCHED_RR or SCHED_FIFO. For Zephyr this is a
 * pseudonym for SCHED_RR.
 */

/**
 * @brief Round-robin or other time-sharing scheduling policy (default).
 */
#define SCHED_OTHER 0

/* Cooperative scheduling policy */

/**
 * @brief First-in first-out (cooperative) scheduling policy.
 */
#define SCHED_FIFO 1

/* Priority based preemptive scheduling policy */

/**
 * @brief Round-robin (preemptive, priority-based) scheduling policy.
 */
#define SCHED_RR 2

#if defined(CONFIG_MINIMAL_LIBC) || defined(CONFIG_PICOLIBC) || defined(CONFIG_ARMCLANG_STD_LIBC) \
	|| defined(CONFIG_ARCMWDT_LIBC)

/**
 * @brief Scheduling parameters used with sched_setparam() and pthread_attr_setschedparam().
 */
struct sched_param {
	/**
	 * @brief Process or thread execution scheduling priority.
	 */
	int sched_priority;
};
#endif

/**
 * @brief Yield the processor to another thread of equal or higher priority.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sched_yield}
 */
int sched_yield(void);

/**
 * @brief Get the minimum priority value for a scheduling policy.
 *
 * @param policy @c SCHED_FIFO, @c SCHED_RR, or @c SCHED_OTHER.
 *
 * @return Minimum priority on success, or -1 with errno set on failure.
 *
 * @posix_func{sched_get_priority_min}
 */
int sched_get_priority_min(int policy);

/**
 * @brief Get the maximum priority value for a scheduling policy.
 *
 * @param policy @c SCHED_FIFO, @c SCHED_RR, or @c SCHED_OTHER.
 *
 * @return Maximum priority on success, or -1 with errno set on failure.
 *
 * @posix_func{sched_get_priority_max}
 */
int sched_get_priority_max(int policy);

/**
 * @brief Get scheduling parameters for a process.
 *
 * @param pid   Process ID (0 = calling process).
 * @param param Output: current scheduling parameters.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sched_getparam}
 */
int sched_getparam(pid_t pid, struct sched_param *param);

/**
 * @brief Get the scheduling policy of a process.
 *
 * @param pid Process ID (0 = calling process).
 *
 * @return Scheduling policy (@c SCHED_FIFO, @c SCHED_RR, @c SCHED_OTHER),
 *         or -1 with errno set on failure.
 *
 * @posix_func{sched_getscheduler}
 */
int sched_getscheduler(pid_t pid);

/**
 * @brief Set scheduling parameters for a process.
 *
 * @param pid   Process ID (0 = calling process).
 * @param param New scheduling parameters.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sched_setparam}
 */
int sched_setparam(pid_t pid, const struct sched_param *param);

/**
 * @brief Set the scheduling policy and parameters of a process.
 *
 * @param pid    Process ID (0 = calling process).
 * @param policy @c SCHED_FIFO, @c SCHED_RR, or @c SCHED_OTHER.
 * @param param  New scheduling parameters.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sched_setscheduler}
 */
int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);

/**
 * @brief Get the round-robin time quantum for a process.
 *
 * @param pid      Process ID (0 = calling process).
 * @param interval Output: round-robin interval.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sched_rr_get_interval}
 */
int sched_rr_get_interval(pid_t pid, struct timespec *interval);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SCHED_H_ */
