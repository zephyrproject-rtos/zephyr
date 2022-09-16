/*
 * Copyright (c) 2017 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
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

	/* no need to protect cur, all threads have the same prio */
	results[cur++] = id;

	k_sem_give(&sem[id]);
}

#define STACKSIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

static K_THREAD_STACK_ARRAY_DEFINE(stacks, NUM_TIMEOUTS, STACKSIZE);
static struct k_thread threads[NUM_TIMEOUTS];

/**
 * @addtogroup kernel_common_tests
 * @{
 */

/**
 * @brief Test timeout ordering
 *
 * @details Timeouts, when expiring on the same tick, should be handled
 * in the same order they were queued.
 *
 * @see k_timer_start()
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


	uint32_t uptime = k_uptime_get_32();

	/* sync on tick */
	while (uptime == k_uptime_get_32()) {
#if defined(CONFIG_ARCH_POSIX)
		k_busy_wait(50);
#endif
	}

	for (ii = 0; ii < NUM_TIMEOUTS; ii++) {
		k_timer_start(&timer[ii], K_MSEC(100), K_NO_WAIT);
	}

	/* Wait for all timers to fire */
	k_msleep(125);

	/* Check results */
	for (ii = 0; ii < NUM_TIMEOUTS; ii++) {
		zassert_equal(results[ii], ii, "");
	}

	/* Clean up */
	for (ii = 0; ii < NUM_TIMEOUTS; ii++) {
		k_timer_stop(&timer[ii]);
		k_thread_join(&threads[ii], K_FOREVER);
	}
}

/**
 * @}
 */
