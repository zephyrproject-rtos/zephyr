/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Where gen_isr_tables.py put a statically connected handler, against where
 * z_get_sw_isr_table_idx() looks for it at runtime.
 *
 * The two indices are computed by entirely separate code: the build-time one by
 * the Kconfig arithmetic in gen_isr_tables.py, the runtime one by adding the
 * local IRQ to the offset an aggregator registered with IRQ_PARENT_ENTRY_DEFINE
 * (INTC_INST_ISR_TBL_OFFSET() for a devicetree-declared controller). Nothing
 * makes them agree but the two using the same window sizes, and when they
 * disagree every interrupt on the affected aggregator dispatches to the wrong
 * slot with no build diagnostic at all.
 *
 * Level-2 aggregator 0 is the platform's own interrupt controller, so that half
 * of the check is end to end. The other aggregators are fabricated: their
 * offsets come from this file and their sources are never enabled, only placed.
 */

#include "sw_isr_common.h"

#include <zephyr/irq_multilevel.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/ztest.h>

#ifdef CONFIG_TEST_ISR_TABLE_PLACEMENT

/* CPU line of level-2 aggregator 0, the platform interrupt controller. */
#define AGG2_0_LINE CONFIG_2ND_LVL_INTR_00_OFFSET
/* CPU line of level-2 aggregator 1, fabricated by this test. */
#define AGG2_1_LINE CONFIG_2ND_LVL_INTR_01_OFFSET
/* Level-2 local IRQs hosting the two level-3 aggregators. */
#define AGG3_0_L2 CONFIG_3RD_LVL_INTR_00_OFFSET
#define AGG3_1_L2 CONFIG_3RD_LVL_INTR_01_OFFSET

/* Local IRQs connected inside each aggregator's window. */
#define L2_LOCAL 20
#define L3_LOCAL 6

#define AGG2_1_IRQN AGG2_1_LINE
#define L2_0_IRQN   (IRQ_TO_L2(L2_LOCAL) | AGG2_0_LINE)
#define L2_1_IRQN   (IRQ_TO_L2(L2_LOCAL) | AGG2_1_LINE)
#define AGG3_0_IRQN (IRQ_TO_L2(AGG3_0_L2) | AGG2_0_LINE)
#define AGG3_1_IRQN (IRQ_TO_L2(AGG3_1_L2) | AGG2_0_LINE)
#define L3_0_IRQN   (IRQ_TO_L3(L3_LOCAL) | AGG3_0_IRQN)
#define L3_1_IRQN   (IRQ_TO_L3(L3_LOCAL) | AGG3_1_IRQN)

/* The fixed-width window arithmetic, spelled out once. Each aggregator owns
 * MAX_IRQ_PER_<n>_LEVEL_AGGREGATOR consecutive slots from its level's base,
 * picked by its position in the CONFIG_<n>_LVL_INTR_NN_OFFSET list.
 */
#define L2_SLOT(agg, local)                                                                        \
	(CONFIG_2ND_LVL_ISR_TBL_OFFSET + ((agg) * CONFIG_MAX_IRQ_PER_2ND_LEVEL_AGGREGATOR) +       \
	 (local))
#define L3_SLOT(agg, local)                                                                        \
	(CONFIG_3RD_LVL_ISR_TBL_OFFSET + ((agg) * CONFIG_MAX_IRQ_PER_3RD_LEVEL_AGGREGATOR) +       \
	 (local))

BUILD_ASSERT(L3_SLOT(0, 0) >= L2_SLOT(CONFIG_NUM_2ND_LEVEL_AGGREGATORS, 0),
	     "level-3 window overlaps the level-2 one");
BUILD_ASSERT(L3_SLOT(CONFIG_NUM_3RD_LEVEL_AGGREGATORS, 0) <= CONFIG_NUM_IRQS,
	     "level-3 window runs past the end of the table");

/* Fake devices, only ever compared by pointer. */
#define AGG2_1_DEV UINT_TO_POINTER(0x21)
#define AGG3_0_DEV UINT_TO_POINTER(0x31)
#define AGG3_1_DEV UINT_TO_POINTER(0x32)

IRQ_PARENT_ENTRY_DEFINE(placement_l2_1, AGG2_1_DEV, AGG2_1_IRQN, L2_SLOT(1, 0), 2);
IRQ_PARENT_ENTRY_DEFINE(placement_l3_0, AGG3_0_DEV, AGG3_0_IRQN, L3_SLOT(0, 0), 3);
IRQ_PARENT_ENTRY_DEFINE(placement_l3_1, AGG3_1_DEV, AGG3_1_IRQN, L3_SLOT(1, 0), 3);

#define PARAM_AGG2_1 UINT_TO_POINTER(0xA1)
#define PARAM_L2_0   UINT_TO_POINTER(0xB0)
#define PARAM_L2_1   UINT_TO_POINTER(0xB1)
#define PARAM_L3_0   UINT_TO_POINTER(0xC0)
#define PARAM_L3_1   UINT_TO_POINTER(0xC1)

/* Never called: this test places handlers, it does not dispatch them. */
#define PLACEHOLDER_HANDLER(name)                                                                  \
	static void name(const void *arg)                                                          \
	{                                                                                          \
		ARG_UNUSED(arg);                                                                   \
	}

PLACEHOLDER_HANDLER(agg2_1_handler)
PLACEHOLDER_HANDLER(l2_0_handler)
PLACEHOLDER_HANDLER(l2_1_handler)
PLACEHOLDER_HANDLER(l3_0_handler)
PLACEHOLDER_HANDLER(l3_1_handler)

/* Z_ISR_DECLARE rather than IRQ_CONNECT: it is the half of IRQ_CONNECT that
 * feeds .intList, without the z_riscv_irq_priority_set() call that would
 * program a controller for IRQs this test only ever places.
 */
Z_ISR_DECLARE(AGG2_1_IRQN, 0, agg2_1_handler, PARAM_AGG2_1);
Z_ISR_DECLARE(L2_0_IRQN, 0, l2_0_handler, PARAM_L2_0);
Z_ISR_DECLARE(L2_1_IRQN, 0, l2_1_handler, PARAM_L2_1);
Z_ISR_DECLARE(L3_0_IRQN, 0, l3_0_handler, PARAM_L3_0);
Z_ISR_DECLARE(L3_1_IRQN, 0, l3_1_handler, PARAM_L3_1);

static void assert_placed_at(unsigned int slot, void (*isr)(const void *), const void *arg,
			     const char *what)
{
	zassert_true(slot < CONFIG_NUM_IRQS, "%s: slot %u past the end of the table", what, slot);
	zassert_equal_ptr(_sw_isr_table[slot - CONFIG_GEN_IRQ_START_VECTOR].isr, isr,
			  "%s: slot %u holds %p, expected %p", what, slot,
			  _sw_isr_table[slot - CONFIG_GEN_IRQ_START_VECTOR].isr, isr);
	zassert_equal_ptr(_sw_isr_table[slot - CONFIG_GEN_IRQ_START_VECTOR].arg, arg,
			  "%s: slot %u carries argument %p, expected %p", what, slot,
			  _sw_isr_table[slot - CONFIG_GEN_IRQ_START_VECTOR].arg, arg);
}

/**
 * @brief The generator places each level's handlers in its own window
 *
 * @details Read back the slot the fixed-width arithmetic predicts for a level 1,
 * level 2 and level 3 handler and check the generated _sw_isr_table holds that
 * handler and its argument. The level-2 and level-3 windows have different
 * sizes here, so a placement that used one size for both lands elsewhere.
 */
ZTEST(intc_isr_table_placement, test_generator_placement)
{
	assert_placed_at(AGG2_1_IRQN, agg2_1_handler, PARAM_AGG2_1, "level 1");
	assert_placed_at(L2_SLOT(0, L2_LOCAL), l2_0_handler, PARAM_L2_0, "level 2 aggregator 0");
	assert_placed_at(L2_SLOT(1, L2_LOCAL), l2_1_handler, PARAM_L2_1, "level 2 aggregator 1");
	assert_placed_at(L3_SLOT(0, L3_LOCAL), l3_0_handler, PARAM_L3_0, "level 3 aggregator 0");
	assert_placed_at(L3_SLOT(1, L3_LOCAL), l3_1_handler, PARAM_L3_1, "level 3 aggregator 1");
}

/**
 * @brief The runtime lookup resolves to the slot the generator wrote
 *
 * @details z_get_sw_isr_table_idx() derives its index from the offset an
 * aggregator registered with IRQ_PARENT_ENTRY_DEFINE, which for the platform
 * interrupt controller comes from INTC_INST_ISR_TBL_OFFSET(). This is the check
 * that the header macro and the generator size their windows the same way.
 */
ZTEST(intc_isr_table_placement, test_runtime_lookup_agrees)
{
	const struct {
		unsigned int irqn;
		unsigned int slot;
		void (*isr)(const void *arg);
	} cases[] = {
		{L2_0_IRQN, L2_SLOT(0, L2_LOCAL), l2_0_handler},
		{L2_1_IRQN, L2_SLOT(1, L2_LOCAL), l2_1_handler},
		{L3_0_IRQN, L3_SLOT(0, L3_LOCAL), l3_0_handler},
		{L3_1_IRQN, L3_SLOT(1, L3_LOCAL), l3_1_handler},
	};

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		unsigned int idx = z_get_sw_isr_table_idx(cases[i].irqn);

		zassert_equal(idx, cases[i].slot - CONFIG_GEN_IRQ_START_VECTOR,
			      "IRQ %#x: runtime index %u, generator slot %u", cases[i].irqn, idx,
			      cases[i].slot - CONFIG_GEN_IRQ_START_VECTOR);
		zassert_equal_ptr(_sw_isr_table[idx].isr, cases[i].isr,
				  "IRQ %#x: runtime index %u holds the wrong handler",
				  cases[i].irqn, idx);
	}
}

/**
 * @brief Windows do not spill into their neighbours
 *
 * @details A window sized from the wrong Kconfig symbol still places its first
 * entries correctly and only overruns at the end, so check the slots framing
 * each window are untouched.
 */
ZTEST(intc_isr_table_placement, test_windows_do_not_overlap)
{
	const unsigned int boundaries[] = {
		L2_SLOT(0, 0),
		L2_SLOT(1, 0) - 1,
		L2_SLOT(1, 0),
		L2_SLOT(CONFIG_NUM_2ND_LEVEL_AGGREGATORS, 0) - 1,
		L3_SLOT(0, 0),
		L3_SLOT(1, 0) - 1,
		L3_SLOT(1, 0),
		L3_SLOT(CONFIG_NUM_3RD_LEVEL_AGGREGATORS, 0) - 1,
	};

	for (size_t i = 0; i < ARRAY_SIZE(boundaries); i++) {
		unsigned int slot = boundaries[i] - CONFIG_GEN_IRQ_START_VECTOR;

		zassert_equal_ptr(_sw_isr_table[slot].isr, z_irq_spurious,
				  "slot %u should be unconnected, holds %p", boundaries[i],
				  _sw_isr_table[slot].isr);
	}
}

/**
 * @brief The fixed-width layout emits no interrupt-matrix dispatchers
 *
 * @details Under CONFIG_INTERRUPT_MATRIX_LAYOUT a CPU line carrying several
 * level-2 sources gets z_soc_2nd_lvl_isr in its own slot. Here the line's slot
 * belongs to the line alone, and the sources stay in the window. A platform
 * that grew the matrix placement by accident would not link, so this only has
 * to show the line's slot is still its own.
 */
ZTEST(intc_isr_table_placement, test_line_slot_is_not_a_dispatcher)
{
	unsigned int slot = AGG2_1_IRQN - CONFIG_GEN_IRQ_START_VECTOR;

	zassert_equal_ptr(_sw_isr_table[slot].isr, agg2_1_handler);
	zassert_equal_ptr(_sw_isr_table[slot].arg, PARAM_AGG2_1);
}

ZTEST_SUITE(intc_isr_table_placement, NULL, NULL, NULL, NULL, NULL);

#endif /* CONFIG_TEST_ISR_TABLE_PLACEMENT */
