/*
 * Copyright (c) 2026 harshit kudhial
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/irq.h>

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) || defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
#include <cmsis_core.h>
#endif

#define TEST_IRQ (CONFIG_NUM_IRQS - 1)
#define TEST_IRQ_PRIO 1

static volatile bool isr_executed;

static void test_isr(const void *arg)
{
	ARG_UNUSED(arg);
	isr_executed = true;
}

static int test_early_interrupt(void)
{
	/* Connect and enable the test IRQ */
	IRQ_CONNECT(TEST_IRQ, TEST_IRQ_PRIO, test_isr, NULL, 0);
	irq_enable(TEST_IRQ);

	/* Temporarily unlock interrupts to allow our test IRQ to fire.
	 * Normally, interrupts are locked during PRE_KERNEL_1/2.
	 * This simulates the condition in issue #88929 where an interrupt
	 * incorrectly fires during early boot.
	 */
	unsigned int key = irq_lock();

	irq_unlock(0);

	/* Trigger the interrupt */
	NVIC_SetPendingIRQ(TEST_IRQ);

	/* Wait a bit to ensure it fired */
	__asm__ volatile("nop" : : : "memory");
	__asm__ volatile("nop" : : : "memory");

	/* Restore interrupts to previous locked state */
	irq_unlock(key);

	return 0;
}

/* Run this at PRE_KERNEL_1, which is when arch_kernel_init()
 * used to prematurely set MSP to z_interrupt_stacks.
 */
SYS_INIT(test_early_interrupt, PRE_KERNEL_1, 0);

ZTEST(msp_psp_overlap, test_interrupt_fired)
{
	zassert_true(isr_executed, "Interrupt did not fire during early boot");
}

ZTEST_SUITE(msp_psp_overlap, NULL, NULL, NULL, NULL, NULL);
