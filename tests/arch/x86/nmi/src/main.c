/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>

#include <kernel_internal.h>
#if defined(__GNUC__)
#include "test_asm_inline_gcc.h"
#else
#include "test_asm_inline_other.h"
#endif

static volatile int int_handler_executed;

extern uint8_t z_x86_nmi_stack[];
extern uint8_t z_x86_nmi_stack1[];
extern uint8_t z_x86_nmi_stack2[];
extern uint8_t z_x86_nmi_stack3[];

uint8_t *nmi_stacks[] = {
	z_x86_nmi_stack,
#if CONFIG_MP_MAX_NUM_CPUS > 1
	z_x86_nmi_stack1,
#if CONFIG_MP_MAX_NUM_CPUS > 2
	z_x86_nmi_stack2,
#if CONFIG_MP_MAX_NUM_CPUS > 3
	z_x86_nmi_stack3
#endif
#endif
#endif
};

bool z_x86_do_kernel_nmi(const z_arch_esf_t *esf)
{
	uint64_t stack;

	_get_esp(stack);

	TC_PRINT("ESP: 0x%llx CPU %d nmi_stack %p\n", stack,
		 arch_curr_cpu()->id, nmi_stacks[arch_curr_cpu()->id]);

	zassert_true(stack > (uint64_t)nmi_stacks[arch_curr_cpu()->id] &&
		     stack < (uint64_t)nmi_stacks[arch_curr_cpu()->id] +
		     CONFIG_X86_EXCEPTION_STACK_SIZE, "Incorrect stack");

	int_handler_executed++;

	return true;
}

ZTEST(nmi, test_nmi_handler)
{
	TC_PRINT("Testing to see interrupt handler executes properly\n");

	_trigger_isr_handler(IV_NON_MASKABLE_INTERRUPT);

	zassert_not_equal(int_handler_executed, 0,
			  "Interrupt handler did not execute");
	zassert_equal(int_handler_executed, 1,
		      "Interrupt handler executed more than once! (%d)\n",
		      int_handler_executed);
}

ZTEST_SUITE(nmi, NULL, NULL, NULL, NULL, NULL);
