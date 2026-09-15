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
#define FPSCR_MASK (~FPU_FPDSCR_LTPSIZE_Msk)
#else
#define FPSCR_MASK (0xffffffffU)
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
		printk("Wrong crash type got %d expected %d\n", reason, expected_reason);
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

	__ASSERT((psp >= main_stack_base) && (psp <= main_stack_top),
		 "PSP out of bounds: 0x%x (0x%x - 0x%x)", psp, main_stack_base, main_stack_top);

#if defined(CONFIG_FPU)
	__ASSERT((__get_FPSCR() & FPSCR_MASK) == 0, "FPSCR not zero (0x%x)", __get_FPSCR());
#endif

#if defined(CONFIG_BUILTIN_STACK_GUARD)
	uint32_t psplim = (uint32_t)__get_PSPLIM();
	__ASSERT((psplim == main_stack_base), "PSPLIM not set to main stack base: (0x%x)", psplim);
#endif

	__ASSERT(arch_cpu_irqs_are_enabled(), "IRQs locked in main()");

	/* Verify activating the PendSV IRQ triggers a K_ERR_SPURIOUS_IRQ */
	expected_reason = K_ERR_CPU_EXCEPTION;
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	/* Determine an NVIC IRQ line that is not currently in use. */
	int i, flag = test_flag;

	__ASSERT(flag == 0, "Test flag not initialized to 0\n");

	i = get_available_nvic_line(CONFIG_NUM_IRQS);
	printk("Available IRQ line: %u\n", i);

	NVIC_SetPendingIRQ(i);

	arch_irq_connect_dynamic(i, 0 /* highest priority */, arm_isr_handler, NULL, 0);

	NVIC_EnableIRQ(i);

	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	flag = test_flag;

	__ASSERT(flag > 0, "Test flag not set by IRQ\n");

	printk("ARM no multithreading test successful\n");
}
