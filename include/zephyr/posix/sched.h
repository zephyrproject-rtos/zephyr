/*
 * Copyright (c) 2018-2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_SCHED_H_
#define ZEPHYR_INCLUDE_POSIX_SCHED_H_

#include <zephyr/kernel.h>

#include "posix_types.h"

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Other mandatory scheduling policy. Must be numerically distinct. May
 * execute identically to SCHED_RR or SCHED_FIFO. For Zephyr this is a
 * pseudonym for SCHED_RR.
 */
#define SCHED_OTHER 0

/* Cooperative scheduling policy */
#define SCHED_FIFO 1

/* Priority based preemptive scheduling policy */
#define SCHED_RR 2

#if defined(CONFIG_MINIMAL_LIBC) || defined(CONFIG_PICOLIBC) || defined(CONFIG_ARMCLANG_STD_LIBC) \
	|| defined(CONFIG_ARCMWDT_LIBC)
struct sched_param {
	int sched_priority;
};
#endif

/**
 * @brief Yield the processor
 *
 * See IEEE 1003.1
 */
static inline int sched_yield(void)
{
	k_yield();
	return 0;
}

int sched_get_priority_min(int policy);
int sched_get_priority_max(int policy);

int sched_getparam(pid_t pid, struct sched_param *param);
int sched_getscheduler(pid_t pid);

int sched_setparam(pid_t pid, const struct sched_param *param);
int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);
int sched_rr_get_interval(pid_t pid, struct timespec *interval);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SCHED_H_ */
