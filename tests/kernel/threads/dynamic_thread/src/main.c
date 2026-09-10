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
 * @brief Verify that object permissions do not survive a reused thread index.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * Thread indexes are recycled, so a newly allocated dynamic thread may occupy
 * the index a destroyed one used. Its permission bits must be cleared when the
 * old thread is destroyed, otherwise the new thread silently inherits access
 * to objects that were granted to its predecessor. The dynamic creation cases
 * of this suite run before this one and leave behind destroyed threads that
 * had access to both semaphores; this case then creates a thread granted
 * access to only one of them and has it touch the other, which must fault
 * rather than succeed.
 *
 * Test steps:
 * - Allocate a dynamic thread object and create a user thread from it,
 *   granting access to the start semaphore only.
 * - Start the thread and release it, so it attempts to give the end semaphore
 *   it was not granted.
 * - Take the end semaphore with a timeout from the test thread.
 *
 * Expected result:
 * - The thread raises a kernel oops on the object it was not granted, and the
 *   end semaphore is never given, so the take times out.
 *
 * @see k_object_alloc()
 * @see k_object_access_grant()
 */
ZTEST(thread_dynamic, test_dyn_thread_perms)
{
	if (!(IS_ENABLED(CONFIG_USERSPACE))) {
		ztest_test_skip();
	}

	permission_test();

	TC_PRINT("===== must have access denied on k_sem %p\n", &end_sem);
}

/**
 * @brief Verify dynamic thread object indexes are exhausted and recycled.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * Allocates kernel thread objects until allocation fails, then proves the
 * failure is caused by thread-index exhaustion rather than lack of heap by
 * allocating an equally-sized plain kernel object. Freeing one thread object
 * must make its index available for a subsequent allocation.
 *
 * Test steps:
 * - Allocate K_OBJ_THREAD objects until k_object_alloc() returns NULL.
 * - Allocate a same-size dynamic kernel object to show heap remains.
 * - Free one thread object and allocate a new one.
 *
 * Expected result:
 * - At least one thread object is created, the heap allocation succeeds,
 *   and the freed thread index is reused for the new allocation.
 *
 * @see k_object_alloc()
 * @see k_object_free()
 */
ZTEST(thread_dynamic, test_dyn_thread_index_recycle)
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
 * @brief Verify that a kernel thread can create a dynamic user thread.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * A thread object obtained from k_object_alloc() must be usable as the target
 * of k_thread_create() exactly like a statically defined one, and the thread
 * created from it must be able to run and use the objects it was granted. The
 * created thread runs to completion through a pair of semaphores, so a
 * dynamic object that is mis-initialized shows up as the handshake failing
 * rather than as a silent no-op. Skipped without userspace, where dynamic
 * kernel objects do not exist.
 *
 * Test steps:
 * - Allocate a thread object with k_object_alloc() from the kernel-mode test
 *   thread.
 * - Create a user thread from it and grant it both semaphores.
 * - Start the thread, give the start semaphore and wait on the end semaphore.
 * - Abort the thread and release the object.
 *
 * Expected result:
 * - The allocation succeeds and the dynamic user thread completes the
 *   handshake within the timeout.
 *
 * @see k_object_alloc()
 * @see k_thread_create()
 */
ZTEST(thread_dynamic, test_dyn_thread_create_from_kernel)
{
	if (!(IS_ENABLED(CONFIG_USERSPACE))) {
		ztest_test_skip();
	}

	create_dynamic_thread();
}

/**
 * @brief Verify that a user thread can create a dynamic user thread.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * The same dynamic creation path has to work when the caller is itself a user
 * thread, where k_object_alloc() and the grants that follow it are reached
 * through system calls and are subject to the caller's own permissions. The
 * check is therefore repeated with the test case running in user mode.
 *
 * Test steps:
 * - Allocate a thread object with k_object_alloc() from a user-mode test
 *   thread.
 * - Create a user thread from it and grant it both semaphores.
 * - Start the thread, give the start semaphore and wait on the end semaphore.
 * - Abort the thread and release the object.
 *
 * Expected result:
 * - The allocation succeeds and the dynamic user thread completes the
 *   handshake within the timeout.
 *
 * @see k_object_alloc()
 * @see k_thread_create()
 */
ZTEST_USER(thread_dynamic, test_dyn_thread_create_from_user)
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
