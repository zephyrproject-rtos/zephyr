/*
 * Copyright (c) 2026 Göthel Software
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __ZEPHYR__

#include <pthread.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define STACKSIZE 2000
static K_THREAD_STACK_DEFINE(kthread__stack, STACKSIZE);

/* #define DEBUG_OUTPUT 1 */

#else

/* define _GNU_SOURCE */
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define DEBUG_OUTPUT 1

#endif

#ifdef DEBUG_OUTPUT

#define dbg_PRINT(fmt, ...)                                                                        \
	{                                                                                          \
		char name[20];                                                                     \
                                                                                                   \
		pthread_getname_np(pthread_self(), name, sizeof(name));                            \
		fputs(name, stderr);                                                               \
		fputs(": ", stderr);                                                               \
		fprintf(stderr, (fmt)__VA_OPT__(,) __VA_ARGS__);                                   \
		fflush(stderr);                                                                    \
	}

#else

#define dbg_PRINT(fmt, ...)

#endif

#ifndef __ZEPHYR__

#define zassert_ok(v)                                                                              \
	{                                                                                          \
		if ((v)) {                                                                         \
			dbg_PRINT("ERROR line:%d: v %zd, %s\n", __LINE__, (ssize_t)(v),            \
				  strerror(v));                                                    \
			exit(0);                                                                   \
		}                                                                                  \
	}
#define zassert_equal(a, b)                                                                        \
	{                                                                                          \
		if ((ssize_t)(a) != (ssize_t)(b)) {                                                \
			dbg_PRINT("ERROR line:%d : %zd != %zd\n", __LINE__, (ssize_t)(a),          \
				  (ssize_t)(b));                                                   \
			exit(0);                                                                   \
		}                                                                                  \
	}
#define zassert_equal_ptr(a, b)                                                                    \
	{                                                                                          \
		if ((a) != (b)) {                                                                  \
			dbg_PRINT("ERROR line:%d : %p != %p\n", __LINE__, (a), (b));               \
			exit(0);                                                                   \
		}                                                                                  \
	}
#define zassert_not_equal(a, b)                                                                    \
	{                                                                                          \
		if ((a) == (b)) {                                                                  \
			dbg_PRINT("ERROR line:%d : %zd != %zd\n", __LINE__, (ssize_t)(a),          \
				  (ssize_t)(b));                                                   \
			exit(0);                                                                   \
		}                                                                                  \
	}
#define zassert_is_null(a)                                                                         \
	{                                                                                          \
		if ((a)) {                                                                         \
			dbg_PRINT("ERROR line:%d: !null %p\n", __LINE__, (a));                     \
			exit(0);                                                                   \
		}                                                                                  \
	}
#define zassert_not_null(a)                                                                        \
	{                                                                                          \
		if (!(a)) {                                                                        \
			dbg_PRINT("ERROR line:%d : null %p\n", __LINE__, (a));                     \
			exit(0);                                                                   \
		}                                                                                  \
	}

#endif

static pthread_key_t key, key2, key3, key4;
static uint8_t const tagP = 0x11U;
static uint8_t const tagK = 0x22U;
static uint8_t const tagM = 0x33U;
static uint8_t const tagP2 = 0x12U;
static uint8_t const tagK2 = 0x23U;
static uint8_t const tagM2 = 0x34U;

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

#define STAGE_ZERO 0

static volatile int test_stage = STAGE_ZERO;

/**
 * Simple round-robin thread traversal
 * using `test_stage` as index.
 *
 *  Main          T1                  T2
 *
 *  0              1                   2
 *  3              4                   5
 *  6              7                   8
 *  9             10                  11
 * 12             13                  14
 * 15
 */

/* thread-entry start-point wait */
static void test_stage_wait(int stage)
{
	dbg_PRINT("stage: wait %d (has %d)\n", stage, test_stage);
	while (test_stage != stage) {
		zassert_ok(pthread_cond_wait(&cv, &m));
	}
	dbg_PRINT("stage: cont %d\n", test_stage);
}

/* regular round-robin, next awaited-stage is +3 (number of threads) */
static void test_stage_incr_wait(int *stage)
{
	dbg_PRINT("stage: incr %d -> %d\n", test_stage, test_stage + 1);
	test_stage = test_stage + 1;
	zassert_ok(pthread_cond_broadcast(&cv));

	*stage = *stage + 3;
	dbg_PRINT("stage: wait %d (has %d)\n", *stage, test_stage);
	while (test_stage != *stage) {
		zassert_ok(pthread_cond_wait(&cv, &m));
	}
	dbg_PRINT("stage: cont %d\n", test_stage);
}

/* tail-out, unlock mutex and terminate, mutex has to be unlocked thereafter */
static void test_stage_incr(void)
{
	dbg_PRINT("stage: incr %d -> %d\n", test_stage, test_stage + 1);
	test_stage = test_stage + 1;
	zassert_ok(pthread_cond_broadcast(&cv));
}

static void *thread_1(void *arg)
{
	pthread_t pself = pthread_self();
	uint8_t *val;
	int stage = 1;

	(void)arg;
	pthread_setname_np(pself, "thread_1");
	zassert_ok(pthread_mutex_lock(&m));
	test_stage_wait(stage);
	zassert_not_equal(0, pself);

	/* key -> tagP */
	val = (uint8_t *)pthread_getspecific(key);
	zassert_is_null(val);
	zassert_ok(pthread_setspecific(key, (void *)&tagP));
	val = (uint8_t *)pthread_getspecific(key);
	zassert_not_null(val);
	zassert_equal_ptr(val, &tagP);
	zassert_equal(*val, tagP);

	/* key2 -> tagP2 */
	val = (uint8_t *)pthread_getspecific(key2);
	zassert_is_null(val);
	zassert_ok(pthread_setspecific(key2, (void *)&tagP2));
	val = (uint8_t *)pthread_getspecific(key2);
	zassert_not_null(val);
	zassert_equal_ptr(val, &tagP2);
	zassert_equal(*val, tagP2);
	/* re-validate key after key2 setup */
	val = (uint8_t *)pthread_getspecific(key);
	zassert_not_null(val);
	zassert_equal_ptr(val, &tagP);
	zassert_equal(*val, tagP);

	test_stage_incr_wait(&stage);

	/* key2 deleted by host */
	zassert_equal(EINVAL, pthread_key_delete(key2));
	val = (uint8_t *)pthread_getspecific(key2);
	dbg_PRINT("p11: val %p\n", val);
	zassert_is_null(val);
	/* re-validate key after key2 deletion */
	val = (uint8_t *)pthread_getspecific(key);
	dbg_PRINT("p12: val %p\n", val);
	zassert_not_null(val);
	zassert_equal_ptr(val, &tagP);
	zassert_equal(*val, tagP);

	test_stage_incr_wait(&stage);

	/* delete key, check NULL value */
	zassert_ok(pthread_key_delete(key));
	zassert_equal(EINVAL, pthread_key_delete(key));
	val = (uint8_t *)pthread_getspecific(key);
	dbg_PRINT("p13: val %p (key deleted)\n", val);
	zassert_is_null(val);

	test_stage_incr_wait(&stage);

	/* key3 creation, NULL value check and key -> tagP */
	zassert_ok(pthread_key_create(&key3, NULL));
	val = (uint8_t *)pthread_getspecific(key3);
	zassert_is_null(val);
	zassert_ok(pthread_setspecific(key3, (void *)&tagP));
	val = (uint8_t *)pthread_getspecific(key3);
	zassert_not_null(val);
	zassert_equal_ptr(val, &tagP);
	zassert_equal(*val, tagP);
	dbg_PRINT("p14: val %p (key3 created)\n", val);

	test_stage_incr_wait(&stage);

	/* key3 and key4 deleted by host */
	zassert_equal(EINVAL, pthread_key_delete(key3));
	val = (uint8_t *)pthread_getspecific(key3);
	dbg_PRINT("p20: val %p\n", val);
	zassert_is_null(val);

	zassert_equal(EINVAL, pthread_key_delete(key4));
	val = (uint8_t *)pthread_getspecific(key4);
	dbg_PRINT("p21: val %p\n", val);
	zassert_is_null(val);

	dbg_PRINT("pxx: DONE\n");
	test_stage_incr();
	zassert_ok(pthread_mutex_unlock(&m) /* NOSONAR */);
	return NULL;
}

#ifdef __ZEPHYR__
static void thread_2(void)
#else
static void *thread_2(void *arg)
#endif
{
	pthread_t pself = pthread_self();
	uint8_t *val;

#ifndef __ZEPHYR__
	(void)arg;
#endif
	int stage = 2;

	pthread_setname_np(pself, "thread_2");
	zassert_ok(pthread_mutex_lock(&m));
	test_stage_wait(stage);
	dbg_PRINT("stage thread_2 %p\n", (void *)pself);
	zassert_not_equal(0, pself);

	/* key -> tagK */
	val = (uint8_t *)pthread_getspecific(key);
	zassert_is_null(val);
	zassert_ok(pthread_setspecific(key, (void *)&tagK));
	val = (uint8_t *)pthread_getspecific(key);
	zassert_not_null(val);
	zassert_equal_ptr(val, &tagK);
	zassert_equal(*val, tagK);

	/* key2 -> tagK2 */
	val = (uint8_t *)pthread_getspecific(key2);
	zassert_is_null(val);
	zassert_ok(pthread_setspecific(key2, (void *)&tagK2));
	val = (uint8_t *)pthread_getspecific(key2);
	zassert_not_null(val);
	zassert_equal_ptr(val, &tagK2);
	zassert_equal(*val, tagK2);
	/* re-validate key after key2 setup */
	val = (uint8_t *)pthread_getspecific(key);
	zassert_not_null(val);
	zassert_equal_ptr(val, &tagK);
	zassert_equal(*val, tagK);

	test_stage_incr_wait(&stage);

	/* key2 deleted by host */
	zassert_equal(EINVAL, pthread_key_delete(key2));
	val = (uint8_t *)pthread_getspecific(key2);
	dbg_PRINT("k11: val %p\n", val);
	zassert_is_null(val);
	/* re-validate key after key2 deletion */
	val = (uint8_t *)pthread_getspecific(key);
	dbg_PRINT("k12: val %p\n", val);
	zassert_not_null(val);
	zassert_equal_ptr(val, &tagK);
	zassert_equal(*val, tagK);

	test_stage_incr_wait(&stage);

	/* key deleted by thread_1 */
	zassert_equal(EINVAL, pthread_key_delete(key));
	val = (uint8_t *)pthread_getspecific(key);
	dbg_PRINT("k13: val %p\n", val);
	zassert_is_null(val);

	/* key4 creation, NULL value check and key -> tagK */
	zassert_ok(pthread_key_create(&key4, NULL));
	val = (uint8_t *)pthread_getspecific(key4);
	zassert_is_null(val);
	zassert_ok(pthread_setspecific(key4, (void *)&tagK));
	val = (uint8_t *)pthread_getspecific(key4);
	zassert_not_null(val);
	zassert_equal_ptr(val, &tagK);
	zassert_equal(*val, tagK);
	dbg_PRINT("k14: val %p (key4 created)\n", val);

	test_stage_incr_wait(&stage);
	dbg_PRINT("k15\n");
	test_stage_incr_wait(&stage);
	dbg_PRINT("k16\n");

	/* key3 and key4 deleted by host */
	zassert_equal(EINVAL, pthread_key_delete(key3));
	val = (uint8_t *)pthread_getspecific(key3);
	dbg_PRINT("k20: val %p\n", val);
	zassert_is_null(val);

	zassert_equal(EINVAL, pthread_key_delete(key4));
	val = (uint8_t *)pthread_getspecific(key4);
	dbg_PRINT("k21: val %p\n", val);
	zassert_is_null(val);

	dbg_PRINT("kxx: DONE\n");
	test_stage_incr();
	zassert_ok(pthread_mutex_unlock(&m) /* NOSONAR */);

#ifndef __ZEPHYR__
	return NULL;
#endif
}

#ifdef __ZEPHYR__
ZTEST(pthread_specific, test_pthread_specific_01)
#else
int main(int argc, char *argv[])
#endif
{
	pthread_t self = pthread_self();
	pthread_t thread1_id;
#ifdef __ZEPHYR__
	struct k_thread thread2_id;
#else
	pthread_t thread2_id;
#endif
	uint8_t *val;
	int stage = 0;

	pthread_setname_np(self, "thread_M");

	dbg_PRINT("stage main %p\n", (void *)self);
	dbg_PRINT("h00: tagM %p, tagM2 %p\n", &tagM, &tagM2);
	dbg_PRINT("h00: tagP %p, tagP2 %p\n", &tagP, &tagP2);
	dbg_PRINT("h00: tagK %p, tagK2 %p\n", &tagK, &tagK2);

	/* key creation, NULL value check and key -> tagM */
	zassert_ok(pthread_key_create(&key, NULL));
	val = (uint8_t *)pthread_getspecific(key);
	zassert_is_null(val);
	zassert_ok(pthread_setspecific(key, (void *)&tagM));
	val = (uint8_t *)pthread_getspecific(key);
	zassert_not_null(val);
	zassert_equal_ptr(val, &tagM);
	zassert_equal(*val, tagM);
	/* delete key, check NULL value, re-create key */
	{
		zassert_ok(pthread_key_delete(key));
		zassert_equal(EINVAL, pthread_key_delete(key));
		val = (uint8_t *)pthread_getspecific(key);
		zassert_is_null(val);
		zassert_ok(pthread_key_create(&key, NULL));
		val = (uint8_t *)pthread_getspecific(key);
		zassert_is_null(val);
		zassert_ok(pthread_setspecific(key, (void *)&tagM));
		val = (uint8_t *)pthread_getspecific(key);
		zassert_not_null(val);
		zassert_equal_ptr(val, &tagM);
		zassert_equal(*val, tagM);
	}

	/* key2 creation, NULL value check and key2 -> tagM2 */
	zassert_ok(pthread_key_create(&key2, NULL));
	val = (uint8_t *)pthread_getspecific(key2);
	zassert_is_null(val);
	zassert_ok(pthread_setspecific(key2, (void *)&tagM2));
	val = (uint8_t *)pthread_getspecific(key2);
	zassert_not_null(val);
	zassert_equal_ptr(val, &tagM2);
	zassert_equal(*val, tagM2);
	/* re-validate key after key2 setup */
	val = (uint8_t *)pthread_getspecific(key);
	zassert_not_null(val);
	zassert_equal_ptr(val, &tagM);
	zassert_equal(*val, tagM);
	dbg_PRINT("h01\n");

	/* Enter multi-threading tests */
	zassert_ok(pthread_mutex_lock(&m));
	{
		zassert_ok(pthread_create(&thread1_id, NULL, &thread_1, NULL));
#ifdef __ZEPHYR__
		k_thread_create(&thread2_id, kthread__stack, STACKSIZE, (k_thread_entry_t)thread_2,
				NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
#else
		zassert_ok(pthread_create(&thread2_id, NULL, &thread_2, NULL));
#endif
		dbg_PRINT("h02\n");
	}
	test_stage_incr_wait(&stage);
	dbg_PRINT("h04\n");
	/* re-validate this thread's key-pairs after child threads STAGE0 + STAGE1 */
	{
		val = (uint8_t *)pthread_getspecific(key);
		dbg_PRINT("h10: val %p\n", val);
		zassert_not_null(val);
		zassert_equal_ptr(val, &tagM);
		zassert_equal(*val, tagM);

		val = (uint8_t *)pthread_getspecific(key2);
		dbg_PRINT("h11: val %p\n", val);
		zassert_not_null(val);
		zassert_equal_ptr(val, &tagM2);
		zassert_equal(*val, tagM2);
	}
	/* delete key2, check NULL value, re-validate key */
	{
		zassert_ok(pthread_key_delete(key2));
		zassert_equal(EINVAL, pthread_key_delete(key2));
		val = (uint8_t *)pthread_getspecific(key2);
		dbg_PRINT("h12: val %p (key2 deleted)\n", val);
		zassert_is_null(val);
		/* re-validate key after key2 deletion */
		val = (uint8_t *)pthread_getspecific(key);
		dbg_PRINT("h13: val %p\n", val);
		zassert_not_null(val);
		zassert_equal_ptr(val, &tagM);
		zassert_equal(*val, tagM);
	}
	test_stage_incr_wait(&stage);
	test_stage_incr_wait(&stage);

	/* key deleted by thread_1 */
	/* Same behavior Zephyr + Linux: zassert_equal(EINVAL, pthread_key_delete(key)); */
	val = (uint8_t *)pthread_getspecific(key);
	dbg_PRINT("h14: val %p\n", val);
	zassert_is_null(val);

	/* re-validate deleted key2 by this thread */
	zassert_equal(EINVAL, pthread_key_delete(key2));
	val = (uint8_t *)pthread_getspecific(key2);
	dbg_PRINT("h15: val %p\n", val);
	zassert_is_null(val);

	/* re-create key and check NULL value, delete again */
	zassert_ok(pthread_key_create(&key, NULL));
	val = (uint8_t *)pthread_getspecific(key);
	dbg_PRINT("h16: val %p\n", val);
	zassert_is_null(val);
	zassert_ok(pthread_key_delete(key));
	zassert_equal(EINVAL, pthread_key_delete(key));

	test_stage_incr_wait(&stage);

	/* check key3 and key4 created by child threads and delete them */
	val = (uint8_t *)pthread_getspecific(key3);
	dbg_PRINT("h20: val %p\n", val);
	zassert_is_null(val);
	val = (uint8_t *)pthread_getspecific(key4);
	dbg_PRINT("h21: val %p\n", val);
	zassert_is_null(val);
	zassert_ok(pthread_key_delete(key3));
	zassert_equal(EINVAL, pthread_key_delete(key3));
	zassert_ok(pthread_key_delete(key4));
	zassert_equal(EINVAL, pthread_key_delete(key4));

	dbg_PRINT("h22\n");
	test_stage_incr();
	dbg_PRINT("h23\n");
	zassert_ok(pthread_mutex_unlock(&m) /* NOSONAR */);
	dbg_PRINT("h24\n");

	zassert_ok(pthread_join(thread1_id, NULL));
#ifdef __ZEPHYR__
	zassert_ok(k_thread_join(&thread2_id, K_FOREVER));
#else
	zassert_ok(pthread_join(thread2_id, NULL));
#endif
	dbg_PRINT("hXX: DONE\n");
}

#ifdef __ZEPHYR__

static void before(void *arg)
{
	ARG_UNUSED(arg);

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		/* skip redundant testing if there is no thread pool / heap allocation */
		ztest_test_skip();
	}
}

ZTEST_SUITE(pthread_specific, NULL, NULL, before, NULL, NULL);

#endif
