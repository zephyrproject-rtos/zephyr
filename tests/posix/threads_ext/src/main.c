/*
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define BIOS_FOOD           0xB105F00D
#define SCHED_INVALID       4242
#define INVALID_DETACHSTATE 7373

static bool attr_valid;
static pthread_attr_t attr;
static const pthread_attr_t uninit_attr;

ZTEST(posix_threads_ext, test_pthread_attr_getguardsize)
{
	size_t guardsize;

	/* degenerate cases */
	{
		if (false) {
			/* undefined behaviour */
			zassert_equal(pthread_attr_getguardsize(NULL, NULL), EINVAL);
			zassert_equal(pthread_attr_getguardsize(NULL, &guardsize), EINVAL);
			zassert_equal(pthread_attr_getguardsize(&uninit_attr, &guardsize), EINVAL);
		}
		zassert_equal(pthread_attr_getguardsize(&attr, NULL), EINVAL);
	}

	guardsize = BIOS_FOOD;
	zassert_ok(pthread_attr_getguardsize(&attr, &guardsize));
	zassert_not_equal(guardsize, BIOS_FOOD);
}

ZTEST(posix_threads_ext, test_pthread_attr_setguardsize)
{
	size_t guardsize = CONFIG_POSIX_PTHREAD_ATTR_GUARDSIZE_DEFAULT;
	size_t sizes[] = {0, BIT_MASK(CONFIG_POSIX_PTHREAD_ATTR_GUARDSIZE_BITS / 2),
			  BIT_MASK(CONFIG_POSIX_PTHREAD_ATTR_GUARDSIZE_BITS)};

	/* valid value */
	zassert_ok(pthread_attr_getguardsize(&attr, &guardsize));

	/* degenerate cases */
	{
		if (false) {
			/* undefined behaviour */
			zassert_equal(pthread_attr_setguardsize(NULL, SIZE_MAX), EINVAL);
			zassert_equal(pthread_attr_setguardsize(NULL, guardsize), EINVAL);
			zassert_equal(pthread_attr_setguardsize((pthread_attr_t *)&uninit_attr,
								guardsize),
				      EINVAL);
		}
		zassert_equal(pthread_attr_setguardsize(&attr, SIZE_MAX), EINVAL);
	}

	ARRAY_FOR_EACH(sizes, i) {
		zassert_ok(pthread_attr_setguardsize(&attr, sizes[i]));
		guardsize = ~sizes[i];
		zassert_ok(pthread_attr_getguardsize(&attr, &guardsize));
		zassert_equal(guardsize, sizes[i]);
	}
}

ZTEST(posix_threads_ext, test_pthread_mutexattr_gettype)
{
	int type;
	pthread_mutexattr_t attr;

	/* degenerate cases */
	{
		if (false) {
			/* undefined behaviour */
			zassert_equal(EINVAL, pthread_mutexattr_gettype(&attr, &type));
		}
		zassert_equal(EINVAL, pthread_mutexattr_gettype(NULL, NULL));
		zassert_equal(EINVAL, pthread_mutexattr_gettype(NULL, &type));
		zassert_equal(EINVAL, pthread_mutexattr_gettype(&attr, NULL));
	}

	zassert_ok(pthread_mutexattr_init(&attr));
	zassert_ok(pthread_mutexattr_gettype(&attr, &type));
	zassert_equal(type, PTHREAD_MUTEX_DEFAULT);
	zassert_ok(pthread_mutexattr_destroy(&attr));
}

ZTEST(posix_threads_ext, test_pthread_mutexattr_settype)
{
	int type;
	pthread_mutexattr_t attr;

	/* degenerate cases */
	{
		if (false) {
			/* undefined behaviour */
			zassert_equal(EINVAL,
				      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_DEFAULT));
		}
		zassert_equal(EINVAL, pthread_mutexattr_settype(NULL, 42));
		zassert_equal(EINVAL, pthread_mutexattr_settype(NULL, PTHREAD_MUTEX_NORMAL));
		zassert_equal(EINVAL, pthread_mutexattr_settype(&attr, 42));
	}

	zassert_ok(pthread_mutexattr_init(&attr));

	zassert_ok(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_DEFAULT));
	zassert_ok(pthread_mutexattr_gettype(&attr, &type));
	zassert_equal(type, PTHREAD_MUTEX_DEFAULT);

	zassert_ok(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL));
	zassert_ok(pthread_mutexattr_gettype(&attr, &type));
	zassert_equal(type, PTHREAD_MUTEX_NORMAL);

	zassert_ok(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE));
	zassert_ok(pthread_mutexattr_gettype(&attr, &type));
	zassert_equal(type, PTHREAD_MUTEX_RECURSIVE);

	zassert_ok(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK));
	zassert_ok(pthread_mutexattr_gettype(&attr, &type));
	zassert_equal(type, PTHREAD_MUTEX_ERRORCHECK);

	zassert_ok(pthread_mutexattr_destroy(&attr));
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

	zassert_ok(pthread_attr_init(&attr));
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

ZTEST_SUITE(posix_threads_ext, NULL, NULL, before, after, NULL);
