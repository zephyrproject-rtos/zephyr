/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <syscall_handler.h>
#include <ztest.h>
#include <kernel_internal.h>

#define SEM_ARRAY_SIZE	16
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)

/* Show that extern declarations don't interfere with detecting kernel
 * objects, this was at one point a problem.
 */
extern struct k_sem sem1;
static struct k_sem semarray[SEM_ARRAY_SIZE];
static struct k_sem *dyn_sem[SEM_ARRAY_SIZE];
static struct k_sem *test_dyn_sem0;
static struct k_mutex *test_dyn_mutex;

K_SEM_DEFINE(sem1, 0, 1);
K_THREAD_STACK_DEFINE(user_stack_thr0, STACK_SIZE);
struct k_thread user_thr0;

static struct k_sem sem2;
static char bad_sem[sizeof(struct k_sem)];
static struct k_sem sem3;

static int test_object(struct k_sem *sem, int retval)
{
	int ret;

	if (retval) {
		/* Expected to fail; bypass z_obj_validation_check() so we don't
		 * fill the logs with spam
		 */
		ret = z_object_validate(z_object_find(sem), K_OBJ_SEM, 0);
	} else {
		ret = z_obj_validation_check(z_object_find(sem), sem,
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
 * @brief Test to verify object permission
 *
 * @details
 * - The kernel must be able to associate kernel object memory addresses
 *   with whether the calling thread has access to that object, the object is
 *   of the expected type, and the object is of the expected init state.
 * - Test support freeing kernel objects allocated at runtime manually.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_object_alloc(), k_object_access_grant()
 */
void test_generic_object(void)
{
	struct k_sem stack_sem;

	/* None of these should be even in the table */
	zassert_false(test_object(&stack_sem, -EBADF), NULL);
	zassert_false(test_object((struct k_sem *)&bad_sem, -EBADF), NULL);
	zassert_false(test_object((struct k_sem *)0xFFFFFFFF, -EBADF), NULL);
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
		zassert_false(test_object(dyn_sem[i], -EINVAL), NULL);
		k_object_access_revoke(dyn_sem[i], k_current_get());
		object_permission_checks(dyn_sem[i], false);
		k_object_free(dyn_sem[i]);
		zassert_false(test_object(dyn_sem[i], -EBADF), NULL);
	}
}

/**
 * @brief Test requestor thread will implicitly be assigned permission on the
 * dynamically allocated object
 *
 * @details
 * - Create kernel object semaphore, dynamically allocate it from the calling
 *   thread's resource pool.
 * - Check that object's address is in bounds of that memory pool.
 * - Then check the requestor thread will implicitly be assigned permission on
 *   the allocated object by using semaphore API k_sem_init()
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_object_alloc()
 */
void test_kobj_assign_perms_on_alloc_obj(void)
{
	test_dyn_sem0 = k_object_alloc(K_OBJ_SEM);
	zassert_not_null(test_dyn_sem0, "Cannot allocate sem k_object");

	struct k_thread *thread = _current;

	intptr_t sem_addr = (intptr_t)&test_dyn_sem0;
	intptr_t start_addr = (intptr_t)&thread->resource_pool;
	size_t size_heap = CONFIG_HEAP_MEM_POOL_SIZE;

	/* check that kernel object may be dynamically allocated,
	 * drawing memory from the calling thread's memory pool
	 */
#if defined(CONFIG_X86) || (CONFIG_ARM)
	zassert_true(
		(sem_addr > start_addr) && (sem_addr <= (start_addr + size_heap)),
		"semaphore not in bound of thread's memory pool");
#elif defined(CONFIG_ARC)
	zassert_true(
		(sem_addr < start_addr) && (sem_addr > (start_addr - size_heap)),
		"semaphore not in bound of thread's memory pool");
#else
#error "Not implemented for this architecture"
	zassert_unreachable("Not implemented for this architecture");
#endif

	k_sem_init(test_dyn_sem0, 1, 1);
	zassert_equal(k_sem_count_get(test_dyn_sem0), 1,
			"sem count not equal to the init count");
}

/* Helper function inits dynamic kernel object and then revokes access */
static void dyn_object_release(void *p1, void *p2, void *p3)
{
	struct z_object *ko;

	test_dyn_mutex = k_object_alloc(K_OBJ_MUTEX);
	zassert_not_null(&test_dyn_mutex, "Can't allocate dynamical kobj");

	struct k_thread *thread = _current;

	ko = z_object_find(test_dyn_mutex);
	zassert_true(ko, "Kernel object test_dyn_mutex not found");

	k_object_access_revoke(test_dyn_mutex, thread);
}

/**
 * @brief Test dynamically allocated kernel object release memory
 *
 * @details Dynamically allocated kernel objects whose access is controlled by
 * the permission system will use object permission as a reference count.
 * If no threads have access to an object, the object's memory released.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_object_alloc()
 */
void test_no_ref_dyn_kobj_release_mem(void)
{
	k_thread_create(&user_thr0, user_stack_thr0,
			 K_THREAD_STACK_SIZEOF(user_stack_thr0),
			 dyn_object_release,
			 NULL, NULL, NULL,
			 K_INHERIT_PERMS, 0, K_NO_WAIT);

	k_thread_join(&user_thr0, K_FOREVER);

	struct z_object *ko = z_object_find(test_dyn_mutex);

	ko = z_object_find(test_dyn_mutex);

	zassert_false(ko, "Kernel object test_dyn_mutex not found");
}

void test_main(void)
{
	k_thread_system_pool_assign(k_current_get());
	ztest_test_suite(object_validation,
			 ztest_unit_test(test_generic_object),
			 ztest_unit_test(test_kobj_assign_perms_on_alloc_obj),
			 ztest_unit_test(test_no_ref_dyn_kobj_release_mem)
			 );
	ztest_run_test_suite(object_validation);
}
