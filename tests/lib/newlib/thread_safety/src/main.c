/*
 * Copyright (c) 2021 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file Newlib thread safety test
 *
 * This file contains a set of tests to verify that the C standard functions
 * provided by newlib are thread safe (i.e. synchronised) and that the thread-
 * specific contexts are properly handled (i.e. re-entrant).
 */

#include <zephyr.h>
#include <ztest.h>

#include <stdio.h>
#include <stdlib.h>

#define THREAD_COUNT	(64)
#define STACK_SIZE	(512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define TEST_INTERVAL	(30) /* seconds */

static struct k_thread tdata[THREAD_COUNT];
static K_THREAD_STACK_ARRAY_DEFINE(tstack, THREAD_COUNT, STACK_SIZE);

void malloc_thread(void *p1, void *p2, void *p3)
{
	static atomic_t count;
	bool *aborted = p1;
	int *volatile ptr;
	int val;

	while (*aborted == false) {
		/* Compute unique value specific to this iteration. */
		val = atomic_inc(&count);

		/* Allocate memory block and write a unique value to it. */
		ptr = malloc(sizeof(int));
		zassert_not_null(ptr, "Out of memory");
		*ptr = val;

		/* Busy wait to increase the likelihood of preemption. */
		k_busy_wait(10);

		/*
		 * Verify that the unique value previously written to the
		 * memory block is valid.  This value will become corrupted if
		 * the newlib heap is not properly synchronised.
		 */
		zassert_equal(*ptr, val, "Corrupted memory block");

		/* Free memory block. */
		free(ptr);
	}
}

/**
 * @brief Test thread safety of newlib memory management functions
 *
 * This test calls the malloc() and free() functions from multiple threads to
 * verify that no corruption occurs in the newlib memory heap.
 */
void test_malloc_thread_safety(void)
{
	int i;
	k_tid_t tid[THREAD_COUNT];
	bool aborted = false;

	/* Create worker threads. */
	for (i = 0; i < ARRAY_SIZE(tid); i++) {
		tid[i] = k_thread_create(&tdata[i], tstack[i], STACK_SIZE,
					 malloc_thread, &aborted, NULL, NULL,
					 K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	}

	TC_PRINT("Created %d worker threads.\n", THREAD_COUNT);

	/* Wait and see if any failures occur. */
	TC_PRINT("Waiting %d seconds to see if any failures occur ...\n",
		 TEST_INTERVAL);

	k_sleep(K_SECONDS(TEST_INTERVAL));

	/* Abort all worker threads. */
	aborted = true;

	for (i = 0; i < ARRAY_SIZE(tid); i++) {
		k_thread_join(tid[i], K_FOREVER);
	}
}

void test_main(void)
{
	ztest_test_suite(newlib_thread_safety,
			 ztest_unit_test(test_malloc_thread_safety));

	ztest_run_test_suite(newlib_thread_safety);
}
