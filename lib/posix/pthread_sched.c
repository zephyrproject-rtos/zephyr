/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <posix/posix_sched.h>

static inline bool valid_posix_policy(int policy)
{
	return policy == SCHED_FIFO || policy == SCHED_RR;
}

/**
 * @brief Get minimum priority value for a given policy
 *
 * See IEEE 1003.1
 */
int sched_get_priority_min(int policy)
{
	if (!valid_posix_policy(policy)) {
		errno = EINVAL;
		return -1;
	}

	return 0;
}

/**
 * @brief Get maximum priority value for a given policy
 *
 * See IEEE 1003.1
 */
int sched_get_priority_max(int policy)
{
	if (IS_ENABLED(CONFIG_COOP_ENABLED) && policy == SCHED_FIFO) {
		return CONFIG_NUM_COOP_PRIORITIES - 1;
	} else if (IS_ENABLED(CONFIG_PREEMPT_ENABLED) && policy == SCHED_RR) {
		return CONFIG_NUM_PREEMPT_PRIORITIES - 1;
	}

	errno = EINVAL;
	return -1;
}
