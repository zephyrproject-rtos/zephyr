/*
 * Copyright (c) 2022 SeediqQ from Issue 48056
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#define TIMEOUT_MS 100
#define STACKSZ	   8192

static struct k_mutex mutex;
static struct k_thread thread;

K_THREAD_STACK_DEFINE(stack, STACKSZ);

static void test_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	zassert_equal(-EAGAIN, k_mutex_lock(&mutex, K_MSEC(TIMEOUT_MS)));
}

/**
 * @brief Test fix for subtle race during priority inversion
 *
 * - A low priority thread (Tlow) locks mutex A.
 * - A high priority thread (Thigh) blocks on mutex A, boosting the priority
 *   of Tlow.
 * - Thigh times out waiting for mutex A.
 * - Before Thigh has a chance to execute, Tlow unlocks mutex A (which now
 *   has no owner) and drops its own priority.
 * - Thigh now gets a chance to execute and finds that it timed out, and
 *   then enters the block of code to lower the priority of the thread that
 *   owns mutex A (now nobody).
 * - Thigh tries to the dereference the owner of mutex A (which is nobody,
 *   and thus it is NULL). This leads to an exception.
 *
 * @ingroup kernel_mutex_tests
 *
 * @see k_mutex_lock()
 */
ZTEST(mutex_timeout_race_during_priority_inversion, test_mutex_timeout_error)
{
	k_mutex_init(&mutex);

	/* align to tick boundary */
	k_sleep(K_TICKS(1));
	k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), test_thread, NULL, NULL, NULL,
			8, 0, K_NO_WAIT);

	k_mutex_lock(&mutex, K_FOREVER);

	k_sleep(K_MSEC(TIMEOUT_MS));

	k_mutex_unlock(&mutex);
}

ZTEST_SUITE(mutex_timeout_race_during_priority_inversion, NULL, NULL, NULL, NULL, NULL);
