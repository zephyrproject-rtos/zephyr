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
	pthread_condattr_t att = {0};

	zassert_ok(pthread_condattr_init(&att));

	zassert_ok(pthread_condattr_destroy(&att));
}

/**
 * @brief Test pthread_cond_init() with a pre-existing initialized attribute.
 */
ZTEST(cond, test_cond_init_existing_initialized_condattr)
{
	pthread_cond_t cond;
	pthread_condattr_t att = {0};

	zassert_ok(pthread_condattr_init(&att));
	zassert_ok(pthread_cond_init(&cond, &att), "pthread_cond_init failed with valid attr");

	/* Clean up */
	zassert_ok(pthread_cond_destroy(&cond));
	zassert_ok(pthread_condattr_destroy(&att));
}

/**
 * @brief Test pthread_cond_broadcast call with a statically initialized pthread_cond_t,
 *        i.e. PTHREAD_COND_INITIALIZER assigned.
 */
ZTEST(cond, test_cond_broadcast_static_init_pthread_cond_t)
{
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER; /* is considered statically initialized */

	zassert_ok(pthread_cond_broadcast(&cond));
	zassert_ok(pthread_cond_destroy(&cond));
}

/**
 * @brief Test pthread_cond_signal call with a statically initialized pthread_cond_t,
 *        i.e. PTHREAD_COND_INITIALIZER assigned.
 */
ZTEST(cond, test_cond_signal_static_init_pthread_cond_t)
{
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER; /* is considered statically initialized */

	zassert_ok(pthread_cond_signal(&cond));
	zassert_ok(pthread_cond_destroy(&cond));
}

struct static_cond_test_data {
	pthread_cond_t *cond;
	pthread_mutex_t *mutex;
	bool signaled;
};

static void *static_cond_waiter(void *arg)
{
	struct static_cond_test_data *data = arg;

	zassert_ok(pthread_mutex_lock(data->mutex));
	while (data->signaled == false) {
		zassert_ok(pthread_cond_wait(data->cond, data->mutex));
	}
	zassert_ok(pthread_mutex_unlock(data->mutex));

	return NULL;
}

/**
 * @brief Test concurrent wait and signal with statically initialized condvar
 *
 * @details A PTHREAD_COND_INITIALIZER condvar must auto-initialize on first use
 *          and reliably wake up waiters across concurrent threads, exercising
 *          the serialized lazy-init path in to_posix_cond().
 */
ZTEST(cond, test_cond_static_initializer_multithread)
{
	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		/* skip redundant testing if there is no thread pool / heap allocation */
		ztest_test_skip();
		return;
	}

	static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	struct static_cond_test_data data = {
		.cond = &cond,
		.mutex = &mutex,
		.signaled = false,
	};
	pthread_t th;

	zassert_ok(pthread_create(&th, NULL, static_cond_waiter, &data));

	/* Allow child thread to run and block in pthread_cond_wait */
	k_msleep(50);

	zassert_ok(pthread_mutex_lock(&mutex));
	data.signaled = true;
	zassert_ok(pthread_cond_signal(&cond));
	zassert_ok(pthread_mutex_unlock(&mutex));

	zassert_ok(pthread_join(th, NULL));

	zassert_ok(pthread_cond_destroy(&cond));
	zassert_ok(pthread_mutex_destroy(&mutex));
}

ZTEST_SUITE(cond, NULL, NULL, NULL, NULL, NULL);
