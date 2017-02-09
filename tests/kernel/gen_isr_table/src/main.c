/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <irq.h>
#include <tc_util.h>
#include <sw_isr_table.h>

extern uint32_t _irq_vector_table[];

#if defined(_ARCH_IRQ_DIRECT_CONNECT) && defined(CONFIG_GEN_IRQ_VECTOR_TABLE)
#define HAS_DIRECT_IRQS
#endif

#ifdef HAS_DIRECT_IRQS
ISR_DIRECT_DECLARE(isr1)
{
	printk("isr1\n");
	return 0;
}

ISR_DIRECT_DECLARE(isr2)
{
	printk("isr2\n");
	return 1;
}
#endif

void isr3(void *param)
{
	printk("isr3 %p\n", param);
}


void isr4(void *param)
{
	printk("isr4 %p\n", param);
}

#define ISR1_OFFSET	0
#define ISR2_OFFSET	1
#define ISR3_OFFSET	2
#define ISR4_OFFSET	3

#define IRQ_LINE(offset)	(CONFIG_NUM_IRQS - ((offset) + 1))
#define TABLE_INDEX(offset)	(IRQ_TABLE_SIZE - ((offset) + 1))

#define ISR3_ARG	0xb01dface
#define ISR4_ARG	0xca55e77e

#ifdef CONFIG_GEN_IRQ_VECTOR_TABLE
static int check_vector(void *isr, int offset)
{
	TC_PRINT("Checking _irq_vector_table entry %d for irq %d\n",
		 TABLE_INDEX(offset), IRQ_LINE(offset));

	if (_irq_vector_table[TABLE_INDEX(offset)] != (uint32_t)isr) {
		TC_PRINT("bad entry %d in vector table\n", TABLE_INDEX(offset));
		return -1;
	}
	return 0;
}
#endif

#ifdef CONFIG_GEN_SW_ISR_TABLE
static int check_sw_isr(void *isr, uint32_t arg, int offset)
{
	struct _isr_table_entry *e = &_sw_isr_table[TABLE_INDEX(offset)];
#ifdef CONFIG_GEN_IRQ_VECTOR_TABLE
	void *v = (void *)_irq_vector_table[TABLE_INDEX(offset)];
#endif

	TC_PRINT("Checking _sw_isr_table entry %d for irq %d\n",
		 TABLE_INDEX(offset), IRQ_LINE(offset));

	if (e->arg != (void *)arg) {
		TC_PRINT("bad argument in SW isr table\n");
		TC_PRINT("expected %p got %p\n", (void *)arg, e->arg);
		return -1;
	}
	if (e->isr != isr) {
		TC_PRINT("Bad ISR in SW isr table\n");
		TC_PRINT("expected %p got %p\n", (void *)isr, e->isr);
		return -1;
	}
#ifdef CONFIG_GEN_IRQ_VECTOR_TABLE
	if (v != _isr_wrapper) {
		TC_PRINT("Vector does not point to _isr_wrapper\n");
		TC_PRINT("expected %p got %p\n", _isr_wrapper, v);
		return -1;
	}
#endif

	return 0;
}
#endif

void main(void)
{
	int rv;

	TC_START("Test gen_isr_tables");

	TC_PRINT("IRQ configuration (total lines %d):\n", CONFIG_NUM_IRQS);

#ifdef HAS_DIRECT_IRQS
	IRQ_DIRECT_CONNECT(IRQ_LINE(ISR1_OFFSET), 0, isr1, 0);
	IRQ_DIRECT_CONNECT(IRQ_LINE(ISR2_OFFSET), 0, isr2, 0);
	TC_PRINT("isr1 isr=%p irq=%d\n", isr1, IRQ_LINE(ISR1_OFFSET));
	TC_PRINT("isr2 isr=%p irq=%d\n", isr2, IRQ_LINE(ISR2_OFFSET));

	if (check_vector(isr1, ISR1_OFFSET)) {
		rv = TC_FAIL;
		goto done;
	}

	if (check_vector(isr2, ISR2_OFFSET)) {
		rv = TC_FAIL;
		goto done;
	}
#endif

#ifdef CONFIG_GEN_SW_ISR_TABLE
	IRQ_CONNECT(IRQ_LINE(ISR3_OFFSET), 1, isr3, ISR3_ARG, 0);
	IRQ_CONNECT(IRQ_LINE(ISR4_OFFSET), 2, isr4, ISR4_ARG, 0);
	TC_PRINT("isr3 isr=%p irq=%d param=%p\n", isr3, IRQ_LINE(ISR3_OFFSET),
		 (void *)ISR3_ARG);
	TC_PRINT("isr4 isr=%p irq=%d param=%p\n", isr4, IRQ_LINE(ISR4_OFFSET),
		 (void *)ISR4_ARG);
	TC_PRINT("_sw_isr_table at location %p\n", _sw_isr_table);

	if (check_sw_isr(isr3, ISR3_ARG, ISR3_OFFSET)) {
		rv = TC_FAIL;
		goto done;
	}

	if (check_sw_isr(isr4, ISR4_ARG, ISR4_OFFSET)) {
		rv = TC_FAIL;
		goto done;
	}
#endif

	rv = TC_PASS;
done:
	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}


