/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/internal/critical_section_hooks.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/ztest.h>

BUILD_ASSERT(!IS_ENABLED(CONFIG_CRITICAL_SECTION_MONITOR));

enum hook_event { BEGIN, END, IDLE, ATTEMPT, ACQUIRED, ABORT, UNLOCKED, RELEASED };

struct hook_capture {
	enum hook_event events[6];
	size_t count;
	bool irqs_enabled;
	bool wrong_lock_state;
	struct k_spinlock *owned_lock;
	const struct k_spinlock *lock;
};

static struct hook_capture *captures[CONFIG_MP_MAX_NUM_CPUS];
static bool ready;

static int hooks_init(void)
{
	ready = true;
	return 0;
}

SYS_INIT(hooks_init, POST_KERNEL, 0);

static void record(enum hook_event event, const struct k_spinlock *lock)
{
	if (!ready) {
		return;
	}
	struct hook_capture *capture = captures[arch_curr_cpu()->id];

	if (capture == NULL) {
		return;
	}
	if (capture->count < ARRAY_SIZE(capture->events)) {
		capture->events[capture->count] = event;
	}
	capture->count++;
	capture->irqs_enabled |= arch_cpu_irqs_are_enabled();
	capture->lock = lock;
	/* This private fixture can be inspected after release: it is not shared or freed. */
	if (IS_ENABLED(CONFIG_SMP) && capture->owned_lock != NULL &&
	    (event == ACQUIRED || event == UNLOCKED || event == RELEASED)) {
		capture->wrong_lock_state |=
			z_spin_is_locked(capture->owned_lock) != (event == ACQUIRED);
	}
}

void z_crit_hook_irq_begin(unsigned int key, uintptr_t caller)
{
	ARG_UNUSED(key);
	ARG_UNUSED(caller);
	record(BEGIN, NULL);
}

void z_crit_hook_irq_end(unsigned int key)
{
	ARG_UNUSED(key);
	record(END, NULL);
}

void z_crit_hook_irq_idle(unsigned int key)
{
	ARG_UNUSED(key);
	record(IDLE, NULL);
}

#ifdef CONFIG_SMP
void z_crit_hook_spin_attempt(const struct k_spinlock *lock, unsigned int key)
{
	ARG_UNUSED(key);
	record(ATTEMPT, lock);
}

void z_crit_hook_spin_abort(const struct k_spinlock *lock, unsigned int key)
{
	ARG_UNUSED(key);
	record(ABORT, lock);
}
#endif

void z_crit_hook_spin_acquired(const struct k_spinlock *lock, unsigned int key)
{
	ARG_UNUSED(key);
	record(ACQUIRED, lock);
}

void z_crit_hook_spin_unlocked(const struct k_spinlock *lock, unsigned int key)
{
	ARG_UNUSED(key);
	record(UNLOCKED, lock);
}

void z_crit_hook_spin_released(const struct k_spinlock *lock)
{
	record(RELEASED, lock);
}

static void check_capture(struct hook_capture *capture, const enum hook_event *events, size_t count)
{
	zassert_false(capture->irqs_enabled, "Hooks must run with local IRQs locked");
	zassert_false(capture->wrong_lock_state, "Hook is on the wrong side of the lock operation");
	zassert_equal(capture->count, count);
	zassert_mem_equal(capture->events, events, count * sizeof(*events));
}

ZTEST(critical_section_hooks, test_irq_hooks)
{
	struct hook_capture capture = {0};
	const enum hook_event events[] = {BEGIN, END, IDLE};
	unsigned int raw_key = arch_irq_lock();

	/* Capture on this CPU only; unrelated kernel callbacks remain unobserved. */
	captures[arch_curr_cpu()->id] = &capture;
	unsigned int key = irq_lock();

	irq_unlock(key);
	z_crit_irq_idle_enter();
	captures[arch_curr_cpu()->id] = NULL;
	arch_irq_unlock(raw_key);
	check_capture(&capture, events, ARRAY_SIZE(events));
}

ZTEST(critical_section_hooks, test_spinlock_hooks)
{
	struct hook_capture capture = {0};
	struct k_spinlock lock = {0};
	const enum hook_event events[] = {
		IF_ENABLED(CONFIG_SMP, (ATTEMPT,)) ACQUIRED, UNLOCKED,
		IF_ENABLED(CONFIG_SMP, (ATTEMPT,)) ACQUIRED, RELEASED,
	};
	unsigned int raw_key = arch_irq_lock();

	capture.owned_lock = &lock;
	captures[arch_curr_cpu()->id] = &capture;
	k_spinlock_key_t key = k_spin_lock(&lock);

	k_spin_unlock(&lock, key);
	int result = k_spin_trylock(&lock, &key);

	if (result == 0) {
		k_spin_release(&lock);
		arch_irq_unlock(key.key);
	}
	captures[arch_curr_cpu()->id] = NULL;
	arch_irq_unlock(raw_key);
	zassert_ok(result);
	zassert_equal_ptr(capture.lock, &lock);
	check_capture(&capture, events, ARRAY_SIZE(events));
}

#ifdef CONFIG_SMP
ZTEST(critical_section_hooks, test_failed_trylock)
{
	struct hook_capture capture = {0};
	struct k_spinlock lock = {0};
	const enum hook_event events[] = {ATTEMPT, ABORT};
	k_spinlock_key_t key;
	unsigned int raw_key = arch_irq_lock();

	/* This private lock models a busy lock without another CPU or a wait. */
#ifdef CONFIG_TICKET_SPINLOCKS
	lock.tail = 1;
#else
	lock.locked = 1;
#endif
	captures[arch_curr_cpu()->id] = &capture;
	int result = k_spin_trylock(&lock, &key);

	captures[arch_curr_cpu()->id] = NULL;
	arch_irq_unlock(raw_key);
	zassert_equal(result, -EBUSY);
	check_capture(&capture, events, ARRAY_SIZE(events));
}
#endif

ZTEST_SUITE(critical_section_hooks, NULL, NULL, NULL, NULL, NULL);
