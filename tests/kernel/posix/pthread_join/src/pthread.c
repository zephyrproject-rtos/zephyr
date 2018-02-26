/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tc_util.h>
#include <pthread.h>

#define N_THR 3
#define STACKSZ 1024
#define ONE_SECOND 1
#define THREAD_PRIORITY 2

K_THREAD_STACK_ARRAY_DEFINE(stacks, N_THR, STACKSZ);
int exit_count;

void *thread_top(void *p1)
{
	pthread_t pthread;
	u32_t policy, seconds;
	struct sched_param param;

	pthread = (pthread_t) pthread_self();
	pthread_getschedparam(pthread, &policy, &param);
	TC_PRINT("Thread %d scheduling policy = %d & priority %d started\n",
	       (s32_t) p1, policy, param.priority);
	seconds = (s32_t)p1;
	sleep(seconds * ONE_SECOND);
	exit_count++;
	TC_PRINT("Exiting thread %d\n", (s32_t)p1);
	pthread_exit(p1);
	return NULL;
}

static bool is_sched_prio_valid(int prio, int policy)
{
	if (prio < sched_get_priority_min(policy) ||
	    prio > sched_get_priority_max(policy)) {
		return false;
	}

	return true;
}

void main(void)
{
	s32_t i, ret, status = TC_FAIL;
	pthread_attr_t attr[N_THR];
	struct sched_param schedparam;
	pthread_t newthread[N_THR];
	u32_t detachstate, schedpolicy = SCHED_RR;
	void *retval;
	size_t stack_size;

	TC_START("POSIX pthread join API");
	/* Creating threads with lowest application priority */
	for (i = 0; i < N_THR; i++) {
		pthread_attr_init(&attr[i]);

		/* Setting pthread as JOINABLE */
		pthread_attr_getdetachstate(&attr[i], &detachstate);
		if (detachstate != PTHREAD_CREATE_JOINABLE) {
			pthread_attr_setdetachstate(&attr[i],
						    PTHREAD_CREATE_JOINABLE);
		}

		/* Setting premptive scheduling policy */
		pthread_attr_getschedpolicy(&attr[i], &schedpolicy);
		if (schedpolicy != SCHED_RR) {
			schedpolicy = SCHED_RR;
			pthread_attr_setschedpolicy(&attr[i], schedpolicy);
		}

		/* Setting scheduling priority */
		pthread_attr_getschedparam(&attr[i], &schedparam);
		if (schedparam.priority != THREAD_PRIORITY) {
			schedparam.priority = THREAD_PRIORITY;
			if (is_sched_prio_valid(schedparam.priority,
						schedpolicy)) {
				pthread_attr_setschedparam(&attr[i],
							   &schedparam);
			} else {
				TC_ERROR("Scheduling priority invalid\n");
				goto done;
			}
		}

		/* Setting stack */
		pthread_attr_getstacksize(&attr[i], &stack_size);
		if (stack_size != STACKSZ) {
			stack_size = STACKSZ;
			pthread_attr_setstack(&attr[i], &stacks[i][0],
					      stack_size);
		}

		ret = pthread_create(&newthread[i], &attr[i], thread_top,
				     (void *)i);
		if (ret != 0) {
			TC_ERROR("Number of threads exceeds Maximum Limit\n");
			goto done;
		}

		pthread_attr_destroy(&attr[i]);
	}

	for (i = 0; i < N_THR; i++) {
		TC_PRINT("Waiting for pthread %d to Join\n", i);
		pthread_join(newthread[i], &retval);
		TC_PRINT("Pthread %d joined to %s\n", i, __func__);
	}

	/* Test PASS if all threads have exited before main exit */
	status = exit_count == N_THR ? TC_PASS : TC_FAIL;
done:
	TC_END_REPORT(status);
}
