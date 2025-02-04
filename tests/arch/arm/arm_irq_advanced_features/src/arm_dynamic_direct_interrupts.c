/*
 * Copyright (c) 2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/arch/cpu.h>
#include <cmsis_core.h>
#include <zephyr/sys/barrier.h>

/* Offset for the Direct interrupt used in this test. */
#define DIRECT_ISR_OFFSET (CONFIG_NUM_IRQS - 1)

static volatile int test_flag;

void arm_direct_isr_handler_0(const void *args)
{
	ARG_UNUSED(args);

	test_flag = 1;
}

void arm_direct_isr_handler_1(const void *args)
{
	ARG_UNUSED(args);

	test_flag = 2;
}

ZTEST(arm_irq_advanced_features, test_arm_dynamic_direct_interrupts)
{
	int post_flag = 0;

	/* Place the dynamic interrupt dispatcher (with no rescheduling)
	 * in the ROM ISR table.
	 */
	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(DIRECT_ISR_OFFSET, 0, 0, no_reschedule);

	/* Ensure the IRQ is disabled before enabling it at run time */
	irq_disable(DIRECT_ISR_OFFSET);

	/* Attach the ISR handler at run time. */
	irq_connect_dynamic(DIRECT_ISR_OFFSET, 0 /* highest priority */, arm_direct_isr_handler_0,
			    NULL, 0);

	/* Enable and pend the interrupt */
	irq_enable(DIRECT_ISR_OFFSET);
	NVIC_SetPendingIRQ(DIRECT_ISR_OFFSET);

	/*
	 * Instruction barriers to make sure the NVIC IRQ is
	 * set to pending state before 'test_flag' is checked.
	 */
	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	/* Confirm test flag is set by the dynamic direct ISR handler. */
	post_flag = test_flag;
	zassert_true(post_flag == 1, "Test flag not set by ISR0\n");

	post_flag = 0;
	irq_disable(DIRECT_ISR_OFFSET);

	/* Attach an alternative ISR handler at run-time. */
	irq_connect_dynamic(DIRECT_ISR_OFFSET, 0 /* highest priority */, arm_direct_isr_handler_1,
			    NULL, 0);

	/* Enable and pend the interrupt */
	irq_enable(DIRECT_ISR_OFFSET);
	NVIC_SetPendingIRQ(DIRECT_ISR_OFFSET);

	/*
	 * Instruction barriers to make sure the NVIC IRQ is
	 * set to pending state before 'test_flag' is checked.
	 */
	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	/* Confirm test flag is set by the dynamic direct ISR handler. */
	post_flag = test_flag;
	zassert_true(post_flag == 2, "Test flag not set by ISR1\n");
}
/**
 * @}
 */
