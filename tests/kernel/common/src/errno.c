/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/sys/errno_private.h>


#define N_THREADS 2
#define STACK_SIZE (384 + CONFIG_TEST_EXTRA_STACK_SIZE)

static K_THREAD_STACK_ARRAY_DEFINE(stacks, N_THREADS, STACK_SIZE);
static struct k_thread threads[N_THREADS];

K_THREAD_STACK_DEFINE(eno_stack, STACK_SIZE);
struct k_thread eno_thread;

static int errno_values[N_THREADS + 1] = {
	0xbabef00d,
	0xdeadbeef,
	0xabad1dea,
};

struct result {
	void *q;
	int pass;
};

struct result result[N_THREADS];

struct k_fifo fifo;

static void errno_thread(void *_n, void *_my_errno, void *_unused)
{
	int n = POINTER_TO_INT(_n);
	int my_errno = POINTER_TO_INT(_my_errno);

	errno = my_errno;

	k_msleep(30 - (n * 10));
	if (errno == my_errno) {
		result[n].pass = TC_PASS;
	}

	zassert_equal(errno, my_errno);

	k_fifo_put(&fifo, &result[n]);
}
/**
 * @defgroup kernel_errno_tests Error Number
 * @ingroup all_tests
 * @{
 * @}
 *
 * @addtogroup kernel_errno_tests
 * @{
 */
/**
 * @brief Verify each thread sees a private errno value preserved across context switches.
 *
 * @ingroup kernel_errno_tests
 *
 * @details
 * Confirms that errno is thread-local: concurrent threads assigning distinct errno
 * values, sleeping for different durations to force interleaving, must each still
 * observe their own value when resumed, and the main thread's errno must be
 * unaffected. Passing proves errno is saved and restored per thread on context switch.
 *
 * Test steps:
 * - Set the main thread's errno to a known value and record it.
 * - Spawn N_THREADS threads, each assigning a unique errno then sleeping a staggered
 *   interval before re-reading and reporting its result via a FIFO.
 * - Collect each thread's pass/fail result and re-check the main thread's errno.
 *
 * Expected result:
 * - Every thread observes its own errno value and the main thread's errno is unchanged.
 *
 * @see k_thread_create()
 */
ZTEST(common_errno, test_thread_context)
{
	int rv = TC_PASS, test_errno;

	k_fifo_init(&fifo);

	errno = errno_values[N_THREADS];
	test_errno = errno;

	for (int ii = 0; ii < N_THREADS; ii++) {
		result[ii].pass = TC_FAIL;
	}

	/**TESTPOINT: thread- threads stacks are separate */
	for (int ii = 0; ii < N_THREADS; ii++) {
		k_thread_create(&threads[ii], stacks[ii], STACK_SIZE,
				errno_thread, INT_TO_POINTER(ii),
				INT_TO_POINTER(errno_values[ii]), NULL,
				K_PRIO_PREEMPT(ii + 5), 0, K_NO_WAIT);
	}

	for (int ii = 0; ii < N_THREADS; ii++) {
		struct result *p = k_fifo_get(&fifo, K_MSEC(100));

		if (!p || (p->pass != TC_PASS)) {
			rv = TC_FAIL;
		}
	}

	zassert_equal(errno, test_errno);

	if (errno != errno_values[N_THREADS]) {
		rv = TC_FAIL;
	}

	/* Make sure all the test thread end. */
	for (int ii = 0; ii < N_THREADS; ii++) {
		k_thread_join(&threads[ii], K_FOREVER);
	}

	if (rv != TC_PASS) {
		ztest_test_fail();
	}
}


#define ERROR_ANY 0xfc

void thread_entry_user(void *p1, void *p2, void *p3)
{
#ifdef CONFIG_NATIVE_LIBC
	/* The errno when using the host C library will be handled by it, so we skip it.
	 */
	ztest_test_skip();
#else
	int got_errno;

	/* assign the error number to C standard errno */
	errno = ERROR_ANY;

	/* got errno zephyr stored */
	got_errno = *(z_errno());

	zassert_equal(errno, got_errno, "errno is not corresponding.");
#endif
}

/**
 * @brief Verify the C standard errno and Zephyr's per-thread errno storage stay in sync.
 *
 * @ingroup kernel_errno_tests
 *
 * @details
 * Confirms that writing to the standard errno updates the same location returned by
 * Zephyr's per-thread errno accessor, whether or not thread-local storage is used.
 * Passing proves errno reads and writes resolve to Zephyr's per-thread slot. Skipped
 * when the native host C library owns errno.
 *
 * Test steps:
 * - From a (possibly user-mode) thread, assign a known value to errno.
 * - Read the value back through Zephyr's per-thread errno pointer accessor.
 * - Join the spawned thread.
 *
 * Expected result:
 * - The value read via the per-thread accessor equals the value assigned to errno.
 *
 * @see z_errno()
 */
ZTEST_USER(common_errno, test_errno)
{
	k_tid_t tid;
	uint32_t perm = K_INHERIT_PERMS;

	if (k_is_user_context()) {
		perm = perm | K_USER;
	}

	tid = k_thread_create(&eno_thread, eno_stack, STACK_SIZE,
				thread_entry_user, NULL, NULL, NULL,
				K_PRIO_PREEMPT(1), perm,
				K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

/**
 * @}
 */
extern void *common_setup(void);
ZTEST_SUITE(common_errno, NULL, common_setup, NULL, NULL, NULL);
