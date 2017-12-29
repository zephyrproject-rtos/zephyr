/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <pthread.h>

/*
 *  ======== sched_get_priority_min ========
 *  Provide Minimum Priority for the given policy
 *
 *  In case of Cooperative priority value returned
 *  is negative of the priority value.
 *
 *  returns:	priority corresponing to policy
 *		-EINVAL for Error
 */
int sched_get_priority_min(int policy)
{

	int priority = -EINVAL;

	if (!((policy == CONFIG_COOP_ENABLED) || (policy == SCHED_PREMPT))) {
		return -EINVAL;
	}
#if defined(CONFIG_COOP_ENABLED) && defined(CONFIG_PREEMPT_ENABLED)
	priority = (policy == SCHED_COOP) ? (-1 * -1) :
		K_LOWEST_APPLICATION_THREAD_PRIO;
#elif defined(CONFIG_COOP_ENABLED)
	priority = (policy == SCHED_COOP) ?
		(-1 * K_LOWEST_APPLICATION_THREAD_PRIO) : -EINVAL;
#elif defined(CONFIG_PREEMPT_ENABLED)
	priority = (policy == SCHED_PREMPT) ? K_LOWEST_APPLICATION_THREAD_PRIO :
		-EINVAL;
#endif
	return priority;
}

/*
 * ======== sched_get_priority_max ========
 * Provide Maximum Priority for the given policy
 *
 * In case of Cooperative priority value returned
 * is negative of the priority value.
 *
 * returns:	priority corresponing to policy
 *		-EINVAL for Error
 */
int sched_get_priority_max(int policy)
{
	int priority = -EINVAL;

	if (!((policy == CONFIG_COOP_ENABLED) || (policy == SCHED_PREMPT))) {
		return -EINVAL;
	}
#if defined(CONFIG_COOP_ENABLED) && defined(CONFIG_PREEMPT_ENABLED)
	priority = (policy == SCHED_COOP) ?
		(-1 * K_HIGHEST_APPLICATION_THREAD_PRIO) : 0;
#elif defined(CONFIG_COOP_ENABLED)
	priority = (policy == SCHED_COOP) ?
		(-1 * K_HIGHEST_APPLICATION_THREAD_PRIO) : -EINVAL;
#elif defined(CONFIG_PREEMPT_ENABLED)
	priority = (policy == SCHED_PREMPT) ?
		K_HIGHEST_APPLICATION_THREAD_PRIO : -EINVAL;
#endif
	return priority;
}

/*
 *  ======== sched_yield ========
 */
int sched_yield(void)
{
	k_yield();
	return 0;
}

