/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>

#include <zephyr/ztest.h>

/**
 * @brief Test to demonstrate limited condition variable resources
 *
 * @details Exactly CONFIG_MAX_PTHREAD_COND_COUNT can be in use at once.
 */
ZTEST(cond, test_cond_resource_exhausted)
{
	size_t i;
	pthread_cond_t m[CONFIG_MAX_PTHREAD_COND_COUNT + 1];

	for (i = 0; i < CONFIG_MAX_PTHREAD_COND_COUNT; ++i) {
		zassert_ok(pthread_cond_init(&m[i], NULL), "failed to init cond %zu", i);
	}

	/* try to initialize one more than CONFIG_MAX_PTHREAD_COND_COUNT */
	zassert_equal(i, CONFIG_MAX_PTHREAD_COND_COUNT);
	zassert_not_equal(0, pthread_cond_init(&m[i], NULL), "should not have initialized cond %zu",
			  i);

	for (; i > 0; --i) {
		zassert_ok(pthread_cond_destroy(&m[i - 1]), "failed to destroy cond %zu", i - 1);
	}
}

/**
 * @brief Test to that there are no condition variable resource leaks
 *
 * @details Demonstrate that condition variables may be used over and over again.
 */
ZTEST(cond, test_cond_resource_leak)
{
	pthread_cond_t cond;

	for (size_t i = 0; i < 2 * CONFIG_MAX_PTHREAD_COND_COUNT; ++i) {
		zassert_ok(pthread_cond_init(&cond, NULL), "failed to init cond %zu", i);
		zassert_ok(pthread_cond_destroy(&cond), "failed to destroy cond %zu", i);
	}
}

ZTEST(cond, test_pthread_condattr)
{
	clockid_t clock_id;
	pthread_condattr_t att = {0};

	zassert_ok(pthread_condattr_init(&att));

	zassert_ok(pthread_condattr_getclock(&att, &clock_id), "pthread_condattr_getclock failed");
	zassert_equal(clock_id, CLOCK_MONOTONIC, "clock attribute not set correctly");

	zassert_ok(pthread_condattr_setclock(&att, CLOCK_REALTIME),
		   "pthread_condattr_setclock failed");

	zassert_ok(pthread_condattr_getclock(&att, &clock_id), "pthread_condattr_setclock failed");
	zassert_equal(clock_id, CLOCK_REALTIME, "clock attribute not set correctly");

	zassert_equal(pthread_condattr_setclock(&att, 42), -EINVAL,
		      "pthread_condattr_setclock did not return EINVAL");

	zassert_ok(pthread_condattr_destroy(&att));
}

ZTEST_SUITE(cond, NULL, NULL, NULL, NULL, NULL);
