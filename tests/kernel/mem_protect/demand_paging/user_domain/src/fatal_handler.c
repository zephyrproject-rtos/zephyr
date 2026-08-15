/*
 * Copyright (c) 2026 Process Mission
 *
 * Shared fatal error hook for the user/domain demand paging suites.
 *
 * By default a fatal error (e.g. an unexpected user-mode page fault)
 * is reported as a test failure instead of halting, so that the test
 * harness can report the result. Suites that expect a fault set
 * expect_fault first; the hook then lets the kernel abort the faulting
 * thread and records nothing.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

volatile bool expect_fault;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	printk("Caught system error -- reason %d\n", reason);

	if (!expect_fault) {
		ztest_test_fail();
		return;
	}

	expect_fault = false;
	printk("System error was expected\n");
}
