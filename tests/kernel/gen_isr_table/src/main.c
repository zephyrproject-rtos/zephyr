/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/tc_util.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/interrupt_util.h>
#include <zephyr/sys/barrier.h>

extern uint32_t _irq_vector_table[];

#if defined(ARCH_IRQ_DIRECT_CONNECT) && defined(CONFIG_GEN_IRQ_VECTOR_TABLE)
#define HAS_DIRECT_IRQS
#endif

#if defined(CONFIG_RISCV)
#if defined(CONFIG_NRFX_CLIC)
#define ISR1_OFFSET	15
#define ISR3_OFFSET	16
#define ISR5_OFFSET	17
#define TRIG_CHECK_SIZE	18
#elif defined(CONFIG_RISCV_HAS_CLIC)
#define ISR1_OFFSET	3
#define ISR3_OFFSET	17
#define ISR5_OFFSET	18
#define TRIG_CHECK_SIZE	19
#else

/* RISC-V has very few IRQ lines which can be triggered from software */
#define ISR3_OFFSET	1

/* Since we have so few lines we have to share the same line for two different
 * tests
 */
#ifdef HAS_DIRECT_IRQS
#define ISR1_OFFSET	5
#else
#define ISR5_OFFSET	5
#endif
#define TRIG_CHECK_SIZE	6
#endif

#define IRQ_LINE(offset)        offset
#if defined(CONFIG_RISCV_RESERVED_IRQ_ISR_TABLES_OFFSET)
#define TABLE_INDEX(offset)     offset + CONFIG_RISCV_RESERVED_IRQ_ISR_TABLES_OFFSET
#else
#define TABLE_INDEX(offset)     offset
#endif

#else
#define ISR1_OFFSET	0
#define ISR2_OFFSET	1
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
#define TEST_NUM_IRQS	26
#elif defined(CONFIG_SOC_SERIES_NPCX7) || defined(CONFIG_SOC_SERIES_NPCX9)
/* Both NPCX7 and NPCX9 series use the IRQs at the end of the vector table, for
 * example, the IRQ 60 and 61 used for Multi-Input Wake-Up Unit (MIWU) devices
 * by default, and conflicts with ISR used for testing. Move IRQs for this
 * test suite to solve the issue.
 */
#define TEST_NUM_IRQS	44
#elif defined(CONFIG_SOC_LPC55S16)
/* IRQ 57 is reserved in the NXP LPC55S16 SoC. Thus, limit the number
 * of interrupts reported to the test, so that it does not try to use
 * it.
 */
#define TEST_NUM_IRQS	57
#else
#define TEST_NUM_IRQS	CONFIG_NUM_IRQS
#endif

#define TEST_IRQ_TABLE_SIZE	(IRQ_TABLE_SIZE - \
				 (CONFIG_NUM_IRQS - TEST_NUM_IRQS))
#define IRQ_LINE(offset)	(TEST_NUM_IRQS - ((offset) + 1))
#define TABLE_INDEX(offset)	(TEST_IRQ_TABLE_SIZE - ((offset) + 1))
#define TRIG_CHECK_SIZE		6
#endif

#define ISR3_ARG	0xb01dface
#define ISR4_ARG	0xca55e77e
#define ISR5_ARG	0xf0ccac1a
#define ISR6_ARG	0xba5eba11

#if defined(CONFIG_RISCV_HAS_CLIC)
#define IRQ_FLAGS 1 /* rising edge */
#else
#define IRQ_FLAGS 0
#endif

static volatile int trigger_check[TRIG_CHECK_SIZE];

#ifdef HAS_DIRECT_IRQS
#ifdef ISR1_OFFSET
ISR_DIRECT_DECLARE(isr1)
{
	printk("isr1 ran\n");
	trigger_check[ISR1_OFFSET]++;
	return 0;
}
#endif

#ifdef ISR2_OFFSET
ISR_DIRECT_DECLARE(isr2)
{
	printk("isr2 ran\n");
	trigger_check[ISR2_OFFSET]++;
	return 1;
}
#endif
#endif

#ifdef ISR3_OFFSET
void isr3(const void *param)
{
	printk("%s ran with parameter %p\n", __func__, param);
	trigger_check[ISR3_OFFSET]++;
}
#endif

#ifdef ISR4_OFFSET
void isr4(const void *param)
{
	printk("%s ran with parameter %p\n", __func__, param);
	trigger_check[ISR4_OFFSET]++;
}
#endif

#ifdef ISR5_OFFSET
void isr5(const void *param)
{
	printk("%s ran with parameter %p\n", __func__, param);
	trigger_check[ISR5_OFFSET]++;
}
#endif

#ifdef ISR6_OFFSET
void isr6(const void *param)
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
__no_optimization
#endif
int test_irq(int offset)
{
#ifndef NO_TRIGGER_FROM_SW
	TC_PRINT("triggering irq %d\n", IRQ_LINE(offset));
	trigger_irq(IRQ_LINE(offset));
#ifdef CONFIG_CPU_CORTEX_M
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
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
/*
 * The problem with an IRQ table where the entries are jump opcodes is that it
 * the destination address is encoded in the opcode and strictly depending on
 * the address of the instruction itself (and very much architecture
 * dependent). For the sake of simplicity just skip the checks.
 */
#ifndef CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_CODE
	TC_PRINT("Checking _irq_vector_table entry %d for irq %d\n",
		 TABLE_INDEX(offset), IRQ_LINE(offset));

	if (_irq_vector_table[TABLE_INDEX(offset)] != (uint32_t)isr) {
		TC_PRINT("bad entry %d in vector table\n", TABLE_INDEX(offset));
		return -1;
	}
#endif /* !CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_CODE */

	if (test_irq(offset)) {
		return -1;
	}

	return 0;
}
#endif

#ifdef CONFIG_GEN_SW_ISR_TABLE
static int check_sw_isr(void *isr, uintptr_t arg, int offset)
{
	struct _isr_table_entry *e = &_sw_isr_table[TABLE_INDEX(offset)];

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
#if defined(CONFIG_GEN_IRQ_VECTOR_TABLE) && !defined(CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_CODE)
	void *v = (void *)_irq_vector_table[TABLE_INDEX(offset)];
	if (v != _isr_wrapper) {
		TC_PRINT("Vector does not point to _isr_wrapper\n");
		TC_PRINT("expected %p got %p\n", _isr_wrapper, v);
		return -1;
	}
#endif /* CONFIG_GEN_IRQ_VECTOR_TABLE && !CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_CODE */

	if (test_irq(offset)) {
		return -1;
	}
	return 0;
}
#endif

/**
 * @ingroup kernel_interrupt_tests
 * @brief test to validate direct interrupt
 *
 * @details initialize two direct interrupt handler using IRQ_DIRECT_CONNECT
 * api at build time. For ‘direct’ interrupts, address of handler function will
 * be placed in the irq vector table. And each entry contains the pointer to
 * isr and the corresponding parameters.
 *
 * At the end according to architecture, we manually trigger the interrupt.
 * And all irq handler should get called.
 *
 * @see IRQ_DIRECT_CONNECT(), irq_enable()
 *
 */
ZTEST(gen_isr_table, test_build_time_direct_interrupt)
{
#ifndef HAS_DIRECT_IRQS
	ztest_test_skip();
#else

#ifdef ISR1_OFFSET
	IRQ_DIRECT_CONNECT(IRQ_LINE(ISR1_OFFSET), 0, isr1, IRQ_FLAGS);
	irq_enable(IRQ_LINE(ISR1_OFFSET));
	TC_PRINT("isr1 isr=%p irq=%d\n", isr1, IRQ_LINE(ISR1_OFFSET));
	zassert_ok(check_vector(isr1, ISR1_OFFSET),
			"check direct interrpt isr1 failed");
#endif

#ifdef ISR2_OFFSET
	IRQ_DIRECT_CONNECT(IRQ_LINE(ISR2_OFFSET), 0, isr2, IRQ_FLAGS);
	irq_enable(IRQ_LINE(ISR2_OFFSET));
	TC_PRINT("isr2 isr=%p irq=%d\n", isr2, IRQ_LINE(ISR2_OFFSET));


	zassert_ok(check_vector(isr2, ISR2_OFFSET),
			"check direct interrpt isr2 failed");
#endif
#endif
}

/**
 * @ingroup kernel_interrupt_tests
 * @brief test to validate gen_isr_table and interrupt
 *
 * @details initialize two normal interrupt handler using IRQ_CONNECT api at
 * build time. For 'regular' interrupts, the address of the common software isr
 * table is placed in the irq vector table, and software ISR table is an array
 * of struct _isr_table_entry. And each entry contains the pointer to isr and
 * the corresponding parameters.
 *
 * At the end according to architecture, we manually trigger the interrupt.
 * And all irq handler should get called.
 *
 * @see IRQ_CONNECT(), irq_enable()
 *
 */
ZTEST(gen_isr_table, test_build_time_interrupt)
{
#ifndef CONFIG_GEN_SW_ISR_TABLE
	ztest_test_skip();
#else
	TC_PRINT("_sw_isr_table at location %p\n", _sw_isr_table);

#ifdef ISR3_OFFSET
	IRQ_CONNECT(IRQ_LINE(ISR3_OFFSET), 1, isr3, ISR3_ARG, IRQ_FLAGS);
	irq_enable(IRQ_LINE(ISR3_OFFSET));
	TC_PRINT("isr3 isr=%p irq=%d param=%p\n", isr3, IRQ_LINE(ISR3_OFFSET),
		 (void *)ISR3_ARG);

	zassert_ok(check_sw_isr(isr3, ISR3_ARG, ISR3_OFFSET),
			"check interrupt isr3 failed");
#endif

#ifdef ISR4_OFFSET
	IRQ_CONNECT(IRQ_LINE(ISR4_OFFSET), 1, isr4, ISR4_ARG, IRQ_FLAGS);
	irq_enable(IRQ_LINE(ISR4_OFFSET));
	TC_PRINT("isr4 isr=%p irq=%d param=%p\n", isr4, IRQ_LINE(ISR4_OFFSET),
		 (void *)ISR4_ARG);

	zassert_ok(check_sw_isr(isr4, ISR4_ARG, ISR4_OFFSET),
			"check interrupt isr4 failed");
#endif
#endif
}

/**
 * @ingroup kernel_interrupt_tests
 * @brief test to validate gen_isr_table and dynamic interrupt
 *
 * @details initialize two dynamic interrupt handler using irq_connect_dynamic
 * api at run time. For dynamic interrupts, the address of the common software
 * isr table is also placed in the irq vector table. Software ISR table is an
 * array of struct _isr_table_entry. And each entry contains the pointer to isr
 * and the corresponding parameters.
 *
 * At the end according to architecture, we manually trigger the interrupt.
 * And all irq handler should get called.
 *
 * @see irq_connect_dynamic(), irq_enable()
 *
 */
ZTEST(gen_isr_table, test_run_time_interrupt)
{

#ifndef CONFIG_GEN_SW_ISR_TABLE
	ztest_test_skip();
#else

#ifdef ISR5_OFFSET
	irq_connect_dynamic(IRQ_LINE(ISR5_OFFSET), 1, isr5,
			    (const void *)ISR5_ARG, IRQ_FLAGS);
	irq_enable(IRQ_LINE(ISR5_OFFSET));
	TC_PRINT("isr5 isr=%p irq=%d param=%p\n", isr5, IRQ_LINE(ISR5_OFFSET),
		 (void *)ISR5_ARG);
	zassert_ok(check_sw_isr(isr5, ISR5_ARG, ISR5_OFFSET),
			"test dynamic interrupt isr5 failed");
#endif

#ifdef ISR6_OFFSET
	irq_connect_dynamic(IRQ_LINE(ISR6_OFFSET), 1, isr6,
			    (const void *)ISR6_ARG, IRQ_FLAGS);
	irq_enable(IRQ_LINE(ISR6_OFFSET));
	TC_PRINT("isr6 isr=%p irq=%d param=%p\n", isr6, IRQ_LINE(ISR6_OFFSET),
		 (void *)ISR6_ARG);

	zassert_ok(check_sw_isr(isr6, ISR6_ARG, ISR6_OFFSET),
			"check dynamic interrupt isr6 failed");
#endif
#endif
}

static void *gen_isr_table_setup(void)
{
	TC_PRINT("IRQ configuration (total lines %d):\n", CONFIG_NUM_IRQS);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-label"

	return NULL;
}

#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
static void test_multi_level_bit_masks_fn(uint32_t irq1, uint32_t irq2, uint32_t irq3)
{
	const uint32_t l2_shift = CONFIG_1ST_LEVEL_INTERRUPT_BITS;
	const uint32_t l3_shift = CONFIG_1ST_LEVEL_INTERRUPT_BITS + CONFIG_2ND_LEVEL_INTERRUPT_BITS;
	const uint32_t hwirq1 = irq1;
	const uint32_t hwirq2 = irq2 - 1;
	const uint32_t hwirq3 = irq3 - 1;
	const bool has_l3 = irq3 > 0;
	const bool has_l2 = irq2 > 0;
	const uint32_t level = has_l3 ? 3 : has_l2 ? 2 : 1;
	const uint32_t irqn_l1 = irq1;
	const uint32_t irqn_l2 = (irq2 << l2_shift) | irqn_l1;
	const uint32_t irqn = (irq3 << l3_shift) | irqn_l2;

	zassert_equal(level, irq_get_level(irqn));

	if (has_l2) {
		zassert_equal(hwirq2, irq_from_level_2(irqn));
		zassert_equal(hwirq2, irq_from_level(irqn, 2));
		zassert_equal((hwirq2 + 1) << l2_shift, irq_to_level_2(hwirq2));
		zassert_equal((hwirq2 + 1) << l2_shift, irq_to_level(hwirq2, 2));
		zassert_equal(hwirq1, irq_parent_level_2(irqn));
		zassert_equal(hwirq1, irq_parent_level(irqn, 2));
	}

	if (has_l3) {
		zassert_equal(hwirq3, irq_from_level_3(irqn));
		zassert_equal(hwirq3, irq_from_level(irqn, 3));
		zassert_equal((hwirq3 + 1) << l3_shift, irq_to_level_3(hwirq3));
		zassert_equal((hwirq3 + 1) << l3_shift, irq_to_level(hwirq3, 3));
		zassert_equal(hwirq2, irq_parent_level_3(irqn));
		zassert_equal(hwirq2, irq_parent_level(irqn, 3));
	}

	if (has_l3) {
		zassert_equal(irqn_l2, irq_get_intc_irq(irqn));
	} else if (has_l2) {
		zassert_equal(irqn_l1, irq_get_intc_irq(irqn));
	} else {
		/* degenerate cases */
		if (false) {
			zassert_equal(irqn, irq_get_intc_irq(irqn));
		}
	}
}

ZTEST(gen_isr_table, test_multi_level_bit_masks_l1)
{
	uint32_t irq1;

	/* First IRQ of level 1 */
	irq1 = 0;
	test_multi_level_bit_masks_fn(irq1, 0, 0);

	/* Somewhere in-between */
	irq1 = BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS) >> 1;
	test_multi_level_bit_masks_fn(irq1, 0, 0);

	/* Last IRQ of level 1 */
	irq1 = BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS);
	test_multi_level_bit_masks_fn(irq1, 0, 0);
}

ZTEST(gen_isr_table, test_multi_level_bit_masks_l2)
{
	if (!IS_ENABLED(CONFIG_2ND_LEVEL_INTERRUPTS)) {
		ztest_test_skip();
	}

	uint32_t irq1, irq2;

	/* First IRQ of level 2 */
	irq1 = 0;
	/* First irq of level 2 and onwards is 1, as 0 means that the irq is not present */
	irq2 = 1;
	test_multi_level_bit_masks_fn(irq1, irq2, 0);

	/* Somewhere in-between */
	irq1 = BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS) >> 1;
	irq2 = BIT_MASK(CONFIG_2ND_LEVEL_INTERRUPT_BITS) >> 1;
	test_multi_level_bit_masks_fn(irq1, irq2, 0);

	/* Last IRQ of level 2 */
	irq1 = BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS);
	irq2 = BIT_MASK(CONFIG_2ND_LEVEL_INTERRUPT_BITS);
	test_multi_level_bit_masks_fn(irq1, irq2, 0);
}

ZTEST(gen_isr_table, test_multi_level_bit_masks_l3)
{
	if (!IS_ENABLED(CONFIG_3RD_LEVEL_INTERRUPTS)) {
		ztest_test_skip();
	}

	uint32_t irq1, irq2, irq3;

	/* First IRQ of level 3 */
	irq1 = 0;
	/* First irq of level 2 and onwards is 1, as 0 means that the irq is not present */
	irq2 = 1;
	irq3 = 1;
	test_multi_level_bit_masks_fn(irq1, irq2, irq3);

	/* Somewhere in-between */
	irq1 = BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS) >> 1;
	irq2 = BIT_MASK(CONFIG_2ND_LEVEL_INTERRUPT_BITS) >> 1;
	irq3 = BIT_MASK(CONFIG_3RD_LEVEL_INTERRUPT_BITS) >> 1;
	test_multi_level_bit_masks_fn(irq1, irq2, irq3);

	/* Last IRQ of level 3 */
	irq1 = BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS);
	irq2 = BIT_MASK(CONFIG_2ND_LEVEL_INTERRUPT_BITS);
	irq3 = BIT_MASK(CONFIG_3RD_LEVEL_INTERRUPT_BITS);
	test_multi_level_bit_masks_fn(irq1, irq2, irq3);
}

#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */

ZTEST_SUITE(gen_isr_table, NULL, gen_isr_table_setup, NULL, NULL, NULL);
