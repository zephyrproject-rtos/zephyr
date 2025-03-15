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
	       .rel_deadline_msec = 3000,
	       .period_msec = 6000,
	       .wcet_msec = 1000};

edf_t task2 = {.id = 2,
	       .initial_delay_msec = 1000,
	       .rel_deadline_msec = 6000,
	       .period_msec = 8000,
	       .wcet_msec = 2000};

edf_t task3 = {.id = 3,
	       .initial_delay_msec = 1000,
	       .rel_deadline_msec = 10000,
	       .period_msec = 10000,
	       .wcet_msec = 3000};

void trigger(struct k_timer *timer)
{
	const char t = 1;
	edf_t *task = (edf_t *)k_timer_user_data_get(timer);
	int deadline = MSEC_TO_CYC(task->rel_deadline_msec);

	/*
	 * the message itself is not relevant -
	 * the *act* of sending the message is.
	 * That's what unblocks the thread to
	 * execute a cycle.
	 */
	k_msgq_put(&task->queue, &t, K_NO_WAIT);

	/*
	 * sets the task deadline. Note that
	 * after setting the deadline, we need
	 * to trigger a rescheduling so the
	 * earliest thread gets properly selected.
	 */
	k_thread_deadline_set(task->thread, deadline);
	k_reschedule();
}

void thread_function(void *task_props, void *a2, void *a3)
{
	char buffer[10];
	char message;
	int32_t deadline;
	edf_t *task = (edf_t *)task_props;

	task->thread = k_current_get();

	/* value passed to k_busy_wait needs to be in usec */
	uint32_t wcet = MSEC_TO_USEC(task->wcet_msec);

	k_msgq_init(&task->queue, buffer, sizeof(char), 10);
	k_timer_init(&task->timer, trigger, NULL);
	k_timer_user_data_set(&task->timer, (void *)task);
	k_timer_start(&task->timer, K_MSEC(task->initial_delay_msec), K_MSEC(task->period_msec));

	/* allow other threads to set up */
	k_sleep(K_MSEC(1));

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
		 * just if the edf scheduler works alright
		 */
		k_msgq_get(&task->queue, &message, K_FOREVER);
		deadline = k_thread_deadline_get(task->thread);
		cycle(task->id, wcet, deadline);
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
