/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq_multilevel.h>

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS) && defined(CONFIG_SOC_SERIES_ESP32S3)

#include <zephyr/dt-bindings/interrupt-controller/espressif-esp32s3-intmux.h>

#define UART0_CPU_LINE  5
#define SYSTIMER_CPU_LINE 10

ZTEST(mli_encoding, test_uart0_composite_irq)
{
	const unsigned int irqn = DT_IRQN(DT_NODELABEL(uart0));
	const unsigned int expected = IRQ_TO_L2(UART0_INTR_SOURCE) | UART0_CPU_LINE;

	zassert_equal(irqn, expected, "uart0 DT_IRQN mismatch: got 0x%x want 0x%x", irqn, expected);
	zassert_equal(irq_get_level(irqn), 2);
	zassert_equal(irq_from_level_2(irqn), UART0_INTR_SOURCE);
	zassert_equal(irq_parent_level_2(irqn), UART0_CPU_LINE);
}

ZTEST(mli_encoding, test_systimer0_composite_irq)
{
	const unsigned int irqn = DT_IRQN(DT_NODELABEL(systimer0));
	const unsigned int expected =
		IRQ_TO_L2(SYSTIMER_TARGET2_EDGE_INTR_SOURCE) | SYSTIMER_CPU_LINE;

	zassert_equal(irqn, expected, "systimer0 DT_IRQN mismatch: got 0x%x want 0x%x", irqn,
		      expected);
	zassert_equal(irq_get_level(irqn), 2);
	zassert_equal(irq_from_level_2(irqn), SYSTIMER_TARGET2_EDGE_INTR_SOURCE);
	zassert_equal(irq_parent_level_2(irqn), SYSTIMER_CPU_LINE);
}

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lcd_cam_l3_intc), okay)

#define LCD_CAM_CPU_LINE 17

/*
 * A level-3 leaf encodes all three fields. The cell in the consumer's
 * "interrupts" is the raw status-register bit, its parent is the aggregator's
 * INTMUX source, and that in turn sits on a CPU line.
 */
ZTEST(mli_encoding, test_lcd_cam_level3_irq)
{
	const unsigned int irqn = DT_IRQN_BY_IDX(DT_NODELABEL(lcd_cam), 2);
	const unsigned int expected =
		IRQ_TO_L3(2) | IRQ_TO_L2(LCD_CAM_INTR_SOURCE) | LCD_CAM_CPU_LINE;

	zassert_equal(irqn, expected, "lcd_cam flag 2 DT_IRQN mismatch: got 0x%x want 0x%x", irqn,
		      expected);
	zassert_equal(irq_get_level(irqn), 3);
	zassert_equal(irq_from_level_3(irqn), 2, "level-3 field is the status-register bit");
	zassert_equal(irq_from_level_2(irqn), LCD_CAM_INTR_SOURCE);
	zassert_equal(irq_parent_level_3(irqn), LCD_CAM_INTR_SOURCE);
	zassert_equal(irqn & BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS), LCD_CAM_CPU_LINE);
}

/*
 * The four flag IRQs, expanded at build time: DT_IRQN_BY_IDX() takes a literal
 * index, so the index cannot come from a loop variable.
 */
#define LCD_CAM_IRQN(i, _) DT_IRQN_BY_IDX(DT_NODELABEL(lcd_cam), i)
static const unsigned int lcd_cam_irqn[] = {LISTIFY(4, LCD_CAM_IRQN, (,))};

/* All four flags share one INTMUX source and differ only in the level-3 field. */
ZTEST(mli_encoding, test_lcd_cam_flags_share_one_source)
{
	for (unsigned int bit = 0; bit < ARRAY_SIZE(lcd_cam_irqn); bit++) {
		zassert_equal(irq_from_level_3(lcd_cam_irqn[bit]), bit, "flag %u", bit);
		zassert_equal(irq_from_level_2(lcd_cam_irqn[bit]),
			      irq_from_level_2(lcd_cam_irqn[0]),
			      "flag %u must share the aggregator's source", bit);
	}
}

#endif /* lcd_cam_l3_intc okay */

ZTEST_SUITE(mli_encoding, NULL, NULL, NULL, NULL, NULL);

#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS && CONFIG_SOC_SERIES_ESP32S3 */
