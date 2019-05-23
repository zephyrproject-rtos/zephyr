/*
 * Copyright (c) 2019 Kevin Townsend.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <profiling.h>

/* Declare our profiling task instance with a unique ID. */
static struct dbg_prof_task t1 = {
	.name = "basic_counter"
};

void main(void)
{
	dbg_prof_init();
	dbg_prof_task_init(&t1);

	/* Perform a dummy workload. */
	dbg_prof_task_start(&t1);
	for (size_t i = 0; i < 10000; i++) {
		if (i % 1000 == 0) {
			dbg_prof_task_update(&t1);
		}
		__asm volatile ("nop");
	}
	dbg_prof_task_stop(&t1);
}
