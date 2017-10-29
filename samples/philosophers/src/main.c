/*
 * Copyright (c) 2011-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Dining philosophers demo
 *
 * The demo can be configured to use different object types for its
 * synchronization: SEMAPHORES, MUTEXES, STACKS, FIFOS and LIFOS. To configure
 * a specific object, set the value of FORKS to one of these.
 *
 * By default, the demo uses MUTEXES.
 *
 * The demo can also be configured to work with static objects or dynamic
 * objects. The behavior will change depending if STATIC_OBJS is set to 0 or
 * 1.
 *
 * By default, the demo uses dynamic objects.
 *
 * The demo can be configured to work with threads of the same priority or
 * not. If using different priorities, two threads will be cooperative
 * threads, and the other four will be preemptible threads; if using one
 * priority, there will be six preemptible threads of priority 0. This is
 * changed via SAME_PRIO.
 *
 * By default, the demo uses different priorities.
 *
 * The number of threads is set via NUM_PHIL. The demo has only been tested
 * with six threads. In theory it should work with any number of threads, but
 * not without making changes to the forks[] array in the phil_obj_abstract.h
 * header file.
 */

#include <zephyr.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#else
#include <misc/printk.h>
#endif

#include <misc/__assert.h>

#define SEMAPHORES 1
#define MUTEXES 2
#define STACKS 3
#define FIFOS 4
#define LIFOS 5

/**************************************/
/* control the behaviour of the demo **/

#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF 0
#endif

#ifndef NUM_PHIL
#define NUM_PHIL 6
#endif

#ifndef STATIC_OBJS
#define STATIC_OBJS 0
#endif

#ifndef FORKS
#define FORKS MUTEXES
#if 0
#define FORKS SEMAPHORES
#define FORKS STACKS
#define FORKS FIFOS
#define FORKS LIFOS
#endif
#endif

#define SAME_PRIO 0

/* end - control behaviour of the demo */
/***************************************/

#define STACK_SIZE 768

/*
 * There are multiple threads doing printfs and they may conflict.
 * Therefore use puts() instead of printf().
 */
#if defined(CONFIG_STDOUT_CONSOLE)
#define PRINTF(...) { char output[256]; \
		      sprintf(output, __VA_ARGS__); puts(output); }
#else
#define PRINTF(...) printk(__VA_ARGS__)
#endif

#if DEBUG_PRINTF
#define PR_DEBUG PRINTF
#else
#define PR_DEBUG(...)
#endif

#include "phil_obj_abstract.h"

#define fork(x) (forks[x])

static void set_phil_state_pos(int id)
{
#if !DEBUG_PRINTF
	PRINTF("\x1b[%d;%dH", id + 1, 1);
#endif
}

#include <stdarg.h>
static void print_phil_state(int id, const char *fmt, s32_t delay)
{
	int prio = k_thread_priority_get(k_current_get());

	set_phil_state_pos(id);

	PRINTF("Philosopher %d [%s:%s%d] ",
	       id, prio < 0 ? "C" : "P",
	       prio < 0 ? "" : " ",
	       prio);

	if (delay) {
		PRINTF(fmt, delay < 1000 ? " " : "", delay);
	} else {
		PRINTF(fmt, "");
	}

	PRINTF("\n");
}

static s32_t get_random_delay(int id, int period_in_ms)
{
	/*
	 * The random delay is unit-less, and is based on the philosopher's ID
	 * and the current uptime to create some pseudo-randomness. It produces
	 * a value between 0 and 31.
	 */
	k_enable_sys_clock_always_on();
	s32_t delay = (k_uptime_get_32()/100 * (id + 1)) & 0x1f;
	k_disable_sys_clock_always_on();

	/* add 1 to not generate a delay of 0 */
	s32_t ms = (delay + 1) * period_in_ms;

	return ms;
}

static inline int is_last_philosopher(int id)
{
	return id == (NUM_PHIL - 1);
}

void philosopher(void *id, void *unused1, void *unused2)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	fork_t fork1;
	fork_t fork2;

	int my_id = (int)id;

	/* Djkstra's solution: always pick up the lowest numbered fork first */
	if (is_last_philosopher(my_id)) {
		fork1 = fork(0);
		fork2 = fork(my_id);
	} else {
		fork1 = fork(my_id);
		fork2 = fork(my_id + 1);
	}

	while (1) {
		s32_t delay;

		print_phil_state(my_id, "       STARVING       ", 0);
		take(fork1);
		print_phil_state(my_id, "   HOLDING ONE FORK   ", 0);
		take(fork2);

		delay = get_random_delay(my_id, 25);
		print_phil_state(my_id, "  EATING  [ %s%d ms ] ", delay);
		k_sleep(delay);

		drop(fork2);
		print_phil_state(my_id, "   DROPPED ONE FORK   ", 0);
		drop(fork1);

		delay = get_random_delay(my_id, 25);
		print_phil_state(my_id, " THINKING [ %s%d ms ] ", delay);
		k_sleep(delay);
	}

}

static int new_prio(int phil)
{
#if defined(CONFIG_COOP_ENABLED) && defined(CONFIG_PREEMPT_ENABLED)
#if SAME_PRIO
	return 0;
#else
	return -(phil - (NUM_PHIL/2));
#endif
#else
#if defined(CONFIG_COOP_ENABLED)
	return -phil - 2;
#elif defined(CONFIG_PREEMPT_ENABLED)
	return phil;
#else
	#error unpossible
#endif
#endif
}

static void init_objects(void)
{
#if !STATIC_OBJS
	for (int i = 0; i < NUM_PHIL; i++) {
		fork_init(fork(i));
	}
#endif
}

static void start_threads(void)
{
	/*
	 * create two coop. threads (prios -2/-1) and four preemptive threads
	 * : (prios 0-3)
	 */
	for (int i = 0; i < NUM_PHIL; i++) {
		int prio = new_prio(i);

		k_thread_create(&threads[i], &stacks[i][0], STACK_SIZE,
				philosopher, (void *)i, NULL, NULL, prio,
				K_USER, K_FOREVER);

		k_object_access_grant(fork(i), &threads[i]);
		k_object_access_grant(fork((i + 1) % NUM_PHIL), &threads[i]);

		k_thread_start(&threads[i]);
	}
}

#define DEMO_DESCRIPTION  \
	"\x1b[2J\x1b[15;1H"   \
	"Demo Description\n"  \
	"----------------\n"  \
	"An implementation of a solution to the Dining Philosophers\n" \
	"problem (a classic multi-thread synchronization problem).\n" \
	"This particular implementation demonstrates the usage of multiple\n" \
	"preemptible and cooperative threads of differing priorities, as\n" \
	"well as %s %s and thread sleeping.\n", obj_init_type, fork_type_str

static void display_demo_description(void)
{
#if !DEBUG_PRINTF
	PRINTF(DEMO_DESCRIPTION);
#endif
}

void main(void)
{
	display_demo_description();

	k_sched_time_slice_set(5000, 0);

	init_objects();
	start_threads();
}
