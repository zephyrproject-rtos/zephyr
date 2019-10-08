/*
 * Copyright (c) 2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <arch/cpu.h>
#include <arch/arm/cortex_m/cmsis.h>

static volatile int test_flag;
static volatile int expected_reason = -1;

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
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

	expected_reason = -1;
}

void arm_isr_handler(void *args)
{
	ARG_UNUSED(args);

	test_flag++;

	if (test_flag == 1) {
		/* Intentional Kernel oops */
		expected_reason = K_ERR_KERNEL_OOPS;
		k_oops();
	} else if (test_flag == 2) {
		/* Intentional Kernel panic */
		expected_reason = K_ERR_KERNEL_PANIC;
		k_panic();
	} else if (test_flag == 3) {
		/* Intentional ASSERT */
		expected_reason = K_ERR_KERNEL_PANIC;
		__ASSERT(0, "Intentional assert\n");
	}
}

void test_arm_interrupt(void)
{
	/* Determine an NVIC IRQ line that is not currently in use. */
	int i;
	int init_flag, post_flag;

	init_flag = test_flag;

	zassert_false(init_flag, "Test flag not initialized to zero\n");

	for (i = CONFIG_NUM_IRQS - 1; i >= 0; i--) {
		if (NVIC_GetEnableIRQ(i) == 0) {
			/*
			 * Interrupts configured statically with IRQ_CONNECT(.)
			 * are automatically enabled. NVIC_GetEnableIRQ()
			 * returning false, here, implies that the IRQ line is
			 * either not implemented or it is not enabled, thus,
			 * currently not in use by Zephyr.
			 */

			/* Set the NVIC line to pending. */
			NVIC_SetPendingIRQ(i);

			if (NVIC_GetPendingIRQ(i)) {
				/* If the NVIC line is pending, it is
				 * guaranteed that it is implemented.
				 */
				break;
			}
		}
	}

	zassert_true(i >= 0,
		"No available IRQ line to use in the test\n");

	TC_PRINT("Available IRQ line: %u\n", i);

	z_arch_irq_connect_dynamic(i, 0 /* highest priority */,
		arm_isr_handler,
		NULL,
		0);

	NVIC_ClearPendingIRQ(i);
	NVIC_EnableIRQ(i);

	for (int j = 1; j <= 3; j++) {

		/* Set the dynamic IRQ to pending state. */
		NVIC_SetPendingIRQ(i);

		/*
		 * Instruction barriers to make sure the NVIC IRQ is
		 * set to pending state before 'test_flag' is checked.
		 */
		__DSB();
		__ISB();

		/* Returning here implies the thread was not aborted. */

		/* Confirm test flag is set by the ISR handler. */
		post_flag = test_flag;
		zassert_true(post_flag == j, "Test flag not set by ISR\n");
	}
}
/**
 * @}
 */
