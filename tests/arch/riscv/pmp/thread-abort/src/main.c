/*
 * Copyright (c) 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

static volatile bool valid_fault;

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
static struct k_thread tdata;
static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);

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

static void thread_call_thread_abort(void *p1, void *p2, void *p3)
{
	valid_fault = true;

	k_tid_t thread = k_current_get();
	k_thread_abort(thread);

	if (valid_fault) {
		/*
		 * valid_fault gets cleared if an expected exception took place
		 */
		TC_PRINT("test function was supposed to fault but didn't\n");
		ztest_test_fail();
	}
}

ZTEST(riscv_thread_abort, test_essential_thread_abort)
{
	/* Testing an essential thread. */
	k_tid_t tid;
	tid = k_thread_create(&tdata, tstack, STACK_SIZE, thread_call_thread_abort,
			      NULL, NULL, NULL, 0, K_ESSENTIAL, K_NO_WAIT);
	k_thread_join(tid, K_FOREVER);

	zassert_unreachable("Write to stack guard did not fault");
	TC_END_REPORT(TC_FAIL);
}

ZTEST_SUITE(riscv_thread_abort, NULL, NULL, NULL, NULL, NULL);
