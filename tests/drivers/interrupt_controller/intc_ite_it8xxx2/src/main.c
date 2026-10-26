/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/dt-bindings/interrupt-controller/ite-intc.h>
#include <zephyr/ztest.h>

#include <soc.h>

volatile uint8_t fake_intc_regs[0x100];
struct wdt_it8xxx2_regs fake_wdt_regs;
struct gctrl_it8xxx2_regs fake_gctrl_regs;
volatile uint8_t fake_smb_slviselr;

static void it8xxx2_intc_before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset((void *)fake_intc_regs, 0, sizeof(fake_intc_regs));
	memset(&fake_wdt_regs, 0, sizeof(fake_wdt_regs));
	memset(&fake_gctrl_regs, 0, sizeof(fake_gctrl_regs));
	fake_smb_slviselr = 0;
	soc_interrupt_init();
}

ZTEST_SUITE(it8xxx2_intc, NULL, NULL, it8xxx2_intc_before, NULL, NULL);

ZTEST(it8xxx2_intc, test_max_valid_irq_185)
{
	const unsigned int max_valid_irq = CONFIG_NUM_IRQS - 1;

	zassert_equal(CONFIG_NUM_IRQS, 186);
	zassert_equal(ite_intc_irq_is_enable(max_valid_irq), 0);

	ite_intc_irq_enable(max_valid_irq);
	zassert_not_equal(ite_intc_irq_is_enable(max_valid_irq), 0);

	ite_intc_irq_disable(max_valid_irq);
	zassert_equal(ite_intc_irq_is_enable(max_valid_irq), 0);
}

ZTEST(it8xxx2_intc, test_oob_irq_bounds_rejected)
{
	const unsigned int oob_irq = CONFIG_NUM_IRQS;
	uint8_t zero_regs[sizeof(fake_intc_regs)] = { 0 };

	/*
	 * Verify that irq == CONFIG_NUM_IRQS and irq > CONFIG_NUM_IRQS are
	 * rejected by all INTC operations without mutating any hardware
	 * register.
	 */
	ite_intc_irq_enable(oob_irq);
	ite_intc_irq_polarity_set(oob_irq, IRQ_TYPE_EDGE_FALLING);
	ite_intc_isr_clear(oob_irq);
	zassert_equal(ite_intc_irq_is_enable(oob_irq), 0);
	zassert_mem_equal((const void *)fake_intc_regs, zero_regs,
			  sizeof(fake_intc_regs));

	ite_intc_irq_enable(oob_irq + 1);
	ite_intc_irq_polarity_set(oob_irq + 1, IRQ_TYPE_LEVEL_HIGH);
	ite_intc_isr_clear(oob_irq + 1);
	zassert_equal(ite_intc_irq_is_enable(oob_irq + 1), 0);
	zassert_mem_equal((const void *)fake_intc_regs, zero_regs,
			  sizeof(fake_intc_regs));

	/*
	 * Pre-fill registers with 0xFF and verify out-of-bounds disable and
	 * is_enable still reject irq == CONFIG_NUM_IRQS without touching
	 * registers or returning enabled.
	 */
	memset((void *)fake_intc_regs, 0xFF, sizeof(fake_intc_regs));
	zassert_equal(ite_intc_irq_is_enable(oob_irq), 0);
	ite_intc_irq_disable(oob_irq);
	for (size_t i = 0; i < sizeof(fake_intc_regs); i++) {
		zassert_equal(fake_intc_regs[i], 0xFF,
			      "reg[0x%zx] modified on OOB disable", i);
	}
}
