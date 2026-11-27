/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Tests for Clang Thread Safety Analysis annotations.
 *
 * These tests verify that the TSA annotations on k_spinlock compile cleanly
 * with correct lock usage patterns. When built with Clang and -Wthread-safety,
 * incorrect usage would produce compile-time warnings.
 */

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/thread_safety.h>
#include <zephyr/ztest.h>

/*
 * Spinlock tests
 */

static struct k_spinlock test_spinlock;
static int test_spinlock_data Z_GUARDED_BY(test_spinlock);

ZTEST(thread_safety, test_spinlock_lock_unlock)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&test_spinlock);
	test_spinlock_data = 42;
	k_spin_unlock(&test_spinlock, key);

	zassert_true(true, "spinlock lock/unlock compiled cleanly with TSA");
}

ZTEST(thread_safety, test_spinlock_trylock)
{
	k_spinlock_key_t key;
	int ret;

	ret = k_spin_trylock(&test_spinlock, &key);
	if (ret == 0) {
		test_spinlock_data = 99;
		k_spin_unlock(&test_spinlock, key);
	}

	zassert_true(true, "spinlock trylock compiled cleanly with TSA");
}

ZTEST(thread_safety, test_spinlock_trylock_after_unlock)
{
	k_spinlock_key_t key;
	k_spinlock_key_t key2;
	int ret;

	key = k_spin_lock(&test_spinlock);
	test_spinlock_data = 1;
	k_spin_unlock(&test_spinlock, key);

	ret = k_spin_trylock(&test_spinlock, &key2);
	zassert_equal(ret, 0, "trylock on a free lock must succeed");
	if (ret == 0) {
		test_spinlock_data = 2;
		k_spin_unlock(&test_spinlock, key2);
	}
}

static void set_data_locked(int value) Z_REQUIRES(test_spinlock)
{
	test_spinlock_data = value;
}

static k_spinlock_key_t acquire_test_lock(void) Z_ACQUIRES(test_spinlock)
{
	return k_spin_lock(&test_spinlock);
}

static void release_test_lock(k_spinlock_key_t key) Z_RELEASES(test_spinlock)
{
	k_spin_unlock(&test_spinlock, key);
}

ZTEST(thread_safety, test_spinlock_contract_helpers)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&test_spinlock);
	set_data_locked(7);
	k_spin_unlock(&test_spinlock, key);

	key = acquire_test_lock();
	set_data_locked(8);
	release_test_lock(key);

	zassert_true(true, "Z_REQUIRES/Z_ACQUIRES/Z_RELEASES helpers compiled cleanly");
}

/*
 * K_SPINLOCK bodies are not analyzed: the lock is never seen as held inside
 * the block, so touching guarded data requires suppressing the function.
 */
Z_NO_THREAD_SAFETY_ANALYSIS static void k_spinlock_block_access(void)
{
	K_SPINLOCK(&test_spinlock) {
		test_spinlock_data = 3;
	}
}

ZTEST(thread_safety, test_k_spinlock_block)
{
	k_spinlock_block_access();
	zassert_true(true, "K_SPINLOCK block compiled cleanly with TSA");
}

ZTEST_SUITE(thread_safety, NULL, NULL, NULL, NULL, NULL);
