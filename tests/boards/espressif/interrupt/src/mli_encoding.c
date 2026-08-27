/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/dt-bindings/interrupt-controller/espressif-esp32-l3-intc.h>

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)

/* Every level-2 and level-3 IRQ carries the intmux key line in its level-1 field */
#define INTMUX_KEY_LINE DT_IRQ(DT_NODELABEL(intmux), irq)

ZTEST(mli_encoding, test_shared_consumer_is_level_2)
{
	const unsigned int irqn = DT_IRQN(DT_NODELABEL(uart0));

	zassert_equal(irq_get_level(irqn), 2U);
	zassert_equal(irq_from_level_2(irqn), DT_IRQ(DT_NODELABEL(uart0), irq),
		      "level-2 field must be the interrupt matrix source");
	zassert_equal(irq_parent_level_2(irqn), INTMUX_KEY_LINE);
}

ZTEST(mli_encoding, test_flags_cell)
{
	zassert_equal(DT_IRQ(DT_NODELABEL(uart0), flags) & ESP_INTR_FLAG_IRAM, 0,
		      "uart0 must not be marked ESP_INTR_FLAG_IRAM");
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(systimer0))
	zassert_not_equal(DT_IRQ(DT_NODELABEL(systimer0), flags) & ESP_INTR_FLAG_IRAM, 0,
			  "the system tick must be marked ESP_INTR_FLAG_IRAM");
#endif
}

#if defined(CONFIG_3RD_LEVEL_INTERRUPTS) &&                                                       \
	(DT_NODE_EXISTS(DT_NODELABEL(rtc_l3_intc)) || DT_NODE_EXISTS(DT_NODELABEL(gpio0_l3_intc)))

static void check_level_3(unsigned int irqn, unsigned int input, unsigned int source)
{
	zassert_equal(irq_get_level(irqn), 3U);
	zassert_equal(irq_from_level_3(irqn), input, "level-3 field must be the input index");
	zassert_equal(irq_from_level_2(irqn), source,
		      "level-2 field must be the aggregator's source");
	zassert_equal(irq_parent_level_2(irqn), INTMUX_KEY_LINE);
}

#if DT_NODE_EXISTS(DT_NODELABEL(rtc_l3_intc))
ZTEST(mli_encoding, test_rtc_timer_is_level_3)
{
	check_level_3(DT_IRQN(DT_NODELABEL(rtc_timer)), DT_IRQ(DT_NODELABEL(rtc_timer), irq),
		      DT_IRQ(DT_NODELABEL(rtc_l3_intc), irq));
}
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(gpio0_l3_intc))
BUILD_ASSERT(DT_PROP_LAST(DT_NODELABEL(gpio0_l3_intc), bits) == ESP_L3_CATCH_ALL,
	     "the last gpio0_l3_intc input must be ESP_L3_CATCH_ALL");

ZTEST(mli_encoding, test_gpio_is_catch_all_input)
{
	const unsigned int source = DT_IRQ(DT_NODELABEL(gpio0_l3_intc), irq);

	check_level_3(DT_IRQN(DT_NODELABEL(gpio0)),
		      DT_PROP_LEN(DT_NODELABEL(gpio0_l3_intc), bits) - 1U, source);
	check_level_3(DT_IRQN(DT_NODELABEL(ana_cmpr0)), 0U, source);
}
#endif

#endif /* CONFIG_3RD_LEVEL_INTERRUPTS && level-3 consumers */

ZTEST_SUITE(mli_encoding, NULL, NULL, NULL, NULL, NULL);

#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */
