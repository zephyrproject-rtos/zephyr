/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

/*
 * An address inside the first page, which is deliberately left
 * inaccessible so that null pointer dereferences fault instead of
 * quietly returning data. Kept within the smallest region any of the
 * null pointer detection implementations covers (4 bytes), and low
 * enough to be valid on both 32 and 64 bit targets.
 *
 * Held in a volatile pointer so the compiler cannot decide it knows what
 * dereferencing this means.
 */
static char *volatile bad_address = (char *)0x4;

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tthread;

static void fault_inside_printk(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);

	/*
	 * The %s conversion is performed with the printk lock held, so the
	 * resulting fault is taken from inside printk's own critical
	 * section. Reporting it must not need that lock again: the lock is
	 * not recursive, so a fault handler printing through the locking
	 * path would block here forever and this test would time out
	 * instead of completing.
	 */
	printk("this should fault while printing: %s\n", bad_address);

	/*
	 * Reaching this point means the platform let the access through
	 * instead of faulting, so there was nothing to report and nothing
	 * to deadlock on. Not a failure, just untestable here.
	 */
	ztest_set_fault_valid(false);
	ztest_test_skip();
}

/**
 * @brief A fault taken inside printk() is still reported
 *
 * @details Dereference an unmapped string from within printk(), so the
 * fault happens while printk holds its spinlock, and check that the
 * kernel manages to report it and carry on rather than deadlocking in
 * the fault handler.
 *
 * @ingroup kernel_fatal_tests
 */
ZTEST(printk_reentrancy, test_fault_inside_printk)
{
	k_tid_t tid = k_thread_create(&tthread, tstack, STACK_SIZE,
				      fault_inside_printk, NULL, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	zassert_equal(k_thread_join(tid, K_SECONDS(5)), 0,
		      "faulting thread never completed, "
		      "the fault was likely not reported");
}

ZTEST_SUITE(printk_reentrancy, NULL, NULL, NULL, NULL, NULL);
