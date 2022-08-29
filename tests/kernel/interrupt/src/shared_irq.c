/*
 * Copyright 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/interrupt_util.h>

#define DURATION        5

#if defined(CONFIG_GIC)
#define ISR0_OFFSET	10
#define ISR1_2_OFFSET	11
#define ISR3_4_5_OFFSET	12
#define ISR6_OFFSET	13
#else
/* Offset for the interrupt used in this test. */
#define ISR0_OFFSET     (CONFIG_NUM_IRQS - 7)
#define ISR1_2_OFFSET   (CONFIG_NUM_IRQS - 6)
#define ISR3_4_5_OFFSET (CONFIG_NUM_IRQS - 5)
#define ISR6_OFFSET     (CONFIG_NUM_IRQS - 4)
#endif

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

	/* Place the non-direct shared interrupts */
	IRQ_CONNECT(ISR0_OFFSET,     0, isr0,       (void *)1, 0);
	IRQ_CONNECT(ISR1_2_OFFSET,   0, share_isr1, (void *)2, 0);
	IRQ_CONNECT(ISR1_2_OFFSET,   0, share_isr2, (void *)3, 0);
	IRQ_CONNECT(ISR3_4_5_OFFSET, 0, share_isr3, (void *)4, 0);
	IRQ_CONNECT(ISR3_4_5_OFFSET, 0, share_isr4, (void *)5, 0);
	IRQ_CONNECT(ISR3_4_5_OFFSET, 0, share_isr5, (void *)6, 0);
	IRQ_CONNECT(ISR6_OFFSET,     0, isr6,       (void *)7, 0);

	/* Enable and pend the interrupts */
	irq_enable(ISR0_OFFSET);
	irq_enable(ISR1_2_OFFSET);
	irq_enable(ISR3_4_5_OFFSET);
	irq_enable(ISR6_OFFSET);

	/* Trigger the interrupts */
	trigger_irq(ISR0_OFFSET);
	trigger_irq(ISR1_2_OFFSET);
	trigger_irq(ISR3_4_5_OFFSET);
	trigger_irq(ISR6_OFFSET);
	k_busy_wait(MS_TO_US(DURATION));

	for (int i = 0; i < MAX_ISR_TESTS; i++) {
		zassert_true(test_flag[i] == i + 1, "Test flag not set by ISR%d\n", i);
	}
}
