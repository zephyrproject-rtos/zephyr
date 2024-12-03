/*
 * Copyright (c) 2024 Marvell.
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
	if (!valid_fault) {
		TC_PRINT("Fatal error was unexpected, aborting...\n");
		rv = TC_FAIL;
	}
	TC_END_RESULT_CUSTOM(rv, "test_pmp");
	TC_END_REPORT(rv);
	arch_system_halt(reason);
}

#ifdef CONFIG_PMP_STACK_GUARD
static void check_isr_stack_guard(void)
{
	char *isr_stack = (char *)z_interrupt_stacks;

	valid_fault = true;
	*isr_stack = 42;
}

static void check_main_stack_guard(void)
{
	char *main_stack = (char *)z_main_stack;

	valid_fault = true;
	*main_stack = 42;
}

#else

static void check_isr_stack_guard(void)
{
	ztest_test_skip();
}

static void check_main_stack_guard(void)
{
	ztest_test_skip();
}

#endif /* CONFIG_PMP_STACK_GUARD */

typedef void (*pmp_test_func_t)(void);

static const pmp_test_func_t pmp_test_func[] = {
	check_isr_stack_guard,
	check_main_stack_guard,
};

/**
 * @brief Verify RISC-V specific PMP stack guard regions.
 * @details Manually write to the protected stack region to trigger fatal error.
 */
ZTEST(riscv_pmp_no_mt, test_pmp)
{
#ifndef PMP_TEST_FUNC_IDX
#define PMP_TEST_FUNC_IDX 0
#endif
	pmp_test_func[PMP_TEST_FUNC_IDX]();

	zassert_unreachable("Write to stack guard did not fault");
	TC_END_REPORT(TC_FAIL);
}

ZTEST_SUITE(riscv_pmp_no_mt, NULL, NULL, NULL, NULL, NULL);
