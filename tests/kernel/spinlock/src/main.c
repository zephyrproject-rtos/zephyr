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

#define CPU1_STACK_SIZE 1024

K_THREAD_STACK_DEFINE(cpu1_stack, CPU1_STACK_SIZE);

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

void cpu1_fn(int key, void *arg)
{
	ARG_UNUSED(key);
	ARG_UNUSED(arg);

	while (1) {
		bounce_once(4321);
	}
}

/**
 * @brief Test spinlock with bounce
 *
 * @ingroup kernel_spinlock_tests
 *
 * @see _arch_start_cpu()
 */
void test_spinlock_bounce(void)
{
	int i;

	_arch_start_cpu(1, cpu1_stack, CPU1_STACK_SIZE, cpu1_fn, 0);

	k_busy_wait(10);

	for (i = 0; i < 10000; i++) {
		bounce_once(1234);
	}

	bounce_done = 1;
}

void test_main(void)
{
	ztest_test_suite(spinlock,
			 ztest_unit_test(test_spinlock_basic),
			 ztest_unit_test(test_spinlock_bounce));
	ztest_run_test_suite(spinlock);
}
