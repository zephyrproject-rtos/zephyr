/*
 * Copyright (c) 2017 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/printk.h>
#include <ztest.h>

#define NUM_TIMEOUTS 3

static struct k_timer timer[NUM_TIMEOUTS];
static struct k_sem sem[NUM_TIMEOUTS];

static int results[NUM_TIMEOUTS], cur;

static void thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	uintptr_t id = (uintptr_t)p1;

	k_timer_status_sync(&timer[id]);
	printk("%s %" PRIxPTR " synced on timer %" PRIxPTR "\n",
	       __func__, id, id);

	/* no need to protect cur, all threads have the same prio */
	results[cur++] = id;

	k_sem_give(&sem[id]);
}

#define STACKSIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)

static K_THREAD_STACK_ARRAY_DEFINE(stacks, NUM_TIMEOUTS, STACKSIZE);
static struct k_thread threads[NUM_TIMEOUTS];

/**
 * @addtogroup kernel_common_tests
 * @{
 */

/**
 * @brief Test timer functionalities
 *
 * @details Test polling events with timers
 *
 * @see k_timer_start(), k_poll_event_init()
 */
void test_timeout_order(void)
{
	int ii, prio = k_thread_priority_get(k_current_get()) + 1;

	for (ii = 0; ii < NUM_TIMEOUTS; ii++) {
		(void)k_thread_create(&threads[ii], stacks[ii], STACKSIZE,
				      thread, INT_TO_POINTER(ii), 0, 0,
				      prio, 0, K_NO_WAIT);
		k_timer_init(&timer[ii], 0, 0);
		k_sem_init(&sem[ii], 0, 1);
		results[ii] = -1;
	}


	u32_t uptime = k_uptime_get_32();

	/* sync on tick */
	while (uptime == k_uptime_get_32()) {
#if defined(CONFIG_ARCH_POSIX)
		k_busy_wait(50);
#endif
	}

	for (ii = 0; ii < NUM_TIMEOUTS; ii++) {
		k_timer_start(&timer[ii], K_MSEC(100), K_NO_WAIT);
	}

	struct k_poll_event poll_events[NUM_TIMEOUTS];

	for (ii = 0; ii < NUM_TIMEOUTS; ii++) {
		k_poll_event_init(&poll_events[ii], K_POLL_TYPE_SEM_AVAILABLE,
				  K_POLL_MODE_NOTIFY_ONLY, &sem[ii]);
	}

	/* drop prio to get all poll events together */
	k_thread_priority_set(k_current_get(), prio + 1);

	zassert_equal(k_poll(poll_events, NUM_TIMEOUTS, K_MSEC(2000)), 0, "");

	k_thread_priority_set(k_current_get(), prio - 1);

	for (ii = 0; ii < NUM_TIMEOUTS; ii++) {
		zassert_equal(poll_events[ii].state,
			      K_POLL_STATE_SEM_AVAILABLE, "");
	}
	for (ii = 0; ii < NUM_TIMEOUTS; ii++) {
		zassert_equal(results[ii], ii, "");
	}
}

/**
 * @}
 */
