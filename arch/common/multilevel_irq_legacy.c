/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2024 Meta.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sw_isr_table.h>
#include <zephyr/sys/util.h>

/**
 * @file
 * @brief This file houses the deprecated legacy macros-generated multi-level interrupt lookup
 * table code, compiled when `CONFIG_LEGACY_MULTI_LEVEL_TABLE_GENERATION` is enabled.
 */

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

#define INIT_IRQ_PARENT_OFFSET_2ND(n, d, i, o)                                                     \
	IRQ_PARENT_ENTRY_DEFINE(intc_l2_##n, DEVICE_DT_GET_OR_NULL(d), i, o, 2)

#define IRQ_INDEX_TO_OFFSET(i, base) (base + i * CONFIG_MAX_IRQ_PER_AGGREGATOR)

#define CAT_2ND_LVL_LIST(i, base)                                                                  \
	INIT_IRQ_PARENT_OFFSET_2ND(i, INTC_DT_IRQN_GET(CONFIG_2ND_LVL_INTR_0##i##_OFFSET),         \
				   CONFIG_2ND_LVL_INTR_0##i##_OFFSET,                              \
				   IRQ_INDEX_TO_OFFSET(i, base))

LISTIFY(CONFIG_NUM_2ND_LEVEL_AGGREGATORS, CAT_2ND_LVL_LIST, (;), CONFIG_2ND_LVL_ISR_TBL_OFFSET);

#ifdef CONFIG_3RD_LEVEL_INTERRUPTS

BUILD_ASSERT((CONFIG_NUM_3RD_LEVEL_AGGREGATORS * CONFIG_MAX_IRQ_PER_AGGREGATOR) <=
		     BIT(CONFIG_3RD_LEVEL_INTERRUPT_BITS),
	     "L3 bits not enough to cover the number of L3 IRQs");

#define INIT_IRQ_PARENT_OFFSET_3RD(n, d, i, o)                                                     \
	IRQ_PARENT_ENTRY_DEFINE(intc_l3_##n, DEVICE_DT_GET_OR_NULL(d), i, o, 3)

#define CAT_3RD_LVL_LIST(i, base)                                                                  \
	INIT_IRQ_PARENT_OFFSET_3RD(i, INTC_DT_IRQN_GET(CONFIG_3RD_LVL_INTR_0##i##_OFFSET),         \
				   CONFIG_3RD_LVL_INTR_0##i##_OFFSET,                              \
				   IRQ_INDEX_TO_OFFSET(i, base))

LISTIFY(CONFIG_NUM_3RD_LEVEL_AGGREGATORS, CAT_3RD_LVL_LIST, (;), CONFIG_3RD_LVL_ISR_TBL_OFFSET);

#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */
