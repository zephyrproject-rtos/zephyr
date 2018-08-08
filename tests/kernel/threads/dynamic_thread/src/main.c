/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>
#include <misc/stack.h>

#define STACKSIZE (256 + CONFIG_TEST_EXTRA_STACKSIZE)

#if defined(CONFIG_USERSPACE) && defined(CONFIG_DYNAMIC_OBJECTS)

static K_THREAD_STACK_DEFINE(dyn_thread_stack, STACKSIZE);
static K_SEM_DEFINE(start_sem, 0, 1);
static K_SEM_DEFINE(end_sem, 0, 1);

static void dyn_thread_entry(void *p1, void *p2, void *p3)
{
	k_sem_take(&start_sem, K_FOREVER);

	k_sem_give(&end_sem);
}

static void prep(void)
{
	k_thread_access_grant(k_current_get(), dyn_thread_stack,
			      &start_sem, &end_sem,
			      NULL);
}

static void create_dynamic_thread(void)
{
	struct k_thread *dyn_thread;
	k_tid_t tid;

	dyn_thread = k_object_alloc(K_OBJ_THREAD);

	zassert_not_null(dyn_thread, "Cannot allocate thread k_object!");

	tid = k_thread_create(dyn_thread, dyn_thread_stack, STACKSIZE,
			      dyn_thread_entry, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(0), K_USER, 0);

	k_object_access_grant(&start_sem, tid);
	k_object_access_grant(&end_sem, tid);

	k_sem_give(&start_sem);

	zassert_true(k_sem_take(&end_sem, K_SECONDS(1)) == 0,
		     "k_sem_take(end_sem) failed");

	k_thread_abort(tid);

	k_object_release(dyn_thread);
}

static void permission_test(void)
{
	struct k_thread *dyn_thread;
	k_tid_t tid;

	dyn_thread = k_object_alloc(K_OBJ_THREAD);

	zassert_not_null(dyn_thread, "Cannot allocate thread k_object!");

	tid = k_thread_create(dyn_thread, dyn_thread_stack, STACKSIZE,
			      dyn_thread_entry, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(0), K_USER, 0);

	k_object_access_grant(&start_sem, tid);

	/*
	 * Notice dyn_thread will not have permission to access
	 * end_sem, which will cause kernel oops.
	 */

	k_sem_give(&start_sem);

	/*
	 * If dyn_thread has permission to access end_sem,
	 * k_sem_take() would be able to take the semaphore.
	 */
	zassert_true(k_sem_take(&end_sem, K_SECONDS(1)) != 0,
		     "Semaphore end_sem has incorrect permission");

	k_thread_abort(tid);

	k_object_release(dyn_thread);
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test object permission on dynamic user thread when index is reused
 *
 * @details This creates one dynamic thread with permissions to both
 * semaphores so there is no fault. Then a new thread is created and will be
 * re-using the thread index in first pass. Except the second thread does
 * not have permission to one of the semaphore. If permissions are cleared
 * correctly when thread is destroyed, the second should raise kernel oops.
 */
static void test_dyn_thread_perms(void)
{
	permission_test();

	TC_PRINT("===== must have access denied on k_sem %p\n", &end_sem);
}

#else /* (CONFIG_USERSPACE && CONFIG_DYNAMIC_OBJECTS) */

static void prep(void)
{
}

static void create_dynamic_thread(void)
{
	TC_PRINT("Test skipped. Userspace and dynamic objects required.\n");
	ztest_test_skip();
}

static void test_dyn_thread_perms(void)
{
	TC_PRINT("Test skipped. Userspace and dynamic objects required.\n");
	ztest_test_skip();
}

#endif /* !(CONFIG_USERSPACE && CONFIG_DYNAMIC_OBJECTS) */

/**
 * @ingroup kernel_thread_tests
 * @brief Test creation of dynamic user thread under kernel thread
 *
 * @details This is a simple test to create a user thread
 * dynamically via k_object_alloc() under a kernel thread.
 */
static void test_kernel_create_dyn_user_thread(void)
{
	create_dynamic_thread();
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test creation of dynamic user thread under user thread
 *
 * @details This is a simple test to create a user thread
 * dynamically via k_object_alloc() under a user thread.
 */
static void test_user_create_dyn_user_thread(void)
{
	create_dynamic_thread();
}

/* test case main entry */
void test_main(void)
{
	k_thread_system_pool_assign(k_current_get());

	prep();

	ztest_test_suite(thread_dynamic,
			 ztest_unit_test(test_kernel_create_dyn_user_thread),
			 ztest_user_unit_test(test_user_create_dyn_user_thread),
			 ztest_unit_test(test_dyn_thread_perms)
			 );
	ztest_run_test_suite(thread_dynamic);
}
