/* main.c - Hello World demo */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>

/*
 * The hello world demo has two threads that utilize semaphores and sleeping
 * to take turns printing a greeting message at a controlled rate. The demo
 * shows both the static and dynamic approaches for spawning a thread; a real
 * world application would likely use the static approach for both threads.
 */


/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

/* delay between greetings (in ms) */
#define SLEEPTIME 500


/*
 * @param my_name      thread identification string
 * @param my_sem       thread's own semaphore
 * @param other_sem    other thread's semaphore
 */
void hello_loop(const char *my_name,
	       struct k_sem *my_sem, struct k_sem *other_sem)
{
	while (1) {
		/* take my semaphore */
		k_sem_take(my_sem, K_FOREVER);

		/* say "hello" */
		printk("%s: Hello World from %s!\n", my_name, CONFIG_ARCH);

		/* wait a while, then let other thread have a turn */
		k_sleep(SLEEPTIME);
		k_sem_give(other_sem);
	}
}

/* define semaphores */

K_SEM_DEFINE(threada_sem, 1, 1);	/* starts off "available" */
K_SEM_DEFINE(threadb_sem, 0, 1);	/* starts off "not available" */


/* threadb is a dynamic thread that is spawned by threadA */

void threadb(void *dummy1, void *dummy2, void *dummy3)
{
	/* invoke routine to ping-pong hello messages with threadA */
	hello_loop(__func__, &threadb_sem, &threada_sem);
}

K_THREAD_STACK_DEFINE(threadb_stack_area, STACKSIZE);
static struct k_thread threadb_data;

/* threada is a static thread that is spawned automatically */

void threada(void *dummy1, void *dummy2, void *dummy3)
{
	/* spawn threadB */
	k_thread_create(&threadb_data, threadb_stack_area, STACKSIZE, threadb,
			NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);

	/* invoke routine to ping-pong hello messages with threadB */
	hello_loop(__func__, &threada_sem, &threadb_sem);
}

K_THREAD_DEFINE(threada_id, STACKSIZE, threada, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
