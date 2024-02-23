/*
 * Copyright (c) Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <ksched.h>

static volatile int expected_reason = -1;

void z_thread_essential_clear(struct k_thread *thread);

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);

	if (expected_reason == -1) {
		printk("Was not expecting a crash\n");
		printk("PROJECT EXECUTION FAILED\n");
		k_fatal_halt(reason);
	}

	if (reason != expected_reason) {
		printk("Wrong crash type got %d expected %d\n", reason,
		       expected_reason);
		printk("PROJECT EXECUTION FAILED\n");
		k_fatal_halt(reason);
	}

	printk("Fatal error expected as part of test case.\n");

	expected_reason = -1;
}

/**
 * @brief This test case verifies when fatal error
 *        log message can be captured.
 * @details
 * Test Objective:
 * - When the fatal error is triggered, if the debugging message function
 *   is turned on, the system can capture the log information.
 *
 * Prerequisite Conditions:
 * - N/A
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Writing a function deliberately triggers a koops exception.
 * -# When the log module is enabled, it will log some information
 *    in the process of exception.
 * -# The regex in testcase.yaml verify the kernel will dump thread id
 *    information and error type when exception occurs.
 *
 * Expected Test Result:
 * - The expected log message is caught.
 *
 * Pass/Fail Criteria:
 * - Success if the log matching regex in step 3.
 * - Failure if the log is not matching regex in step 3.
 *
 * Assumptions and Constraints:
 * - N/A
 * @ingroup kernel_fatal_tests
 */
void test_message_capture(void)
{
	unsigned int key;

	expected_reason = K_ERR_KERNEL_OOPS;

	key = irq_lock();
	k_oops();
	printk("SHOULD NEVER SEE THIS\n");
	irq_unlock(key);
}

int main(void)
{
	/* main() is an essential thread, and we try to OOPS it.  When
	 * this test was written, that worked (even though it wasn't
	 * supposed to per docs).  Now we trap a different error (a
	 * panic and not an oops).  Set the thread non-essential as a
	 * workaround.
	 */
	z_thread_essential_clear(_current);

	test_message_capture();
	return 0;
}
