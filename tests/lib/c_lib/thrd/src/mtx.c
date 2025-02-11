/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "thrd.h"

#include <stdint.h>
#include <threads.h>

#include <zephyr/sys_clock.h>
#include <zephyr/ztest.h>

static const int valid_mtx_types[] = {
	mtx_plain,
	mtx_timed,
	mtx_plain | mtx_recursive,
	mtx_timed | mtx_recursive,
};

static mtx_t  mutex;
static thrd_t th;

ZTEST(libc_mtx, test_mtx_init)
{
	zassert_not_equal(thrd_success, mtx_init(NULL, FORTY_TWO));
	zassert_not_equal(thrd_success, mtx_init(&mutex, FORTY_TWO));

	if (false) {
		/* pthread_mutexattr_init() is not hardened against this */
		zassert_not_equal(thrd_success, mtx_init(NULL, mtx_plain));
		zassert_not_equal(thrd_success, mtx_init((mtx_t *)BIOS_FOOD, FORTY_TWO));
	}

	for (size_t i = 0; i < ARRAY_SIZE(valid_mtx_types); ++i) {
		int type = valid_mtx_types[i];

		zassert_equal(thrd_success, mtx_init(&mutex, type));
		mtx_destroy(&mutex);
	}
}

ZTEST(libc_mtx, test_mtx_destroy)
{
	if (false) {
		/* degenerate cases */
		/* pthread_mutex_destroy() is not hardened against these */
		mtx_destroy(NULL);
		mtx_destroy((mtx_t *)BIOS_FOOD);
	}

	zassert_equal(thrd_success, mtx_init(&mutex, mtx_plain));
	mtx_destroy(&mutex);
}

ZTEST(libc_mtx, test_mtx_lock)
{
	if (false) {
		/* pthread_mutex_lock() is not hardened against this */
		zassert_not_equal(thrd_success, mtx_lock(NULL));
		zassert_not_equal(thrd_success, mtx_lock((mtx_t *)BIOS_FOOD));
	}

	/* test plain mutex */
	for (size_t i = 0; i < ARRAY_SIZE(valid_mtx_types); ++i) {
		int type = valid_mtx_types[i];

		zassert_equal(thrd_success, mtx_init(&mutex, type));
		zassert_equal(thrd_success, mtx_lock(&mutex));
		if ((type & mtx_recursive) == 0) {
			if (false) {
				/* pthread_mutex_lock() is not hardened against this */
				zassert_not_equal(thrd_success, mtx_lock((&mutex)));
			}
		} else {
			zassert_equal(thrd_success, mtx_lock(&mutex));
			zassert_equal(thrd_success, mtx_unlock(&mutex));
		}
		zassert_equal(thrd_success, mtx_unlock(&mutex));
		mtx_destroy(&mutex);
	}
}

#define TIMEDLOCK_TIMEOUT_MS       200
#define TIMEDLOCK_TIMEOUT_DELAY_MS 100

BUILD_ASSERT(TIMEDLOCK_TIMEOUT_DELAY_MS >= 100, "TIMEDLOCK_TIMEOUT_DELAY_MS too small");
BUILD_ASSERT(TIMEDLOCK_TIMEOUT_MS >= 2 * TIMEDLOCK_TIMEOUT_DELAY_MS,
	     "TIMEDLOCK_TIMEOUT_MS too small");

static int mtx_timedlock_fn(void *arg)
{
	struct timespec time_point;
	mtx_t *mtx = (mtx_t *)arg;

	zassume_ok(clock_gettime(CLOCK_MONOTONIC, &time_point));
	timespec_add_ms(&time_point, TIMEDLOCK_TIMEOUT_MS);

	return mtx_timedlock(mtx, &time_point);
}

ZTEST(libc_mtx, test_mtx_timedlock)
{
	int ret;

	/*
	 * mtx_timed here is technically unnecessary, because all pthreads can
	 * be used for timed locks, but that is sort of peeking into the
	 * implementation
	 */
	zassert_equal(thrd_success, mtx_init(&mutex, mtx_timed));

	printk("Expecting timedlock with timeout of %d ms to fail\n", TIMEDLOCK_TIMEOUT_MS);
	zassert_equal(thrd_success, mtx_lock(&mutex));
	zassert_equal(thrd_success, thrd_create(&th, mtx_timedlock_fn, &mutex));
	zassert_equal(thrd_success, thrd_join(th, &ret));
	/* ensure timeout occurs */
	zassert_equal(thrd_timedout, ret);

	printk("Expecting timedlock with timeout of %d ms to succeed after 100ms\n",
	       TIMEDLOCK_TIMEOUT_MS);
	zassert_equal(thrd_success, thrd_create(&th, mtx_timedlock_fn, &mutex));
	/* unlock before timeout expires */
	k_msleep(TIMEDLOCK_TIMEOUT_DELAY_MS);
	zassert_equal(thrd_success, mtx_unlock(&mutex));
	zassert_equal(thrd_success, thrd_join(th, &ret));
	/* ensure lock is successful, in spite of delay  */
	zassert_equal(thrd_success, ret);

	mtx_destroy(&mutex);
}

static int mtx_trylock_fn(void *arg)
{
	mtx_t *mtx = (mtx_t *)arg;

	return mtx_trylock(mtx);
}

ZTEST(libc_mtx, test_mtx_trylock)
{
	int ret;

	zassert_equal(thrd_success, mtx_init(&mutex, mtx_plain));

	/* ensure trylock fails when lock is held */
	zassert_equal(thrd_success, mtx_lock(&mutex));
	zassert_equal(thrd_success, thrd_create(&th, mtx_trylock_fn, &mutex));
	zassert_equal(thrd_success, thrd_join(th, &ret));
	/* ensure lock fails */
	zassert_equal(thrd_busy, ret);

	mtx_destroy(&mutex);
}

ZTEST(libc_mtx, test_mtx_unlock)
{
	mtx_t mtx = (mtx_t)BIOS_FOOD;

	/* degenerate case */
	zassert_not_equal(thrd_success, mtx_unlock(&mtx));

	zassert_equal(thrd_success, mtx_init(&mtx, mtx_plain));
	zassert_equal(thrd_success, mtx_lock(&mtx));
	zassert_equal(thrd_success, mtx_unlock(&mtx));
	mtx_destroy(&mtx);
}

ZTEST_SUITE(libc_mtx, NULL, NULL, NULL, NULL, NULL);
