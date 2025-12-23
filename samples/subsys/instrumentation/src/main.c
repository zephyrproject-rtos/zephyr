/*
 * Copyright 2023 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#define SLEEPTIME 10
#define STACKSIZE 1024
#define PRIORITY  7

void __no_optimization loop_0(void)
{
	/* Wait to spend some cycles */
	k_busy_wait(10);
}

void __no_optimization loop_1(void)
{
	/* Wait to spend some cycles */
	k_busy_wait(1000);
}

/*
 * 'main' thread can take its mutex promptly and run. 'thread_A' needs to wait 'main' give its
 * (thread_A) mutex to run.
 */
K_SEM_DEFINE(main_sem, 1, 1);		/* Initialized as ready to be taken */
K_SEM_DEFINE(thread_a_sem, 0, 1);	/* Initialized as already taken (blocked) */

K_THREAD_STACK_DEFINE(thread_a_stack, STACKSIZE);
static struct k_thread thread_a_data;

static int counter;

void get_sem_and_exec_function(struct k_sem *my_sem, struct k_sem *other_sem, void (*func)(void))
{
	while (counter < 4) {
		k_sem_take(my_sem, K_FOREVER);

		func();
		k_msleep(SLEEPTIME);

		counter++;
		k_sem_give(other_sem);
	}
}

void thread_A(void *notused0, void *notused1, void *notused2)
{
	ARG_UNUSED(notused0);
	ARG_UNUSED(notused1);
	ARG_UNUSED(notused2);

	get_sem_and_exec_function(&thread_a_sem, &main_sem, loop_0);
}

int main(void)
{
	k_tid_t thread_a;

	/* Create Thread A */
	thread_a = k_thread_create(&thread_a_data, thread_a_stack, STACKSIZE,
				   thread_A, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(thread_a, "thread_A");

	/* Start ping-pong between 'main' and 'thread_A' */
	get_sem_and_exec_function(&main_sem, &thread_a_sem, loop_1);

	return 0;
}
