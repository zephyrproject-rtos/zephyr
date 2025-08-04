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

/* Without PAC this function would have returned to test_arm_pacbti() but with PAC enabled
 * the AUT instruction should result in a USAGE FAULT since the `lr` was corrupted on stack.
 */
__asm__ (".thumb\n"
	 ".thumb_func\n"
	 ".global corrupt_lr_on_stack\n"
	 "corrupt_lr_on_stack:\n"
	 "    pacbti r12, lr, sp\n"
	 "    stmdb sp!, {ip, lr}\n"
	 "    ldr r0,=test_arm_pacbti\n"
	 "    str r0, [sp, #4]\n"
	 "    ldmia.w sp!, {ip, lr}\n"
	 "    aut r12, lr, sp\n"
	 "    bx lr\n");
void corrupt_lr_on_stack();

static int set_invalid_pac_key(void)
{
	struct pac_keys test_pac_keys = {0};

	__get_PAC_KEY_P((uint32_t *)&test_pac_keys);

	/* Change PAC KEY for the current thread */
	test_pac_keys.key_0 += 1;
	test_pac_keys.key_1 += 1;
	test_pac_keys.key_2 += 1;
	test_pac_keys.key_3 += 1;

	__set_PAC_KEY_P((uint32_t *)&test_pac_keys);

	/* The AUT instruction before this test returns should now result in a USAGE FAULT */
	return 1;
}

ZTEST(arm_pacbti, test_arm_pac_corrupt_lr)
{
	ztest_set_fault_valid(true);

	corrupt_lr_on_stack();
}

ZTEST(arm_pacbti, test_arm_pac_invalid_key)
{
	ztest_set_fault_valid(true);

	if (set_invalid_pac_key()) {
		printf("set_invalid_pac_key should never have returned if AUT was enforced\n");
		ztest_test_fail();
	}
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
