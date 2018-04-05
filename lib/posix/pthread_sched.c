/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <posix/posix_sched.h>

static bool valid_posix_policy(int policy)
{
	if (policy != SCHED_FIFO && policy != SCHED_RR) {
		return false;
	}

	return true;
}

/**
 * @brief Get minimum priority value for a given policy
 *
 * See IEEE 1003.1
 */
int sched_get_priority_min(int policy)
{
	if (valid_posix_policy(policy) == false) {
		errno = EINVAL;
		return -1;
	}

	if (IS_ENABLED(CONFIG_COOP_ENABLED)) {
		if (policy == SCHED_FIFO) {
			return 0;
		}
	}

	if (IS_ENABLED(CONFIG_PREEMPT_ENABLED)) {
		if (policy == SCHED_RR) {
			return 0;
		}
	}

	errno = EINVAL;
	return -1;
}

/**
 * @brief Get maximum priority value for a given policy
 *
 * See IEEE 1003.1
 */
int sched_get_priority_max(int policy)
{
	if (valid_posix_policy(policy) == false) {
		errno = EINVAL;
		return -1;
	}

	if (IS_ENABLED(CONFIG_COOP_ENABLED)) {
		if (policy == SCHED_FIFO) {
			/* Posix COOP priority starts from 0
			 * whereas zephyr starts from -1
			 */
			return (CONFIG_NUM_COOP_PRIORITIES - 1);
		}

	}

	if (IS_ENABLED(CONFIG_PREEMPT_ENABLED)) {
		if (policy == SCHED_RR) {
			return CONFIG_NUM_PREEMPT_PRIORITIES;
		}
	}

	errno = EINVAL;
	return -1;
}
