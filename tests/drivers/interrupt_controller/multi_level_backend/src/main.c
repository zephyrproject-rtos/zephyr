/*
 * Copyright (c) 2024 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sw_isr_common.h"

#include <stdint.h>

#include <zephyr/irq_multilevel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/ztest.h>

#ifdef CONFIG_DUMP_INTC_TABLE
#define DEBUG_PRINT(...) PRINT(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

/**
 * Fake device pointers
 */
/* Device pointer to level 2 intc 1 */
#define INTC_1_DEV UINT_TO_POINTER(21)
/* Device pointer to level 2 intc 2 */
#define INTC_2_DEV UINT_TO_POINTER(22)
/* Device pointer to level 3 intc 3*/
#define INTC_3_DEV UINT_TO_POINTER(31)
/* Device pointer to level 3 intc 4 */
#define INTC_4_DEV UINT_TO_POINTER(32)

/**
 * Interrupt controller's local IRQ
 */
/* Local IRQ of level 2 intc 1 */
#define INTC_1_IRQ 4
/* Local IRQ of level 2 intc 2 */
#define INTC_2_IRQ 5
/* Local IRQ of level 3 intc 3 */
#define INTC_3_IRQ 9
/* Local IRQ of level 3 intc 4 */
#define INTC_4_IRQ 10

/**
 * Interrupt controller's IRQ in Zephyr format
 */
/* Zephyr IRQ of level 2 intc 1 */
#define INTC_1_IRQN INTC_1_IRQ
/* Zephyr IRQ of level 2 intc 2 */
#define INTC_2_IRQN INTC_2_IRQ
/* Zephyr IRQ of level 3 intc 3*/
#define INTC_3_IRQN (IRQ_TO_L2(INTC_3_IRQ) | INTC_1_IRQN)
/* Zephyr IRQ of level 3 intc 4 */
#define INTC_4_IRQN (IRQ_TO_L2(INTC_4_IRQ) | INTC_2_IRQN)

/**
 *  Register all interrupt controller with the intc table
 */
/* Helper to calculate the stride for each intc in the ISR table */
#define INTC_COUNT(n) (n * CONFIG_MAX_IRQ_PER_AGGREGATOR)
#define INTC_1_OFFSET INTC_COUNT(1)
#define INTC_2_OFFSET INTC_COUNT(2)
#define INTC_3_OFFSET INTC_COUNT(3)
#define INTC_4_OFFSET INTC_COUNT(4)
IRQ_PARENT_ENTRY_DEFINE(intc_l2_1, INTC_1_DEV, INTC_1_IRQN, INTC_1_OFFSET, 2);
IRQ_PARENT_ENTRY_DEFINE(intc_l2_2, INTC_2_DEV, INTC_2_IRQN, INTC_2_OFFSET, 2);
IRQ_PARENT_ENTRY_DEFINE(intc_l3_3, INTC_3_DEV, INTC_3_IRQN, INTC_3_OFFSET, 3);
IRQ_PARENT_ENTRY_DEFINE(intc_l3_4, INTC_4_DEV, INTC_4_IRQN, INTC_4_OFFSET, 3);

/**
 *  Test IRQs in local format
 */
#define TEST_IRQ_1 2
#define TEST_IRQ_2 3
#define TEST_IRQ_3 4
#define TEST_IRQ_4 5

/**
 *  Test IRQs in Zephyr format
 */
/* TEST_IRQ_1 handled by intc_l2_1 */
#define TEST_IRQN_1 (IRQ_TO_L2(TEST_IRQ_1) | INTC_1_IRQN)
/* TEST_IRQ_2 handled by intc_l2_2 */
#define TEST_IRQN_2 (IRQ_TO_L2(TEST_IRQ_2) | INTC_2_IRQN)
/* TEST_IRQ_3 handled by intc_l3_1 */
#define TEST_IRQN_3 (IRQ_TO_L3(TEST_IRQ_3) | INTC_3_IRQN)
/* TEST_IRQ_4 handled by intc_l3_2 */
#define TEST_IRQN_4 (IRQ_TO_L3(TEST_IRQ_4) | INTC_4_IRQN)

ZTEST(intc_multi_level_backend, test_irq_from_device)
{
	/* degenerate cases */
	if (!IS_ENABLED(CONFIG_ASSERT)) {
		/* Return 0 if dev not found in the LUT */
		zassert_equal(z_get_sw_isr_irq_from_device(UINT_TO_POINTER(42)), 0);
	}

	zassert_equal(z_get_sw_isr_irq_from_device(INTC_1_DEV), INTC_1_IRQN);
	zassert_equal(z_get_sw_isr_irq_from_device(INTC_2_DEV), INTC_2_IRQN);
	zassert_equal(z_get_sw_isr_irq_from_device(INTC_3_DEV), INTC_3_IRQN);
	zassert_equal(z_get_sw_isr_irq_from_device(INTC_4_DEV), INTC_4_IRQN);
}

ZTEST(intc_multi_level_backend, test_device_from_irq)
{
	/* degenerate cases */
	if (!IS_ENABLED(CONFIG_ASSERT)) {
		/* Return NULL if can't find anything to handle the IRQ */
		zassert_equal_ptr(z_get_sw_isr_device_from_irq(IRQ_TO_L2(9) | 8), NULL);
	}

	zassert_equal_ptr(z_get_sw_isr_device_from_irq(TEST_IRQN_1), INTC_1_DEV);
	zassert_equal_ptr(z_get_sw_isr_device_from_irq(TEST_IRQN_2), INTC_2_DEV);
	zassert_equal_ptr(z_get_sw_isr_device_from_irq(TEST_IRQN_3), INTC_3_DEV);
	zassert_equal_ptr(z_get_sw_isr_device_from_irq(TEST_IRQN_4), INTC_4_DEV);
}

ZTEST(intc_multi_level_backend, test_table_idx_from_irq)
{
	/* degenerate cases */
	if (!IS_ENABLED(CONFIG_ASSERT)) {
		/* 2nd level aggregator that doesn't exist */
		const unsigned int first_level_agg = 8;
		const unsigned int unhandled_irqn = IRQ_TO_L2(TEST_IRQ_1) | first_level_agg;

		zassert_equal(z_get_sw_isr_table_idx(unhandled_irqn),
			      unhandled_irqn - CONFIG_GEN_IRQ_START_VECTOR);

		/* local_irq exceeded CONFIG_MAX_IRQ_PER_AGGREGATOR */
		const unsigned int local_irq = CONFIG_MAX_IRQ_PER_AGGREGATOR + 1;
		const unsigned int overflown_irqn = IRQ_TO_L2(local_irq) | INTC_1_IRQN;

		zassert_equal(z_get_sw_isr_table_idx(overflown_irqn),
			      local_irq + INTC_1_OFFSET - CONFIG_GEN_IRQ_START_VECTOR);

		/* Overflow SW ISR table */
		const unsigned int local_irq2 = (CONFIG_MAX_IRQ_PER_AGGREGATOR - 1);
		const unsigned int overflown_irqn2 = IRQ_TO_L3(local_irq2) | INTC_4_IRQN;

		zassert_equal(z_get_sw_isr_table_idx(overflown_irqn2),
			      local_irq2 + INTC_4_OFFSET - CONFIG_GEN_IRQ_START_VECTOR);
	}

	/* Level 1 */
	zassert_equal(z_get_sw_isr_table_idx(INTC_1_IRQN),
		      INTC_1_IRQN - CONFIG_GEN_IRQ_START_VECTOR);
	zassert_equal(z_get_sw_isr_table_idx(1), 1 - CONFIG_GEN_IRQ_START_VECTOR);

	/* Level 2 */
	zassert_equal(z_get_sw_isr_table_idx(TEST_IRQN_1),
		      TEST_IRQ_1 + INTC_1_OFFSET - CONFIG_GEN_IRQ_START_VECTOR);
	zassert_equal(z_get_sw_isr_table_idx(TEST_IRQN_2),
		      TEST_IRQ_2 + INTC_2_OFFSET - CONFIG_GEN_IRQ_START_VECTOR);

	/* Level 3 */
	zassert_equal(z_get_sw_isr_table_idx(TEST_IRQN_3),
		      TEST_IRQ_3 + INTC_3_OFFSET - CONFIG_GEN_IRQ_START_VECTOR);
	zassert_equal(z_get_sw_isr_table_idx(TEST_IRQN_4),
		      TEST_IRQ_4 + INTC_4_OFFSET - CONFIG_GEN_IRQ_START_VECTOR);
}

static void *setup(void)
{
	DEBUG_PRINT("=============== intc table ===============\n");
	DEBUG_PRINT("         dev |  level |    irq |  offset\n");
	DEBUG_PRINT("==========================================\n");
	STRUCT_SECTION_FOREACH_ALTERNATE(intc_table, _irq_parent_entry, intc)
	{
		DEBUG_PRINT("%12p | %6u | %6X | %7u\n", intc->dev, intc->level, intc->irq,
			    intc->offset);
	}
	DEBUG_PRINT("==========================================\n");

	return NULL;
}

ZTEST_SUITE(intc_multi_level_backend, NULL, setup, NULL, NULL, NULL);
