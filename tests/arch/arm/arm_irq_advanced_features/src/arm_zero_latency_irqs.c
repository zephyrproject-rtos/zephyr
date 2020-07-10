/*
 * Copyright (c) 2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

#if defined(CONFIG_ZERO_LATENCY_IRQS)

static volatile int test_flag;

void arm_zero_latency_isr_handler(const void *args)
{
	ARG_UNUSED(args);

	test_flag = 1;
}

void test_arm_zero_latency_irqs(void)
{
	/* Determine an NVIC IRQ line that is not currently in use. */
	int i, key;
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
		"No available IRQ line to configure as zero-latency\n");

	TC_PRINT("Available IRQ line: %u\n", i);

	/* Configure the available IRQ line as zero-latency. */

	arch_irq_connect_dynamic(i, 0 /* Unused */,
		arm_zero_latency_isr_handler,
		NULL,
		IRQ_ZERO_LATENCY);

	NVIC_ClearPendingIRQ(i);
	NVIC_EnableIRQ(i);

	/* Lock interrupts */
	key = irq_lock();

	/* Set the zero-latency IRQ to pending state. */
	NVIC_SetPendingIRQ(i);

	/*
	 * Instruction barriers to make sure the NVIC IRQ is
	 * set to pending state before 'test_flag' is checked.
	 */
	__DSB();
	__ISB();

	/* Confirm test flag is set by the zero-latency ISR handler. */
	post_flag = test_flag;
	zassert_true(post_flag == 1, "Test flag not set by ISR\n");

	irq_unlock(key);
}
#else
void test_arm_zero_latency_irqs(void)
{
	TC_PRINT("Skipped (Cortex-M Mainline only)\n");
}
#endif /* CONFIG_ZERO_LATENCY_IRQS */
/**
 * @}
 */
