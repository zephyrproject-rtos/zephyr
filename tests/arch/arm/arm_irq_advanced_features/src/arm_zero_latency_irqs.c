/*
 * Copyright (c) 2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/arch/cpu.h>
#include <cmsis_core.h>
#include <zephyr/interrupt_util.h>
#include <zephyr/sys/barrier.h>

#if CONFIG_2ND_LVL_ISR_TBL_OFFSET > 0
#define TEST_1ST_LEVEL_INTERRUPTS_MAX CONFIG_2ND_LVL_ISR_TBL_OFFSET
#else
#define TEST_1ST_LEVEL_INTERRUPTS_MAX CONFIG_NUM_IRQS
#endif

static volatile int test_flag;

void arm_zero_latency_isr_handler(const void *args)
{
	ARG_UNUSED(args);

	test_flag = 1;
}

/**
 * @brief Test ARM Zero latency Interrupt functionality.
 * @ingroup kernel_arch_interrupt_tests
 */
ZTEST(arm_irq_advanced_features, test_arm_zero_latency_irqs)
{

	if (!IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS)) {
		TC_PRINT("Skipped (Cortex-M Mainline only)\n");

		return;
	}

	/* Determine an NVIC IRQ line that is not currently in use. */
	int i, key;
	int init_flag, post_flag;

	init_flag = test_flag;

	zassert_false(init_flag, "Test flag not initialized to zero\n");

	i = get_available_nvic_line(TEST_1ST_LEVEL_INTERRUPTS_MAX);
	TC_PRINT("Available IRQ line: %u\n", i);

	/* Configure the available IRQ line as zero-latency. */

	arch_irq_connect_dynamic(i, 0 /* Unused */, arm_zero_latency_isr_handler, NULL,
				 IRQ_ZERO_LATENCY);

	NVIC_EnableIRQ(i);

	/* Lock interrupts */
	key = irq_lock();

	/* Set the zero-latency IRQ to pending state. */
	NVIC_SetPendingIRQ(i);

	/*
	 * Instruction barriers to make sure the NVIC IRQ is
	 * set to pending state before 'test_flag' is checked.
	 */
	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	/* Confirm test flag is set by the zero-latency ISR handler. */
	post_flag = test_flag;
	zassert_true(post_flag == 1, "Test flag not set by ISR\n");

	irq_unlock(key);
}
