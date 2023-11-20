/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sw_isr_common.h"

#include <zephyr/ztest.h>

extern const struct _irq_parent_entry _lvl2_irq_list[];

#if DT_HAS_COMPAT_STATUS_OKAY(sifive_plic_1_0_0)
#define INTC_SUPPORTS_MULTI_INSTANCE 1
#endif

/**
 * @brief Test sw_isr_table index-related function(s)
 *
 * @details Validates that:
 * - z_get_sw_isr_table_idx() returns the corresponding SW ISR table index for an IRQN
 */
ZTEST(interrupt_feature, test_sw_isr_irq_parent_table_idx)
{
	unsigned int parent_irq;
	unsigned int parent_isr_offset;
	unsigned int test_irq;
	unsigned int test_isr_offset;

	for (size_t i = 0; i < CONFIG_NUM_2ND_LEVEL_AGGREGATORS; i++) {
		parent_irq = _lvl2_irq_list[i].irq;
		parent_isr_offset = _lvl2_irq_list[i].offset;

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

/**
 * @brief Test sw_isr_table device-related function(s)
 *
 * This test only works on driver that supports multi-instances
 *
 * @details Validates that:
 * - z_get_sw_isr_device_from_irq() returns the parent interrupt controller for an IRQN
 * - z_get_sw_isr_irq_from_device() returns the IRQN of a parent interrupt controller
 */
ZTEST(interrupt_feature, test_sw_isr_irq_parent_table_dev)
{
	const struct device *parent_dev;
	unsigned int parent_irq;
	const struct device *test_dev;
	unsigned int test_irq;

	Z_TEST_SKIP_IFNDEF(INTC_SUPPORTS_MULTI_INSTANCE);

	for (size_t i = 0; i < CONFIG_NUM_2ND_LEVEL_AGGREGATORS; i++) {
		parent_dev = _lvl2_irq_list[i].dev;
		parent_irq = _lvl2_irq_list[i].irq;

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
	}
}
