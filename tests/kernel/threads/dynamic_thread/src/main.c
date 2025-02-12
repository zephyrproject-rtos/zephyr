/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include <zephyr/debug/stack.h>

#define STACKSIZE (256 + CONFIG_TEST_EXTRA_STACK_SIZE)

static K_THREAD_STACK_DEFINE(dyn_thread_stack, STACKSIZE);
static K_SEM_DEFINE(start_sem, 0, 1);
static K_SEM_DEFINE(end_sem, 0, 1);
static ZTEST_BMEM struct k_thread *dyn_thread;
static struct k_thread *dynamic_threads[CONFIG_MAX_THREAD_BYTES * 8];

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	if (reason != K_ERR_KERNEL_OOPS) {
		printk("wrong error reason\n");
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}
	if (k_current_get() != dyn_thread) {
		printk("wrong thread crashed\n");
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}
}

static void dyn_thread_entry(void *p1, void *p2, void *p3)
{
	k_sem_take(&start_sem, K_FOREVER);

	k_sem_give(&end_sem);
}

static void prep(void)
{
	k_thread_access_grant(k_current_get(), dyn_thread_stack,
			      &start_sem, &end_sem);
}

static void create_dynamic_thread(void)
{
	k_tid_t tid;

	dyn_thread = k_object_alloc(K_OBJ_THREAD);

	zassert_not_null(dyn_thread, "Cannot allocate thread k_object!");

	tid = k_thread_create(dyn_thread, dyn_thread_stack, STACKSIZE,
			      dyn_thread_entry, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(0), K_USER, K_FOREVER);

	k_object_access_grant(&start_sem, tid);
	k_object_access_grant(&end_sem, tid);

	k_thread_start(tid);

	k_sem_give(&start_sem);

	zassert_true(k_sem_take(&end_sem, K_SECONDS(1)) == 0,
		     "k_sem_take(end_sem) failed");

	k_thread_abort(tid);

	k_object_release(dyn_thread);
}

static void permission_test(void)
{
	k_tid_t tid;

	dyn_thread = k_object_alloc(K_OBJ_THREAD);

	zassert_not_null(dyn_thread, "Cannot allocate thread k_object!");

	tid = k_thread_create(dyn_thread, dyn_thread_stack, STACKSIZE,
			      dyn_thread_entry, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(0), K_USER, K_FOREVER);

	k_object_access_grant(&start_sem, tid);

	k_thread_start(tid);

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
ZTEST(thread_dynamic, test_dyn_thread_perms)
{
	if (!(IS_ENABLED(CONFIG_USERSPACE))) {
		ztest_test_skip();
	}

	permission_test();

	TC_PRINT("===== must have access denied on k_sem %p\n", &end_sem);
}

ZTEST(thread_dynamic, test_thread_index_management)
{
	int i, ctr = 0;

	/* Create thread objects until we run out of ids */
	while (true) {
		struct k_thread *t = k_object_alloc(K_OBJ_THREAD);

		if (t == NULL) {
			break;
		}

		dynamic_threads[ctr] = t;
		ctr++;
	}

	zassert_true(ctr != 0, "unable to create any thread objects");

	TC_PRINT("created %d thread objects\n", ctr);

	/* Show that the above NULL return value wasn't because we ran out of
	 * heap space. For that we need to duplicate how objects are allocated
	 * in kernel/userspace.c. We pessimize the alignment to the worst
	 * case to simplify things somewhat.
	 */
	size_t ret = 1024 * 1024;  /* sure-to-fail initial value */
	void *blob;

	switch (K_OBJ_THREAD) {
	/** @cond keep_doxygen_away */
	#include <zephyr/otype-to-size.h>
	/** @endcond */
	}
	blob = k_object_create_dynamic_aligned(16, ret);
	zassert_true(blob != NULL, "out of heap memory");

	/* Free one of the threads... */
	k_object_free(dynamic_threads[0]);

	/* And show that we can now create another one, the freed thread's
	 * index should have been garbage collected.
	 */
	dynamic_threads[0] = k_object_alloc(K_OBJ_THREAD);
	zassert_true(dynamic_threads[0] != NULL,
		     "couldn't create thread object\n");

	/* TODO: Implement a test that shows that thread IDs are properly
	 * recycled when a thread object is garbage collected due to references
	 * dropping to zero. For example, we ought to be able to exit here
	 * without calling k_object_free() on any of the threads we created
	 * here; their references would drop to zero and they would be
	 * automatically freed. However, it is known that the thread IDs are
	 * not properly recycled when this happens, see #17023.
	 */
	for (i = 0; i < ctr; i++) {
		k_object_free(dynamic_threads[i]);
	}
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test creation of dynamic user thread under kernel thread
 *
 * @details This is a simple test to create a user thread
 * dynamically via k_object_alloc() under a kernel thread.
 */
ZTEST(thread_dynamic, test_kernel_create_dyn_user_thread)
{
	if (!(IS_ENABLED(CONFIG_USERSPACE))) {
		ztest_test_skip();
	}

	create_dynamic_thread();
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test creation of dynamic user thread under user thread
 *
 * @details This is a simple test to create a user thread
 * dynamically via k_object_alloc() under a user thread.
 */
ZTEST_USER(thread_dynamic, test_user_create_dyn_user_thread)
{
	create_dynamic_thread();
}

/* test case main entry */
void *thread_test_setup(void)
{
	k_thread_system_pool_assign(k_current_get());

	prep();

	return NULL;
}

ZTEST_SUITE(thread_dynamic, NULL, thread_test_setup, NULL, NULL, NULL);
