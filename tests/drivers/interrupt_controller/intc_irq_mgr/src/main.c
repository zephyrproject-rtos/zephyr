/*
 * Copyright (c) 2024 Meta Platforms.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include "config.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq_mgr.h>
#include <zephyr/ztest.h>

#define DT_DRV_COMPAT zephyr_irq_manager

#define IRQ_MGMT_DEV  (DEVICE_DT_INST_GET(0))
#define TEST_IRQ_BASE (IRQ_TO_L2(TEST_DT_IRQ_BASE) | 11)

ZTEST_SUITE(intc_irq_mgr, NULL, NULL, NULL, NULL, NULL);

ZTEST(intc_irq_mgr, test_intc_irq_mgr)
{
	uint32_t irq_base, nr_irqs;

	/* Allocate NR_IRQS IRQs */
	irq_base = 0;
	zassert_ok(irq_mgr_alloc(IRQ_MGMT_DEV, &irq_base, TEST_NR_IRQS));
	zassert_equal(irq_base, TEST_IRQ_BASE);

	/* No more IRQs available to allocate, should fail */
	zassert_not_ok(irq_mgr_alloc(IRQ_MGMT_DEV, &irq_base, 1));

	/* Free second half of IRQs */
	nr_irqs = TEST_NR_IRQS / 2;
	irq_base = TEST_IRQ_BASE + nr_irqs - 1;
	zassert_ok(irq_mgr_free(IRQ_MGMT_DEV, irq_base, nr_irqs));

	/* Allocate NR_IRQS/2 IRQs */
	irq_base = 0;
	zassert_ok(irq_mgr_alloc(IRQ_MGMT_DEV, &irq_base, nr_irqs));
	/* Allocated IRQ should start from `irq_base` */
	zassert_equal(irq_base, irq_base);

	/* Just to be sure - allocate one more after full should fail */
	zassert_not_ok(irq_mgr_alloc(IRQ_MGMT_DEV, &irq_base, 1));
}
