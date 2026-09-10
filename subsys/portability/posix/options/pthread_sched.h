/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_POSIX_POSIX_PTHREAD_SCHED_H_
#define ZEPHYR_LIB_POSIX_POSIX_PTHREAD_SCHED_H_

#include <errno.h>
#include <stdbool.h>

#include <zephyr/posix/sched.h>

static inline bool valid_posix_policy(int policy)
{
	return policy == SCHED_FIFO || policy == SCHED_RR || policy == SCHED_OTHER;
}

static inline int posix_sched_priority_min(int policy)
{
	if (!valid_posix_policy(policy)) {
		errno = EINVAL;
		return -1;
	}

	return 0;
}

static inline int posix_sched_priority_max(int policy)
{
	if (IS_ENABLED(CONFIG_COOP_ENABLED) && policy == SCHED_FIFO) {
		return CONFIG_NUM_COOP_PRIORITIES - 1;
	} else if (IS_ENABLED(CONFIG_PREEMPT_ENABLED) &&
		   (policy == SCHED_RR || policy == SCHED_OTHER)) {
		return CONFIG_NUM_PREEMPT_PRIORITIES - 1;
	}

	errno = EINVAL;
	return -1;
}

#endif
