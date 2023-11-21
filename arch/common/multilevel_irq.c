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

/*
 * Insert code if the node_id is an interrupt controller
 */
#define Z_IF_DT_IS_INTC(node_id, code)                                                             \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, interrupt_controller), (code))

/*
 * Expands to node_id if its IRQN is equal to `_irq`, nothing otherwise
 * This only works for `_irq` between 0 & 4095, see `IS_EQ`
 */
#define Z_IF_DT_INTC_IRQN_EQ(node_id, _irq) IF_ENABLED(IS_EQ(DT_IRQ(node_id, irq), _irq), (node_id))

/*
 * Expands to node_id if it's an interrupt controller & its IRQN is `irq`, or nothing otherwise
 */
#define Z_DT_INTC_GET_IRQN(node_id, _irq)                                                          \
	Z_IF_DT_IS_INTC(node_id, Z_IF_DT_INTC_IRQN_EQ(node_id, _irq))

/**
 * Loop through child of "/soc" and get root interrupt controllers with `_irq` as IRQN,
 * this assumes only one device has the IRQN
 * @param _irq irq number
 * @return node_id(s) that has the `_irq` number, or empty if none of them has the `_irq`
 */
#define INTC_DT_IRQN_GET(_irq)                                                                     \
	DT_FOREACH_CHILD_STATUS_OKAY_VARGS(DT_PATH(soc), Z_DT_INTC_GET_IRQN, _irq)

/* If can't find any matching interrupt controller, fills with `NULL` */
#define INTC_DEVICE_INIT(node_id) .dev = DEVICE_DT_GET_OR_NULL(node_id),

#define INIT_IRQ_PARENT_OFFSET(d, i, o) { \
	INTC_DEVICE_INIT(d) \
	.irq = i, \
	.offset = o, \
}

#define IRQ_INDEX_TO_OFFSET(i, base) (base + i * CONFIG_MAX_IRQ_PER_AGGREGATOR)

#define CAT_2ND_LVL_LIST(i, base) \
	INIT_IRQ_PARENT_OFFSET(INTC_DT_IRQN_GET(CONFIG_2ND_LVL_INTR_0##i##_OFFSET), \
			       CONFIG_2ND_LVL_INTR_0##i##_OFFSET, IRQ_INDEX_TO_OFFSET(i, base))
const struct _irq_parent_entry _lvl2_irq_list[CONFIG_NUM_2ND_LEVEL_AGGREGATORS]
	= { LISTIFY(CONFIG_NUM_2ND_LEVEL_AGGREGATORS, CAT_2ND_LVL_LIST, (,),
		CONFIG_2ND_LVL_ISR_TBL_OFFSET) };

#ifdef CONFIG_3RD_LEVEL_INTERRUPTS

BUILD_ASSERT((CONFIG_NUM_3RD_LEVEL_AGGREGATORS * CONFIG_MAX_IRQ_PER_AGGREGATOR) <=
		     BIT(CONFIG_3RD_LEVEL_INTERRUPT_BITS),
	     "L3 bits not enough to cover the number of L3 IRQs");

#define CAT_3RD_LVL_LIST(i, base) \
	INIT_IRQ_PARENT_OFFSET(INTC_DT_IRQN_GET(CONFIG_3RD_LVL_INTR_0##i##_OFFSET), \
			       CONFIG_3RD_LVL_INTR_0##i##_OFFSET, IRQ_INDEX_TO_OFFSET(i, base))

const struct _irq_parent_entry _lvl3_irq_list[CONFIG_NUM_3RD_LEVEL_AGGREGATORS]
	 = { LISTIFY(CONFIG_NUM_3RD_LEVEL_AGGREGATORS, CAT_3RD_LVL_LIST, (,),
		CONFIG_3RD_LVL_ISR_TBL_OFFSET) };

#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

static const struct _irq_parent_entry *get_parent_entry(unsigned int parent_irq,
				      const struct _irq_parent_entry list[],
				      unsigned int length)
{
	unsigned int i;
	const struct _irq_parent_entry *entry = NULL;

	for (i = 0U; i < length; ++i) {
		if (list[i].irq == parent_irq) {
			entry = &list[i];
			break;
		}
	}

	__ASSERT(i != length, "Invalid argument: %i", parent_irq);

	return entry;
}

const struct device *z_get_sw_isr_device_from_irq(unsigned int irq)
{
	const struct device *dev = NULL;
	unsigned int level, parent_irq;
	const struct _irq_parent_entry *entry = NULL;

	level = irq_get_level(irq);

	if (level == 2U) {
		parent_irq = irq_parent_level_2(irq);
		entry = get_parent_entry(parent_irq,
					 _lvl2_irq_list,
					 CONFIG_NUM_2ND_LEVEL_AGGREGATORS);
	}
#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
	else if (level == 3U) {
		parent_irq = irq_parent_level_3(irq);
		entry = get_parent_entry(parent_irq,
					 _lvl3_irq_list,
					 CONFIG_NUM_3RD_LEVEL_AGGREGATORS);
	}
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */
	dev = entry != NULL ? entry->dev : NULL;

	return dev;
}

unsigned int z_get_sw_isr_irq_from_device(const struct device *dev)
{
	for (size_t i = 0U; i < CONFIG_NUM_2ND_LEVEL_AGGREGATORS; ++i) {
		if (_lvl2_irq_list[i].dev == dev) {
			return _lvl2_irq_list[i].irq;
		}
	}

#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
	for (size_t i = 0U; i < CONFIG_NUM_3RD_LEVEL_AGGREGATORS; ++i) {
		if (_lvl3_irq_list[i].dev == dev) {
			return _lvl3_irq_list[i].irq;
		}
	}
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

	return 0;
}

unsigned int z_get_sw_isr_table_idx(unsigned int irq)
{
	unsigned int table_idx, level, parent_irq, local_irq, parent_offset;
	const struct _irq_parent_entry *entry = NULL;

	level = irq_get_level(irq);

	if (level == 2U) {
		local_irq = irq_from_level_2(irq);
		__ASSERT_NO_MSG(local_irq < CONFIG_MAX_IRQ_PER_AGGREGATOR);
		parent_irq = irq_parent_level_2(irq);
		entry = get_parent_entry(parent_irq,
					 _lvl2_irq_list,
					 CONFIG_NUM_2ND_LEVEL_AGGREGATORS);
		parent_offset = entry != NULL ? entry->offset : 0U;
		table_idx = parent_offset + local_irq;
	}
#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
	else if (level == 3U) {
		local_irq = irq_from_level_3(irq);
		__ASSERT_NO_MSG(local_irq < CONFIG_MAX_IRQ_PER_AGGREGATOR);
		parent_irq = irq_parent_level_3(irq);
		entry = get_parent_entry(parent_irq,
					 _lvl3_irq_list,
					 CONFIG_NUM_3RD_LEVEL_AGGREGATORS);
		parent_offset = entry != NULL ? entry->offset : 0U;
		table_idx = parent_offset + local_irq;
	}
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */
	else {
		table_idx = irq;
	}

	table_idx -= CONFIG_GEN_IRQ_START_VECTOR;

	__ASSERT_NO_MSG(table_idx < IRQ_TABLE_SIZE);

	return table_idx;
}
