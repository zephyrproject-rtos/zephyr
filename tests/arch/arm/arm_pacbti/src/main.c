/*
 * Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <cmsis_core.h>

void test_arm_pacbti(void)
{
	printf("%s This should never have been called if BTI was enforced\n", __func__);

	/* If func call was successful then BTI didn't work as expected */
	ztest_test_fail();
}

ZTEST(arm_pacbti, test_arm_bti)
{
	/* Try jumping to middle of a random function and mark that fault as expected because if
	 * BTI is enforced and an indirect jump is made on a non-BTI instruction then a usage fault
	 * is generated.
	 */
	ztest_set_fault_valid(true);

	__asm__ volatile(" ldr r1, =test_arm_pacbti\n"
			 " add r1, #4\n"
			 " bx r1\n");
}

ZTEST_SUITE(arm_pacbti, NULL, NULL, NULL, NULL, NULL);
