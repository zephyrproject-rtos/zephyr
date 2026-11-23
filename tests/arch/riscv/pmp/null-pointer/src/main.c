/*
 * Copyright (c) 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

static volatile bool valid_fault;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	TC_PRINT("Caught system error -- reason %d %d\n", reason, valid_fault);
	if (!valid_fault) {
		TC_PRINT("fatal error was unexpected, aborting\n");
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}

	TC_PRINT("fatal error expected as part of test case\n");
	valid_fault = false; /* reset back to normal */
	TC_END_REPORT(TC_PASS);
}

static void check_null_ptr_guard(void)
{
	volatile int *null_ptr = NULL;

	valid_fault = true;
	*null_ptr = 42;

	if (valid_fault) {
		/*
		 * valid_fault gets cleared if an expected exception took place
		 */
		TC_PRINT("test function was supposed to fault but didn't\n");
		ztest_test_fail();
	}
}

ZTEST(riscv_pmp_null_pointer, test_null_pointer_access)
{
	check_null_ptr_guard();

	zassert_unreachable("Write to stack guard did not fault");
	TC_END_REPORT(TC_FAIL);
}

ZTEST_SUITE(riscv_pmp_null_pointer, NULL, NULL, NULL, NULL, NULL);
