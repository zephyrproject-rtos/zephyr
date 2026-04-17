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

/*
 * Mutex tests — no TSA annotations since k_mutex supports recursive locking,
 * which is incompatible with Clang TSA's binary capability model.
 */

static struct k_mutex test_mutex;
static int test_mutex_data;

ZTEST(thread_safety, test_mutex_lock_unlock)
{
	k_mutex_init(&test_mutex);

	k_mutex_lock(&test_mutex, K_FOREVER);
	test_mutex_data = 10;
	test_mutex_data += 1;
	k_mutex_unlock(&test_mutex);

	zassert_equal(test_mutex_data, 11, "mutex protected data has expected value");
}

/*
 * Test that Z_NO_THREAD_SAFETY_ANALYSIS compiles cleanly
 */
Z_NO_THREAD_SAFETY_ANALYSIS static void unanalyzed_function(struct k_spinlock *lock)
{
	k_spinlock_key_t key;

	key = k_spin_lock(lock);
	k_spin_unlock(lock, key);
}

ZTEST(thread_safety, test_no_analysis_annotation)
{
	unanalyzed_function(&test_spinlock);
	zassert_true(true, "Z_NO_THREAD_SAFETY_ANALYSIS compiled cleanly");
}

ZTEST_SUITE(thread_safety, NULL, NULL, NULL, NULL, NULL);
