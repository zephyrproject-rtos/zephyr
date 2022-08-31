/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/interrupt_util.h>

/*
 * Other arch has already been tested in testcase of gen_isr_table,
 * so we only test x86 series here.
 */

#define TEST_IRQ_LINE_1	27
#define TEST_IRQ_LINE_2	28

#define TEST_IRQ_PRIO	2


volatile uint32_t reg_int_executed[2];

void isr_comm(const void *param)
{
	int choice = POINTER_TO_INT(param);

	switch (choice) {
	case TEST_IRQ_LINE_1:
		reg_int_executed[0]++;
	break;
	case TEST_IRQ_LINE_2:
		reg_int_executed[1]++;
	break;

	default:
	break;
	}
}

/**
 * @brief Test regular interrupt
 *
 * @details Validate regular interrupt works as expected.
 * - Register two regular interrupt at build time.
 * - Trigger interrupt and check if isr handler has executed or not.
 * - Also check irq_enable and irq_disable works.
 *
 * @ingroup kernel_interrupt_tests
 *
 * @see IRQ_CONNECT(), irq_enable(), irq_disable(),
 * irq_unlock(),
 */
ZTEST(interrupt_feature, test_isr_regular)
{
	int trig_vec1, trig_vec2;

	IRQ_CONNECT(TEST_IRQ_LINE_1, TEST_IRQ_PRIO, isr_comm, (void *)TEST_IRQ_LINE_1, 0);
	IRQ_CONNECT(TEST_IRQ_LINE_2, TEST_IRQ_PRIO, isr_comm, (void *)TEST_IRQ_LINE_2, 0);

	trig_vec1 = Z_IRQ_TO_INTERRUPT_VECTOR(TEST_IRQ_LINE_1);
	trig_vec2 = Z_IRQ_TO_INTERRUPT_VECTOR(TEST_IRQ_LINE_2);

	TC_PRINT("irq(%d)=vector(%d)\n", TEST_IRQ_LINE_1, trig_vec1);
	TC_PRINT("irq(%d)=vector(%d)\n", TEST_IRQ_LINE_2, trig_vec2);

	irq_enable(TEST_IRQ_LINE_1);
	irq_enable(TEST_IRQ_LINE_2);

	trigger_irq(trig_vec1);

	zassert_true(reg_int_executed[0] == 1 &&
			reg_int_executed[1] == 0,
			"ISR1 should execute");

	trigger_irq(trig_vec2);

	zassert_true(reg_int_executed[0] == 1 &&
			reg_int_executed[1] == 1,
			"Both ISR should execute");

	unsigned int key = irq_lock();

	/* trigger under irq locked */
	trigger_irq(trig_vec1);
	trigger_irq(trig_vec2);

	zassert_true(reg_int_executed[0] == 1 &&
			reg_int_executed[1] == 1,
			"Both ISR should not execute again(%d)(%d)",
			reg_int_executed[0], reg_int_executed[1]);

	irq_unlock(key);

	/* interrupt serve after irq unlocked */
	zassert_true(reg_int_executed[0] == 2 &&
			reg_int_executed[1] == 2,
			"Both ISR should execute again(%d)(%d)",
			reg_int_executed[0], reg_int_executed[1]);

	/* trigger after irq unlocked */
	trigger_irq(trig_vec1);
	trigger_irq(trig_vec2);

	zassert_true(reg_int_executed[0] == 3 &&
			reg_int_executed[1] == 3,
			"Both ISR should execute again(%d)(%d)",
			reg_int_executed[0], reg_int_executed[1]);
}
