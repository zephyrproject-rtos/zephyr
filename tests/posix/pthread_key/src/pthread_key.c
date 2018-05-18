/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <posix/pthread.h>

#define STACKSZ 1024
#define BUFFSZ 48

K_THREAD_STACK_ARRAY_DEFINE(stacks, 1, STACKSZ);

pthread_key_t key;
static pthread_once_t key_once;

void *thread_top(void *p1)
{
	int ret = -1;

	void *value;
	void *getval;
	char *buffer[BUFFSZ];

	zassert_true((int)(value = k_malloc(sizeof(buffer))),
			"thread could not allocate storage");

	ret = pthread_setspecific(key, (void *)value);

	/* TESTPOINT: Check if thread's value is associated with key */
	zassert_false(ret, "pthread_setspecific failed");

	getval = 0;

	getval = pthread_getspecific(key);

	/* TESTPOINT: Check if pthread_getspecific returns the same value
	 * set by pthread_setspecific
	 */
	zassert_equal(value, getval,
			"set and retrieved values are different");

	printk("\nset value = %d and retrieved value = %d\n",
			(int)value, (int)getval);

	ret = pthread_key_delete(key);

	/* TESTPOINT: Check if key is deleted */
	zassert_false(ret, "attempt to delete key failed");

	return NULL;
}

static void make_key(void)
{
	int ret = 0;

	ret = pthread_key_create(&key, NULL);
	zassert_false(ret, "insufficient memory to create key");
}

/**
 * @brief Test to demonstrate pthread_key APIs usage
 *
 * @details The test spawns a thread which uses pthread_once() to
 * create a key via pthread_key_create() API. It then sets the
 * thread-specific value to the key using pthread_set_specific() and
 * gets it back using pthread_get_specific and asserts that they
 * are equal. It then deletes the key using pthread_key_delete().
 */

void test_pthread_key(void)
{
	int ret = -1;

	pthread_attr_t attr;
	struct sched_param schedparam;
	pthread_t newthread;

	ret = pthread_once(&key_once, make_key);

	/* TESTPOINT: Check if key is created */
	zassert_false(ret, "attempt to create key failed");

	pthread_attr_init(&attr);
	schedparam.priority = 2;
	pthread_attr_setschedparam(&attr, &schedparam);
	pthread_attr_setstack(&attr, &stacks[0][0], STACKSZ);

	ret = pthread_create(&newthread, &attr, thread_top,
			(void *)0);

	zassert_false(ret, "attempt to create thread failed");

	pthread_join(newthread, NULL);
}

void test_main(void)
{
	ztest_test_suite(test_pthread_keys,
			ztest_unit_test(test_pthread_key));
	ztest_run_test_suite(test_pthread_keys);
}
