/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <pthread.h>
#include <stdio.h>
#include <atomic.h>
#include "ksched.h"
#include "wait_q.h"

/* Kernel Structure for Number of threads */
struct k_thread threads[CONFIG_MAX_PTHREAD_COUNT];

static int is_prio_valid(int priority)
{
	if ((priority < K_HIGHEST_APPLICATION_THREAD_PRIO)
	    || (priority > K_HIGHEST_APPLICATION_THREAD_PRIO)) {
		return -1;
	} else       {
		return 0;
	}

}

int pthread_attr_setschedparam(pthread_attr_t *attr,
			       const struct sched_param *schedparam)
{
	int priority = schedparam->priority;

	/*
	 *  Task priorities can range from  K_LOWEST_THREAD_PRIO to
	 *  K_HIGHEST_THREAD_PRIO.
	 */
	if (is_prio_valid(priority) != 0) {
		/* Invalid Prioritty */
		return EINVAL;
	}
	attr->priority = priority;
	return 0;
}

int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr,
			  size_t stacksize)
{

	if ((stackaddr == NULL) || (stacksize <= K_LOWEST_STACK_SIZE)) {
		return EINVAL;
	}
	attr->stack = stackaddr;
	attr->stacksize = stacksize;
	return 0;
}

static void zephyr_thread_wrapper(void *arg1, void *arg2, void *arg3)
{
	void * (*fun_ptr)(void *);

	fun_ptr = arg3;
	(*fun_ptr)((void *)arg1);
}

int pthread_create(pthread_t *newthread, const pthread_attr_t *attr,
		   void *(*threadroutine)(void *), void *arg)
{
	static int thread_count;

	if ((attr == NULL) || (attr->stack == NULL)) {
		return EINVAL;
	}
	if (thread_count < CONFIG_MAX_PTHREAD_COUNT) {
		*newthread = (pthread_t) k_thread_create(&threads[thread_count],
			attr->stack, attr->stacksize,
			(k_thread_entry_t)zephyr_thread_wrapper, (void *)arg,
			NULL, threadroutine, attr->priority, attr->options,
			attr->delayedstart);
		atomic_inc(&thread_count);
	} else {
		/* Insuffecient Resource, Maximum Thread Count reached */
		return  EAGAIN;
	}

	return 0;
}

int pthread_setcancelstate(int state, int *oldstate)
{
	*oldstate = _is_thread_essential() ? PTHREAD_NON_CANCELABLE :
			PTHREAD_CANCELABLE;

	if (state == *oldstate) {
		return	EINVAL;
	}
	unsigned int key = irq_lock();

	if (state & PTHREAD_NON_CANCELABLE) {
		_thread_essential_set();
	} else {
		_thread_essential_clear();
	}
	irq_unlock(key);

	/* TODO : Need to Handle Already received Cancel Pending request */

	return 0;
}

int pthread_setschedparam(pthread_t pthread, int policy,
			  const struct sched_param *param)
{
	k_tid_t *thread = (k_tid_t *)pthread;
	int new_prio = param->priority;
	int cur_prio;

	if (is_prio_valid(new_prio) != 0) {
		return EINVAL;
	}

	cur_prio = k_thread_priority_get(*thread);

	if (new_prio == cur_prio) {
		return EINVAL;
	}

	k_thread_priority_set(*thread, new_prio);

	return 0;
}

