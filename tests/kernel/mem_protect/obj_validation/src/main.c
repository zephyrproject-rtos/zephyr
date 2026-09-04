/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/ztest.h>
#include <kernel_internal.h>

#define SEM_ARRAY_SIZE	16

/* Show that extern declarations don't interfere with detecting kernel
 * objects, this was at one point a problem.
 */
extern struct k_sem sem1;

static struct k_sem semarray[SEM_ARRAY_SIZE];
static struct k_sem *dyn_sem[SEM_ARRAY_SIZE];

static struct k_mutex *test_dyn_mutex;

K_SEM_DEFINE(sem1, 0, 1);
static struct k_sem sem2;
static char bad_sem[sizeof(struct k_sem)];
static struct k_sem sem3;

static int test_object(struct k_sem *sem, int retval)
{
	int ret;

	if (retval) {
		/* Expected to fail; bypass k_object_validation_check() so we don't
		 * fill the logs with spam
		 */
		ret = k_object_validate(k_object_find(sem), K_OBJ_SEM, 0);
	} else {
		ret = k_object_validation_check(k_object_find(sem), sem,
					    K_OBJ_SEM, 0);
	}

	if (ret != retval) {
		TC_PRINT("FAIL check of %p is not %d, got %d instead\n", sem,
			 retval, ret);
		return 1;
	}
	return 0;
}

void object_permission_checks(struct k_sem *sem, bool skip_init)
{
	/* Should fail because we don't have perms on this object */
	zassert_false(test_object(sem, -EPERM),
		      "object should not have had permission granted");

	k_object_access_grant(sem, k_current_get());

	if (!skip_init) {
		/* Should fail, not initialized and we have no permissions */
		zassert_false(test_object(sem, -EINVAL),
			      "object should not have been initialized");
		k_sem_init(sem, 0, 1);
	}

	/* This should succeed now */
	zassert_false(test_object(sem, 0),
		      "object should have had sufficient permissions");
}

/**
 * @brief Verify that kernel object validation reports address, permission,
 *        type and init state.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Every system call that takes a kernel object has to answer three questions
 * about it before trusting it: is this address a kernel object at all, does
 * the calling thread have access to it, and is it initialized and of the
 * expected type. Each answer has its own error code, so the test drives an
 * address through every state in turn -- unknown to the table, known but not
 * granted, granted but uninitialized, and fully usable -- and checks the code
 * that comes back. Dynamically allocated objects are put through the same
 * sequence and then freed, which also covers an object leaving the table.
 *
 * Test steps:
 * - Validate a stack variable, a bad address and a wild pointer, none of
 *   which are in the object table.
 * - Validate static objects with and without permission granted, before and
 *   after initialization.
 * - Allocate objects at run time, granting an extra reference so they survive
 *   revocation, and validate them through the same states.
 * - Free each dynamic object and validate it once more.
 *
 * Expected result:
 * - Addresses outside the table report -EBADF, objects without permission or
 *   not yet initialized report -EINVAL, usable objects validate successfully,
 *   and a freed object goes back to reporting -EBADF.
 *
 * @see k_object_alloc()
 * @see k_object_access_grant()
 * @see k_object_free()
 */
ZTEST(object_validation, test_kobj_validate_states)
{
	struct k_sem stack_sem = {};

	/* None of these should be even in the table */
	zassert_false(test_object(&stack_sem, -EBADF));
	zassert_false(test_object((struct k_sem *)&bad_sem, -EBADF));
	zassert_false(test_object((struct k_sem *)0xFFFFFFFF, -EBADF));
	object_permission_checks(&sem3, false);
	object_permission_checks(&sem1, true);
	object_permission_checks(&sem2, false);

	for (int i = 0; i < SEM_ARRAY_SIZE; i++) {
		object_permission_checks(&semarray[i], false);
		dyn_sem[i] = k_object_alloc(K_OBJ_SEM);
		zassert_not_null(dyn_sem[i], "couldn't allocate semaphore");
		/* Give an extra reference to another thread so the object
		 * doesn't disappear if we revoke our own
		 */
		k_object_access_grant(dyn_sem[i], &z_main_thread);
	}

	/* dynamic object table well-populated with semaphores at this point */
	for (int i = 0; i < SEM_ARRAY_SIZE; i++) {
		/* Should have permission granted but be uninitialized */
		zassert_false(test_object(dyn_sem[i], -EINVAL));
		k_object_access_revoke(dyn_sem[i], k_current_get());
		object_permission_checks(dyn_sem[i], false);
		k_object_free(dyn_sem[i]);
		zassert_false(test_object(dyn_sem[i], -EBADF));
	}
}

/**
 * @brief Verify that allocating an object grants the caller permission on it.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * A thread that allocates a kernel object must be able to use it straight
 * away, without a separate grant: the allocation implicitly gives the
 * requesting thread permission. The object also has to come out of that
 * thread's own resource pool, so its address is checked against the pool
 * bounds, and it is then used through an ordinary API call, which would fail
 * the permission check if the implicit grant were missing.
 *
 * Test steps:
 * - Allocate a semaphore object with k_object_alloc().
 * - Check its address lies within the calling thread's resource pool.
 * - Call k_sem_init() on it from the same thread.
 *
 * Expected result:
 * - The allocation succeeds, the object lies in the thread's pool, and the
 *   thread can initialize it without being granted access explicitly.
 *
 * @see k_object_alloc()
 * @see k_sem_init()
 */
ZTEST(object_validation, test_kobj_assign_perms_on_alloc_obj)
{
	static struct k_sem *test_dyn_sem;
	struct k_thread *thread = _current;

	uintptr_t start_addr, end_addr;
	size_t size_heap = K_HEAP_MEM_POOL_SIZE;

	/* dynamically allocate kernel object semaphore */
	test_dyn_sem = k_object_alloc(K_OBJ_SEM);
	zassert_not_null(test_dyn_sem, "Cannot allocate sem k_object");

	start_addr = *((uintptr_t *)(void *)thread->resource_pool);
	end_addr = start_addr + size_heap;

	/* check semaphore initialized within thread's mem pool address space */
	zassert_true(((uintptr_t)test_dyn_sem > start_addr) &&
				 ((uintptr_t)test_dyn_sem < end_addr),
				 "semaphore object not in bound of thread's memory pool");

	/* try to init that object, thread should have permissions implicitly */
	k_sem_init(test_dyn_sem, 1, 1);
}

/**
 * @brief Verify that a kernel object is freed once nothing references it.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * For a dynamically allocated object the set of threads holding permission on
 * it doubles as its reference count, so revoking the last permission has to
 * release the memory rather than leak it. The object is looked up again
 * afterwards, and its disappearance from the object table is what shows the
 * release happened.
 *
 * Test steps:
 * - Allocate a mutex object with k_object_alloc().
 * - Revoke the allocating thread's access, leaving no thread with permission.
 * - Look the object up and validate it.
 *
 * Expected result:
 * - The object is no longer in the table, reported as -EBADF, so its memory
 *   was released.
 *
 * @see k_object_alloc()
 * @see k_object_access_revoke()
 */
ZTEST(object_validation, test_kobj_freed_when_unreferenced)
{
	int ret;

	/* dynamically allocate kernel object mutex */
	test_dyn_mutex = k_object_alloc(K_OBJ_MUTEX);
	zassert_not_null(test_dyn_mutex,
					 "Can not allocate dynamic kernel object");

	struct k_thread *thread = _current;

	/* revoke access from the current thread */
	k_object_access_revoke(test_dyn_mutex, thread);

	/* check object was released, when no threads have access to it */
	ret = k_object_validate(k_object_find(test_dyn_mutex), K_OBJ_MUTEX, 0);
	zassert_true(ret == -EBADF, "Dynamic kernel object not released");
}

void *object_validation_setup(void)
{
	k_thread_system_pool_assign(k_current_get());

	return NULL;
}

ZTEST_SUITE(object_validation, NULL, object_validation_setup, NULL, NULL, NULL);
