/*
 * Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP).
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include "lib/helper.h"

/*
 * A simple EDF sample with three threads.
 * Their properties are defined in the
 * structs below, so feel free to change
 * them as you will. Keep in mind that
 * the sum of all threads' wcet/deadline
 * should be <= 1 for guarantees that no
 * deadline will be missed.
 */

edf_t task1 = {.id = 1,
	       .initial_delay_msec = 1000,
	       .rel_deadline_msec = 30,
	       .period_msec = 2000,
	       .wcet_msec = 6};

edf_t task2 = {.id = 2,
	       .initial_delay_msec = 1000,
	       .rel_deadline_msec = 20,
	       .period_msec = 6000,
	       .wcet_msec = 10};

edf_t task3 = {.id = 3,
	       .initial_delay_msec = 1000,
	       .rel_deadline_msec = 10,
	       .period_msec = 12000,
	       .wcet_msec = 1};

void trigger(struct k_timer *timer)
{
	edf_t *task = (edf_t *)k_timer_user_data_get(timer);
	const char t = 1;
	/*
	 * the message itself is not relevant -
	 * the *act* of sending the message is.
	 * That's what unblocks the thread to
	 * execute a cycle.
	 *
	 * However, the deadline needs to be set
	 * BEFORE unblocking the thread because
	 * the k_thread_deadline_set is NOT a
	 * rescheduling point. Thus, it will
	 * not trigger a preemption even if it
	 * happens to be the earliest deadline.
	 */
	int deadline = MSEC_TO_CYC(task->rel_deadline_msec);
	k_thread_deadline_set(task->thread, deadline);
	k_msgq_put(&task->queue, &t, K_NO_WAIT);
}

void thread_function(void *task_props, void *a2, void *a3)
{
	char buffer[10];
	char message;
	int counter = 0;
	uint32_t deadline = 0;
	uint64_t start = 0;
	uint64_t end = 0;

	edf_t *task = (edf_t *)task_props;
	task->thread = k_current_get();

	k_msgq_init(&task->queue, buffer, sizeof(char), 10);
	k_timer_init(&task->timer, trigger, NULL);
	k_timer_user_data_set(&task->timer, (void *)task);
	k_timer_start(&task->timer, K_MSEC(task->initial_delay_msec), K_MSEC(task->period_msec));
	printf("[ %d ] %d   \tready.\n", task->id, counter);

	for (;;) {
		/*
		 * The periodic thread has no period, ironically.
		 * What makes it periodic is that it has a timer
		 * associated with it that regularly sends new
		 * messages to the thread queue (e.g. every two
		 * seconds).
		 *
		 * This thread itself does nothing special. It
		 * just remains in the processor for a given
		 * amount of time ("busy wait") to emulate
		 * the WCET (worst case execution time). Thus,
		 * what we want to see with this example is
		 * if the threads work alright
		 */
		k_msgq_get(&task->queue, &message, K_FOREVER);
		start = (uint64_t)clock();
		counter++;
		k_busy_wait(MSEC_TO_USEC(task->wcet_msec));
		deadline = (uint32_t)k_thread_deadline_get(task->thread);
		end = (uint64_t)clock();
		printk("[ %d ] %d   \tstart %llu\t end %llu\t dead %u\n", task->id, counter, start,
		       end, deadline);
	}
}

K_THREAD_DEFINE(task1_thread, 2048, thread_function, &task1, NULL, NULL, EDF_PRIORITY, 0, INACTIVE);
K_THREAD_DEFINE(task2_thread, 2048, thread_function, &task2, NULL, NULL, EDF_PRIORITY, 0, INACTIVE);
K_THREAD_DEFINE(task3_thread, 2048, thread_function, &task3, NULL, NULL, EDF_PRIORITY, 0, INACTIVE);

int main(void)
{
	printk("\n");
	k_thread_start(task1_thread);
	k_thread_start(task2_thread);
	k_thread_start(task3_thread);
	return 0;
}