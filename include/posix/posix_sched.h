/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_POSIX_SCHED_H_
#define ZEPHYR_INCLUDE_POSIX_POSIX_SCHED_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Cooperative scheduling policy */
#ifndef SCHED_FIFO
#define SCHED_FIFO 0
#endif /* SCHED_FIFO */

/* Priority based preemptive scheduling policy */
#ifndef SCHED_RR
#define SCHED_RR 1
#endif /* SCHED_RR */

struct sched_param {
	int priority;
};

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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_POSIX_SCHED_H_ */
