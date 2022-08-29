/*
 * Copyright 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>

/* Offset for the interrupt used in this test. */
#define ISR0_OFFSET     (CONFIG_NUM_IRQS - 7)
#define ISR1_2_OFFSET   (CONFIG_NUM_IRQS - 6)
#define ISR3_4_5_OFFSET (CONFIG_NUM_IRQS - 5)
#define ISR6_OFFSET     (CONFIG_NUM_IRQS - 4)

#define MAX_ISR_TESTS 7

static volatile int test_flag[MAX_ISR_TESTS] = {0};

static void isr0(void *arg)
{
	int value = POINTER_TO_INT(arg);

	test_flag[0] = value;
}

static void share_isr1(void *arg)
{
	int value = POINTER_TO_INT(arg);

	test_flag[1] = value;
}

static void share_isr2(void *arg)
{
	int value = POINTER_TO_INT(arg);

	test_flag[2] = value;
}

static void share_isr3(void *arg)
{
	int value = POINTER_TO_INT(arg);

	test_flag[3] = value;
}

static void share_isr4(void *arg)
{
	int value = POINTER_TO_INT(arg);

	test_flag[4] = value;
}

static void share_isr5(void *arg)
{
	int value = POINTER_TO_INT(arg);

	test_flag[5] = value;
}

static void isr6(void *arg)
{
	int value = POINTER_TO_INT(arg);

	test_flag[6] = value;
}

/**
 * @brief Test shared interrupt
 *
 * @details Validate that shared interrupts work as expected.
 * - Register two regular interrupt at build time.
 * - Two shared interrupts at build time.
 * - Three shared interrupts at build time.
 * - Trigger interrupts and check if isrs handler are executed or not.
 *
 * @ingroup kernel_interrupt_tests
 */
ZTEST(interrupt_feature, test_shared_irq)
{
	/* Ensure the IRQ is disabled before enabling it at run time */
	irq_disable(ISR0_OFFSET);
	irq_disable(ISR1_2_OFFSET);
	irq_disable(ISR3_4_5_OFFSET);
	irq_disable(ISR6_OFFSET);

	/* Place the non-direct shared interrupt */
	IRQ_CONNECT(ISR0_OFFSET,     0, isr0,       (void *)1, 0);
	IRQ_CONNECT(ISR1_2_OFFSET,   0, share_isr1, (void *)2, 0);
	IRQ_CONNECT(ISR1_2_OFFSET,   0, share_isr2, (void *)3, 0);
	IRQ_CONNECT(ISR3_4_5_OFFSET, 0, share_isr3, (void *)4, 0);
	IRQ_CONNECT(ISR3_4_5_OFFSET, 0, share_isr4, (void *)5, 0);
	IRQ_CONNECT(ISR3_4_5_OFFSET, 0, share_isr5, (void *)6, 0);
	IRQ_CONNECT(ISR6_OFFSET,     0, isr6,       (void *)7, 0);

	/* Enable and pend the interrupt */
	irq_enable(ISR0_OFFSET);
	irq_enable(ISR1_2_OFFSET);
	irq_enable(ISR3_4_5_OFFSET);
	irq_enable(ISR6_OFFSET);

	NVIC_SetPendingIRQ(ISR0_OFFSET);
	NVIC_SetPendingIRQ(ISR1_2_OFFSET);
	NVIC_SetPendingIRQ(ISR3_4_5_OFFSET);
	NVIC_SetPendingIRQ(ISR6_OFFSET);

	/*
	 * Instruction barriers to make sure the NVIC IRQ is
	 * set to pending state before 'test_flag' is checked.
	 */
	__DSB();
	__ISB();

	for (int i = 0; i < MAX_ISR_TESTS; i++) {
		zassert_true(test_flag[i] == i + 1, "Test flag not set by ISR%d\n", i);
	}
}
