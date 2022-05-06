/*
 * Copyright (c) 2020 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>

#if defined(CONFIG_ARM_SECURE_FIRMWARE) && \
	defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)

extern irq_target_state_t irq_target_state_set(unsigned int irq,
	irq_target_state_t target_state);
extern int irq_target_state_is_secure(unsigned int irq);

void test_arm_irq_target_state(void)
{
	/* Determine an NVIC IRQ line that is implemented
	 * but not currently in use.
	 */
	int i;

	for (i = CONFIG_NUM_IRQS - 1; i >= 0; i--) {
		if (NVIC_GetEnableIRQ(i) == 0) {
			/*
			 * In-use interrupts are automatically enabled by
			 * virtue of IRQ_CONNECT(.). NVIC_GetEnableIRQ()
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
				NVIC_ClearPendingIRQ(i);

				if (!NVIC_GetPendingIRQ(i)) {
					/*
					 * If the NVIC line can be successfully
					 * un-pended, it is guaranteed that it
					 * can be used for software interrupt
					 * triggering.
					 */
					break;
				}
			}
		}
	}

	zassert_true(i >= 0,
		"No available IRQ line to configure as zero-latency\n");

	TC_PRINT("Available IRQ line: %u\n", i);

	/* Set the available IRQ line to Secure and check the result. */
	irq_target_state_t result_state =
		irq_target_state_set(i, IRQ_TARGET_STATE_SECURE);

	zassert_equal(result_state, IRQ_TARGET_STATE_SECURE,
		"Target state not set to Secure\n");

	zassert_equal(irq_target_state_is_secure(i), 1,
		"Target state not set to Secure\n");

	/* Set the available IRQ line to Secure and check the result. */
	result_state =
		irq_target_state_set(i, IRQ_TARGET_STATE_NON_SECURE);

	zassert_equal(result_state, IRQ_TARGET_STATE_NON_SECURE,
		"Target state not set to Secure\n");
	zassert_equal(irq_target_state_is_secure(i), 0,
		"Target state not set to Non-Secure\n");
}
#else
void test_arm_irq_target_state(void)
{
	TC_PRINT("Skipped (TrustZone-M-enabled Cortex-M Mainline only)\n");
}
#endif /* CONFIG_ZERO_LATENCY_IRQS */
/**
 * @}
 */
