/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/spinlock.h>
#include <zephyr/ztest.h>

#define NUMBER_OF_SPINLOCKS 8

SPINLOCK_POOL_DEFINE(lonely_lock, 1);
SPINLOCK_POOL_DEFINE(test_pool, NUMBER_OF_SPINLOCKS);

ZTEST(spinlock_pool, test_spinlock_single)
{
	for (size_t i = 0; i < 128; i++) {
		zassert_equal_ptr(spinlock_find(lonely_lock, i), &lonely_lock[0]);
	}

	zassert_equal_ptr(spinlock_find(lonely_lock, UINTPTR_MAX), &lonely_lock[0]);
}

ZTEST(spinlock_pool, test_spinlock_lookup)
{
	uintptr_t object = 0x1234U;
	struct k_spinlock *lock = spinlock_find(test_pool, object);

	zassume(sizeof(struct k_spinlock) != 0, "pool elements are zero-sized");

	zassert_true(lock >= &test_pool[0]);
	zassert_true(lock < &test_pool[NUMBER_OF_SPINLOCKS]);

	zassert_equal_ptr(spinlock_find(test_pool, object), lock);
}

ZTEST(spinlock_pool, test_spinlock_distribution)
{
	bool seen[NUMBER_OF_SPINLOCKS] = { false };

	zassume(sizeof(struct k_spinlock) != 0, "pool elements are zero-sized");

	for (size_t i = 0; i < NUMBER_OF_SPINLOCKS; i++) {
		struct k_spinlock *lock = spinlock_find(test_pool, i * 4U);
		size_t index = z_spinlock_index(i * 4U, NUMBER_OF_SPINLOCKS);

		zassert_equal_ptr(lock, &test_pool[index], "lock does not match computed slot");
		zassert_false(seen[index], "pool %zu selected more than once", index);
		seen[index] = true;
	}

	for (size_t i = 0; i < NUMBER_OF_SPINLOCKS; i++) {
		zassert_true(seen[i], "pool %zu was not selected", i);
	}
}

ZTEST(spinlock_pool, test_spinlock_lock)
{
	struct k_spinlock *lock = spinlock_find(test_pool, 4U);
	k_spinlock_key_t key = k_spin_lock(lock);

	zassert_true(z_spin_is_locked(lock));
	k_spin_unlock(lock, key);
	zassert_false(z_spin_is_locked(lock));
}


ZTEST(spinlock_pool, test_spinlock_dual_access)
{
	struct k_spinlock *lock1 = spinlock_find(test_pool, 4U);
	struct k_spinlock *lock2 = spinlock_find(test_pool, 8U);

	k_spinlock_key_t key1 = k_spin_lock(lock1);
	k_spinlock_key_t key2 = k_spin_lock(lock2);

	zassert_true(z_spin_is_locked(lock1));
	zassert_true(z_spin_is_locked(lock2));

	k_spin_unlock(lock2, key2);
	k_spin_unlock(lock1, key1);

	zassert_false(z_spin_is_locked(lock1));
	zassert_false(z_spin_is_locked(lock2));
}

ZTEST_SUITE(spinlock_pool, NULL, NULL, NULL, NULL, NULL);
