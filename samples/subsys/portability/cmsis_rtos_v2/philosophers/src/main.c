/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Dining philosophers demo
 *
 * The demo can be configured to use SEMAPHORES or MUTEXES as object types for its
 * synchronization. To configure a specific object, set the value of FORKS to one
 * of these.
 *
 * By default, the demo uses MUTEXES.
 *
 * The demo can be configured to work with threads of the same priority or
 * not. If using different priorities of preemptible threads; if using one
 * priority, there will be six preemptible threads of priority normal.
 *
 * By default, the demo uses different priorities.
 *
 * The number of threads is set via NUM_PHIL. The demo has only been tested
 * with six threads. In theory it should work with any number of threads, but
 * not without making changes to the thread attributes and corresponding kernel
 * object attributes array in the phil_obj_abstract.h
 * header file.
 */

#include <zephyr.h>
#include <kernel.h>
#include <cmsis_os2.h>
#include <sys/printk.h>

#include <sys/__assert.h>

#include "phil_obj_abstract.h"

#define SEMAPHORES 1
#define MUTEXES 2

/**************************************/
/* control the behaviour of the demo **/

#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF 0
#endif

#ifndef FORKS
#define FORKS MUTEXES
#if 0
#define FORKS SEMAPHORES
#endif
#endif

#ifndef SAME_PRIO
#define SAME_PRIO 0
#endif

#ifndef NUM_PHIL
#define NUM_PHIL 6
#endif

/* end - control behaviour of the demo */
/***************************************/
osSemaphoreId_t forks[NUM_PHIL];

#define fork(x) (forks[x])

#define STACK_SIZE CONFIG_CMSIS_V2_THREAD_MAX_STACK_SIZE
static K_THREAD_STACK_ARRAY_DEFINE(stacks, NUM_PHIL, STACK_SIZE);
static osThreadAttr_t thread_attr[] = {
	{
		.name       = "Thread0",
		.stack_mem  = &stacks[0][0],
		.stack_size = STACK_SIZE,
		.priority   = osPriorityHigh
	},
	{
		.name       = "Thread1",
		.stack_mem  = &stacks[1][0],
		.stack_size = STACK_SIZE,
		.priority   = osPriorityHigh
	},
	{
		.name       = "Thread2",
		.stack_mem  = &stacks[2][0],
		.stack_size = STACK_SIZE,
		.priority   = osPriorityHigh
	},
	{
		.name       = "Thread3",
		.stack_mem  = &stacks[3][0],
		.stack_size = STACK_SIZE,
		.priority   = osPriorityHigh
	},
	{
		.name       = "Thread4",
		.stack_mem  = &stacks[4][0],
		.stack_size = STACK_SIZE,
		.priority   = osPriorityHigh
	},
	{
		.name       = "Thread5",
		.stack_mem  = &stacks[5][0],
		.stack_size = STACK_SIZE,
		.priority   = osPriorityHigh
	}
};


#if DEBUG_PRINTF
#define PR_DEBUG printk
#else
#define PR_DEBUG(...)
#endif

#include "phil_obj_abstract.h"

static void set_phil_state_pos(int id)
{
#if !DEBUG_PRINTF
	printk("\x1b[%d;%dH", id + 1, 1);
#endif
}

#include <stdarg.h>
static void print_phil_state(int id, const char *fmt, int32_t delay)
{
	int prio = osThreadGetPriority(osThreadGetId());

	set_phil_state_pos(id);

	printk("Philosopher %d [%s:%s%d] ",
	       id, prio < 0 ? "C" : "P",
	       prio < 0 ? "" : " ",
	       prio);

	if (delay) {
		printk(fmt, delay < 1000 ? " " : "", delay);
	} else {
		printk(fmt, "");
	}

	printk("\n");
}

static int32_t get_random_delay(int id, int period_in_ms)
{
	/*
	 * The random delay is unit-less, and is based on the philosopher's ID
	 * and the current uptime to create some pseudo-randomness. It produces
	 * a value between 0 and 31.
	 */
	int32_t delay = (k_uptime_get_32()/100 * (id + 1)) & 0x1f;

	/* add 1 to not generate a delay of 0 */
	int32_t ms = (delay + 1) * period_in_ms;

	return ms;
}

static inline int is_last_philosopher(int id)
{
	return id == (NUM_PHIL - 1);
}

void philosopher(void *id)
{
	fork_t fork1;
	fork_t fork2;

	int my_id = POINTER_TO_INT(id);

	/* Djkstra's solution: always pick up the lowest numbered fork first */
	if (is_last_philosopher(my_id)) {
		fork1 = fork(0);
		fork2 = fork(my_id);
	} else {
		fork1 = fork(my_id);
		fork2 = fork(my_id + 1);
	}

	while (1) {
		int32_t delay;

		print_phil_state(my_id, "       STARVING       ", 0);
		take(fork1);
		print_phil_state(my_id, "   HOLDING ONE FORK   ", 0);
		take(fork2);

		delay = get_random_delay(my_id, 25);
		print_phil_state(my_id, "  EATING  [ %s%d ms ] ", delay);
		osDelay(delay);

		drop(fork2);
		print_phil_state(my_id, "   DROPPED ONE FORK   ", 0);
		drop(fork1);

		delay = get_random_delay(my_id, 25);
		print_phil_state(my_id, " THINKING [ %s%d ms ] ", delay);
		osDelay(delay);
	}

}

static int new_prio(int phil)
{
#if SAME_PRIO
	return osPriorityNormal;
#else
	return osPriorityLow + phil;
#endif
}

static void init_objects(void)
{
	for (int i = 0; i < NUM_PHIL; i++) {
		forks[i] = fork_init(i);
	}
}

static void start_threads(void)
{
	for (int i = 0; i < NUM_PHIL; i++) {
		int prio = new_prio(i);
		thread_attr[i].priority = prio;
		osThreadNew(philosopher, INT_TO_POINTER(i), &thread_attr[i]);
	}
}

#define DEMO_DESCRIPTION  \
	"\x1b[2J\x1b[15;1H"   \
	"Demo Description\n"  \
	"----------------\n"  \
	"An implementation of a solution to the Dining Philosophers\n" \
	"problem (a classic multi-thread synchronization problem) using\n" \
	"CMSIS RTOS V2 APIs. This particular implementation demonstrates the\n" \
	"usage of multiple preemptible threads of differing\n" \
	"priorities, as well as %s and thread sleeping.\n", fork_type_str

static void display_demo_description(void)
{
#if !DEBUG_PRINTF
	printk(DEMO_DESCRIPTION);
#endif
}

void main(void)
{
	display_demo_description();
	init_objects();
	start_threads();

#ifdef CONFIG_COVERAGE
	/* Wait a few seconds before main() exit, giving the sample the
	 * opportunity to dump some output before coverage data gets emitted
	 */
	k_sleep(K_MSEC(5000));
#endif
}
