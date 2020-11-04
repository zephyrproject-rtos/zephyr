/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <tc_util.h>

#include <kernel_internal.h>
#if defined(__GNUC__)
#include "test_asm_inline_gcc.h"
#else
#include "test_asm_inline_other.h"
#endif

static volatile int int_handler_executed;

bool z_x86_do_kernel_nmi(const z_arch_esf_t *esf)
{
	int_handler_executed++;

	return true;
}

void test_nmi_handler(void)
{
	TC_PRINT("Testing to see interrupt handler executes properly\n");

	_trigger_isr_handler(IV_NON_MASKABLE_INTERRUPT);

	zassert_not_equal(int_handler_executed, 0,
			  "Interrupt handler did not execute");
	zassert_equal(int_handler_executed, 1,
		      "Interrupt handler executed more than once! (%d)\n",
		      int_handler_executed);
}

void test_main(void)
{
	ztest_test_suite(nmi, ztest_unit_test(test_nmi_handler));
	ztest_run_test_suite(nmi);
}
