/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tc_util.h>
#include <pthread.h>

int pthread_cancel(pthread_t pthread);
#define N_THR 4
#define STACKSZ 1024
#define ONE_SECOND 1
#define THREAD_PRIORITY 3

K_THREAD_STACK_ARRAY_DEFINE(stacks, N_THR, STACKSZ);
int exit_count;

void *thread_top(void *p1)
{
	pthread_t self;
	int oldstate;
	int val = (u32_t) p1;

	if (val % 2) {
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
	}

	self = pthread_self();
	TC_PRINT("Canceling thread %d\n", (s32_t) p1);
	pthread_cancel(self);
	TC_PRINT("Thread %d could not be canceled\n", (s32_t) p1);
	sleep(ONE_SECOND);
	exit_count++;
	TC_PRINT("Exiting thread %d\n", (s32_t)p1);
	pthread_exit(p1);
	TC_PRINT("Exited thread %d\n", (s32_t)p1);
	return NULL;
}

void main(void)
{
	s32_t i, ret, status = TC_FAIL;
	pthread_attr_t attr[N_THR];
	struct sched_param schedparam;
	pthread_t newthread[N_THR];
	void *retval;

	TC_START("POSIX thread cancel APIs\n");
	/* Creating 4 threads with lowest application priority */
	for (i = 0; i < N_THR; i++) {
		pthread_attr_init(&attr[i]);

		if (i == 0) {
			pthread_attr_setdetachstate(&attr[i],
						    PTHREAD_CREATE_JOINABLE);
		} else if (i == 1) {
			pthread_attr_setdetachstate(&attr[i],
						    PTHREAD_CREATE_JOINABLE);
		} else if (i == 2) {
			pthread_attr_setdetachstate(&attr[i],
						    PTHREAD_CREATE_DETACHED);
		} else if (i == 3) {
			pthread_attr_setdetachstate(&attr[i],
						    PTHREAD_CREATE_DETACHED);
		}
		schedparam.priority = 2;
		pthread_attr_setschedparam(&attr[i], &schedparam);
		pthread_attr_setstack(&attr[i], &stacks[i][0], STACKSZ);
		ret = pthread_create(&newthread[i], &attr[i], thread_top,
				     (void *)i);
		if (ret != 0) {
			TC_ERROR("Number of threads exceeds Maximum Limit\n");
			goto done;
		}
	}

	for (i = 0; i < N_THR; i++) {
		TC_PRINT("Waiting for pthread %d to Join\n", i);
		pthread_join(newthread[i], &retval);
		TC_PRINT("Pthread %d joined to %s\n", i, __func__);
	}

	TC_PRINT("pthread join test over\n");

	/* Test PASS if all threads have exited before main exit */
	status = exit_count == 1 ? TC_PASS : TC_FAIL;
	sleep(ONE_SECOND);
done:
	TC_END_REPORT(status);
}
