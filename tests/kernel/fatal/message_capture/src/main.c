/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/printk.h>
#include <zephyr/tc_capture.h>

#define STACKSIZE     (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define MAIN_PRIORITY 7
#define PRIORITY      5

/*
 * The kernel fatal path prints the offending thread's identity via
 * EXCEPTION_DUMP(), which resolves to LOG_ERR() when CONFIG_LOG is enabled and
 * to printk() otherwise. The tc_capture helper records whichever path is
 * active, so this test asserts on the dump line without any per-test plumbing.
 */
#define EXPECTED_DUMP_STR "Current thread:"

static K_THREAD_STACK_DEFINE(crash_stack, STACKSIZE);
static struct k_thread crash_thread;

static ZTEST_DMEM volatile int expected_reason = -1;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	printk("Caught system error -- reason %d\n", reason);

	if (reason != expected_reason) {
		printk("Wrong crash type got %d expected %d\n", reason,
		       expected_reason);
		k_fatal_halt(reason);
	}

	expected_reason = -1;
}

static void oops_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	expected_reason = K_ERR_KERNEL_OOPS;
	k_oops();

	/* The thread must be terminated by the fatal path before returning. */
	ztest_test_fail();
}

/**
 * @brief Verify the fatal error path emits the offending thread dump.
 *
 * @details
 * When a fatal error is raised, the kernel prints diagnostic information about
 * the offending thread (via EXCEPTION_DUMP()) before invoking the application
 * fatal handler. Depending on configuration that dump is emitted through the
 * logging subsystem or directly via printk(). Using the tc_capture helper the
 * test records whichever output path is active, raises a fatal error with
 * k_oops() from a cooperative thread, and asserts both that the "Current
 * thread:" dump line was emitted and that the reason code reached
 * k_sys_fatal_error_handler().
 *
 * Test steps:
 * - Start test output capture.
 * - Spawn a cooperative thread that calls k_oops() (K_ERR_KERNEL_OOPS).
 * - Let the kernel dump the diagnostics and invoke the fatal handler, which
 *   returns so the offending thread is terminated.
 * - Stop capture and inspect the recorded output.
 *
 * Expected result:
 * - The captured output contains the "Current thread:" dump line.
 * - The fatal handler observes K_ERR_KERNEL_OOPS (expected_reason cleared).
 *
 * @see k_oops()
 * @see k_sys_fatal_error_handler()
 * @ingroup kernel_fatal_tests
 */
ZTEST(fatal_message_capture, test_fatal_message_capture)
{
	tc_capture_start();

	k_thread_create(&crash_thread, crash_stack,
			K_THREAD_STACK_SIZEOF(crash_stack), oops_entry,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0, K_NO_WAIT);
	k_thread_abort(&crash_thread);

	tc_capture_stop();

	zassert_true(tc_capture_contains(EXPECTED_DUMP_STR),
		     "fatal path did not emit \"%s\"", EXPECTED_DUMP_STR);
	zassert_equal(expected_reason, -1,
		      "fatal handler did not observe K_ERR_KERNEL_OOPS");
}

/*
 * Lower the ztest thread to a preemptible priority so the cooperative crash
 * thread runs and faults deterministically, mirroring the fatal/exception
 * suite. Done per test because each case runs on its own ztest thread.
 */
static void mc_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(MAIN_PRIORITY));
}

ZTEST_SUITE(fatal_message_capture, NULL, NULL, mc_before, NULL, NULL);
