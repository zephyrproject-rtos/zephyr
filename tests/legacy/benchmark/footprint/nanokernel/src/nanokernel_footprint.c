/* nanokernel_footprint.c - nanokernel footprint */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <toolchain.h>

volatile int i = 0;		/* counter used by background task */

#ifndef TEST_min

#include <misc/printk.h>
#include <stdio.h>

#define IRQ_LINE          11  /* just some random value w/o driver conflicts */
#define IRQ_PRIORITY      3
#define TEST_SOFT_INT	  64

#ifdef TEST_max
#define FIBER_STACK_SIZE  1024
#else
#define FIBER_STACK_SIZE  512
#endif /* TEST_max */

#ifdef TEST_max
#define MESSAGE "Running maximal nanokernel configuration\n"
#else
#define MESSAGE "Running regular nanokernel configuration\n"
#endif /* TEST_max */

typedef void* (*pfunc) (void*);

/* stack used by fiber */
static char __stack pStack[FIBER_STACK_SIZE];

static pfunc func_array[] = {
	/* timers */
	(pfunc)k_timer_init,
	(pfunc)k_timer_stop,
	(pfunc)k_timer_status_get,
	(pfunc)k_timer_status_sync,
	(pfunc)k_timer_remaining_get,
	(pfunc)k_uptime_get,
	(pfunc)k_uptime_get_32,
	(pfunc)k_uptime_delta,
	(pfunc)k_uptime_delta_32,
	(pfunc)k_cycle_get_32,

	/* semaphores */
	(pfunc)k_sem_init,
	(pfunc)k_sem_take,
	(pfunc)k_sem_give,
	(pfunc)k_sem_reset,
	(pfunc)k_sem_count_get,

#ifdef TEST_max
	/* LIFOs */
	(pfunc)k_lifo_init,
	(pfunc)k_lifo_put,
	(pfunc)k_lifo_get,

	/* stacks */
	(pfunc)k_stack_init,
	(pfunc)k_stack_push,
	(pfunc)k_stack_pop,

	/* FIFOs */
	(pfunc)k_fifo_init,
	(pfunc)k_fifo_put,
	(pfunc)k_fifo_put_list,
	(pfunc)k_fifo_put_slist,
	(pfunc)k_fifo_get,
#endif
};

/**
 *
 * @brief Dummy ISR
 *
 * @return N/A
 */

void dummyIsr(void *unused)
{
	ARG_UNUSED(unused);
}

/**
 *
 * @brief Trivial fiber
 *
 * @param message   Message to be printed.
 * @param arg1		Unused.
 *
 * @return N/A
 */

static void fiberEntry(int message,	int arg1)
{
	ARG_UNUSED(arg1);

#ifdef TEST_max
	printf((char *)message);
#else
	printk((char *)message);
#endif /* TEST_max */
}

#endif /* !TEST_min */

/**
 *
 * @brief Mainline for background task
 *
 * This routine simply increments a global counter.
 * (Gdb can be used to observe the counter as it increases.)
 *
 * @return N/A
 */

void main(void)
{
#ifdef TEST_reg
	IRQ_CONNECT(IRQ_LINE, IRQ_PRIORITY, dummyIsr, NULL, 0);
#endif
#ifndef TEST_min
	/* start a trivial fiber */
	task_fiber_start(pStack, FIBER_STACK_SIZE, fiberEntry, (int) MESSAGE,
					 (int) func_array, 10, 0);
#endif /* !TEST_min */

	while (1) {
		i++;
	}
}
