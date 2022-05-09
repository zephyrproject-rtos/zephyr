/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file Newlib thread-safety lock test
 *
 * This file contains a set of tests to verify that the newlib retargetable
 * locking interface is functional and the internal newlib locks function as
 * intended.
 */

#include <zephyr/zephyr.h>
#include <ztest.h>

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <envlock.h>

#define STACK_SIZE	(512 + CONFIG_TEST_EXTRA_STACK_SIZE)

#ifdef CONFIG_USERSPACE
#define THREAD_OPT	(K_USER | K_INHERIT_PERMS)
#else
#define THREAD_OPT	(0)
#endif /* CONFIG_USERSPACE */

static struct k_thread tdata;
static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);

/* Newlib internal lock functions */
extern void __sfp_lock_acquire(void);
extern void __sfp_lock_release(void);
extern void __sinit_lock_acquire(void);
extern void __sinit_lock_release(void);
extern void __tz_lock(void);
extern void __tz_unlock(void);

/* Static locks */
extern struct k_mutex __lock___sinit_recursive_mutex;
extern struct k_mutex __lock___sfp_recursive_mutex;
extern struct k_mutex __lock___atexit_recursive_mutex;
extern struct k_mutex __lock___malloc_recursive_mutex;
extern struct k_mutex __lock___env_recursive_mutex;
extern struct k_sem __lock___at_quick_exit_mutex;
extern struct k_sem __lock___tz_mutex;
extern struct k_sem __lock___dd_hash_mutex;
extern struct k_sem __lock___arc4random_mutex;

/**
 * @brief Test retargetable locking non-recursive (semaphore) interface
 *
 * This test verifies that a non-recursive lock (semaphore) can be dynamically
 * created, acquired, released and closed through the retargetable locking
 * interface.
 */
static void test_retargetable_lock_sem(void)
{
	_LOCK_T lock = NULL;

	/* Dynamically allocate and initialise a new lock */
	__retarget_lock_init(&lock);
	zassert_not_null(lock, "non-recursive lock init failed");

	/* Acquire lock and verify acquisition */
	__retarget_lock_acquire(lock);
	zassert_equal(__retarget_lock_try_acquire(lock), 0,
		      "non-recursive lock acquisition failed");

	/* Release lock and verify release */
	__retarget_lock_release(lock);
	zassert_not_equal(__retarget_lock_try_acquire(lock), 0,
			  "non-recursive lock release failed");

	/* Close and deallocate lock */
	__retarget_lock_close(lock);
}

static void retargetable_lock_mutex_thread_acq(void *p1, void *p2, void *p3)
{
	_LOCK_T lock = p1;
	int ret;

	/*
	 * Attempt to lock the recursive lock from child thread and verify
	 * that it fails.
	 */
	ret = __retarget_lock_try_acquire_recursive(lock);
	zassert_equal(ret, 0, "recursive lock acquisition failed");
}

static void retargetable_lock_mutex_thread_rel(void *p1, void *p2, void *p3)
{
	_LOCK_T lock = p1;
	int ret;

	/*
	 * Attempt to lock the recursive lock from child thread and verify
	 * that it fails.
	 */
	ret = __retarget_lock_try_acquire_recursive(lock);
	zassert_not_equal(ret, 0, "recursive lock release failed");
}

/**
 * @brief Test retargetable locking recursive (mutex) interface
 *
 * This test verifies that a recursive lock (mutex) can be dynamically created,
 * acquired, released, and closed through the retargetable locking interface.
 */
static void test_retargetable_lock_mutex(void)
{
	_LOCK_T lock = NULL;
	k_tid_t tid;

	/* Dynamically allocate and initialise a new lock */
	__retarget_lock_init_recursive(&lock);
	zassert_not_null(lock, "recursive lock init failed");

	/* Acquire lock from parent thread */
	__retarget_lock_acquire_recursive(lock);

	/* Spawn a lock acquisition check thread and wait for exit */
	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      retargetable_lock_mutex_thread_acq, lock,
			      NULL, NULL, K_PRIO_PREEMPT(0), THREAD_OPT,
			      K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);

	/* Release lock from parent thread */
	__retarget_lock_release_recursive(lock);

	/* Spawn a lock release check thread and wait for exit */
	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      retargetable_lock_mutex_thread_rel, lock,
			      NULL, NULL, K_PRIO_PREEMPT(0), THREAD_OPT,
			      K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);

	/* Close and deallocate lock */
	__retarget_lock_close_recursive(lock);
}

static void sinit_lock_thread_acq(void *p1, void *p2, void *p3)
{
	int ret;

	/*
	 * Attempt to lock the sinit mutex from child thread using
	 * retargetable locking interface. This operation should fail if the
	 * __sinit_lock_acquire() implementation internally uses the
	 * retargetable locking interface.
	 */
	ret = __retarget_lock_try_acquire_recursive(
			(_LOCK_T)&__lock___sinit_recursive_mutex);

	zassert_equal(ret, 0, "__sinit_lock_acquire() is not using "
			      "retargetable locking interface");
}

static void sinit_lock_thread_rel(void *p1, void *p2, void *p3)
{
	int ret;

	/*
	 * Attempt to lock the sinit mutex from child thread using
	 * retargetable locking interface. This operation should succeed if the
	 * __sinit_lock_release() implementation internally uses the
	 * retargetable locking interface.
	 */
	ret = __retarget_lock_try_acquire_recursive(
			(_LOCK_T)&__lock___sinit_recursive_mutex);

	zassert_not_equal(ret, 0, "__sinit_lock_release() is not using "
				  "retargetable locking interface");

	/* Release sinit lock */
	__retarget_lock_release_recursive(
		(_LOCK_T)&__lock___sinit_recursive_mutex);
}

/**
 * @brief Test sinit lock functions
 *
 * This test calls the __sinit_lock_acquire() and __sinit_lock_release()
 * functions to verify that sinit lock is functional and its implementation
 * is provided by the retargetable locking interface.
 */
static void test_sinit_lock(void)
{
	k_tid_t tid;

	/* Lock the sinit mutex from parent thread */
	__sinit_lock_acquire();

	/* Spawn a lock check thread and wait for exit */
	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      sinit_lock_thread_acq, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(0), THREAD_OPT, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);

	/* Unlock the sinit mutex from parent thread */
	__sinit_lock_release();

	/* Spawn an unlock check thread and wait for exit */
	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      sinit_lock_thread_rel, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(0), THREAD_OPT, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

static void sfp_lock_thread_acq(void *p1, void *p2, void *p3)
{
	int ret;

	/*
	 * Attempt to lock the sfp mutex from child thread using retargetable
	 * locking interface. This operation should fail if the
	 * __sfp_lock_acquire() implementation internally uses the retargetable
	 * locking interface.
	 */
	ret = __retarget_lock_try_acquire_recursive(
			(_LOCK_T)&__lock___sfp_recursive_mutex);

	zassert_equal(ret, 0, "__sfp_lock_acquire() is not using "
			      "retargetable locking interface");
}

static void sfp_lock_thread_rel(void *p1, void *p2, void *p3)
{
	int ret;

	/*
	 * Attempt to lock the sfp mutex from child thread using retargetable
	 * locking interface. This operation should succeed if the
	 * __sfp_lock_release() implementation internally uses the retargetable
	 * locking interface.
	 */
	ret = __retarget_lock_try_acquire_recursive(
			(_LOCK_T)&__lock___sfp_recursive_mutex);

	zassert_not_equal(ret, 0, "__sfp_lock_release() is not using "
				  "retargetable locking interface");

	/* Release sfp lock */
	__retarget_lock_release_recursive(
		(_LOCK_T)&__lock___sfp_recursive_mutex);
}

/**
 * @brief Test sfp lock functions
 *
 * This test calls the __sfp_lock_acquire() and __sfp_lock_release() functions
 * to verify that sfp lock is functional and its implementation is provided by
 * the retargetable locking interface.
 */
static void test_sfp_lock(void)
{
	k_tid_t tid;

	/* Lock the sfp mutex from parent thread */
	__sfp_lock_acquire();

	/* Spawn a lock check thread and wait for exit */
	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      sfp_lock_thread_acq, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(0), THREAD_OPT, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);

	/* Unlock the sfp mutex from parent thread */
	__sfp_lock_release();

	/* Spawn an unlock check thread and wait for exit */
	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      sfp_lock_thread_rel, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(0), THREAD_OPT, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

static void malloc_lock_thread_lock(void *p1, void *p2, void *p3)
{
	int ret;

	/*
	 * Attempt to lock the malloc mutex from child thread using
	 * retargetable locking interface. This operation should fail if the
	 * __malloc_lock() implementation internally uses the retargetable
	 * locking interface.
	 */
	ret = __retarget_lock_try_acquire_recursive(
			(_LOCK_T)&__lock___malloc_recursive_mutex);

	zassert_equal(ret, 0, "__malloc_lock() is not using retargetable "
			      "locking interface");
}

static void malloc_lock_thread_unlock(void *p1, void *p2, void *p3)
{
	int ret;

	/*
	 * Attempt to lock the malloc mutex from child thread using
	 * retargetable locking interface. This operation should succeed if the
	 * __malloc_unlock() implementation internally uses the retargetable
	 * locking interface.
	 */
	ret = __retarget_lock_try_acquire_recursive(
			(_LOCK_T)&__lock___malloc_recursive_mutex);

	zassert_not_equal(ret, 0, "__malloc_unlock() is not using "
				  "retargetable locking interface");

	/* Release malloc lock */
	__retarget_lock_release_recursive(
		(_LOCK_T)&__lock___malloc_recursive_mutex);
}

/**
 * @brief Test malloc lock functions
 *
 * This test calls the __malloc_lock() and __malloc_unlock() functions to
 * verify that malloc lock is functional and its implementation is provided by
 * the retargetable locking interface.
 */
static void test_malloc_lock(void)
{
	k_tid_t tid;

	/* Lock the malloc mutex from parent thread */
	__malloc_lock(_REENT);

	/* Spawn a lock check thread and wait for exit */
	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      malloc_lock_thread_lock, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(0), THREAD_OPT, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);

	/* Unlock the malloc mutex from parent thread */
	__malloc_unlock(_REENT);

	/* Spawn an unlock check thread and wait for exit */
	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      malloc_lock_thread_unlock, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(0), THREAD_OPT, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

static void env_lock_thread_lock(void *p1, void *p2, void *p3)
{
	int ret;

	/*
	 * Attempt to lock the env mutex from child thread using
	 * retargetable locking interface. This operation should fail if the
	 * __env_lock() implementation internally uses the retargetable
	 * locking interface.
	 */
	ret = __retarget_lock_try_acquire_recursive(
			(_LOCK_T)&__lock___env_recursive_mutex);

	zassert_equal(ret, 0, "__env_lock() is not using retargetable "
			      "locking interface");
}

static void env_lock_thread_unlock(void *p1, void *p2, void *p3)
{
	int ret;

	/*
	 * Attempt to lock the env mutex from child thread using
	 * retargetable locking interface. This operation should succeed if the
	 * __env_unlock() implementation internally uses the retargetable
	 * locking interface.
	 */
	ret = __retarget_lock_try_acquire_recursive(
			(_LOCK_T)&__lock___env_recursive_mutex);

	zassert_not_equal(ret, 0, "__env_unlock() is not using "
				  "retargetable locking interface");

	/* Release env lock */
	__retarget_lock_release_recursive(
		(_LOCK_T)&__lock___env_recursive_mutex);
}

/**
 * @brief Test env lock functions
 *
 * This test calls the __env_lock() and __env_unlock() functions to verify
 * that env lock is functional and its implementation is provided by the
 * retargetable locking interface.
 */
static void test_env_lock(void)
{
	k_tid_t tid;

	/* Lock the env mutex from parent thread */
	__env_lock(_REENT);

	/* Spawn a lock check thread and wait for exit */
	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      env_lock_thread_lock, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(0), THREAD_OPT, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);

	/* Unlock the env mutex from parent thread */
	__env_unlock(_REENT);

	/* Spawn an unlock check thread and wait for exit */
	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      env_lock_thread_unlock, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(0), THREAD_OPT, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Test tz lock functions
 *
 * This test calls the __tz_lock() and __tz_unlock() functions to verify that
 * tz lock is functional and its implementation is provided by the retargetable
 * locking interface.
 */
static void test_tz_lock(void)
{
	/* Lock the tz semaphore */
	__tz_lock();

	/* Attempt to acquire lock and verify failure */
	zassert_equal(
		__retarget_lock_try_acquire((_LOCK_T)&__lock___tz_mutex), 0,
		"__tz_lock() is not using retargetable locking interface");

	/* Unlock the tz semaphore */
	__tz_unlock();

	/* Attempt to acquire lock and verify success */
	zassert_not_equal(
		__retarget_lock_try_acquire((_LOCK_T)&__lock___tz_mutex), 0,
		"__tz_unlock() is not using retargetable locking interface");

	/* Clean up */
	__retarget_lock_release((_LOCK_T)&__lock___tz_mutex);
}

void test_newlib_thread_safety_locks(void)
{
#ifdef CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(), &tdata, &tstack);
#endif /* CONFIG_USERSPACE */

	ztest_test_suite(newlib_thread_safety_locks,
			 ztest_user_unit_test(test_retargetable_lock_sem),
			 ztest_user_unit_test(test_retargetable_lock_mutex),
			 ztest_user_unit_test(test_sinit_lock),
			 ztest_user_unit_test(test_sfp_lock),
			 ztest_user_unit_test(test_malloc_lock),
			 ztest_user_unit_test(test_env_lock),
			 ztest_user_unit_test(test_tz_lock));

	ztest_run_test_suite(newlib_thread_safety_locks);
}
