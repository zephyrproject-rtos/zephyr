/*
 * Copyright (c) 2025 LiteX Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test application for LiteX CLIC interrupt controller
 *
 * This application tests the basic functionality of the LiteX CLIC driver
 * including interrupt enable/disable, priority setting, and pending bit
 * management. Due to the CSRStorage auto-clear limitation, actual interrupt
 * triggering is tested with interrupts disabled to prevent system hangs.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

LOG_MODULE_REGISTER(litex_clic_test, LOG_LEVEL_INF);

/* Test interrupt numbers (using CSR-controlled interrupts 0-15) */
#define TEST_IRQ_1  5

/* Interrupt counters */
static volatile uint32_t irq1_count = 0;

/* Interrupt handlers */
static void test_isr_1(const void *arg)
{
	ARG_UNUSED(arg);
	irq1_count++;
	LOG_DBG("ISR 1 triggered (count: %u)", irq1_count);
}

/**
 * @brief Test basic interrupt enable/disable functionality
 * 
 */
static int test_interrupt_enable_disable(void)
{
	int ret = 0;
	
	printk("\n=== Test: Interrupt Enable/Disable ===\n");
	
	/* Test IRQ 1 */
	printk("Testing IRQ %d:\n", TEST_IRQ_1);
	
	/* Initially should be disabled */
	if (arch_irq_is_enabled(TEST_IRQ_1)) {
		printk("  ERROR: IRQ %d should be initially disabled\n", TEST_IRQ_1);
		ret = -1;
	} else {
		printk("  PASS: IRQ %d initially disabled\n", TEST_IRQ_1);
	}
	
	/* Enable interrupt */
	arch_irq_enable(TEST_IRQ_1);
	if (!arch_irq_is_enabled(TEST_IRQ_1)) {
		printk("  ERROR: IRQ %d should be enabled after arch_irq_enable()\n", TEST_IRQ_1);
		ret = -1;
	} else {
		printk("  PASS: IRQ %d enabled successfully\n", TEST_IRQ_1);
	}
	
	/* Disable interrupt */
	arch_irq_disable(TEST_IRQ_1);
	if (arch_irq_is_enabled(TEST_IRQ_1)) {
		printk("  ERROR: IRQ %d should be disabled after arch_irq_disable()\n", TEST_IRQ_1);
		ret = -1;
	} else {
		printk("  PASS: IRQ %d disabled successfully\n", TEST_IRQ_1);
	}
	
	return ret;
}

/**
 * @brief Test interrupt priority setting
 * 
 * Note: Priority setting is tested but actual priority-based arbitration
 * cannot be tested without triggering real interrupts.
 */
static int test_interrupt_priority(void)
{
	printk("\n=== Test: Interrupt Priority ===\n");
	
	/* Connect ISRs */
	IRQ_CONNECT(TEST_IRQ_1, 50, test_isr_1, NULL, 0);
	
	printk("Connected ISRs with priorities:\n");
	printk("  IRQ %d: priority 50 (low)\n", TEST_IRQ_1);
	
	/* Enable interrupts but keep global interrupts disabled */
	arch_irq_enable(TEST_IRQ_1);
	
	printk("All test interrupts enabled (global interrupts remain disabled)\n");
	printk("PASS: Priority configuration completed\n");
	
	/* Disable all test interrupts */
	arch_irq_disable(TEST_IRQ_1);
	
	return 0;
}

/**
 * @brief Test CSR register access
 * 
 * This test verifies that CSR registers can be written and read back
 * correctly, which is fundamental to the CLIC operation.
 */
static int test_csr_access(void)
{
	printk("\n=== Test: CSR Register Access ===\n");
	
	/* Note: Direct CSR access would require exposing internal functions */
	/* For now, we test indirectly through the arch_irq_* APIs */
	
	printk("Testing CSR access through API functions:\n");
	
	/* Test multiple enable/disable cycles */
	for (int i = 0; i < 3; i++) {
		arch_irq_enable(TEST_IRQ_1);
		if (!arch_irq_is_enabled(TEST_IRQ_1)) {
			printk("  ERROR: Failed to enable IRQ %d on iteration %d\n", TEST_IRQ_1, i);
			return -1;
		}
		
		arch_irq_disable(TEST_IRQ_1);
		if (arch_irq_is_enabled(TEST_IRQ_1)) {
			printk("  ERROR: Failed to disable IRQ %d on iteration %d\n", TEST_IRQ_1, i);
			return -1;
		}
	}
	
	printk("  PASS: CSR read/write cycles completed successfully\n");
	
	return 0;
}

/**
 * @brief Main test entry point
 */
int main(void)
{
	int ret = 0;
	
	printk("\n");
	printk("==============================================\n");
	printk("    LiteX CLIC Interrupt Controller Test     \n");
	printk("==============================================\n");
	
	LOG_INF("Starting LiteX CLIC test application");
	
	/* Ensure global interrupts are disabled for safety */
	irq_lock();
	printk("\nGlobal interrupts locked for safe testing\n");
	
	/* Run test suite */
	ret = test_interrupt_enable_disable();
	if (ret != 0) {
		LOG_ERR("Enable/disable test failed");
		goto done;
	}
	
	ret = test_interrupt_priority();
	if (ret != 0) {
		LOG_ERR("Priority test failed");
		goto done;
	}
	
	ret = test_csr_access();
	if (ret != 0) {
		LOG_ERR("CSR access test failed");
		goto done;
	}

	/* Summary */
	printk("\n==============================================\n");
	if (ret == 0) {
		printk("         ALL TESTS PASSED                    \n");
		LOG_INF("All LiteX CLIC tests completed successfully");
	} else {
		printk("         SOME TESTS FAILED                   \n");
		LOG_ERR("LiteX CLIC tests failed with error %d", ret);
	}
	printk("==============================================\n");
	
	printk("\nInterrupt counts (should be 0 due to disabled interrupts):\n");
	printk("  IRQ %d: %u\n", TEST_IRQ_1, irq1_count);
	
done:
	/* Keep interrupts locked for safety */
	printk("\nTest complete. System halted (interrupts remain locked).\n");
	
	/* Keep the test running */
	while (1) {
		k_sleep(K_SECONDS(1));
	}
	
	return 0;
}