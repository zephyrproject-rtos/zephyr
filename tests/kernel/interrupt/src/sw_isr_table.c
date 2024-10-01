/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sw_isr_common.h"

#include <zephyr/ztest.h>

extern const struct _irq_parent_entry _lvl2_irq_list[];

#define PARENT_IRQ_FN(i, _) CONFIG_2ND_LVL_INTR_0##i##_OFFSET
#if DT_HAS_COMPAT_STATUS_OKAY(sifive_plic_1_0_0)
#define PARENT_DEV_FN(i, _) DEVICE_DT_GET(DT_INST(i, sifive_plic_1_0_0))
#define INTC_SUPPORTS_MULTI_INSTANCE 1
#else
#define PARENT_DEV_FN(i, _) (NULL)
#endif

/**
 * @brief Test sw_isr_table index-related function(s)
 *
 * @details Validates that:
 * - z_get_sw_isr_table_idx() returns the corresponding SW ISR table index for an IRQN
 */
ZTEST(interrupt_feature, test_sw_isr_irq_parent_table_idx)
{
	/* ground truths */
	const unsigned int parent_irq[CONFIG_NUM_2ND_LEVEL_AGGREGATORS] = {
		LISTIFY(CONFIG_NUM_2ND_LEVEL_AGGREGATORS, PARENT_IRQ_FN, (,)),
	};
	const unsigned int l2_isr_offset = CONFIG_2ND_LVL_ISR_TBL_OFFSET;

	for (size_t i = 0; i < CONFIG_NUM_2ND_LEVEL_AGGREGATORS; i++) {
		for (unsigned int local_irq = 0;
		     local_irq < CONFIG_MAX_IRQ_PER_AGGREGATOR; local_irq++) {
			unsigned int test_irq = irq_to_level_2(local_irq) | parent_irq[i];
			unsigned int test_isr_offset = z_get_sw_isr_table_idx(test_irq);
			unsigned int isr_offset =
				l2_isr_offset + local_irq + (i * CONFIG_MAX_IRQ_PER_AGGREGATOR);

			zassert_equal(isr_offset, test_isr_offset,
				      "%d: expected offset: %d, got: %d", i,
				      isr_offset, test_isr_offset);
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
	Z_TEST_SKIP_IFNDEF(INTC_SUPPORTS_MULTI_INSTANCE);

	/* ground truths */
	const struct device *parent_dev[CONFIG_NUM_2ND_LEVEL_AGGREGATORS] = {
		LISTIFY(CONFIG_NUM_2ND_LEVEL_AGGREGATORS, PARENT_DEV_FN, (,)),
	};
	const unsigned int parent_irq[CONFIG_NUM_2ND_LEVEL_AGGREGATORS] = {
		LISTIFY(CONFIG_NUM_2ND_LEVEL_AGGREGATORS, PARENT_IRQ_FN, (,)),
	};

	for (size_t i = 0; i < CONFIG_NUM_2ND_LEVEL_AGGREGATORS; i++) {
		const struct device *test_dev;
		unsigned int test_irq;

		zassert_not_null(parent_dev[i]);

		for (unsigned int local_irq = 0;
		     local_irq < CONFIG_MAX_IRQ_PER_AGGREGATOR; local_irq++) {
			test_irq = irq_to_level_2(local_irq) | parent_irq[i];
			test_dev = z_get_sw_isr_device_from_irq(test_irq);
			zassert_equal_ptr(parent_dev[i], test_dev, "%d: expected dev: %p, got: %p",
					  i, parent_dev[i], test_dev);
		}

		test_irq = z_get_sw_isr_irq_from_device(parent_dev[i]);
		zassert_equal(parent_irq[i], test_irq, "%d: expected offset: %d, got: %d", i,
			      parent_irq[i], test_irq);
	}
}
