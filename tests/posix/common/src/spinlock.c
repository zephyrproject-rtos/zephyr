/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>

ZTEST(posix_apis, test_spin_init_destroy)
{
	pthread_spinlock_t lock;

	zassert_equal(pthread_spin_init(NULL, PTHREAD_PROCESS_PRIVATE), EINVAL,
		      "pthread_spin_init() did not return EINVAL with NULL lock pointer");
	zassert_equal(pthread_spin_init(&lock, 42), EINVAL,
		      "pthread_spin_init() did not return EINVAL with invalid pshared");
	zassert_equal(pthread_spin_destroy(NULL), EINVAL,
		      "pthread_spin_destroy() did not return EINVAL with NULL lock pointer");

	zassert_ok(pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE), "pthread_spin_init() failed");
	zassert_ok(pthread_spin_destroy(&lock), "pthread_spin_destroy() failed");
}

ZTEST(posix_apis, test_spin_descriptor_leak)
{
	pthread_spinlock_t lock[CONFIG_MAX_PTHREAD_SPINLOCK_COUNT];

	for (size_t j = 0; j < 2; ++j) {
		for (size_t i = 0; i < ARRAY_SIZE(lock); ++i) {
			zassert_ok(pthread_spin_init(&lock[i], PTHREAD_PROCESS_PRIVATE),
				   "failed to initialize spinlock %zu (rep %zu)", i, j);
		}

		zassert_equal(pthread_spin_init(&lock[CONFIG_MAX_PTHREAD_SPINLOCK_COUNT],
						PTHREAD_PROCESS_PRIVATE),
			      ENOMEM,
			      "should not be able to initialize more than "
			      "CONFIG_MAX_PTHREAD_SPINLOCK_COUNT spinlocks");

		for (size_t i = 0; i < ARRAY_SIZE(lock); ++i) {
			zassert_ok(pthread_spin_destroy(&lock[i]),
				   "failed to destroy spinlock %zu (rep %zu)", i, j);
		}
	}
}

ZTEST(posix_apis, test_spin_lock_unlock)
{
	pthread_spinlock_t lock;

	zassert_equal(pthread_spin_lock(NULL), EINVAL,
		      "pthread_spin_lock() did not return EINVAL with NULL lock pointer");
	zassert_equal(pthread_spin_trylock(NULL), EINVAL,
		      "pthread_spin_lock() did not return EINVAL with NULL lock pointer");
	zassert_equal(pthread_spin_unlock(NULL), EINVAL,
		      "pthread_spin_lock() did not return EINVAL with NULL lock pointer");

	zassert_ok(pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE), "pthread_spin_init() failed");

	zassert_ok(pthread_spin_lock(&lock), "pthread_spin_lock() failed");
	zassert_ok(pthread_spin_unlock(&lock), "pthread_spin_lock() failed");

	zassert_ok(pthread_spin_trylock(&lock), "pthread_spin_trylock() failed");
	zassert_ok(pthread_spin_unlock(&lock), "pthread_spin_unlock() failed");

	zassert_ok(pthread_spin_destroy(&lock), "pthread_spin_init() failed");
	zassert_equal(pthread_spin_destroy(&lock), EINVAL, "pthread_spin_unlock() did not fail");
}
