/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2021 Lexmark International, Inc.
 */

#include <zephyr/zephyr.h>
#include <zephyr/syscall_handler.h>
#include <ztest.h>

ZTEST_BMEM char user_stack[256];

/**
 * @brief Test sys_call does not write to user stack
 *
 * @ingroup kernel_memprotect_tests
 */
void test_user_corrupt_stack_pointer(void)
{
	int ret = 0;
	uint32_t saved_sp;
	size_t i;

	saved_sp = __get_SP();
	__set_SP((uint32_t)&user_stack[0] + (sizeof(user_stack) / 2));

	arch_syscall_invoke0(K_SYSCALL_K_YIELD);

	__set_SP(saved_sp);

	for (i = 0; i < sizeof(user_stack); ++i) {
		if (user_stack[i] != 0U) {
			ret = -1;
			break;
		}
	}

	zassert_equal(ret, 0, "svc exception wrote to user stack");
}

void test_main(void)
{
	ztest_test_suite(arm_mem_protect,
			 ztest_user_unit_test(test_user_corrupt_stack_pointer)
			 );
	ztest_run_test_suite(arm_mem_protect);
}
