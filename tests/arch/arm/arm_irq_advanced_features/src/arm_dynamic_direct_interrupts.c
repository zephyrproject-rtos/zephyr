/*
 * Copyright (c) 2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

/* Offset for the Direct interrupt used in this test. */
#ifdef CONFIG_BOARD_ARTY_A7_ARM_DESIGNSTART_M1
/*
 * IRQ 7 is used as level-detect non-interrupt signal on this board and always
 * pending if no V2C-DAPLINK shield is installed. Use IRQ 6 instead.
 */
#define DIRECT_ISR_OFFSET 6
#else
#define DIRECT_ISR_OFFSET (CONFIG_NUM_IRQS - 1)
#endif

static volatile int test_flag;

void arm_direct_isr_handler_0(void *args)
{
	ARG_UNUSED(args);

	test_flag = 1;
}

void arm_direct_isr_handler_1(void *args)
{
	ARG_UNUSED(args);

	test_flag = 2;
}

void test_arm_dynamic_direct_interrupts(void)
{
	int post_flag = 0;

	/* Place the dynamic interrupt dispatcher (with no rescheduling)
	 * in the ROM ISR table.
	 */
	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(DIRECT_ISR_OFFSET, 0, 0, no_reschedule);

	/* Ensure the IRQ is disabled before enabling it at run time */
	irq_disable(DIRECT_ISR_OFFSET);

	/* Attach the ISR handler at run time. */
	irq_connect_dynamic(DIRECT_ISR_OFFSET, 0 /* highest priority */,
		arm_direct_isr_handler_0,
		NULL,
		0);

	/* Enable and pend the interrupt */
	irq_enable(DIRECT_ISR_OFFSET);
	NVIC_SetPendingIRQ(DIRECT_ISR_OFFSET);

	/*
	 * Instruction barriers to make sure the NVIC IRQ is
	 * set to pending state before 'test_flag' is checked.
	 */
	__DSB();
	__ISB();

	/* Confirm test flag is set by the dynamic direct ISR handler. */
	post_flag = test_flag;
	zassert_true(post_flag == 1, "Test flag not set by ISR0\n");

	post_flag = 0;
	irq_disable(DIRECT_ISR_OFFSET);

	/* Attach an alternative ISR handler at run-time. */
	irq_connect_dynamic(DIRECT_ISR_OFFSET, 0 /* highest priority */,
		arm_direct_isr_handler_1,
		NULL,
		0);

	/* Enable and pend the interrupt */
	irq_enable(DIRECT_ISR_OFFSET);
	NVIC_SetPendingIRQ(DIRECT_ISR_OFFSET);

	/*
	 * Instruction barriers to make sure the NVIC IRQ is
	 * set to pending state before 'test_flag' is checked.
	 */
	__DSB();
	__ISB();

	/* Confirm test flag is set by the dynamic direct ISR handler. */
	post_flag = test_flag;
	zassert_true(post_flag == 2, "Test flag not set by ISR1\n");
}
/**
 * @}
 */
