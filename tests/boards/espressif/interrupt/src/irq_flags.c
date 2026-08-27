/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>
#include <esp_attr.h>
#include <esp_soc_irq.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#define TEST_IRQ_PRIORITY 1

static volatile int iram_isr_count;

static void IRAM_ATTR test_isr_iram_a(const void *arg)
{
	ARG_UNUSED(arg);
	iram_isr_count++;
}

static void IRAM_ATTR test_isr_iram_b(const void *arg)
{
	ARG_UNUSED(arg);
	iram_isr_count++;
}

static void test_isr_flash(const void *arg)
{
	ARG_UNUSED(arg);
	iram_isr_count++;
}

/*
 * Find a free CPU interrupt line. The search is bounded by SOC_CPU_INTR_NUM and
 * not by CONFIG_NUM_IRQS: only the first SOC_CPU_INTR_NUM table entries are CPU
 * lines, the rest being the level-2 source window and the level-3 windows. The
 * IRAM bookkeeping under test is indexed by CPU line, so handing it a slot from
 * one of the higher windows is meaningless and is rejected.
 */
static unsigned int find_unused_irq(void)
{
	unsigned int line;

	for (line = 0; line < SOC_CPU_INTR_NUM; line++) {
		unsigned int table_idx = line + CONFIG_GEN_IRQ_START_VECTOR;

		if (_sw_isr_table[table_idx].isr == z_irq_spurious) {
			return line;
		}
	}

	zassert_unreachable("No unused CPU interrupt line found");
	return 0;
}

ZTEST(esp_irq_flags, test_shared_iram_line_mask)
{
	unsigned int irq = find_unused_irq();
	int rc;

	rc = arch_irq_connect_dynamic(irq, TEST_IRQ_PRIORITY, test_isr_iram_a, NULL,
				      ESP_INTR_FLAG_IRAM);
	zassert_equal(rc, (int)irq, "first IRAM connect failed: %d", rc);
	zassert_equal(z_soc_irq_line_total_clients_get(irq), 1);
	zassert_equal(z_soc_irq_line_non_iram_clients_get(irq), 0);
	zassert_equal(z_soc_irq_non_iram_int_mask_get(irq), 0);

	rc = arch_irq_connect_dynamic(irq, TEST_IRQ_PRIORITY, test_isr_iram_b, NULL,
				      ESP_INTR_FLAG_IRAM);
	zassert_equal(rc, (int)irq, "second IRAM connect failed: %d", rc);
	zassert_equal(z_soc_irq_line_total_clients_get(irq), 2);
	zassert_equal(z_soc_irq_line_non_iram_clients_get(irq), 0);
	zassert_equal(z_soc_irq_non_iram_int_mask_get(irq), 0);

	rc = arch_irq_disconnect_dynamic(irq, TEST_IRQ_PRIORITY, test_isr_iram_a, NULL,
				       ESP_INTR_FLAG_IRAM);
	zassert_equal(rc, 0, "first IRAM disconnect failed: %d", rc);
	zassert_equal(z_soc_irq_line_total_clients_get(irq), 1);
	zassert_equal(z_soc_irq_non_iram_int_mask_get(irq), 0);

	rc = arch_irq_disconnect_dynamic(irq, TEST_IRQ_PRIORITY, test_isr_iram_b, NULL,
				       ESP_INTR_FLAG_IRAM);
	zassert_equal(rc, 0, "second IRAM disconnect failed: %d", rc);
	zassert_equal(z_soc_irq_line_total_clients_get(irq), 0);
	zassert_equal(z_soc_irq_non_iram_int_mask_get(irq), 0);
}

ZTEST(esp_irq_flags, test_shared_mixed_registration_rejected)
{
	unsigned int irq = find_unused_irq();
	int rc;

	rc = arch_irq_connect_dynamic(irq, TEST_IRQ_PRIORITY, test_isr_iram_a, NULL,
				      ESP_INTR_FLAG_IRAM);
	zassert_equal(rc, (int)irq, "IRAM connect failed: %d", rc);

	rc = arch_irq_connect_dynamic(irq, TEST_IRQ_PRIORITY, test_isr_flash, NULL, 0);
	zassert_equal(rc, -EINVAL, "mixed non-IRAM connect should fail");
	zassert_equal(z_soc_irq_line_total_clients_get(irq), 1);
	zassert_equal(z_soc_irq_non_iram_int_mask_get(irq), 0);

	rc = arch_irq_disconnect_dynamic(irq, TEST_IRQ_PRIORITY, test_isr_iram_a, NULL,
				       ESP_INTR_FLAG_IRAM);
	zassert_equal(rc, 0, "IRAM disconnect failed: %d", rc);
}

ZTEST(esp_irq_flags, test_shared_non_iram_sets_mask)
{
	unsigned int irq = find_unused_irq();
	int rc;

	rc = arch_irq_connect_dynamic(irq, TEST_IRQ_PRIORITY, test_isr_flash, NULL, 0);
	zassert_equal(rc, (int)irq, "non-IRAM connect failed: %d", rc);
	zassert_equal(z_soc_irq_line_non_iram_clients_get(irq), 1);
	zassert_equal(z_soc_irq_non_iram_int_mask_get(irq), 1);

	rc = arch_irq_disconnect_dynamic(irq, TEST_IRQ_PRIORITY, test_isr_flash, NULL, 0);
	zassert_equal(rc, 0, "non-IRAM disconnect failed: %d", rc);
	zassert_equal(z_soc_irq_line_total_clients_get(irq), 0);
	zassert_equal(z_soc_irq_non_iram_int_mask_get(irq), 0);
}

ZTEST(esp_irq_flags, test_iram_flag_requires_iram_handler)
{
	unsigned int irq = find_unused_irq();
	int rc;

	rc = arch_irq_connect_dynamic(irq, TEST_IRQ_PRIORITY, test_isr_flash, NULL,
				      ESP_INTR_FLAG_IRAM);
	zassert_equal(rc, -EINVAL, "flash handler with IRAM flag should fail");
}

ZTEST_SUITE(esp_irq_flags, NULL, NULL, NULL, NULL, NULL);
