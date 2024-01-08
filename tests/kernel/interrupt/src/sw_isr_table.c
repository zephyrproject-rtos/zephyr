/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sw_isr_common.h"

#include <zephyr/ztest.h>

extern const struct _irq_parent_entry _lvl2_irq_list[];

/**
 * @brief Test sw_isr_table function
 *
 * @details Validates that:
 * - z_get_sw_isr_device_from_irq() returns the parent interrupt controller for an IRQN
 * - z_get_sw_isr_irq_from_device() returns the IRQN of a parent interrupt controller
 * - z_get_sw_isr_table_idx() returns the corresponding SW ISR table index for an IRQN
 */
ZTEST(interrupt_feature, test_sw_isr_irq_parent_table)
{
	const struct device *parent_dev;
	unsigned int parent_irq;
	unsigned int parent_isr_offset;
	const struct device *test_dev;
	unsigned int test_irq;
	unsigned int test_isr_offset;

	for (size_t i = 0; i < CONFIG_NUM_2ND_LEVEL_AGGREGATORS; i++) {
		parent_dev = _lvl2_irq_list[i].dev;
		parent_irq = _lvl2_irq_list[i].irq;
		parent_isr_offset = _lvl2_irq_list[i].offset;

		for (unsigned int local_irq = 0;
		     local_irq < BIT_MASK(CONFIG_2ND_LEVEL_INTERRUPT_BITS); local_irq++) {
			test_irq = irq_to_level_2(local_irq) | parent_irq;
			test_dev = z_get_sw_isr_device_from_irq(test_irq);
			zassert_equal_ptr(parent_dev, test_dev, "expected dev: %p, got: %p",
					  parent_dev, test_dev);
		}

		test_irq = z_get_sw_isr_irq_from_device(parent_dev);
		zassert_equal(parent_irq, test_irq, "expected offset: %d, got: %d", parent_irq,
			      test_irq);

		for (unsigned int local_irq = 0;
		     local_irq < BIT_MASK(CONFIG_2ND_LEVEL_INTERRUPT_BITS); local_irq++) {
			test_irq = irq_to_level_2(local_irq) | parent_irq;
			test_isr_offset = z_get_sw_isr_table_idx(test_irq);
			zassert_equal(parent_isr_offset + local_irq, test_isr_offset,
				      "expected offset: %d, got: %d", parent_isr_offset + local_irq,
				      test_isr_offset);
		}
	}
}
