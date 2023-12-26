/*
 * Copyright (c) 2018-2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pthread_sched.h"

#include <zephyr/kernel.h>
#include <zephyr/posix/sched.h>

/**
 * @brief Get minimum priority value for a given policy
 *
 * See IEEE 1003.1
 *
 * @param[in] policy  Scheduling policy with Zephyr thread priority ranges:
 *	SCHED_RR:   [0, @ref CONFIG_NUM_PREEMPT_PRIORITIES -1]
 *	SCHED_FIFO: [ @ref CONFIG_NUM_PREEMPT_PRIORITIES,
 *		      @ref CONFIG_NUM_COOP_PRIORITIES -1]
 *	SCHED_OTHER: [0, @ref CONFIG_NUM_PREEMPT_PRIORITIES +
 *			 @ref CONFIG_NUM_COOP_PRIORITIES -1]
 *
 * @retval -1 and errno=EINVAL for a policy which is either not supported
 *	or not allowed with Zephyr config parameters at compile time.
 */
int sched_get_priority_min(int policy)
{
	if (!valid_posix_policy(policy)) {
		errno = EINVAL;
		return -1;
	}

	if (IS_ENABLED(CONFIG_COOP_ENABLED) && policy == SCHED_FIFO) {
		/* min. COOP priority is above PREEMPT level if any. */
		return CONFIG_NUM_PREEMPT_PRIORITIES;
	}

	return 0;
}

/**
 * @brief Get maximum priority value for a given policy
 *
 * See IEEE 1003.1
 *
 * @param[in] policy  Scheduling policy with Zephyr thread priority ranges:
 *	SCHED_RR:   [0, @ref CONFIG_NUM_PREEMPT_PRIORITIES -1]
 *	SCHED_FIFO: [ @ref CONFIG_NUM_PREEMPT_PRIORITIES,
 *		      @ref CONFIG_NUM_COOP_PRIORITIES -1]
 *	SCHED_OTHER: [0, @ref CONFIG_NUM_PREEMPT_PRIORITIES +
 *			 @ref CONFIG_NUM_COOP_PRIORITIES -1]
 *
 * @retval -1 and errno=EINVAL for a policy which is either not supported
 *	or not allowed with Zephyr config parameters at compile time.
 */
int sched_get_priority_max(int policy)
{
	int priority_levels = 0;

	if (!valid_posix_policy(policy)) {
		errno = EINVAL;
		return -1;
	}

	if (IS_ENABLED(CONFIG_PREEMPT_ENABLED)) {
		priority_levels += CONFIG_NUM_PREEMPT_PRIORITIES;
	}

	if (IS_ENABLED(CONFIG_COOP_ENABLED) && policy != SCHED_RR) {
		priority_levels += CONFIG_NUM_COOP_PRIORITIES;
	}

	if (priority_levels == 0) {
		/* Should be already eliminated at compile time. */
		errno = EINVAL;
		return -1;
	}

	return priority_levels - 1;    /* 0 inclusive */
}
