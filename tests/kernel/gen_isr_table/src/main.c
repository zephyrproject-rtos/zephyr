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

#if defined(ARCH_IRQ_DIRECT_CONNECT) && defined(CONFIG_GEN_IRQ_VECTOR_TABLE)
#define HAS_DIRECT_IRQS
#endif

#define ISR1_OFFSET	0
#define ISR2_OFFSET	1

#if defined(CONFIG_RISCV)
/* RISC-V has very few IRQ lines which can be triggered from software */
#define ISR3_OFFSET	1
#define ISR5_OFFSET	5

#define IRQ_LINE(offset)        offset
#define TABLE_INDEX(offset)     offset
#define TRIG_CHECK_SIZE		6
#else
#define ISR3_OFFSET	2
#define ISR4_OFFSET	3
#define ISR5_OFFSET	4
#define ISR6_OFFSET	5

#if defined(CONFIG_SOC_ARC_EMSDP)
/* ARC EMSDP' console will use irq 108 / irq 107, will conflict
 * with isr used here, so add a workaround
 */
#define TEST_NUM_IRQS	105
#elif defined(CONFIG_SOC_NRF5340_CPUAPP) || defined(CONFIG_SOC_NRF9160)
/* In nRF9160 and application core in nRF5340, not all interrupts with highest
 * numbers are implemented. Thus, limit the number of interrupts reported to
 * the test, so that it does not try to use some unavailable ones.
 */
#define TEST_NUM_IRQS	33
#elif defined(CONFIG_SOC_STM32G071XX)
/* In STM32G071XX limit the number of interrupts reported to
 * the test, so that it does not try to use some of the IRQs
 * at the end of the vector table that are already used by
 * the board.
 */
#define TEST_NUM_IRQS	30
#else
#define TEST_NUM_IRQS	CONFIG_NUM_IRQS
#endif

#define TEST_IRQ_TABLE_SIZE 	(IRQ_TABLE_SIZE - \
				 (CONFIG_NUM_IRQS - TEST_NUM_IRQS))
#define IRQ_LINE(offset)	(TEST_NUM_IRQS - ((offset) + 1))
#define TABLE_INDEX(offset)	(TEST_IRQ_TABLE_SIZE - ((offset) + 1))
#define TRIG_CHECK_SIZE		6
#endif

#define ISR3_ARG	0xb01dface
#define ISR4_ARG	0xca55e77e
#define ISR5_ARG	0xf0ccac1a
#define ISR6_ARG	0xba5eba11

static volatile int trigger_check[TRIG_CHECK_SIZE];

#if defined(CONFIG_CPU_CORTEX_M)
#include <arch/arm/aarch32/cortex_m/cmsis.h>

void trigger_irq(int irq)
{
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) || \
	defined(CONFIG_SOC_TI_LM3S6965_QEMU)
	/* QEMU does not simulate the STIR register: this is a workaround */
	NVIC_SetPendingIRQ(irq);
#else
	NVIC->STIR = irq;
#endif
}
#elif defined(CONFIG_RISCV)
void trigger_irq(int irq)
{
	uint32_t mip;

	__asm__ volatile ("csrrs %0, mip, %1\n"
			  : "=r" (mip)
			  : "r" (1 << irq));
}
#elif defined(CONFIG_CPU_ARCV2)
void trigger_irq(int irq)
{
	z_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_HINT, irq);
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
	printk("%s ran with parameter %p\n", __func__, param);
	trigger_check[ISR3_OFFSET]++;
}

#ifdef ISR4_OFFSET
void isr4(void *param)
{
	printk("%s ran with parameter %p\n", __func__, param);
	trigger_check[ISR4_OFFSET]++;
}
#endif

void isr5(void *param)
{
	printk("%s ran with parameter %p\n", __func__, param);
	trigger_check[ISR5_OFFSET]++;
}

#ifdef ISR6_OFFSET
void isr6(void *param)
{
	printk("%s ran with parameter %p\n", __func__, param);
	trigger_check[ISR6_OFFSET]++;
}
#endif

#ifndef CONFIG_CPU_CORTEX_M
/* Need to turn optimization off. Otherwise compiler may generate incorrect
 * code, not knowing that trigger_irq() affects the value of trigger_check,
 * even if declared volatile.
 *
 * A memory barrier does not help, we need an 'instruction barrier' but GCC
 * doesn't support this; we need to tell the compiler not to reorder memory
 * accesses to trigger_check around calls to trigger_irq.
 */
__attribute__((optimize("-O0")))
#endif
int test_irq(int offset)
{
#ifndef NO_TRIGGER_FROM_SW
	TC_PRINT("triggering irq %d\n", IRQ_LINE(offset));
	trigger_irq(IRQ_LINE(offset));
#ifdef CONFIG_CPU_CORTEX_M
	__DSB();
	__ISB();
#endif
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

	if (_irq_vector_table[TABLE_INDEX(offset)] != (uint32_t)isr) {
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

	if (test_irq(offset)) {
		return -1;
	}
	return 0;
}
#endif

/**
 * @ingroup kernel_interrupt_tests
 * @brief test to validate gen_isr_table
 *
 * @details initialize two normal and two direct interrupt handler using
 * IRQ_CONNECT and IRQ_DIRECT_CONNECT api respectively.
 * For ‘direct’ interrupts, address of handler function will be placed in
 * the irq vector table. And for 'regular' interrupts , the address of the
 * common software isr table is placed in the irq vector table.
 * Software ISR table is an array of struct _isr_table_entry.
 * And each entry contains the pointer to isr and the corresponding parameters.
 *
 * At the end according to architecture, we manually trigger the interrupt.
 * And all irq handler should get called.
 *
 * @see IRQ_DIRECT_CONNECT(), IRQ_CONNECT(), irq_enable()
 *
 */

void main(void)
{
	int rv = TC_FAIL;

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
		goto done;
	}

	if (check_vector(isr2, ISR2_OFFSET)) {
		goto done;
	}
#endif
#ifdef CONFIG_GEN_SW_ISR_TABLE
	TC_PRINT("_sw_isr_table at location %p\n", _sw_isr_table);

	IRQ_CONNECT(IRQ_LINE(ISR3_OFFSET), 1, isr3, ISR3_ARG, 0);
	irq_enable(IRQ_LINE(ISR3_OFFSET));
	TC_PRINT("isr3 isr=%p irq=%d param=%p\n", isr3, IRQ_LINE(ISR3_OFFSET),
		 (void *)ISR3_ARG);
	if (check_sw_isr(isr3, ISR3_ARG, ISR3_OFFSET)) {
		goto done;
	}

#ifdef ISR4_OFFSET
	IRQ_CONNECT(IRQ_LINE(ISR4_OFFSET), 1, isr4, ISR4_ARG, 0);
	irq_enable(IRQ_LINE(ISR4_OFFSET));
	TC_PRINT("isr4 isr=%p irq=%d param=%p\n", isr4, IRQ_LINE(ISR4_OFFSET),
		 (void *)ISR4_ARG);
	if (check_sw_isr(isr4, ISR4_ARG, ISR4_OFFSET)) {
		goto done;
	}
#endif

	irq_connect_dynamic(IRQ_LINE(ISR5_OFFSET), 1, isr5, (void *)ISR5_ARG,
			    0);
	irq_enable(IRQ_LINE(ISR5_OFFSET));
	TC_PRINT("isr5 isr=%p irq=%d param=%p\n", isr5, IRQ_LINE(ISR5_OFFSET),
		 (void *)ISR5_ARG);
	if (check_sw_isr(isr5, ISR5_ARG, ISR5_OFFSET)) {
		goto done;
	}

#ifdef ISR6_OFFSET
	irq_connect_dynamic(IRQ_LINE(ISR6_OFFSET), 1, isr6, (void *)ISR6_ARG,
			    0);
	irq_enable(IRQ_LINE(ISR6_OFFSET));
	TC_PRINT("isr6 isr=%p irq=%d param=%p\n", isr6, IRQ_LINE(ISR6_OFFSET),
		 (void *)ISR6_ARG);
	if (check_sw_isr(isr6, ISR6_ARG, ISR6_OFFSET)) {
		goto done;
	}
#endif
#endif /* CONFIG_GEN_SW_ISR_TABLE */
	rv = TC_PASS;
done:
	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
