/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sw_isr_table.h>
#include <arch/cpu.h>
#include <sys/__assert.h>
/*
 * Common code for arches that use software ISR tables (CONFIG_GEN_ISR_TABLES)
 */

#ifdef CONFIG_DYNAMIC_INTERRUPTS

#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS

struct irq_parent_offset {
	unsigned int irq;
	unsigned int offset;
};

#define INIT_IRQ_PARENT_OFFSET(i, o) { \
	.irq = i, \
	.offset = o, \
},

#define IRQ_INDEX_TO_OFFSET(i, base) (base + i * CONFIG_MAX_IRQ_PER_AGGREGATOR)

#ifdef CONFIG_2ND_LEVEL_INTERRUPTS

#define CAT_2ND_LVL_LIST(i, base) \
	INIT_IRQ_PARENT_OFFSET(CONFIG_2ND_LVL_INTR_0##i##_OFFSET, \
		IRQ_INDEX_TO_OFFSET(i, base))
static struct irq_parent_offset lvl2_irq_list[CONFIG_NUM_2ND_LEVEL_AGGREGATORS]
	= { UTIL_LISTIFY(CONFIG_NUM_2ND_LEVEL_AGGREGATORS, CAT_2ND_LVL_LIST,
		CONFIG_2ND_LVL_ISR_TBL_OFFSET) };

#endif/* CONFIG_2ND_LEVEL_INTERRUPTS */

#ifdef CONFIG_3RD_LEVEL_INTERRUPTS

#define CAT_3RD_LVL_LIST(i, base) \
	INIT_IRQ_PARENT_OFFSET(CONFIG_3RD_LVL_INTR_0##i##_OFFSET, \
		IRQ_INDEX_TO_OFFSET(i, base))
static struct irq_parent_offset lvl3_irq_list[CONFIG_NUM_3RD_LEVEL_AGGREGATORS]
	 = { UTIL_LISTIFY(CONFIG_NUM_3RD_LEVEL_AGGREGATORS, CAT_3RD_LVL_LIST,
		CONFIG_3RD_LVL_ISR_TBL_OFFSET) };

#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

unsigned int get_parent_offset(unsigned int parent_irq,
					struct irq_parent_offset list[],
					unsigned int length)
{
	unsigned int i;
	unsigned int offset = 0;

	for (i = 0; i < length; ++i) {
		if (list[i].irq == parent_irq) {
			offset = list[i].offset;
			break;
		}
	}

	__ASSERT(i != length, "Invalid argument: %i", parent_irq);

	return offset;
}

#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */

void z_isr_install(unsigned int irq, void (*routine)(void *), void *param)
{
	unsigned int table_idx;

	__ASSERT(!irq_is_enabled(irq), "IRQ %d is enabled", irq);

#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
	unsigned int level;
	unsigned int parent_irq;
	unsigned int parent_offset;

	level = irq_get_level(irq);

	if (level == 2) {
		parent_irq = irq_parent_level_2(irq);
		parent_offset = get_parent_offset(parent_irq,
			lvl2_irq_list,
			CONFIG_NUM_2ND_LEVEL_AGGREGATORS);
		table_idx = parent_offset + irq_from_level_2(irq);
	}
#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
	else if (level == 3) {
		parent_irq = irq_parent_level_3(irq);
		parent_offset = get_parent_offset(parent_irq,
			lvl3_irq_list,
			CONFIG_NUM_3RD_LEVEL_AGGREGATORS);
		table_idx = parent_offset + irq_from_level_3(irq);
	}
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */
	else {
		table_idx = irq;
	}

	table_idx -= CONFIG_GEN_IRQ_START_VECTOR;
#else
	table_idx = irq - CONFIG_GEN_IRQ_START_VECTOR;
#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */

	/* If dynamic IRQs are enabled, then the _sw_isr_table is in RAM and
	 * can be modified
	 */
	_sw_isr_table[table_idx].arg = param;
	_sw_isr_table[table_idx].isr = routine;
}

/* Some architectures don't/can't interpret flags or priority and have
 * no more processing to do than this.  Provide a generic fallback.
 */
int __weak arch_irq_connect_dynamic(unsigned int irq,
				    unsigned int priority,
				    void (*routine)(void *),
				    void *parameter,
				    u32_t flags)
{
	ARG_UNUSED(flags);
	ARG_UNUSED(priority);

	z_isr_install(irq, routine, parameter);
	return irq;
}

#endif /* CONFIG_DYNAMIC_INTERRUPTS */
