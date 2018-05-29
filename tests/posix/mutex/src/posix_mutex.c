/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <errno.h>
#include <pthread.h>

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACKSIZE)

static K_THREAD_STACK_DEFINE(stack, STACK_SIZE);

#define SLEEP_MS 100

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;

void *normal_mutex_entry(void *p1)
{
	int i, rc;

	/* Sleep for maximum 300 ms as main thread is sleeping for 100 ms */

	for (i = 0; i < 3; i++) {
		rc = pthread_mutex_trylock(&mutex1);
		if (rc == 0) {
			break;
		}
		k_sleep(SLEEP_MS);
	}

	zassert_false(rc, "try lock failed");
	TC_PRINT("mutex lock is taken\n");
	zassert_false(pthread_mutex_unlock(&mutex1),
		      "mutex unlock is falied");
	return NULL;
}

void *recursive_mutex_entry(void *p1)
{
	zassert_false(pthread_mutex_lock(&mutex2), "mutex is not taken");
	zassert_false(pthread_mutex_lock(&mutex2),
		      "mutex is not taken 2nd time");
	TC_PRINT("recrusive mutex lock is taken\n");
	zassert_false(pthread_mutex_unlock(&mutex2),
		      "mutex is not unlocked");
	zassert_false(pthread_mutex_unlock(&mutex2),
		      "mutex is not unlocked");
	return NULL;
}

/**
 * @brief Test to demonstrate PTHREAD_MUTEX_NORMAL
 *
 * @details Mutex type is setup as normal. pthread_mutex_trylock
 *	    and pthread_mutex_lock are tested with mutex type being
 *	    normal.
 */
static void test_mutex_normal(void)
{
	pthread_t thread_1;
	pthread_attr_t attr;
	pthread_mutexattr_t mut_attr;
	struct sched_param schedparam;
	int schedpolicy = SCHED_FIFO;
	int ret, type, protocol, temp;

	schedparam.priority = 2;
	ret = pthread_attr_init(&attr);
	if (ret != 0) {
		zassert_false(pthread_attr_destroy(&attr),
			      "Unable to destroy pthread object attrib");
		zassert_false(pthread_attr_init(&attr),
			      "Unable to create pthread object attrib");
	}

	pthread_attr_setstack(&attr, &stack, STACK_SIZE);
	pthread_attr_setschedpolicy(&attr, schedpolicy);
	pthread_attr_setschedparam(&attr, &schedparam);

	temp = pthread_mutexattr_settype(&mut_attr, PTHREAD_MUTEX_NORMAL);
	zassert_false(temp, "setting mutex type is failed");
	temp = pthread_mutex_init(&mutex1, &mut_attr);
	zassert_false(temp, "mutex initialization is failed");

	temp = pthread_mutexattr_gettype(&mut_attr, &type);
	zassert_false(temp, "reading mutex type is failed");
	temp = pthread_mutexattr_getprotocol(&mut_attr, &protocol);
	zassert_false(temp, "reading mutex protocol is failed");

	pthread_mutex_lock(&mutex1);

	zassert_equal(type, PTHREAD_MUTEX_NORMAL,
		      "mutex type is not normal");

	zassert_equal(protocol, PTHREAD_PRIO_NONE,
		      "mutex protocol is not prio_none");
	ret = pthread_create(&thread_1, &attr, &normal_mutex_entry, NULL);

	if (ret) {
		TC_PRINT("Thread1 creation failed %d", ret);
	}
	k_sleep(SLEEP_MS);
	pthread_mutex_unlock(&mutex1);

	pthread_join(thread_1, NULL);
	temp = pthread_mutex_destroy(&mutex1);
	zassert_false(temp, "Destroying mutex is failed");
}

/**
 * @brief Test to demonstrate PTHREAD_MUTEX_RECURSIVE
 *
 * @details Mutex type is setup as recursive. mutex will be locked
 *	    twice and unlocked for the same number of time.
 *
 */
static void test_recursive_mutex(void)
{
	pthread_t thread_2;
	pthread_attr_t attr2;
	pthread_mutexattr_t mut_attr2;
	struct sched_param schedparam2;
	int schedpolicy = SCHED_FIFO;
	int ret, type, protocol, temp;

	schedparam2.priority = 2;
	ret = pthread_attr_init(&attr2);
	if (ret != 0) {
		zassert_false(pthread_attr_destroy(&attr2),
			      "Unable to destroy pthread object attrib");
		zassert_false(pthread_attr_init(&attr2),
			      "Unable to create pthread object attrib");
	}

	pthread_attr_setstack(&attr2, &stack, STACK_SIZE);
	pthread_attr_setschedpolicy(&attr2, schedpolicy);
	pthread_attr_setschedparam(&attr2, &schedparam2);

	temp = pthread_mutexattr_settype(&mut_attr2, PTHREAD_MUTEX_RECURSIVE);
	zassert_false(temp, "setting mutex2 type is failed");
	temp = pthread_mutex_init(&mutex2, &mut_attr2);
	zassert_false(temp, "mutex2 initialization is failed");

	temp = pthread_mutexattr_gettype(&mut_attr2, &type);
	zassert_false(temp, "reading mutex2 type is failed");
	temp = pthread_mutexattr_getprotocol(&mut_attr2, &protocol);
	zassert_false(temp, "reading mutex2 protocol is failed");

	zassert_equal(type, PTHREAD_MUTEX_RECURSIVE,
		      "mutex2 type is not recursive");

	zassert_equal(protocol, PTHREAD_PRIO_NONE,
		      "mutex2 protocol is not prio_none");
	ret = pthread_create(&thread_2, &attr2, &recursive_mutex_entry, NULL);

	zassert_false(ret, "Thread2 creation failed");

	pthread_join(thread_2, NULL);
	temp = pthread_mutex_destroy(&mutex2);
	zassert_false(temp, "Destroying mutex2 is failed");
}

void test_main(void)
{
	ztest_test_suite(test_mutex, ztest_unit_test(test_mutex_normal),
			ztest_unit_test(test_recursive_mutex));
	ztest_run_test_suite(test_mutex);
}
