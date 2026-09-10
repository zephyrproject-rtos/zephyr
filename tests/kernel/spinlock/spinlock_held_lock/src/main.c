/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

extern struct k_spinlock _sched_spinlock;

static struct k_spinlock lock_a;
static struct k_spinlock lock_b;
static k_spinlock_key_t key_a;
static k_spinlock_key_t key_b;

static ZTEST_DMEM volatile bool expect_assert;
static ZTEST_DMEM bool held_a;
static ZTEST_DMEM bool held_b;
static ZTEST_DMEM bool held_irq;
static unsigned int irq_key;

#ifdef CONFIG_ASSERT_NO_FILE_INFO
void assert_post_action(void)
#else
void assert_post_action(const char *file, unsigned int line)
#endif
{
#ifndef CONFIG_ASSERT_NO_FILE_INFO
	ARG_UNUSED(file);
	ARG_UNUSED(line);
#endif
	if (!expect_assert) {
		k_panic();
	}

	expect_assert = false;
	/* On USE_SWITCH _sched_spinlock is still held at the assert site. */
	if (IS_ENABLED(CONFIG_USE_SWITCH)) {
		k_spin_release(&_sched_spinlock);
	}
	if (held_b) {
		k_spin_unlock(&lock_b, key_b);
		held_b = false;
	}
	if (held_a) {
		k_spin_unlock(&lock_a, key_a);
		held_a = false;
	}
	/* Restore IRQ state after spinlock cleanup. */
	if (held_irq) {
		irq_unlock(irq_key);
		held_irq = false;
	}
	k_wakeup(k_current_get());
	ztest_test_pass();
}

/** @brief Held spinlock address is reported on context switch assert */
ZTEST(spinlock_held_lock, test_context_switch_prints_held_lock)
{
	zassert_is_null(z_spin_get_held_lock(), NULL);

	key_a = k_spin_lock(&lock_a);
	held_a = true;

	zassert_equal_ptr(z_spin_get_held_lock(), &lock_a, NULL);

	expect_assert = true;
	k_sleep(K_MSEC(1));

	ztest_test_fail();
}

/** @brief With nested locks the outermost address is reported */
ZTEST(spinlock_held_lock, test_context_switch_nested_prints_outer)
{
	zassert_is_null(z_spin_get_held_lock(), NULL);

	key_a = k_spin_lock(&lock_a);
	held_a = true;
	key_b = k_spin_lock(&lock_b);
	held_b = true;

	zassert_equal_ptr(z_spin_get_held_lock(), &lock_a, NULL);

	expect_assert = true;
	k_sleep(K_MSEC(1));

	ztest_test_fail();
}

/**
 * @brief Count check catches a hidden lock after ABA unlock
 *
 * lock(A), lock(B), unlock(A): slot cleared but count stays 1.
 * The pointer check alone misses this; the count check fires.
 */
ZTEST(spinlock_held_lock, test_context_switch_count_detects_unreleased_lock)
{
	zassert_is_null(z_spin_get_held_lock(), NULL);

	key_a = k_spin_lock(&lock_a);
	key_b = k_spin_lock(&lock_b);
	held_b = true;

	k_spin_unlock(&lock_a, key_a);
	zassert_is_null(z_spin_get_held_lock(), NULL);

	expect_assert = true;
	k_sleep(K_MSEC(1));

	ztest_test_fail();
}

/**
 * @brief Context switch while irq_lock() is held fires the IRQ state assert
 *
 * Skipped on ARM64 where the IRQ state check is disabled (see #35307).
 */
ZTEST(spinlock_held_lock, test_context_switch_with_irq_lock_held)
{
#ifdef CONFIG_ARM64
	ztest_test_skip();
#else
	irq_key = irq_lock();
	held_irq = true;

	expect_assert = true;
	k_sleep(K_MSEC(1));

	irq_unlock(irq_key);
	ztest_test_fail();
#endif
}

/** @brief z_pend_curr lock-swap pattern must not trigger a false positive */
ZTEST(spinlock_held_lock, test_no_false_positive_pend_pattern)
{
	struct k_sem sem;

	k_sem_init(&sem, 0, 1);
	zassert_equal(k_sem_take(&sem, K_MSEC(1)), -EAGAIN);
}

ZTEST_SUITE(spinlock_held_lock, NULL, NULL, NULL, NULL, NULL);
