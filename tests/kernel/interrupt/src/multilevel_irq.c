/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/ztest.h>

ZTEST(interrupt_feature, test_multi_level_api)
{
	/* Zephyr multilevel-encoded IRQ */
	const uint32_t irqn_l2 = DT_IRQN(DT_NODELABEL(test_l2_irq));
	const uint32_t irqn_l1 = DT_IRQN(DT_NODELABEL(test_l1_irq));
	/* Raw IRQ specified in the devicetree */
	const uint32_t raw_l2 = DT_IRQ(DT_NODELABEL(test_l2_irq), irq);
	const uint32_t raw_l1 = DT_IRQ(DT_NODELABEL(test_l1_irq), irq);

	/**
	 *  - irq_get_level()
	 */
	zassert_equal(2, irq_get_level(irqn_l2));
	zassert_equal(1, irq_get_level(irqn_l1));

	/**
	 *  - irq_from_level_2()
	 *  - irq_to_level_2()
	 *  - irq_parent_level_2()
	 */
	zassert_equal(irq_from_level_2(irqn_l2), raw_l2);

	zassert_equal(irq_to_level_2(raw_l2) & irqn_l2, irq_to_level_2(raw_l2));

	zassert_equal(irq_parent_level_2(irqn_l2), raw_l1);

	/**
	 *  - irq_from_level()
	 *  - irq_to_level()
	 *  - irq_parent_level()
	 */
	zassert_equal(irq_from_level(irqn_l2, 2), raw_l2);

	zassert_equal(irq_to_level(raw_l2, 2) & irqn_l2, irq_to_level(raw_l2, 2));

	zassert_equal(irq_parent_level(irqn_l2, 2), raw_l1);

	/**
	 *  - irq_get_intc_irq()
	 */
	zassert_equal(irq_get_intc_irq(irqn_l2), irqn_l1);
	zassert_equal(irq_get_intc_irq(irqn_l1), irqn_l1);

	const uint32_t irqn_l2_inc = DT_IRQN(DT_NODELABEL(test_l2_irq_inc));
	const uint32_t irqn_l1_inc = DT_IRQN(DT_NODELABEL(test_l1_irq_inc));

	/**
	 * - irq_increment()
	 */
	zassert_equal(irq_increment(irqn_l1, 1), irqn_l1_inc);
	zassert_equal(irq_increment(irqn_l2, 2), irqn_l2_inc);
}

#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
ZTEST(interrupt_feature, test_multi_level_api_l3)
{
	/* Zephyr multilevel-encoded IRQ */
	const uint32_t irqn_l2 = DT_IRQN(DT_NODELABEL(test_l2_irq));
	const uint32_t irqn_l3 = DT_IRQN(DT_NODELABEL(test_l3_irq));
	/* Raw IRQ specified in the devicetree */
	const uint32_t raw_l2 = DT_IRQ(DT_NODELABEL(test_l2_irq), irq);
	const uint32_t raw_l3 = DT_IRQ(DT_NODELABEL(test_l3_irq), irq);

	/**
	 *  - irq_get_level()
	 */
	zassert_equal(3, irq_get_level(irqn_l3));

	/**
	 *  - irq_from_level_2()
	 */
	zassert_equal(irq_from_level_2(irqn_l3), raw_l2);

	/**
	 *  - irq_from_level_3()
	 *  - irq_to_level_3()
	 *  - irq_parent_level_3()
	 */
	zassert_equal(irq_from_level_3(irqn_l3), raw_l3);

	zassert_equal(irq_to_level_3(raw_l3) & irqn_l3, irq_to_level_3(raw_l3));

	zassert_equal(irq_parent_level_3(irqn_l3), raw_l2);

	/**
	 *  - irq_from_level()
	 *  - irq_to_level()
	 *  - irq_parent_level()
	 */
	zassert_equal(irq_from_level(irqn_l3, 2), raw_l2);
	zassert_equal(irq_from_level(irqn_l3, 3), raw_l3);

	zassert_equal(irq_to_level(raw_l3, 3) & irqn_l3, irq_to_level(raw_l3, 3));

	zassert_equal(irq_parent_level(irqn_l3, 3), raw_l2);

	/**
	 *  - irq_get_intc_irq()
	 */
	zassert_equal(irq_get_intc_irq(irqn_l3), irqn_l2);

	const uint32_t irqn_l3_inc = DT_IRQN(DT_NODELABEL(test_l3_irq_inc));

	/**
	 * - irq_increment()
	 */
	zassert_equal(irq_increment(irqn_l3, 3), irqn_l3_inc);
}
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

ZTEST_SUITE(gen_isr_table_multilevel, NULL, NULL, NULL, NULL, NULL);
