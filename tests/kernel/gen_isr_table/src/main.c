/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <irq.h>
#include <tc_util.h>
#include <sw_isr_table.h>

extern u32_t _irq_vector_table[];

#if defined(_ARCH_IRQ_DIRECT_CONNECT) && defined(CONFIG_GEN_IRQ_VECTOR_TABLE)
#define HAS_DIRECT_IRQS
#endif

#define ISR1_OFFSET	0
#define ISR2_OFFSET	1

#if defined(CONFIG_RISCV32) && !defined(CONFIG_SOC_RISCV32_PULPINO)
#define ISR3_OFFSET	1
#define ISR4_OFFSET	5

#define IRQ_LINE(offset)        offset
#define TABLE_INDEX(offset)     offset
#define TRIG_CHECK_SIZE         6
#else
#define ISR3_OFFSET	2
#define ISR4_OFFSET	3

#define IRQ_LINE(offset)	(CONFIG_NUM_IRQS - ((offset) + 1))
#define TABLE_INDEX(offset)	(IRQ_TABLE_SIZE - ((offset) + 1))
#define TRIG_CHECK_SIZE         4
#endif

#define ISR3_ARG	0xb01dface
#define ISR4_ARG	0xca55e77e
static volatile int trigger_check[TRIG_CHECK_SIZE];

#if defined(CONFIG_ARM)
#include <arch/arm/cortex_m/cmsis.h>

void trigger_irq(int irq)
{
#if defined(CONFIG_SOC_TI_LM3S6965_QEMU)
	/* QEMU does not simulate the STIR register: this is a workaround */
	NVIC_SetPendingIRQ(irq);
#else
	NVIC->STIR = irq;
#endif
}
#elif defined(CONFIG_RISCV32) && !defined(CONFIG_SOC_RISCV32_PULPINO)
void trigger_irq(int irq)
{
	u32_t mip;

	__asm__ volatile ("csrrs %0, mip, %1\n"
			  : "=r" (mip)
			  : "r" (1 << irq));
}
#elif defined(CONFIG_CPU_ARCV2)
void trigger_irq(int irq)
{
	_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_HINT, irq);
}
#else
/* So far, Nios II does not support this */
#define NO_TRIGGER_FROM_SW
#endif

#ifdef HAS_DIRECT_IRQS
ISR_DIRECT_DECLARE(isr1)
{
	printk("isr1 ran\n");
	trigger_check[ISR1_OFFSET]++;
	return 0;
}

ISR_DIRECT_DECLARE(isr2)
{
	printk("isr2 ran\n");
	trigger_check[ISR2_OFFSET]++;
	return 1;
}
#endif

void isr3(void *param)
{
	printk("isr3 ran with parameter %p\n", param);
	trigger_check[ISR3_OFFSET]++;
}


void isr4(void *param)
{
	printk("isr4 ran with parameter %p\n", param);
	trigger_check[ISR4_OFFSET]++;
}

/* Need to turn optimization off. Otherwise compiler may generate incorrect
 * code, not knowing that trigger_irq() affects the value of trigger_check,
 * even if declared volatile.
 *
 * A memory barrier does not help, we need an 'instruction barrier' but GCC
 * doesn't support this; we need to tell the compiler not to reorder memory
 * accesses to trigger_check around calls to trigger_irq.
 */
__attribute__((optimize("-O0")))
int test_irq(int offset)
{
#ifndef NO_TRIGGER_FROM_SW
	TC_PRINT("triggering irq %d\n", IRQ_LINE(offset));
	trigger_irq(IRQ_LINE(offset));
	if (trigger_check[offset] != 1) {
		TC_PRINT("interrupt %d didn't run once, ran %d times\n",
			 IRQ_LINE(offset),
			 trigger_check[offset]);
		return -1;
	}
#else
	/* This arch doesn't support triggering interrupts from software */
	ARG_UNUSED(offset);
#endif
	return 0;
}



#ifdef HAS_DIRECT_IRQS
static int check_vector(void *isr, int offset)
{
	TC_PRINT("Checking _irq_vector_table entry %d for irq %d\n",
		 TABLE_INDEX(offset), IRQ_LINE(offset));

	if (_irq_vector_table[TABLE_INDEX(offset)] != (u32_t)isr) {
		TC_PRINT("bad entry %d in vector table\n", TABLE_INDEX(offset));
		return -1;
	}

	if (test_irq(offset)) {
		return -1;
	}

	return 0;
}
#endif

#ifdef CONFIG_GEN_SW_ISR_TABLE
static int check_sw_isr(void *isr, u32_t arg, int offset)
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

	if (test_irq(offset)) {
		return -1;
	}
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
	irq_enable(IRQ_LINE(ISR1_OFFSET));
	irq_enable(IRQ_LINE(ISR2_OFFSET));
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
	irq_enable(IRQ_LINE(ISR3_OFFSET));
	irq_enable(IRQ_LINE(ISR4_OFFSET));
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
