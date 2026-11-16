/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/sw_isr_table.h>
#include <sw_isr_common.h>

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)

/* A level-2 IRQ has its slot at the start of the level-2 window plus its source */
ZTEST(mli_dispatch, test_level_2_slot)
{
	const unsigned int irqn = DT_IRQN(DT_NODELABEL(uart0));

	zassert_equal(z_get_sw_isr_table_idx(irqn),
		      CONFIG_2ND_LVL_ISR_TBL_OFFSET + DT_IRQ(DT_NODELABEL(uart0), irq));
}

#if DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_counter)
#define COUNTER_IRQN DT_IRQN(DT_PARENT(DT_INST(0, espressif_esp32_counter)))

ZTEST(mli_dispatch, test_connected_level_2_slot)
{
	const unsigned int idx = z_get_sw_isr_table_idx(COUNTER_IRQN);

	zassert_not_equal(_sw_isr_table[idx].isr, z_irq_spurious,
			  "the counter ISR must sit in its level-2 slot");
}

ZTEST(mli_dispatch, test_enable_state)
{
	zassert_true(irq_is_enabled(COUNTER_IRQN) != 0, "the counter driver enables its IRQ");

	irq_disable(COUNTER_IRQN);
	zassert_true(irq_is_enabled(COUNTER_IRQN) == 0);

	irq_enable(COUNTER_IRQN);
	zassert_true(irq_is_enabled(COUNTER_IRQN) != 0);
}
#endif

#if defined(CONFIG_3RD_LEVEL_INTERRUPTS) && DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio0_l3_intc))
/*
 * A level-3 IRQ has its slot in the window of its aggregator, at the input
 * index. The GPIO driver is connected on the catch-all input.
 */
ZTEST(mli_dispatch, test_level_3_catch_all_slot)
{
	const unsigned int irqn = DT_IRQN(DT_NODELABEL(gpio0));
	unsigned int idx = z_get_sw_isr_table_idx(irqn);

	zassert_true(idx >= CONFIG_3RD_LVL_ISR_TBL_OFFSET);
	zassert_equal((idx - CONFIG_3RD_LVL_ISR_TBL_OFFSET) %
			      CONFIG_MAX_IRQ_PER_3RD_LEVEL_AGGREGATOR,
		      irq_from_level_3(irqn));
	zassert_not_equal(_sw_isr_table[idx].isr, z_irq_spurious,
			  "the GPIO ISR must sit in the catch-all slot");
	idx = z_get_sw_isr_table_idx(DT_IRQN(DT_NODELABEL(gpio0_l3_intc)));
	zassert_not_equal(_sw_isr_table[idx].isr, z_irq_spurious,
			  "the aggregator ISR must sit in its level-2 slot");
}
#endif

ZTEST_SUITE(mli_dispatch, NULL, NULL, NULL, NULL, NULL);

#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */
