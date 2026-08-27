/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/irq_multilevel.h>
#include <esp_soc_irq.h>

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS) && defined(CONFIG_SOC_SERIES_ESP32S3)

#include <zephyr/dt-bindings/interrupt-controller/espressif-esp32s3-intmux.h>

/* Flat single-aggregator leaf window (NUM_2ND_LEVEL_AGGREGATORS == 1). */
#define ESP_L2_BASE CONFIG_2ND_LVL_ISR_TBL_OFFSET

/* Timer0/1/2 intmux children all use CPU line 8 in esp32s3_common.dtsi. */
#define TIMER_SHARED_CPU_LINE 8

void z_soc_2nd_lvl_isr(const void *arg);

ZTEST(mli_dispatch, test_timer_shared_line_has_dispatcher)
{
	/*
	 * This test's soc overlay enables three timers on CPU line 8, so
	 * gen_isr_tables.py installs the shared L2 dispatcher in the L1 slot.
	 */
	zassert_equal(_sw_isr_table[TIMER_SHARED_CPU_LINE].isr, z_soc_2nd_lvl_isr,
		      "CPU line %u should host z_soc_2nd_lvl_isr", TIMER_SHARED_CPU_LINE);
	zassert_equal((uintptr_t)_sw_isr_table[TIMER_SHARED_CPU_LINE].arg,
		      (uintptr_t)TIMER_SHARED_CPU_LINE,
		      "dispatcher arg should be the L1 slot index");
}

ZTEST(mli_dispatch, test_timer_leaves_in_flat_l2_window)
{
	const unsigned int irqn0 = DT_IRQN(DT_NODELABEL(timer0));
	const unsigned int irqn1 = DT_IRQN(DT_NODELABEL(timer1));
	const unsigned int src0 = irq_from_level_2(irqn0);
	const unsigned int src1 = irq_from_level_2(irqn1);

	zassert_equal(src0, TG0_T0_LEVEL_INTR_SOURCE);
	zassert_equal(src1, TG0_T1_LEVEL_INTR_SOURCE);

	zassert_not_equal(_sw_isr_table[ESP_L2_BASE + src0].isr, z_irq_spurious,
			  "timer0 leaf at L2 base + %u", src0);
	zassert_not_equal(_sw_isr_table[ESP_L2_BASE + src1].isr, z_irq_spurious,
			  "timer1 leaf at L2 base + %u", src1);
	zassert_not_equal(_sw_isr_table[ESP_L2_BASE + src0].isr, z_soc_2nd_lvl_isr);
}

ZTEST(mli_dispatch, test_mask_tracks_enable_disable)
{
	const unsigned int irqn = DT_IRQN(DT_NODELABEL(timer0));
	const unsigned int src = irq_from_level_2(irqn);
	const unsigned int line = irq_parent_level_2(irqn);
	uint8_t before;

	zassert_equal(line, TIMER_SHARED_CPU_LINE);

	/* Counter driver irq_enable()'d at init — source bit must be set. */
	zassert_true(z_soc_irq_mli_source_enabled(line, src),
		     "timer0 source should be enabled in line mask");
	before = z_soc_irq_mli_shares_count_get(line);
	zassert_true(before >= 1U, "shared line should have enabled sources");

	irq_disable(irqn);
	zassert_false(z_soc_irq_mli_source_enabled(line, src),
		      "disabled source bit must clear");
	zassert_equal(z_soc_irq_mli_shares_count_get(line), before - 1U);

	irq_enable(irqn);
	zassert_true(z_soc_irq_mli_source_enabled(line, src),
		     "re-enabled source bit must set");
	zassert_equal(z_soc_irq_mli_shares_count_get(line), before);

	/* Idempotent enable must not bump the count twice. */
	irq_enable(irqn);
	zassert_equal(z_soc_irq_mli_shares_count_get(line), before);
}

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lcd_cam_l3_intc), okay)

#define ESP_L3_BASE      CONFIG_3RD_LVL_ISR_TBL_OFFSET
#define LCD_CAM_CPU_LINE 17

void z_soc_3rd_lvl_isr(const void *arg);

/*
 * The four flag IRQs, expanded at build time: DT_IRQN_BY_IDX() takes a literal
 * index, so the index cannot come from a loop variable.
 */
#define LCD_CAM_IRQN(i, _) DT_IRQN_BY_IDX(DT_NODELABEL(lcd_cam), i)
static const unsigned int lcd_cam_irqn[] = {LISTIFY(4, LCD_CAM_IRQN, (,))};

/*
 * An aggregator only gets a window when something actually connects one of its
 * flags. lcd_cam_l3_intc is status "okay" by default on esp32s3, but this test
 * application has no LCD_CAM consumer, so in a plain build there is nothing to
 * assert against. Skip rather than fail, so the gap stays visible in the twister
 * report instead of being compiled out.
 */
static bool l3_window_present(void)
{
	return z_isr_l3_window_num > 0U;
}

#define SKIP_WITHOUT_L3_WINDOW()                                                                   \
	do {                                                                                       \
		if (!l3_window_present()) {                                                        \
			ztest_test_skip();                                                         \
		}                                                                                  \
	} while (0)

/*
 * Rule: a CPU line hosting a 3rd-level aggregator still vectors through the
 * 2nd-level dispatcher, even when the aggregator is its only source. The
 * aggregator's flags live in the 3rd-level window, so there is no single leaf
 * that could have been placed directly on the line.
 */
ZTEST(mli_dispatch, test_l3_line_still_has_l2_dispatcher)
{
	SKIP_WITHOUT_L3_WINDOW();

	zassert_equal(_sw_isr_table[LCD_CAM_CPU_LINE].isr, z_soc_2nd_lvl_isr,
		      "CPU line %u hosts an L3 aggregator and must still vector through"
		      " z_soc_2nd_lvl_isr", LCD_CAM_CPU_LINE);
	zassert_equal((uintptr_t)_sw_isr_table[LCD_CAM_CPU_LINE].arg,
		      (uintptr_t)LCD_CAM_CPU_LINE);
}

/* The aggregator's own level-2 slot holds the status-register demux. */
ZTEST(mli_dispatch, test_l3_source_slot_has_l3_dispatcher)
{
	SKIP_WITHOUT_L3_WINDOW();

	const unsigned int src = irq_from_level_2(lcd_cam_irqn[0]);

	zassert_equal(src, LCD_CAM_INTR_SOURCE);
	zassert_equal(_sw_isr_table[ESP_L2_BASE + src].isr, z_soc_3rd_lvl_isr,
		      "L2 slot of an aggregator source must hold z_soc_3rd_lvl_isr");
	zassert_equal((uintptr_t)_sw_isr_table[ESP_L2_BASE + src].arg, 0U,
		      "the only aggregator is window 0");
}

/*
 * The window is packed densely, one slot per connected bit, so LCD_CAM's four
 * contiguous flags occupy the first four slots of the level-3 region.
 */
ZTEST(mli_dispatch, test_l3_leaves_in_dense_window)
{
	SKIP_WITHOUT_L3_WINDOW();

	for (unsigned int bit = 0; bit < ARRAY_SIZE(lcd_cam_irqn); bit++) {
		zassert_equal(irq_from_level_3(lcd_cam_irqn[bit]), bit);
		zassert_not_equal(_sw_isr_table[ESP_L3_BASE + bit].isr, z_irq_spurious,
				  "lcd_cam flag %u should sit at L3 base + %u", bit, bit);
		zassert_not_equal(_sw_isr_table[ESP_L3_BASE + bit].isr, z_soc_3rd_lvl_isr);
	}
}

/* The generated window must agree with the devicetree the driver joined it to. */
ZTEST(mli_dispatch, test_l3_window_matches_devicetree)
{
	SKIP_WITHOUT_L3_WINDOW();

	zassert_equal(z_isr_l3_window_num, 1U, "esp32s3 declares one L3 aggregator");
	zassert_equal(z_isr_l3_windows[0].l2_src, LCD_CAM_INTR_SOURCE);
	zassert_equal(z_isr_l3_windows[0].mask, 0xfU, "LCD_CAM connects bits 0..3");
	zassert_equal(z_isr_l3_windows[0].win_base, ESP_L3_BASE);
}

#if defined(CONFIG_DYNAMIC_INTERRUPTS)
/* Dense placement is only known to the generator, so runtime attach is refused. */
ZTEST(mli_dispatch, test_l3_dynamic_connect_refused)
{
	const unsigned int irqn = lcd_cam_irqn[0];

	zassert_equal(arch_irq_connect_dynamic(irqn, 0, NULL, NULL, 0), -ENOTSUP,
		      "level-3 IRQs must be connected statically");
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */

#endif /* lcd_cam_l3_intc okay */

ZTEST_SUITE(mli_dispatch, NULL, NULL, NULL, NULL, NULL);

#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS && CONFIG_SOC_SERIES_ESP32S3 */
