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
	struct k_sem started_sem;
	bool signaled;
};

static void *static_cond_waiter(void *arg)
{
	struct static_cond_test_data *data = arg;

	zassert_ok(pthread_mutex_lock(data->mutex));
	k_sem_give(&data->started_sem);
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
		/* skip test if dynamic thread creation is not configured */
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

	k_sem_init(&data.started_sem, 0, 1);

	zassert_ok(pthread_create(&th, NULL, static_cond_waiter, &data));

	/* Wait until waiter thread has acquired mutex and is ready */
	zassert_ok(k_sem_take(&data.started_sem, K_FOREVER));

	/*
	 * Acquire mutex: because waiter acquired mutex before giving started_sem,
	 * we only acquire the mutex once waiter has atomically released it inside
	 * pthread_cond_wait().
	 */
	zassert_ok(pthread_mutex_lock(&mutex));
	data.signaled = true;
	zassert_ok(pthread_cond_signal(&cond));
	zassert_ok(pthread_mutex_unlock(&mutex));

	zassert_ok(pthread_join(th, NULL));

	zassert_ok(pthread_cond_destroy(&cond));
	zassert_ok(pthread_mutex_destroy(&mutex));
}

#define NUM_COND_CONTENDERS 4

struct cond_contender_data {
	pthread_cond_t *cond;
	struct k_sem *start_sem;
};

static void *cond_contender_thread(void *arg)
{
	struct cond_contender_data *data = arg;

	/* Wait for start signal so all threads release concurrently */
	k_sem_take(data->start_sem, K_FOREVER);

	/* Contend on lazy initialization of the static condition variable */
	zassert_ok(pthread_cond_signal(data->cond));

	return NULL;
}

/**
 * @brief Test concurrent first-use contention on a static condition variable
 *
 * @details Multiple threads concurrently invoke pthread_cond_signal() on the
 *          same uninitialized PTHREAD_COND_INITIALIZER variable without prior
 *          mutex synchronization. Verifies that exactly one pool slot is allocated
 *          and no pool slots are leaked under contention.
 */
ZTEST(cond, test_cond_static_initializer_contention)
{
	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		ztest_test_skip();
		return;
	}

	static pthread_cond_t race_cond = PTHREAD_COND_INITIALIZER;
	pthread_cond_t extra_conds[CONFIG_MAX_PTHREAD_COND_COUNT];
	struct k_sem start_sem;
	pthread_t threads[NUM_COND_CONTENDERS];
	struct cond_contender_data data = {
		.cond = &race_cond,
		.start_sem = &start_sem,
	};
	int i;

	k_sem_init(&start_sem, 0, NUM_COND_CONTENDERS);

	for (i = 0; i < NUM_COND_CONTENDERS; ++i) {
		zassert_ok(pthread_create(&threads[i], NULL, cond_contender_thread, &data));
	}

	/* Release all contenders simultaneously */
	for (i = 0; i < NUM_COND_CONTENDERS; ++i) {
		k_sem_give(&start_sem);
	}

	for (i = 0; i < NUM_COND_CONTENDERS; ++i) {
		zassert_ok(pthread_join(threads[i], NULL));
	}

	/*
	 * Verify no slots were leaked: exactly 1 slot should be in use by race_cond.
	 * We should be able to allocate all remaining (CONFIG_MAX_PTHREAD_COND_COUNT - 1)
	 * condition variables successfully without pool exhaustion.
	 */
	for (i = 0; i < CONFIG_MAX_PTHREAD_COND_COUNT - 1; ++i) {
		zassert_ok(pthread_cond_init(&extra_conds[i], NULL));
	}

	/* Clean up all condition variables */
	for (i = 0; i < CONFIG_MAX_PTHREAD_COND_COUNT - 1; ++i) {
		zassert_ok(pthread_cond_destroy(&extra_conds[i]));
	}

	zassert_ok(pthread_cond_destroy(&race_cond));
}

ZTEST_SUITE(cond, NULL, NULL, NULL, NULL, NULL);
