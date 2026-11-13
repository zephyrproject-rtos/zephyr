/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/* Checks that the idle thread's stack usage does not grow across a large
 * number of context switches.
 *
 * Two threads each sleep for a single tick, one of them offset by a busy wait
 * that grows every round, so the point at which a wakeup lands relative to a
 * switch into idle sweeps through the tick period instead of always falling in
 * the same place. Idle is the subject because it is entered whenever every
 * other thread is pending, and it has the smallest stack in the system.
 *
 * The measurement is idle's unused stack before and after, as reported by
 * k_thread_stack_space_get(). Sampling the stack rather than waiting for a
 * fault is deliberate: stack lost a frame at a time runs off the bottom into
 * whatever sits below it rather than tripping a guard, so a fault would
 * surface late and somewhere unrelated.
 */

#define ROUNDS       3000
#define HELPER_STACK 512
#define HELPER_PRIO  K_PRIO_PREEMPT(0)

/* Two samples of idle's high-water mark differ by up to one exception frame,
 * since an interrupt can be nested at its deepest point when one is taken.
 * Past that, stack is not being given back.
 */
#define SLACK 48

static K_THREAD_STACK_DEFINE(helper_stack, HELPER_STACK);
static struct k_thread helper_thread;

static struct k_thread *idle_thread;

static void find_idle_cb(const struct k_thread *thread, void *user_data)
{
	const char *name = k_thread_name_get((k_tid_t)thread);

	ARG_UNUSED(user_data);

	if ((idle_thread == NULL) && (name != NULL) && (strncmp(name, "idle", 4) == 0)) {
		idle_thread = (struct k_thread *)thread;
	}
}

static void helper_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (int i = 0; i < ROUNDS; i++) {
		/* Drift against the tick so successive wakeups land at
		 * different points of the switch into idle.
		 */
		k_busy_wait(i % 101);
		k_sleep(K_TICKS(1));
	}
}

ZTEST(arm_switch_restore, test_idle_stack_survives_switch_restore)
{
	size_t before, after;
	int ret;

	k_thread_foreach_unlocked(find_idle_cb, NULL);
	zassert_not_null(idle_thread, "idle thread not found");

	/* Let the system settle so the reading below is idle's steady state. */
	k_sleep(K_TICKS(10));

	ret = k_thread_stack_space_get(idle_thread, &before);
	zassert_equal(ret, 0, "k_thread_stack_space_get() failed: %d", ret);

	k_thread_create(&helper_thread, helper_stack, K_THREAD_STACK_SIZEOF(helper_stack),
			helper_entry, NULL, NULL, NULL, HELPER_PRIO, 0, K_NO_WAIT);

	for (int i = 0; i < ROUNDS; i++) {
		k_sleep(K_TICKS(1));
	}

	k_thread_join(&helper_thread, K_FOREVER);

	ret = k_thread_stack_space_get(idle_thread, &after);
	zassert_equal(ret, 0, "k_thread_stack_space_get() failed: %d", ret);

	TC_PRINT("idle stack unused: %zu -> %zu bytes over %d rounds\n", before, after, ROUNDS);

	zassert_true(after + SLACK >= before,
		     "idle leaked %zu bytes of stack (%zu unused before, %zu after)",
		     before - after, before, after);
}

ZTEST_SUITE(arm_switch_restore, NULL, NULL, NULL, NULL, NULL);
