/*
 * Copyright (c) 2024, Meta
 * Copyright (c) 2024, Marvin Ouma <pancakesdeath@protonmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define BIOS_FOOD 0xB105F00D

static bool attr_valid;
static pthread_attr_t attr;
static const pthread_attr_t uninit_attr;
static bool detached_thread_has_finished;

/*
 * This should be discarded by the linker, in this specific testsuite, if
 * CONFIG_DYNAMIC_THREAD_ALLOC is not set
 */
#define STATIC_THREAD_STACK_SIZE (MAX(1024, PTHREAD_STACK_MIN + CONFIG_TEST_EXTRA_STACK_SIZE))
static K_THREAD_STACK_DEFINE(static_thread_stack, STATIC_THREAD_STACK_SIZE);

static void *thread_entry(void *arg)
{
	bool joinable = (bool)POINTER_TO_UINT(arg);

	if (!joinable) {
		detached_thread_has_finished = true;
	}

	return NULL;
}

static void create_thread_common_entry(const pthread_attr_t *attrp, bool expect_success,
				       bool joinable, void *(*entry)(void *arg), void *arg)
{
	pthread_t th;

	if (!joinable) {
		detached_thread_has_finished = false;
	}

	if (expect_success) {
		zassert_ok(pthread_create(&th, attrp, entry, arg));
	} else {
		zassert_not_ok(pthread_create(&th, attrp, entry, arg));
		return;
	}

	if (joinable) {
		zassert_ok(pthread_join(th, NULL), "failed to join joinable thread");
		return;
	}

	/* should not be able to join detached thread */
	zassert_not_ok(pthread_join(th, NULL));

	for (size_t i = 0; i < 10; ++i) {
		k_msleep(2 * CONFIG_PTHREAD_RECYCLER_DELAY_MS);
		if (detached_thread_has_finished) {
			break;
		}
	}

	zassert_true(detached_thread_has_finished, "detached thread did not seem to finish");
}

static void create_thread_common(const pthread_attr_t *attrp, bool expect_success, bool joinable)
{
	create_thread_common_entry(attrp, expect_success, joinable, thread_entry,
				   UINT_TO_POINTER(joinable));
}

static inline void can_create_thread(const pthread_attr_t *attrp)
{
	create_thread_common(attrp, true, true);
}

ZTEST(xsi_threads_ext, test_pthread_attr_getstack)
{
	void *stackaddr = (void *)BIOS_FOOD;
	size_t stacksize = BIOS_FOOD;

	/* degenerate cases */
	{
		if (false) {
			/* undefined behaviour */
			zassert_equal(pthread_attr_getstack(NULL, NULL, NULL), EINVAL);
			zassert_equal(pthread_attr_getstack(NULL, NULL, &stacksize), EINVAL);
			zassert_equal(pthread_attr_getstack(NULL, &stackaddr, NULL), EINVAL);
			zassert_equal(pthread_attr_getstack(NULL, &stackaddr, &stacksize), EINVAL);
			zassert_equal(pthread_attr_getstack(&uninit_attr, &stackaddr, &stacksize),
				      EINVAL);
		}
		zassert_equal(pthread_attr_getstack(&attr, NULL, NULL), EINVAL);
		zassert_equal(pthread_attr_getstack(&attr, NULL, &stacksize), EINVAL);
		zassert_equal(pthread_attr_getstack(&attr, &stackaddr, NULL), EINVAL);
	}

	zassert_ok(pthread_attr_getstack(&attr, &stackaddr, &stacksize));
	zassert_not_equal(stackaddr, (void *)BIOS_FOOD);
	zassert_not_equal(stacksize, BIOS_FOOD);
}

ZTEST(xsi_threads_ext, test_pthread_attr_setstack)
{
	void *stackaddr;
	size_t stacksize;
	void *new_stackaddr;
	size_t new_stacksize;

	/* valid values */
	zassert_ok(pthread_attr_getstack(&attr, &stackaddr, &stacksize));

	/* degenerate cases */
	{
		if (false) {
			/* undefined behaviour */
			zassert_equal(pthread_attr_setstack(NULL, NULL, 0), EACCES);
			zassert_equal(pthread_attr_setstack(NULL, NULL, stacksize), EINVAL);
			zassert_equal(pthread_attr_setstack(NULL, stackaddr, 0), EINVAL);
			zassert_equal(pthread_attr_setstack(NULL, stackaddr, stacksize), EINVAL);
			zassert_equal(pthread_attr_setstack((pthread_attr_t *)&uninit_attr,
							    stackaddr, stacksize),
				      EINVAL);
		}
		zassert_equal(pthread_attr_setstack(&attr, NULL, 0), EACCES);
		zassert_equal(pthread_attr_setstack(&attr, NULL, stacksize), EACCES);
		zassert_equal(pthread_attr_setstack(&attr, stackaddr, 0), EINVAL);
	}

	/* ensure we can create and join a thread with the default attrs */
	can_create_thread(&attr);

	/* set stack / addr to the current values of stack / addr */
	zassert_ok(pthread_attr_setstack(&attr, stackaddr, stacksize));
	can_create_thread(&attr);

	/* qemu_x86 seems to be unable to set thread stacks to be anything less than 4096 */
	if (!IS_ENABLED(CONFIG_X86)) {
		/*
		 * check we can set a smaller stacksize
		 * should not require dynamic reallocation
		 * size may get rounded up to some alignment internally
		 */
		zassert_ok(pthread_attr_setstack(&attr, stackaddr, stacksize - 1));
		/* ensure we read back the same values as we specified */
		zassert_ok(pthread_attr_getstack(&attr, &new_stackaddr, &new_stacksize));
		zassert_equal(new_stackaddr, stackaddr);
		zassert_equal(new_stacksize, stacksize - 1);
		can_create_thread(&attr);
	}

	if (IS_ENABLED(CONFIG_DYNAMIC_THREAD_ALLOC)) {
		/* ensure we can set a dynamic stack */
		k_thread_stack_t *stack;

		stack = k_thread_stack_alloc(2 * stacksize, 0);
		zassert_not_null(stack);

		zassert_ok(pthread_attr_setstack(&attr, (void *)stack, 2 * stacksize));
		/* ensure we read back the same values as we specified */
		zassert_ok(pthread_attr_getstack(&attr, &new_stackaddr, &new_stacksize));
		zassert_equal(new_stackaddr, (void *)stack);
		zassert_equal(new_stacksize, 2 * stacksize);
		can_create_thread(&attr);
	}
}

ZTEST(xsi_threads_ext, test_pthread_set_get_concurrency)
{
	/* EINVAL if the value specified by new_level is negative */
	zassert_equal(EINVAL, pthread_setconcurrency(-42));

	/*
	 * Note: the special value 0 indicates the implementation will
	 * maintain the concurrency level at its own discretion.
	 *
	 * pthread_getconcurrency() should return a value of 0 on init.
	 */
	zassert_equal(0, pthread_getconcurrency());

	for (int i = 0; i <= CONFIG_MP_MAX_NUM_CPUS; ++i) {
		zassert_ok(pthread_setconcurrency(i));
		/* verify parameter is saved */
		zassert_equal(i, pthread_getconcurrency());
	}

	/* EAGAIN if the a system resource to be exceeded */
	zassert_equal(EAGAIN, pthread_setconcurrency(CONFIG_MP_MAX_NUM_CPUS + 1));
}

ZTEST(xsi_threads_ext, test_pthread_attr_getstacksize)
{
	size_t stacksize = BIOS_FOOD;

	/* degenerate cases */
	{
		if (false) {
			/* undefined behaviour */
			zassert_equal(pthread_attr_getstacksize(NULL, NULL), EINVAL);
			zassert_equal(pthread_attr_getstacksize(NULL, &stacksize), EINVAL);
			zassert_equal(pthread_attr_getstacksize(&uninit_attr, &stacksize), EINVAL);
		}
		zassert_equal(pthread_attr_getstacksize(&attr, NULL), EINVAL);
	}

	zassert_ok(pthread_attr_getstacksize(&attr, &stacksize));
	zassert_not_equal(stacksize, BIOS_FOOD);
}

ZTEST(xsi_threads_ext, test_pthread_attr_setstacksize)
{
	size_t stacksize;
	size_t new_stacksize;

	/* valid size */
	zassert_ok(pthread_attr_getstacksize(&attr, &stacksize));

	/* degenerate cases */
	{
		if (false) {
			/* undefined behaviour */
			zassert_equal(pthread_attr_setstacksize(NULL, 0), EINVAL);
			zassert_equal(pthread_attr_setstacksize(NULL, stacksize), EINVAL);
			zassert_equal(pthread_attr_setstacksize((pthread_attr_t *)&uninit_attr,
								stacksize),
				      EINVAL);
		}
		zassert_equal(pthread_attr_setstacksize(&attr, 0), EINVAL);
	}

	/* ensure we can spin up a thread with the default stack size */
	can_create_thread(&attr);

	/* set stack / addr to the current values of stack / addr */
	zassert_ok(pthread_attr_setstacksize(&attr, stacksize));
	/* ensure we can read back the values we just set */
	zassert_ok(pthread_attr_getstacksize(&attr, &new_stacksize));
	zassert_equal(new_stacksize, stacksize);
	can_create_thread(&attr);

	/* qemu_x86 seems to be unable to set thread stacks to be anything less than 4096 */
	if (!IS_ENABLED(CONFIG_X86)) {
		zassert_ok(pthread_attr_setstacksize(&attr, stacksize - 1));
		/* ensure we can read back the values we just set */
		zassert_ok(pthread_attr_getstacksize(&attr, &new_stacksize));
		zassert_equal(new_stacksize, stacksize - 1);
		can_create_thread(&attr);
	}

	if (IS_ENABLED(CONFIG_DYNAMIC_THREAD_ALLOC)) {
		zassert_ok(pthread_attr_setstacksize(&attr, 2 * stacksize));
		/* ensure we read back the same values as we specified */
		zassert_ok(pthread_attr_getstacksize(&attr, &new_stacksize));
		zassert_equal(new_stacksize, 2 * stacksize);
		can_create_thread(&attr);
	}
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

	zassert_ok(pthread_attr_init(&attr));
	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD_ALLOC)) {
		zassert_ok(pthread_attr_setstack(&attr, &static_thread_stack,
						 STATIC_THREAD_STACK_SIZE));
	}
	attr_valid = true;
}

static void after(void *arg)
{
	ARG_UNUSED(arg);

	if (attr_valid) {
		(void)pthread_attr_destroy(&attr);
		attr_valid = false;
	}
}

ZTEST_SUITE(xsi_threads_ext, NULL, NULL, before, after, NULL);
