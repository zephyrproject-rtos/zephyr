/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "interrupt_util.h"

/*
 * Other arch has already been tested in testcase of gen_isr_table,
 * so we only test x86 and arch posix here.
 * And x86_64 is also not support this kind of interrupt, we skip it.
 */
#if defined(CONFIG_X86)

#define TEST_DIRECT_IRQ_LINE_1	3
#define TEST_DIRECT_IRQ_LINE_2	4

#define TEST_DIRECT_IRQ_PRIO	0

#elif defined(CONFIG_ARCH_POSIX)
#define TEST_DIRECT_IRQ_LINE_1	5
#define TEST_DIRECT_IRQ_LINE_2	6

#define TEST_DIRECT_IRQ_PRIO	5

#endif

volatile uint32_t direct_int_executed[2];

ISR_DIRECT_DECLARE(direct_isr1)
{
	direct_int_executed[0]++;

	return 0;
}

ISR_DIRECT_DECLARE(direct_isr2)
{
	direct_int_executed[1]++;

	return 1;
}

/**
 * @brief Test direct interrupt function
 *
 * @details validate direct interrupt works as expected.
 * - Register two direct interrupt at build time.
 * - Trigger interrupt and check if isr handler has executed or not.
 * - Also check irq_enable and irq_disable works.
 *
 * @ingroup kernel_interrupt_tests
 *
 * @see IRQ_DIRECT_CONNECT(), ISR_DIRECT_DECLARE()
 */
ZTEST(x86_direct_interrupt, test_direct_interrupt)
{
	int trig_vec1, trig_vec2;

	IRQ_DIRECT_CONNECT(TEST_DIRECT_IRQ_LINE_1, TEST_DIRECT_IRQ_PRIO, direct_isr1, 0);
	IRQ_DIRECT_CONNECT(TEST_DIRECT_IRQ_LINE_2, TEST_DIRECT_IRQ_PRIO, direct_isr2, 0);

#if defined(CONFIG_X86)
	trig_vec1 = Z_IRQ_TO_INTERRUPT_VECTOR(TEST_DIRECT_IRQ_LINE_1);
	trig_vec2 = Z_IRQ_TO_INTERRUPT_VECTOR(TEST_DIRECT_IRQ_LINE_2);
#elif defined(CONFIG_ARCH_POSIX)
	trig_vec1 = TEST_DIRECT_IRQ_LINE_1;
	trig_vec2 = TEST_DIRECT_IRQ_LINE_2;
#endif
	TC_PRINT("irq(%d)=vector(%d)\n", TEST_DIRECT_IRQ_LINE_1, trig_vec1);
	TC_PRINT("irq(%d)=vector(%d)\n", TEST_DIRECT_IRQ_LINE_2, trig_vec2);

	irq_enable(TEST_DIRECT_IRQ_LINE_1);
	irq_enable(TEST_DIRECT_IRQ_LINE_2);

	zassert_true(direct_int_executed[0] == 0 &&
			direct_int_executed[1] == 0,
			"Both ISR should not execute");

	trigger_irq(trig_vec1);

	zassert_true(direct_int_executed[0] == 1 &&
			direct_int_executed[1] == 0,
			"ISR1 should execute");

	trigger_irq(trig_vec2);

	zassert_true(direct_int_executed[0] == 1 &&
			direct_int_executed[1] == 1,
			"Both ISR should execute");

	unsigned int key = irq_lock();

	/* trigger under irq locked */
	trigger_irq(trig_vec1);
	trigger_irq(trig_vec2);

	zassert_true(direct_int_executed[0] == 1 &&
			direct_int_executed[1] == 1,
			"Both ISR should not execute again");

	irq_unlock(key);

	/* interrupt serve after irq unlocked */
	zassert_true(direct_int_executed[0] == 2 &&
			direct_int_executed[1] == 2,
			"Both ISR should execute again(%d)(%d)",
			direct_int_executed[0], direct_int_executed[1]);

	/* trigger after irq unlocked */
	trigger_irq(trig_vec1);
	trigger_irq(trig_vec2);

	zassert_true(direct_int_executed[0] == 3 &&
			direct_int_executed[1] == 3,
			"Both ISR should execute again(%d)(%d)",
			direct_int_executed[0], direct_int_executed[1]);
}

ZTEST_SUITE(x86_direct_interrupt, NULL, NULL, NULL, NULL, NULL);
