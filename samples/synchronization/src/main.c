/* main.c - Synchronization demo */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

/*
 * The synchronization demo has two threads that utilize semaphores and sleeping
 * to take turns printing a greeting message at a controlled rate. The demo
 * shows both the static and dynamic approaches for spawning a thread; a real
 * world application would likely use the static approach for both threads.
 */

#define PIN_THREADS (IS_ENABLED(CONFIG_SMP) && IS_ENABLED(CONFIG_SCHED_CPU_MASK))

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
	const char *tname;
	uint8_t cpu;
	struct k_thread *current_thread;

	while (1) {
		/* take my semaphore */
		k_sem_take(my_sem, K_FOREVER);

		current_thread = k_current_get();
		tname = k_thread_name_get(current_thread);
#if CONFIG_SMP
		cpu = arch_curr_cpu()->id;
#else
		cpu = 0;
#endif
		/* say "hello" */
		if (tname == NULL) {
			printk("%s: Hello World from cpu %d on %s!\n",
				my_name, cpu, CONFIG_BOARD);
		} else {
			printk("%s: Hello World from cpu %d on %s!\n",
				tname, cpu, CONFIG_BOARD);
		}

		/* wait a while, then let other thread have a turn */
		k_busy_wait(100000);
		k_msleep(SLEEPTIME);
		k_sem_give(other_sem);
	}
}

/* define semaphores */
K_SEM_DEFINE(thread_a_sem, 1, 1);	/* starts off "available" */
K_SEM_DEFINE(thread_b_sem, 0, 1);	/* starts off "not available" */

/* thread_a is a dynamic thread that is spawned in main */
void thread_a_entry_point(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	/* invoke routine to ping-pong hello messages with thread_b */
	hello_loop(__func__, &thread_a_sem, &thread_b_sem);
}
K_THREAD_STACK_DEFINE(thread_a_stack_area, STACKSIZE);
static struct k_thread thread_a_data;

/* thread_b is a static thread spawned immediately */
void thread_b_entry_point(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	/* invoke routine to ping-pong hello messages with thread_a */
	hello_loop(__func__, &thread_b_sem, &thread_a_sem);
}
K_THREAD_DEFINE(thread_b, STACKSIZE,
				thread_b_entry_point, NULL, NULL, NULL,
				PRIORITY, 0, 0);
extern const k_tid_t thread_b;

int main(void)
{
	k_thread_create(&thread_a_data, thread_a_stack_area,
			K_THREAD_STACK_SIZEOF(thread_a_stack_area),
			thread_a_entry_point, NULL, NULL, NULL,
			PRIORITY, 0, K_FOREVER);
	k_thread_name_set(&thread_a_data, "thread_a");

#if PIN_THREADS
	if (arch_num_cpus() > 1) {
		k_thread_cpu_pin(&thread_a_data, 0);

		/*
		 * Thread b is a static thread that is spawned immediately. This means that the
		 * following `k_thread_cpu_pin` call can fail with `-EINVAL` if the thread is
		 * actively running. Let's suspend the thread and resume it after the affinity mask
		 * is set.
		 */
		k_thread_suspend(thread_b);
		k_thread_cpu_pin(thread_b, 1);
		k_thread_resume(thread_b);
	}
#endif

	k_thread_start(&thread_a_data);
	return 0;
}
