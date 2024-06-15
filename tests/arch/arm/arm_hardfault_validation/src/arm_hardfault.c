/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>
#include <cmsis_core.h>
#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>

static volatile int expected_reason = -1;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	static bool triggered_synchronous_svc;

	TC_PRINT("Caught system error -- reason %d\n", reason);

	if (expected_reason == -1) {
		printk("Was not expecting a crash\n");
		k_fatal_halt(reason);
	}

	if (reason != expected_reason) {
		printk("Wrong crash type got %d expected %d\n", reason,
			expected_reason);
		k_fatal_halt(reason);
	}

	/* Fault validation succeeded */

	if (triggered_synchronous_svc == false) {
		triggered_synchronous_svc = true;

		/* Trigger a new CPU runtime error from
		 * inside the current runtime error
		 */
		expected_reason = K_ERR_KERNEL_PANIC;
		__ASSERT(0,
		"Assert occurring inside kernel panic");
	}

	expected_reason = -1;
}

ZTEST(arm_hardfault_validation, test_arm_hardfault)
{
	expected_reason = K_ERR_KERNEL_PANIC;

	k_panic();
}
