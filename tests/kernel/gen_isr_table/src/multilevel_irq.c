/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/ztest.h>

static void test_multi_level_bit_masks_fn(uint32_t irq1, uint32_t irq2, uint32_t irq3)
{
	const uint32_t l2_shift = CONFIG_1ST_LEVEL_INTERRUPT_BITS;
	const uint32_t l3_shift = CONFIG_1ST_LEVEL_INTERRUPT_BITS + CONFIG_2ND_LEVEL_INTERRUPT_BITS;
	const uint32_t hwirq1 = irq1;
	const uint32_t hwirq2 = irq2 - 1;
	const uint32_t hwirq3 = irq3 - 1;
	const bool has_l3 = irq3 > 0;
	const bool has_l2 = irq2 > 0;
	const uint32_t level = has_l3 ? 3 : has_l2 ? 2 : 1;
	const uint32_t irqn_l1 = irq1;
	const uint32_t irqn_l2 = (irq2 << l2_shift) | irqn_l1;
	const uint32_t irqn = (irq3 << l3_shift) | irqn_l2;

	zassert_equal(level, irq_get_level(irqn));

	if (has_l2) {
		zassert_equal(hwirq2, irq_from_level_2(irqn));
		zassert_equal(hwirq2, irq_from_level(irqn, 2));
		zassert_equal((hwirq2 + 1) << l2_shift, irq_to_level_2(hwirq2));
		zassert_equal((hwirq2 + 1) << l2_shift, irq_to_level(hwirq2, 2));
		zassert_equal(hwirq1, irq_parent_level_2(irqn));
		zassert_equal(hwirq1, irq_parent_level(irqn, 2));
	}

	if (has_l3) {
		zassert_equal(hwirq3, irq_from_level_3(irqn));
		zassert_equal(hwirq3, irq_from_level(irqn, 3));
		zassert_equal((hwirq3 + 1) << l3_shift, irq_to_level_3(hwirq3));
		zassert_equal((hwirq3 + 1) << l3_shift, irq_to_level(hwirq3, 3));
		zassert_equal(hwirq2, irq_parent_level_3(irqn));
		zassert_equal(hwirq2, irq_parent_level(irqn, 3));
	}

	if (has_l3) {
		zassert_equal(irqn_l2, irq_get_intc_irq(irqn));
	} else if (has_l2) {
		zassert_equal(irqn_l1, irq_get_intc_irq(irqn));
	} else {
		/* degenerate cases */
		if (false) {
			zassert_equal(irqn, irq_get_intc_irq(irqn));
		}
	}
}

ZTEST(gen_isr_table_multilevel, test_multi_level_bit_masks_l1)
{
	uint32_t irq1;

	/* First IRQ of level 1 */
	irq1 = 0;
	test_multi_level_bit_masks_fn(irq1, 0, 0);

	/* Somewhere in-between */
	irq1 = BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS) >> 1;
	test_multi_level_bit_masks_fn(irq1, 0, 0);

	/* Last IRQ of level 1 */
	irq1 = BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS);
	test_multi_level_bit_masks_fn(irq1, 0, 0);
}

ZTEST(gen_isr_table_multilevel, test_multi_level_bit_masks_l2)
{
	if (!IS_ENABLED(CONFIG_2ND_LEVEL_INTERRUPTS)) {
		ztest_test_skip();
	}

	uint32_t irq1, irq2;

	/* First IRQ of level 2 */
	irq1 = 0;
	/* First irq of level 2 and onwards is 1, as 0 means that the irq is not present */
	irq2 = 1;
	test_multi_level_bit_masks_fn(irq1, irq2, 0);

	/* Somewhere in-between */
	irq1 = BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS) >> 1;
	irq2 = BIT_MASK(CONFIG_2ND_LEVEL_INTERRUPT_BITS) >> 1;
	test_multi_level_bit_masks_fn(irq1, irq2, 0);

	/* Last IRQ of level 2 */
	irq1 = BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS);
	irq2 = BIT_MASK(CONFIG_2ND_LEVEL_INTERRUPT_BITS);
	test_multi_level_bit_masks_fn(irq1, irq2, 0);
}

ZTEST(gen_isr_table_multilevel, test_multi_level_bit_masks_l3)
{
	if (!IS_ENABLED(CONFIG_3RD_LEVEL_INTERRUPTS)) {
		ztest_test_skip();
	}

	uint32_t irq1, irq2, irq3;

	/* First IRQ of level 3 */
	irq1 = 0;
	/* First irq of level 2 and onwards is 1, as 0 means that the irq is not present */
	irq2 = 1;
	irq3 = 1;
	test_multi_level_bit_masks_fn(irq1, irq2, irq3);

	/* Somewhere in-between */
	irq1 = BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS) >> 1;
	irq2 = BIT_MASK(CONFIG_2ND_LEVEL_INTERRUPT_BITS) >> 1;
	irq3 = BIT_MASK(CONFIG_3RD_LEVEL_INTERRUPT_BITS) >> 1;
	test_multi_level_bit_masks_fn(irq1, irq2, irq3);

	/* Last IRQ of level 3 */
	irq1 = BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS);
	irq2 = BIT_MASK(CONFIG_2ND_LEVEL_INTERRUPT_BITS);
	irq3 = BIT_MASK(CONFIG_3RD_LEVEL_INTERRUPT_BITS);
	test_multi_level_bit_masks_fn(irq1, irq2, irq3);
}

ZTEST_SUITE(gen_isr_table_multilevel, NULL, NULL, NULL, NULL, NULL);
