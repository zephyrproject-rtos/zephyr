/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define N_THR 2
#define N_KEY  2
#define BUFFSZ 48

static pthread_key_t key;
static pthread_key_t keys[N_KEY];
static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static pthread_once_t keys_once = PTHREAD_ONCE_INIT;

static void *thread_top(void *p1)
{
	void *value;
	char *buffer[BUFFSZ];

	value = k_malloc(sizeof(buffer));
	zassert_not_null(value, "thread could not allocate storage");
	zassert_ok(pthread_setspecific(key, value), "pthread_setspecific failed");
	zassert_equal(pthread_getspecific(key), value, "set and retrieved values are different");
	k_free(value);

	return NULL;
}

static void *thread_func(void *p1)
{
	void *value;
	char *buffer[BUFFSZ];

	value = k_malloc(sizeof(buffer));
	zassert_not_null(value, "thread could not allocate storage");
	for (int i = 0; i < N_KEY; i++) {
		zassert_ok(pthread_setspecific(keys[i], value), "pthread_setspecific failed");
		zassert_equal(pthread_getspecific(keys[i]), value,
			      "set and retrieved values are different");
	}
	k_free(value);
	return NULL;
}

static void make_key(void)
{
	zassert_ok(pthread_key_create(&key, NULL), "insufficient memory to create key");
}

static void make_keys(void)
{
	for (int i = 0; i < N_KEY; i++) {
		zassert_ok(pthread_key_create(&keys[i], NULL),
			   "insufficient memory to create keys");
	}
}

/**
 * @brief Test to demonstrate pthread_key APIs usage
 *
 * @details The tests spawn a thread which uses pthread_once() to
 * create a key via pthread_key_create() API. It then sets the
 * thread-specific value to the key using pthread_setspecific() and
 * gets it back using pthread_getspecific and asserts that they
 * are equal. It then deletes the key using pthread_key_delete().
 * Both the sub-tests do the above, but one with multiple threads
 * using the same key and the other with a single thread using
 * multiple keys.
 */

ZTEST(key, test_key_1toN_thread)
{
	void *retval;
	pthread_t newthread[N_THR];

	zassert_ok(pthread_once(&key_once, make_key), "attempt to create key failed");

	/* Different threads set different values to same key */

	for (int i = 0; i < N_THR; i++) {
		zassert_ok(pthread_create(&newthread[i], NULL, thread_top, NULL),
			   "attempt to create thread %d failed", i);
	}

	for (int i = 0; i < N_THR; i++) {
		zassert_ok(pthread_join(newthread[i], &retval), "failed to join thread %d", i);
	}

	zassert_ok(pthread_key_delete(key), "attempt to delete key failed");
}

ZTEST(key, test_key_Nto1_thread)
{
	pthread_t newthread;

	zassert_ok(pthread_once(&keys_once, make_keys), "attempt to create keys failed");

	/* Single thread associates its value with different keys */

	zassert_ok(pthread_create(&newthread, NULL, thread_func, NULL),
		   "attempt to create thread failed");

	zassert_ok(pthread_join(newthread, NULL), "failed to join thread");

	for (int i = 0; i < N_KEY; i++) {
		zassert_ok(pthread_key_delete(keys[i]), "attempt to delete keys[%d] failed", i);
	}
}

ZTEST(key, test_key_resource_leak)
{
	pthread_key_t key;

	for (size_t i = 0; i < CONFIG_POSIX_THREAD_KEYS_MAX; ++i) {
		zassert_ok(pthread_key_create(&key, NULL), "failed to create key %zu", i);
		zassert_ok(pthread_key_delete(key), "failed to delete key %zu", i);
	}
}

ZTEST(key, test_correct_key_is_deleted)
{
	pthread_key_t key;
	size_t j = CONFIG_POSIX_THREAD_KEYS_MAX - 1;
	pthread_key_t keys[CONFIG_POSIX_THREAD_KEYS_MAX];

	for (size_t i = 0; i < ARRAY_SIZE(keys); ++i) {
		zassert_ok(pthread_key_create(&keys[i], NULL), "failed to create key %zu", i);
	}

	key = keys[j];
	zassert_ok(pthread_key_delete(keys[j]));
	zassert_ok(pthread_key_create(&keys[j], NULL), "failed to create key %zu", j);

	zassert_equal(key, keys[j], "deleted key %x instead of key %x", keys[j], key);

	for (size_t i = 0; i < ARRAY_SIZE(keys); ++i) {
		zassert_ok(pthread_key_delete(keys[i]), "failed to delete key %zu", i);
	}
}

static void *setspecific_thread(void *count)
{
	int value = 42;
	int *alloc_count = count;

	while (1) {
		pthread_key_t key;

		zassert_ok(pthread_key_create(&key, NULL), "failed to create key");
		if (pthread_setspecific(key, &value) == ENOMEM) {
			break;
		};
		*alloc_count += 1;
	}

	return NULL;
}

ZTEST(key, test_thread_specific_data_deallocation)
{
	pthread_t thread;
	static int alloc_count_t0;
	static int alloc_count_t1;

	zassert_ok(pthread_create(&thread, NULL, setspecific_thread, &alloc_count_t0),
		"attempt to create thread failed");
	zassert_ok(pthread_join(thread, NULL), "failed to join thread");
	printk("first thread allocated %d keys", alloc_count_t0);

	zassert_ok(pthread_create(&thread, NULL, setspecific_thread, &alloc_count_t1),
		"attempt to create thread failed");
	zassert_ok(pthread_join(thread, NULL), "failed to join thread");
	printk("second thread allocated %d keys", alloc_count_t1);

	zassert_equal(alloc_count_t0, alloc_count_t1,
		"failed to deallocate thread specific data");
}

static struct k_sem sem_worker_ready;
static struct k_sem sem_key_deleted;
static bool dtor_executed;

static void dtor_active_thread(void *arg)
{
	ARG_UNUSED(arg);
	dtor_executed = true;
}

static void *key_delete_worker(void *arg)
{
	pthread_key_t *key = arg;
	int val = 0x1234;

	zassert_ok(pthread_setspecific(*key, &val),
		   "failed to set thread-specific data");

	/* Signal that data has been bound to the key */
	k_sem_give(&sem_worker_ready);

	/* Wait until key deletion completes before exiting */
	k_sem_take(&sem_key_deleted, K_FOREVER);

	return NULL;
}

/**
 * @brief Verify pthread_key_delete() on key bound to active thread.
 *
 * Verifies that deleting a key while a thread holds thread-specific data
 * unlinks the key from the thread's internal list, suppresses destructor
 * invocation at thread exit per IEEE 1003.1, and does not cause double-free
 * or memory corruption.
 */
ZTEST(key, test_key_delete_active_thread)
{
	pthread_t thread;
	pthread_key_t key;

	dtor_executed = false;
	k_sem_init(&sem_worker_ready, 0, 1);
	k_sem_init(&sem_key_deleted, 0, 1);

	zassert_ok(pthread_key_create(&key, dtor_active_thread),
		   "failed to create key");

	zassert_ok(pthread_create(&thread, NULL, key_delete_worker, &key),
		   "failed to create worker thread");

	/* Wait deterministically for worker thread to bind data */
	k_sem_take(&sem_worker_ready, K_FOREVER);

	/* Delete key while worker thread is still alive and blocked */
	zassert_ok(pthread_key_delete(key), "failed to delete key");

	/* Release worker thread to exit and finalize */
	k_sem_give(&sem_key_deleted);

	/* Wait for worker thread to exit and finalize */
	zassert_ok(pthread_join(thread, NULL), "failed to join worker thread");

	/* Per IEEE 1003.1, destructor must not be called for deleted keys */
	zassert_false(dtor_executed, "destructor was executed for deleted key");
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		/* skip redundant testing if there is no thread pool / heap allocation */
		ztest_test_skip();
	}
}

ZTEST_SUITE(key, NULL, NULL, before, NULL, NULL);
