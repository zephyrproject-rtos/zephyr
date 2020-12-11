/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <tc_util.h>
#include <ztest.h>
#include <kernel.h>
#include <spinlock.h>

BUILD_ASSERT(CONFIG_MP_NUM_CPUS > 1);

#define CPU1_STACK_SIZE 1024

K_THREAD_STACK_DEFINE(cpu1_stack, CPU1_STACK_SIZE);
struct k_thread cpu1_thread;

static struct k_spinlock bounce_lock;

volatile int bounce_owner, bounce_done;

/**
 * @brief Tests for spinlock
 *
 * @defgroup kernel_spinlock_tests Spinlock Tests
 *
 * @ingroup all_tests
 *
 * @{
 * @}
 */

/**
 * @brief Test basic spinlock
 *
 * @ingroup kernel_spinlock_tests
 *
 * @see k_spin_lock(), k_spin_unlock()
 */
void test_spinlock_basic(void)
{
	k_spinlock_key_t key;
	static struct k_spinlock l;

	zassert_true(!l.locked, "Spinlock initialized to locked");

	key = k_spin_lock(&l);

	zassert_true(l.locked, "Spinlock failed to lock");

	k_spin_unlock(&l, key);

	zassert_true(!l.locked, "Spinlock failed to unlock");
}

void bounce_once(int id)
{
	int i, locked;
	k_spinlock_key_t key;

	/* Take the lock, check last owner and release if it was us.
	 * Wait for us to get the lock "after" another CPU
	 */
	locked = 0;
	for (i = 0; i < 10000; i++) {
		key = k_spin_lock(&bounce_lock);

		if (bounce_owner != id) {
			locked = 1;
			break;
		}

		k_spin_unlock(&bounce_lock, key);
		k_busy_wait(100);
	}

	if (!locked && bounce_done) {
		return;
	}

	zassert_true(locked, "Other cpu did not get lock in 10000 tries");

	/* Mark us as the owner, spin for a while validating that we
	 * never see another owner write to the protected data.
	 */
	bounce_owner = id;

	for (i = 0; i < 100; i++) {
		zassert_true(bounce_owner == id, "Locked data changed");
	}

	/* Release the lock */
	k_spin_unlock(&bounce_lock, key);
}

void cpu1_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		bounce_once(4321);
	}
}

/**
 * @brief Test spinlock with bounce
 *
 * @ingroup kernel_spinlock_tests
 *
 * @see arch_start_cpu()
 */
void test_spinlock_bounce(void)
{
	int i;

	k_thread_create(&cpu1_thread, cpu1_stack, CPU1_STACK_SIZE,
			cpu1_fn, NULL, NULL, NULL,
			0, 0, K_NO_WAIT);

	k_busy_wait(10);

	for (i = 0; i < 10000; i++) {
		bounce_once(1234);
	}

	bounce_done = 1;
}

/**
 * @brief Test basic mutual exclusion using interrupt masking
 *
 * @details
 * - Spinlocks can be initialized at run-time.
 * - Spinlocks in uniprocessor context should achieve mutual exclusion using
 *   interrupt masking.
 *
 * @ingroup kernel_spinlock_tests
 *
 * @see k_spin_lock(), k_spin_unlock()
 */
void test_spinlock_mutual_exclusion(void)
{
	k_spinlock_key_t key;
	struct k_spinlock lock_runtime;
	unsigned int irq_key;

	(void)memset(&lock_runtime, 0, sizeof(lock_runtime));

	key = k_spin_lock(&lock_runtime);

	zassert_true(lock_runtime.locked, "Spinlock failed to lock");

	/* check irq has not locked */
	zassert_true(arch_irq_unlocked(key.key),
			"irq should be first locked!");

	/*
	 * We make irq locked nested to check if interrupt
	 * disable happened or not.
	 */
	irq_key = arch_irq_lock();

	/* check irq has already locked */
	zassert_false(arch_irq_unlocked(irq_key),
			"irq should be already locked!");

	arch_irq_unlock(irq_key);

	k_spin_unlock(&lock_runtime, key);

	zassert_true(!lock_runtime.locked, "Spinlock failed to unlock");
}

void test_main(void)
{
	ztest_test_suite(spinlock,
			 ztest_unit_test(test_spinlock_basic),
			 ztest_unit_test(test_spinlock_bounce),
			 ztest_unit_test(test_spinlock_mutual_exclusion));
	ztest_run_test_suite(spinlock);
}
