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
/* Priority 0 for Zero Latency Interrupt */
#define TEST_IRQ_PRIO 0

static volatile bool isr_executed;

static void test_isr(const void *arg)
{
	ARG_UNUSED(arg);
	isr_executed = true;
}

static int test_early_interrupt(void)
{
	/* Connect and enable the test IRQ as a Zero Latency Interrupt */
	IRQ_CONNECT(TEST_IRQ, TEST_IRQ_PRIO, test_isr, NULL, IRQ_ZERO_LATENCY);
	irq_enable(TEST_IRQ);

	/* Trigger the interrupt. Because it's a Zero Latency Interrupt,
	 * it will fire immediately even though BASEPRI is set,
	 * without needing to call irq_unlock(0) which is unsafe
	 * and can cause spurious interrupts or crashes during early boot.
	 */
	NVIC_SetPendingIRQ(TEST_IRQ);

	/* Wait a bit to ensure it fired */
	__asm__ volatile("nop" : : : "memory");
	__asm__ volatile("nop" : : : "memory");

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
