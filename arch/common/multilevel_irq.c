/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2023 Meta.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

BUILD_ASSERT((CONFIG_NUM_2ND_LEVEL_AGGREGATORS * CONFIG_MAX_IRQ_PER_AGGREGATOR) <=
		     BIT(CONFIG_2ND_LEVEL_INTERRUPT_BITS),
	     "L2 bits not enough to cover the number of L2 IRQs");

/**
 * @brief Get the aggregator that's responsible for the given irq
 *
 * @param irq IRQ number to query
 *
 * @return Aggregator entry, NULL if irq is level 1 or not found.
 */
static const struct _irq_parent_entry *get_intc_entry_for_irq(unsigned int irq)
{
	const unsigned int level = irq_get_level(irq);

	/* 1st level aggregator is not registered */
	if (level == 1) {
		return NULL;
	}

	const unsigned int intc_irq = irq_get_intc_irq(irq);

	/* Find an aggregator entry that matches the level & intc_irq */
	STRUCT_SECTION_FOREACH_ALTERNATE(intc_table, _irq_parent_entry, intc) {
		if ((intc->level == level) && (intc->irq == intc_irq)) {
			return intc;
		}
	}

	return NULL;
}

const struct device *z_get_sw_isr_device_from_irq(unsigned int irq)
{
	const struct _irq_parent_entry *intc = get_intc_entry_for_irq(irq);

	__ASSERT(intc != NULL, "can't find an aggregator to handle irq(%X)", irq);

	return intc != NULL ? intc->dev : NULL;
}

unsigned int z_get_sw_isr_irq_from_device(const struct device *dev)
{
	/* Get the IRQN for the aggregator */
	STRUCT_SECTION_FOREACH_ALTERNATE(intc_table, _irq_parent_entry, intc) {
		if (intc->dev == dev) {
			return intc->irq;
		}
	}

	__ASSERT(false, "dev(%p) not found", dev);

	return 0;
}

unsigned int z_get_sw_isr_table_idx(unsigned int irq)
{
	unsigned int table_idx, local_irq;
	const struct _irq_parent_entry *intc = get_intc_entry_for_irq(irq);
	const unsigned int level = irq_get_level(irq);

	if (intc != NULL) {
		local_irq = irq_from_level(irq, level);
		__ASSERT_NO_MSG(local_irq < CONFIG_MAX_IRQ_PER_AGGREGATOR);

		table_idx = intc->offset + local_irq;
	} else {
		/* irq level must be 1 if no intc entry */
		__ASSERT(level == 1, "can't find an aggregator to handle irq(%X)", irq);
		table_idx = irq;
	}

	table_idx -= CONFIG_GEN_IRQ_START_VECTOR;

	__ASSERT_NO_MSG(table_idx < IRQ_TABLE_SIZE);

	return table_idx;
}
