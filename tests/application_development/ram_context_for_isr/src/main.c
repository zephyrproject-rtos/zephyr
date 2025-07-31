/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test for driver relocation to RAM for ISRs
 *
 * This test demonstrates how to use the fake_driver and verify
 * that the callback are properly relocated to RAM.
 */

#if !defined(CONFIG_CPU_CORTEX_M)
#error project can only run on Cortex-M for now
#endif

#include <zephyr/ztest.h>
#include "fake_driver.h"

static volatile bool test_flag;

static void test_irq_callback(const struct device *dev, void *user_data)
{
	uintptr_t func_addr = (uintptr_t)test_irq_callback;
	uintptr_t driver_isr_addr, arch_isr_wrapper_addr;

	test_flag = true;

	/* Retrieve the caller address (the arch specific isr wrapper since driver isr will be
	 * optimized by the compiler)
	 */
	__asm__ volatile("mov %0, lr" : "=r"(arch_isr_wrapper_addr));

	/* retrieve the fake_driver_isr function address that was stored in user_data */
	driver_isr_addr = (uintptr_t)user_data;

	/* Check that the function and its call stack are in RAM */
	zassert_true(func_addr >= CONFIG_SRAM_BASE_ADDRESS &&
			     func_addr <= CONFIG_SRAM_BASE_ADDRESS + CONFIG_SRAM_SIZE * 1024,
		     "%s is not in RAM! Address: 0x%x", __func__, func_addr);

	zassert_true(driver_isr_addr >= CONFIG_SRAM_BASE_ADDRESS &&
			     driver_isr_addr <= CONFIG_SRAM_BASE_ADDRESS + CONFIG_SRAM_SIZE * 1024,
		     "fake_driver_isr is not in RAM! Address: 0x%x", driver_isr_addr);

	zassert_true(arch_isr_wrapper_addr >= CONFIG_SRAM_BASE_ADDRESS &&
			     arch_isr_wrapper_addr <=
				     CONFIG_SRAM_BASE_ADDRESS + CONFIG_SRAM_SIZE * 1024,
		     "arch_isr_wrapper_addr is not in RAM! Address: 0x%x", arch_isr_wrapper_addr);

	TC_PRINT("Callback function address: 0x%lx\n", func_addr);
	TC_PRINT("Driver ISR address: 0x%lx\n", driver_isr_addr);
	TC_PRINT("Arch ISR wrapper address: 0x%lx\n", arch_isr_wrapper_addr);
}

ZTEST(ram_context_for_isr, test_fake_driver_in_ram)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(fakedriver));
	const struct fake_driver_api *api = DEVICE_API_GET(fake, dev);
	uintptr_t dev_addr = (uintptr_t)dev;

	zassert_true(dev_addr >= CONFIG_SRAM_BASE_ADDRESS &&
			     dev_addr <= CONFIG_SRAM_BASE_ADDRESS + CONFIG_SRAM_SIZE * 1024,
		     "fake driver device is not in RAM! Address: 0x%x", dev_addr);

	TC_PRINT("Fake driver device address: 0x%lx\n", dev_addr);

	zassert_not_null(api, "Failed to get fake driver API");

	api->register_irq_callback(dev, test_irq_callback, NULL);
	test_flag = false;

	NVIC_SetPendingIRQ(TEST_IRQ_NUM);

	k_busy_wait(1000);

	zassert_true(test_flag, "ISR callback was not called");
}

ZTEST_SUITE(ram_context_for_isr, NULL, NULL, NULL, NULL, NULL);
