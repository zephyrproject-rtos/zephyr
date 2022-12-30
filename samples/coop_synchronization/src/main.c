/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

/*
 * The coop synchronization demo has two threads that utilize semaphores and k_yield
 * to take turns printing a greeting message with a somewhat uncontrolled rate.
 * The rate is controlled via a busy waiting loop. The advantage of this demo is that
 * it'll work even if there are no timing utilities provided by the architecture.
 */

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* number of NOPs to churn through for busy waiting */
#if defined(CONFIG_TIMING_FUNCTIONS)
#define NUM_NOPS() timing_freq_get()
#else
#define NUM_NOPS() (1000000)
#endif

/* arch_curr_cpu() is defined only for certain architectures */
#if defined(CONFIG_ARC) || defined(CONFIG_ARM) || defined(CONFIG_X86_64) \
	|| defined(CONFIG_RISCV) || defined(CONFIG_XTENSA)
	#define GET_CURR_CPU() arch_curr_cpu()->id
#else
	#define GET_CURR_CPU() (0)
#endif

/*
 * @param my_name      thread identification string
 * @param my_sem       thread's own semaphore
 * @param other_sem    other thread's semaphore
 */
void helloLoop(const char *my_name, struct k_sem *my_sem,
	struct k_sem *other_sem, int incrementer)
{
	const char *tname;
	uint8_t cpu = 0;
	struct k_thread *current_thread;
	int task_local_counter = 0;

	while (1) {
		/* take my semaphore */
		k_sem_take(my_sem, K_FOREVER);

		current_thread = k_current_get();
		tname = k_thread_name_get(current_thread);
		cpu = GET_CURR_CPU();

		/* say "hello" */
		printk("%s: Hello World from cpu %d on %s! counter = %d\n", tname ?: my_name, cpu,
						 CONFIG_BOARD, task_local_counter += incrementer);

		/* We wait a while here, then let other thread have a turn.
		 * We use a busy wait here because arch provided k_busy_wait()
		 * implementation is still likely to depend on the system timer,
		 * for which the assumption is to be non-existent/disabled/broken.
		 * On another note: native posix doesn't play nice with busy waits
		 * as it requires a different API (Z_SPIN_DELAY) for the threads to
		 * be preemtable inside the busy wait. But the point of this sample
		 * is not to be preempted by timers (nor UART if possible).
		 */
		for (int i = 0; i < NUM_NOPS(); i++) {
			arch_nop();
		}

		k_sem_give(other_sem);
		k_yield();
	}
}

/* define semaphores */

K_SEM_DEFINE(threadA_sem, 1, 1); /* starts off "available" */
K_SEM_DEFINE(threadB_sem, 0, 1); /* starts off "not available" */

/* threadB is a dynamic thread that is spawned by threadA */

void threadB(void *arg1, void *arg2, void *arg3)
{
	int incrementer = (intptr_t)arg1;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	/* invoke routine to ping-pong hello messages with threadA */
	helloLoop(__func__, &threadB_sem, &threadA_sem, incrementer);
}

K_THREAD_STACK_DEFINE(threadA_stack_area, STACKSIZE);
static struct k_thread threadA_data;

K_THREAD_STACK_DEFINE(threadB_stack_area, STACKSIZE);
static struct k_thread threadB_data;

/* threadA is a static thread that is spawned automatically */

void threadA(void *arg1, void *arg2, void *arg3)
{
	int incrementer = (intptr_t)arg1;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	/* invoke routine to ping-pong hello messages with threadB */
	helloLoop(__func__, &threadA_sem, &threadB_sem, incrementer);
}

int main(void)
{
	k_thread_create(&threadA_data, threadA_stack_area,
			K_THREAD_STACK_SIZEOF(threadA_stack_area),
			threadA, (void *)1, NULL, NULL,
			0, 0, K_FOREVER);
	k_thread_name_set(&threadA_data, "thread_a");

	k_thread_create(&threadB_data, threadB_stack_area,
			K_THREAD_STACK_SIZEOF(threadB_stack_area),
			threadB, (void *)-1, NULL, NULL,
			0, 0, K_FOREVER);
	k_thread_name_set(&threadB_data, "thread_b");

	k_thread_start(&threadA_data);
	k_thread_start(&threadB_data);

	return 0;
}
