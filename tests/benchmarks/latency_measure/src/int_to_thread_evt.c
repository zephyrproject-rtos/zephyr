/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief measure time from ISR to a rescheduled thread
 *
 * This file contains test that measures time to switch from an interrupt
 * handler to executing a thread after rescheduling. In other words, execution
 * after interrupt handler resume in a different thread than the one which got
 * interrupted.
 */

#include <zephyr.h>
#include <irq_offload.h>

#include "utils.h"

static timing_t timestamp_start;
static timing_t timestamp_end;
static struct k_work work;

K_SEM_DEFINE(INTSEMA, 0, 1);
K_SEM_DEFINE(WORKSEMA, 0, 1);

/**
 *
 * @brief Test ISR used to measure best case interrupt latency
 *
 * The interrupt handler gets the second timestamp.
 *
 * @return N/A
 */
static void latency_test_isr(const void *unused)
{
	ARG_UNUSED(unused);

	k_work_submit(&work);
	timestamp_start = timing_counter_get();
}

static void worker(struct k_work *item)
{
	(void)item;

	timestamp_end = timing_counter_get();
	k_sem_give(&WORKSEMA);
}

/**
 *
 * @brief Software interrupt generating thread
 *
 * Lower priority thread that, when it starts, it waits for a semaphore. When
 * it gets it, released by the main thread, sets up the interrupt handler and
 * generates the software interrupt
 *
 * @return 0 on success
 */
void int_thread(void)
{
	k_sem_take(&INTSEMA, K_FOREVER);
	irq_offload(latency_test_isr, NULL);
	k_thread_suspend(k_current_get());
}

K_THREAD_DEFINE(int_thread_id, 512, (k_thread_entry_t)int_thread, NULL, NULL,
		NULL, 11, 0, 0);

/**
 *
 * @brief The test main function
 *
 * @return 0 on success
 */
int int_to_thread_evt(void)
{
	uint32_t diff;

	k_work_init(&work, worker);

	timing_start();
	TICK_SYNCH();
	k_sem_give(&INTSEMA);
	k_sem_take(&WORKSEMA, K_FOREVER);
	timing_stop();

	diff = timing_cycles_get(&timestamp_start, &timestamp_end);

	PRINT_STATS("Time from ISR to executing a different thread", diff)

	return 0;
}
