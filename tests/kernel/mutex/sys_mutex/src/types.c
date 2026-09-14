/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/mutex.h>
#include <zephyr/ztest.h>

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

#ifdef CONFIG_USERSPACE
#define ZTEST_USER_OR_NOT ZTEST_USER
#define OTHER_OPTIONS     (K_USER | K_INHERIT_PERMS)
#else
#define ZTEST_USER_OR_NOT ZTEST
#define OTHER_OPTIONS     0
#endif

static K_THREAD_STACK_DEFINE(other_stack, STACK_SIZE);
static struct k_thread other_thread;
static ZTEST_BMEM SYS_MUTEX_DEFINE(recursive_mutex);
static ZTEST_DMEM SYS_MUTEX_DEFINE_EXT(normal_mutex, K_MUTEX_NORMAL);
static ZTEST_DMEM SYS_MUTEX_DEFINE_EXT(errorcheck_mutex, K_MUTEX_ERRORCHECK);
static ZTEST_BMEM int other_ret;
/* dynamically initialized; shared with the other thread, so not on this thread's stack */
static ZTEST_BMEM struct sys_mutex dynamic;
/* the other thread cannot read this thread's stack */
static ZTEST_BMEM k_timeout_t other_timeout;

static void other_lock(void *p1, void *p2, void *p3)
{
	struct sys_mutex *mutex = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	other_ret = sys_mutex_lock(mutex, other_timeout);
	if (other_ret == 0) {
		other_ret = sys_mutex_unlock(mutex);
	}
}

static void other_unlock(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	other_ret = sys_mutex_unlock(p1);
}

static int from_other(k_thread_entry_t entry, struct sys_mutex *mutex, k_timeout_t timeout)
{
	other_ret = 1;
	other_timeout = timeout;
	k_thread_create(&other_thread, other_stack, STACK_SIZE, entry, mutex, NULL, NULL,
			K_PRIO_PREEMPT(1), OTHER_OPTIONS, K_NO_WAIT);
	zassert_ok(k_thread_join(&other_thread, K_SECONDS(2)));

	return other_ret;
}

static void contended(struct sys_mutex *mutex)
{
	zassert_ok(sys_mutex_lock(mutex, K_FOREVER));
	zassert_equal(from_other(other_lock, mutex, K_NO_WAIT), -EBUSY);
	zassert_equal(from_other(other_lock, mutex, K_MSEC(10)), -EAGAIN);
	zassert_ok(sys_mutex_unlock(mutex));
	zassert_equal(from_other(other_lock, mutex, K_FOREVER), 0);
	zassert_equal(sys_mutex_unlock(mutex), -EINVAL, "unlocked mutex");
}

ZTEST_USER_OR_NOT(sys_mutex_types, test_sys_mutex_types)
{
	/* recursive: the default of SYS_MUTEX_DEFINE() and sys_mutex_init() */
	zassert_ok(sys_mutex_lock(&recursive_mutex, K_NO_WAIT));
	zassert_ok(sys_mutex_lock(&recursive_mutex, K_NO_WAIT));
	zassert_ok(sys_mutex_unlock(&recursive_mutex));
	zassert_equal(from_other(other_lock, &recursive_mutex, K_NO_WAIT), -EBUSY);
	zassert_equal(from_other(other_unlock, &recursive_mutex, K_NO_WAIT), -EPERM);
	zassert_ok(sys_mutex_unlock(&recursive_mutex));
	contended(&recursive_mutex);

	/* normal: relocking by the owner is a contended lock */
	zassert_ok(sys_mutex_lock(&normal_mutex, K_NO_WAIT));
	zassert_equal(sys_mutex_lock(&normal_mutex, K_NO_WAIT), -EBUSY);
	zassert_equal(sys_mutex_lock(&normal_mutex, K_MSEC(10)), -EAGAIN);
	zassert_ok(sys_mutex_unlock(&normal_mutex));
	contended(&normal_mutex);

	/* error checking: relocking is detected, foreign unlock is refused */
	zassert_ok(sys_mutex_lock(&errorcheck_mutex, K_NO_WAIT));
	zassert_equal(sys_mutex_lock(&errorcheck_mutex, K_NO_WAIT), -EDEADLK);
	zassert_equal(sys_mutex_lock(&errorcheck_mutex, K_FOREVER), -EDEADLK);
	zassert_equal(from_other(other_unlock, &errorcheck_mutex, K_NO_WAIT), -EPERM);
	zassert_ok(sys_mutex_unlock(&errorcheck_mutex));
	contended(&errorcheck_mutex);

	/* dynamic initialization, destroy */
	sys_mutex_init(&dynamic);
	zassert_ok(sys_mutex_lock(&dynamic, K_NO_WAIT));
	zassert_equal(sys_mutex_destroy(&dynamic), -EBUSY);
	zassert_ok(sys_mutex_unlock(&dynamic));
	zassert_ok(sys_mutex_destroy(&dynamic));
	zassert_equal(sys_mutex_init_ext(&dynamic, K_MUTEX_TYPE_MASK), -EINVAL);
	zassert_equal(sys_mutex_init_ext(&dynamic, BIT(7)), -EINVAL);
	zassert_ok(sys_mutex_init_ext(&dynamic, K_MUTEX_ERRORCHECK));
	zassert_ok(sys_mutex_lock(&dynamic, K_NO_WAIT));
	zassert_equal(sys_mutex_lock(&dynamic, K_NO_WAIT), -EDEADLK);
	zassert_ok(sys_mutex_unlock(&dynamic));
	zassert_ok(sys_mutex_destroy(&dynamic));

	/* priority inheritance: the kernel enforces ownership and releases on destroy */
	zassert_ok(sys_mutex_init_ext(&dynamic, K_MUTEX_RECURSIVE | K_MUTEX_PRIO_INHERIT));
	zassert_ok(sys_mutex_lock(&dynamic, K_NO_WAIT));
	zassert_ok(sys_mutex_lock(&dynamic, K_NO_WAIT));
	zassert_equal(from_other(other_unlock, &dynamic, K_NO_WAIT), -EPERM);
	zassert_ok(sys_mutex_unlock(&dynamic));
	zassert_equal(sys_mutex_destroy(&dynamic), -EBUSY);
	zassert_ok(sys_mutex_unlock(&dynamic));
	contended(&dynamic);
	zassert_ok(sys_mutex_destroy(&dynamic));
	zassert_ok(sys_mutex_destroy(&dynamic), "destroying twice");
}

static void *types_setup(void)
{
#ifdef CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(), &other_thread, &other_stack);
#endif
	return NULL;
}

ZTEST_SUITE(sys_mutex_types, NULL, types_setup, NULL, NULL, NULL);
