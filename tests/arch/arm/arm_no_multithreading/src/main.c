/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <cmsis_core.h>
#include <zephyr/sys/barrier.h>

#if !defined(CONFIG_CPU_CORTEX_M)
  #error test can only run on Cortex-M MCUs
#endif

#if defined(CONFIG_ARMV8_1_M_MAINLINE)
/*
 * For ARMv8.1-M, the FPSCR[18:16] LTPSIZE field may always read 0b010 if MVE
 * is not implemented, so mask it when validating the value of the FPSCR.
 */
#define FPSCR_MASK		(~FPU_FPDSCR_LTPSIZE_Msk)
#else
#define FPSCR_MASK		(0xffffffffU)
#endif

K_THREAD_STACK_DECLARE(z_main_stack, CONFIG_MAIN_STACK_SIZE);

static volatile int test_flag;
static unsigned int expected_reason;

void arm_isr_handler(const void *args)
{
	ARG_UNUSED(args);

	test_flag++;
}

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);

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

void test_main(void)
{
	printk("ARM no-multithreading test\n");

	uint32_t psp = (uint32_t)__get_PSP();
	uint32_t main_stack_base = (uint32_t)K_THREAD_STACK_BUFFER(z_main_stack);
	uint32_t main_stack_top = (uint32_t)(K_THREAD_STACK_BUFFER(z_main_stack) +
		K_THREAD_STACK_SIZEOF(z_main_stack));

	__ASSERT(
		(psp >= main_stack_base) && (psp <= main_stack_top),
			"PSP out of bounds: 0x%x (0x%x - 0x%x)",
			psp, main_stack_base, main_stack_top);

#if defined(CONFIG_FPU)
	__ASSERT((__get_FPSCR() & FPSCR_MASK) == 0,
		"FPSCR not zero (0x%x)", __get_FPSCR());
#endif

#if defined(CONFIG_BUILTIN_STACK_GUARD)
	uint32_t psplim = (uint32_t)__get_PSPLIM();
	__ASSERT(
		(psplim == main_stack_base),
			"PSPLIM not set to main stack base: (0x%x)",
			psplim);
#endif

	int key = arch_irq_lock();
	__ASSERT(arch_irq_unlocked(key),
		"IRQs locked in main()");

	arch_irq_unlock(key);

	/* Verify activating the PendSV IRQ triggers a K_ERR_SPURIOUS_IRQ */
	expected_reason = K_ERR_CPU_EXCEPTION;
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	/* Determine an NVIC IRQ line that is not currently in use. */
	int i, flag = test_flag;

	__ASSERT(flag == 0, "Test flag not initialized to 0\n");

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
				 * guaranteed that it is implemented; clear the
				 * line.
				 */
				NVIC_ClearPendingIRQ(i);

				if (!NVIC_GetPendingIRQ(i)) {
					/*
					 * If the NVIC line can be successfully
					 * un-pended, it is guaranteed that it
					 * can be used for software interrupt
					 * triggering. Trigger it.
					 */
					NVIC_SetPendingIRQ(i);
					break;
				}
			}
		}
	}

	if (i >= 0) {

		printk("Available IRQ line: %u\n", i);

		arch_irq_connect_dynamic(i, 0 /* highest priority */,
			arm_isr_handler,
			NULL,
			0);

		NVIC_EnableIRQ(i);

		barrier_dsync_fence_full();
		barrier_isync_fence_full();

		flag = test_flag;

		__ASSERT(flag > 0, "Test flag not set by IRQ\n");

		printk("ARM no multithreading test successful\n");
	} else {
		__ASSERT(0, "No available IRQ line to use in the test\n");
	}
}
