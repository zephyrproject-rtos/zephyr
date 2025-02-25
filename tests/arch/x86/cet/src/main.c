/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/arch/exception.h>

#define IV_CTRL_PROTECTION_EXCEPTION 21

#define CTRL_PROTECTION_ERRORCODE_ENDBRANCH 3

extern int should_work(int a);
extern int should_not_work(int a);

static bool expect_fault;
static int expect_code;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	if (expect_fault) {
#ifdef CONFIG_X86_64
		zassert_equal(pEsf->vector, IV_CTRL_PROTECTION_EXCEPTION,
					  "unexpected exception");
		zassert_equal(pEsf->code, expect_code, "unexpected error code");
#else
		zassert_equal(z_x86_exception_vector, IV_CTRL_PROTECTION_EXCEPTION,
					  "unexpected exception");
		zassert_equal(pEsf->errorCode, expect_code, "unexpected error code");
#endif
		printk("fatal error expected as part of test case\n");
		expect_fault = false;
	} else {
		printk("fatal error was unexpected, aborting\n");
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}
}

/* Round trip to trick optimisations and ensure the calls are indirect */
int do_call(int (*func)(int), int a)
{
	return func(a);
}

ZTEST(cet, test_ibt)
{
	zassert_equal(do_call(should_work, 1), 2, "should_work failed");

	expect_fault = true;
	expect_code = CTRL_PROTECTION_ERRORCODE_ENDBRANCH;
	do_call(should_not_work, 1);
	zassert_unreachable("should_not_work did not fault");
}

ZTEST_SUITE(cet, NULL, NULL, NULL, NULL, NULL);
