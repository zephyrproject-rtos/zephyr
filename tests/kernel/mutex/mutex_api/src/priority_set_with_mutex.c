/*
 * Copyright 2024 by Garmin Ltd. or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

#define PRIO_HIGH K_PRIO_PREEMPT(1)
#define PRIO_MID  K_PRIO_PREEMPT(3)
#define PRIO_LOW  K_PRIO_PREEMPT(5)

#if Z_MUTEX_PI_ENABLED
static struct k_mutex mutex;
static K_THREAD_STACK_DEFINE(stack_high, STACK_SIZE);
static struct k_thread t_high;
static K_SEM_DEFINE(sem_waiter_started, 0, 1);

static void t_waiter(void *p1, void *p2, void *p3)
{
	struct k_mutex *m = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_give(&sem_waiter_started);
	k_mutex_lock(m, K_FOREVER);
	k_mutex_unlock(m);
}
#endif /* Z_MUTEX_PI_ENABLED */

/**
 * Verify that changing a mutex owner's base priority does not drop an active
 * priority-inheritance boost, and that unlock restores the new base priority.
 */
ZTEST(mutex_api, test_priority_set_with_mutex_locked)
{
#if Z_MUTEX_PI_ENABLED
	int original_priority = k_thread_priority_get(k_current_get());
	int priority_after_waiter;
	int priority_after_set;
	int priority_after_unlock;
	int attempt;

	k_sem_reset(&sem_waiter_started);
	k_mutex_init(&mutex);
	k_thread_priority_set(k_current_get(), PRIO_LOW);
	k_mutex_lock(&mutex, K_FOREVER);

	k_thread_create(&t_high, stack_high, STACK_SIZE, t_waiter, &mutex, NULL, NULL,
			PRIO_HIGH, 0, K_NO_WAIT);
	k_sem_take(&sem_waiter_started, K_FOREVER);

	/* Wait until the waiter blocks on the mutex and boosts its owner. */
	for (attempt = 0; attempt < 100; attempt++) {
		if (k_thread_priority_get(k_current_get()) == PRIO_HIGH) {
			break;
		}
		k_sleep(K_MSEC(1));
	}
	priority_after_waiter = k_thread_priority_get(k_current_get());

	/* Updating the base priority must not override the active PI boost. */
	k_thread_priority_set(k_current_get(), PRIO_MID);
	priority_after_set = k_thread_priority_get(k_current_get());

	k_mutex_unlock(&mutex);
	priority_after_unlock = k_thread_priority_get(k_current_get());

	k_thread_join(&t_high, K_FOREVER);
	k_thread_priority_set(k_current_get(), original_priority);

	zassert_equal(priority_after_waiter, PRIO_HIGH,
		      "Expected PI boost to %d, got %d", PRIO_HIGH,
		      priority_after_waiter);
	zassert_equal(priority_after_set, PRIO_HIGH,
		      "Expected PI boost to remain %d after priority_set, got %d",
		      PRIO_HIGH, priority_after_set);
	zassert_equal(priority_after_unlock, PRIO_MID,
		      "Expected priority to restore to %d after unlock, got %d",
		      PRIO_MID, priority_after_unlock);
#else
	ztest_test_skip();
#endif /* Z_MUTEX_PI_ENABLED */
}
