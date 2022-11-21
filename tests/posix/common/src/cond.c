/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <pthread.h>

/**
 * @brief Test to demonstrate limited condition variable resources
 *
 * @details Exactly CONFIG_MAX_PTHREAD_COND_COUNT can be in use at once.
 */
ZTEST(posix_apis, test_posix_cond_resource_exhausted)
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
ZTEST(posix_apis, test_posix_cond_resource_leak)
{
	pthread_cond_t m;

	for (size_t i = 0; i < 2 * CONFIG_MAX_PTHREAD_COND_COUNT; ++i) {
		zassert_ok(pthread_cond_init(&m, NULL), "failed to init cond %zu", i);
		zassert_ok(pthread_cond_destroy(&m), "failed to destroy cond %zu", i);
	}
}
