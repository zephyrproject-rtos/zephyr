/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define N_THR 2
#define N_KEY 2
#define STACKSZ (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define BUFFSZ 48

K_THREAD_STACK_ARRAY_DEFINE(stackp, N_THR, STACKSZ);

pthread_key_t key, keys[N_KEY];
static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static pthread_once_t keys_once = PTHREAD_ONCE_INIT;

void *thread_top(void *p1)
{
	int ret = -1;

	void *value;
	void *getval;
	char *buffer[BUFFSZ];

	value = k_malloc(sizeof(buffer));

	zassert_true((int) POINTER_TO_INT(value),
		     "thread could not allocate storage");

	ret = pthread_setspecific(key, value);

	/* TESTPOINT: Check if thread's value is associated with key */
	zassert_false(ret, "pthread_setspecific failed");

	getval = 0;

	getval = pthread_getspecific(key);

	/* TESTPOINT: Check if pthread_getspecific returns the same value
	 * set by pthread_setspecific
	 */
	zassert_equal(value, getval,
			"set and retrieved values are different");

	printk("set value = %d and retrieved value = %d\n",
		(int) POINTER_TO_INT(value), (int) POINTER_TO_INT(getval));

	return NULL;
}

void *thread_func(void *p1)
{
	int i, ret = -1;

	void *value;
	void *getval;
	char *buffer[BUFFSZ];

	value = k_malloc(sizeof(buffer));

	zassert_true((int) POINTER_TO_INT(value),
		     "thread could not allocate storage");

	for (i = 0; i < N_KEY; i++) {
		ret = pthread_setspecific(keys[i], value);

		/* TESTPOINT: Check if thread's value is associated with keys */
		zassert_false(ret, "pthread_setspecific failed");
	}

	for (i = 0; i < N_KEY; i++) {
		getval = 0;
		getval = pthread_getspecific(keys[i]);

		/* TESTPOINT: Check if pthread_getspecific returns the same
		 * value set by pthread_setspecific for each of the keys
		 */
		zassert_equal(value, getval,
				"set and retrieved values are different");

		printk("key %d: set value = %d and retrieved value = %d\n",
				i, (int) POINTER_TO_INT(value),
				(int) POINTER_TO_INT(getval));
	}
	return NULL;
}

static void make_key(void)
{
	int ret = 0;

	ret = pthread_key_create(&key, NULL);
	zassert_false(ret, "insufficient memory to create key");
}

static void make_keys(void)
{
	int i, ret = 0;

	for (i = 0; i < N_KEY; i++) {
		ret = pthread_key_create(&keys[i], NULL);
		zassert_false(ret, "insufficient memory to create keys");
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

ZTEST(posix_apis, test_key_1toN_thread)
{
	int i, ret = -1;

	pthread_attr_t attr[N_THR];
	struct sched_param schedparam;
	pthread_t newthread[N_THR];
	void *retval;

	ret = pthread_once(&key_once, make_key);

	/* TESTPOINT: Check if key is created */
	zassert_false(ret, "attempt to create key failed");

	printk("\nDifferent threads set different values to same key:\n");

	/* Creating threads with lowest application priority */
	for (i = 0; i < N_THR; i++) {
		ret = pthread_attr_init(&attr[i]);
		if (ret != 0) {
			zassert_false(pthread_attr_destroy(&attr[i]),
					"Unable to destroy pthread object attr");
			zassert_false(pthread_attr_init(&attr[i]),
					"Unable to create pthread object attr");
		}

		schedparam.sched_priority = 2;
		pthread_attr_setschedparam(&attr[i], &schedparam);
		pthread_attr_setstack(&attr[i], &stackp[i][0], STACKSZ);

		ret = pthread_create(&newthread[i], &attr[i], thread_top,
				INT_TO_POINTER(i));

		/* TESTPOINT: Check if threads are created successfully */
		zassert_false(ret, "attempt to create threads failed");
	}

	for (i = 0; i < N_THR; i++) {
		printk("thread %d: ", i);
		pthread_join(newthread[i], &retval);
	}

	ret = pthread_key_delete(key);

	/* TESTPOINT: Check if key is deleted */
	zassert_false(ret, "attempt to delete key failed");
	printk("\n");
}

ZTEST(posix_apis, test_key_Nto1_thread)
{
	int i, ret = -1;

	pthread_attr_t attr;
	struct sched_param schedparam;
	pthread_t newthread;

	ret = pthread_once(&keys_once, make_keys);

	/* TESTPOINT: Check if keys are created successfully */
	zassert_false(ret, "attempt to create keys failed");

	printk("\nSingle thread associates its value with different keys:\n");
	ret = pthread_attr_init(&attr);
	if (ret != 0) {
		zassert_false(pthread_attr_destroy(&attr),
				"Unable to destroy pthread object attr");
		zassert_false(pthread_attr_init(&attr),
				"Unable to create pthread object attr");
	}

	schedparam.sched_priority = 2;
	pthread_attr_setschedparam(&attr, &schedparam);
	pthread_attr_setstack(&attr, &stackp[0][0], STACKSZ);

	ret = pthread_create(&newthread, &attr, thread_func,
			(void *)0);

	/*TESTPOINT: Check if thread is created successfully */
	zassert_false(ret, "attempt to create thread failed");

	pthread_join(newthread, NULL);

	for (i = 0; i < N_KEY; i++) {
		ret = pthread_key_delete(keys[i]);

		/* TESTPOINT: Check if keys are deleted */
		zassert_false(ret, "attempt to delete keys failed");
	}
	printk("\n");
}
