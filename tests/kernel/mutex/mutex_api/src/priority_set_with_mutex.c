/*
 * Copyright 2024 by Garmin Ltd. or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

#define PRIO_HIGH K_PRIO_PREEMPT(1)
#define PRIO_MID  K_PRIO_PREEMPT(3)
#define PRIO_LOW  K_PRIO_PREEMPT(5)

#define TEST_MUTEX_PRIORITIES (Z_MUTEX_PI_ENABLED && CONFIG_NUM_PREEMPT_PRIORITIES > PRIO_LOW && \
			       CONFIG_PRIORITY_CEILING <= PRIO_HIGH)
#define TEST_USER_NO_RAISE (IS_ENABLED(CONFIG_USERSPACE) && \
			    !IS_ENABLED(CONFIG_USERSPACE_THREAD_MAY_RAISE_PRIORITY))

#if TEST_MUTEX_PRIORITIES
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
	zassert_ok(k_mutex_lock(m, K_FOREVER));
	zassert_ok(k_mutex_unlock(m));
}

static int start_mutex_waiter(void)
{
	int orig_prio = k_thread_priority_get(k_current_get());

	k_sem_reset(&sem_waiter_started);
	zassert_ok(k_mutex_init(&mutex));
	k_thread_priority_set(k_current_get(), PRIO_LOW);
	zassert_ok(k_mutex_lock(&mutex, K_FOREVER));
	k_thread_create(&t_high, stack_high, K_THREAD_STACK_SIZEOF(stack_high), t_waiter,
			&mutex, NULL, NULL, PRIO_HIGH, 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&sem_waiter_started, K_FOREVER));

	return orig_prio;
}

static int unlock_and_restore_owner(int orig_prio)
{
	zassert_ok(k_mutex_unlock(&mutex));
	int unlocked_prio = k_thread_priority_get(k_current_get());

	zassert_ok(k_thread_join(&t_high, K_FOREVER));
	k_thread_priority_set(k_current_get(), orig_prio);

	return unlocked_prio;
}
#endif /* TEST_MUTEX_PRIORITIES */

/**
 * Verify that changing a mutex owner's base priority does not drop an active
 * priority-inheritance boost, and that unlock restores the new base priority.
 */
ZTEST(mutex_api, test_priority_set_with_mutex_locked)
{
#if TEST_MUTEX_PRIORITIES
	int original_priority = start_mutex_waiter();
	int priority_after_waiter;
	int priority_after_set;
	int priority_after_unlock;
	int attempt;

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

	priority_after_unlock = unlock_and_restore_owner(original_priority);

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
#endif /* TEST_MUTEX_PRIORITIES */
}

#if TEST_MUTEX_PRIORITIES
static K_THREAD_STACK_DEFINE(stack_setter, STACK_SIZE);
static struct k_thread t_setter;

static void set_thread_priority(void *thread, void *unused1, void *unused2)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	k_thread_priority_set(thread, PRIO_MID);
}

#if TEST_USER_NO_RAISE && defined(CONFIG_ZTEST_FATAL_HOOK)
static int setter_fatal_reason;

void ztest_post_fatal_error_hook(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	setter_fatal_reason = (int)reason;
}

static void try_raise_owner_priority(void *owner, void *unused1, void *unused2)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	/* Verify access to the target before marking a fault as expected. */
	(void)k_thread_priority_get(owner);
	ztest_set_fault_valid(true);
	k_thread_priority_set(owner, PRIO_HIGH);
	ztest_set_fault_valid(false);
}
#endif
#endif /* TEST_MUTEX_PRIORITIES */

ZTEST(mutex_api_1cpu, test_priority_set_boosted_owner_from_another_thread)
{
#if TEST_MUTEX_PRIORITIES
	k_tid_t owner = k_current_get();
	int orig_prio = start_mutex_waiter();
	int boosted_prio = k_thread_priority_get(owner);

	k_thread_create(&t_setter, stack_setter, K_THREAD_STACK_SIZEOF(stack_setter),
			set_thread_priority, owner, NULL, NULL, PRIO_MID, 0, K_NO_WAIT);
	zassert_ok(k_thread_join(&t_setter, K_FOREVER));

	int updated_prio = k_thread_priority_get(owner);
	int unlocked_prio = unlock_and_restore_owner(orig_prio);

	zassert_equal(boosted_prio, PRIO_HIGH);
	zassert_equal(updated_prio, PRIO_HIGH);
	zassert_equal(unlocked_prio, PRIO_MID);
#else
	ztest_test_skip();
#endif
}

ZTEST(mutex_api_1cpu, test_user_cannot_raise_boosted_owner_priority)
{
#if TEST_MUTEX_PRIORITIES && TEST_USER_NO_RAISE && defined(CONFIG_ZTEST_FATAL_HOOK)
	k_tid_t owner = k_current_get();
	int orig_prio = start_mutex_waiter();
	int boosted_prio = k_thread_priority_get(owner);

	setter_fatal_reason = -1;
	k_thread_create(&t_setter, stack_setter, K_THREAD_STACK_SIZEOF(stack_setter),
			try_raise_owner_priority, owner, NULL, NULL, PRIO_MID, K_USER, K_FOREVER);
	k_thread_access_grant(&t_setter, owner);
	k_thread_start(&t_setter);
	zassert_ok(k_thread_join(&t_setter, K_FOREVER));

	int updated_prio = k_thread_priority_get(owner);
	int unlocked_prio = unlock_and_restore_owner(orig_prio);

	zassert_equal(boosted_prio, PRIO_HIGH);
	zassert_equal(setter_fatal_reason, K_ERR_KERNEL_OOPS);
	zassert_equal(updated_prio, PRIO_HIGH);
	zassert_equal(unlocked_prio, PRIO_LOW);
#else
	ztest_test_skip();
#endif
}

ZTEST(mutex_api, test_user_can_lower_priority_without_mutex)
{
#if TEST_MUTEX_PRIORITIES && TEST_USER_NO_RAISE
	k_thread_create(&t_setter, stack_setter, K_THREAD_STACK_SIZEOF(stack_setter),
			set_thread_priority, &t_setter, NULL, NULL, PRIO_HIGH, K_USER, K_FOREVER);
	/* Seed a stale saved priority before the thread starts. With no mutex held,
	 * the downgrade check must use its current priority (PRIO_HIGH).
	 */
	t_setter.orig_prio = PRIO_LOW;
	k_thread_start(&t_setter);
	zassert_ok(k_thread_join(&t_setter, K_FOREVER));

	zassert_equal(k_thread_priority_get(&t_setter), PRIO_MID);
#else
	ztest_test_skip();
#endif
}
