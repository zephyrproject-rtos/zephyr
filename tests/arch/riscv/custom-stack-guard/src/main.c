/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

static volatile ZTEST_BMEM bool valid_fault;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	int rv = TC_PASS;

	TC_PRINT("Caught system error -- reason %d %d\n", reason, valid_fault);
	if (!valid_fault || reason != K_ERR_STACK_CHK_FAIL) {
		TC_PRINT("Fatal error was unexpected, aborting...\n");
		rv = TC_FAIL;
	}
	TC_END_RESULT_CUSTOM(rv, "test_custom_stack_guard");
	TC_END_REPORT(rv);
	arch_system_halt(reason);
}

static void (*volatile overflow_stack_fn)(int);

/*
 * Recurse with a frame large enough that the stack pointer eventually
 * crosses the custom stack guard bound. The depth parameter and the
 * post-call use of the frame buffer defeat tail-call and frame
 * optimizations. The call goes through a volatile function pointer so
 * that the compiler does not diagnose the (intentional) infinite
 * recursion.
 */
static __noinline void overflow_stack(int depth)
{
	char frame[256];

	frame[0] = (char)depth;
	overflow_stack_fn(depth + 1);
	__asm__ volatile("" : : "r"(frame[0]) : "memory");
}

/**
 * @brief Verify that the custom stack guard catches a stack overflow.
 * @details Overflow the current stack by unbounded recursion and expect
 * the custom stack guard to raise a fatal error. With MULTITHREADING=n
 * this also exercises the no-multithreading boot path, where the guard
 * is enabled for the main stack via a NULL thread argument.
 */
ZTEST(riscv_custom_stack_guard, test_stack_overflow)
{
	valid_fault = true;
	overflow_stack_fn = overflow_stack;
	overflow_stack(0);

	zassert_unreachable("Stack overflow did not fault");
	TC_END_REPORT(TC_FAIL);
}

ZTEST_SUITE(riscv_custom_stack_guard, NULL, NULL, NULL, NULL, NULL);
