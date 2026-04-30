/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#if CONFIG_TEST_BLOAT_SIZE > 0
static const volatile uint8_t padding[CONFIG_TEST_BLOAT_SIZE] = {[0] = 0xAA};
#endif

int main(void)
{
#if CONFIG_TEST_BLOAT_SIZE > 0
	/* Reference the array so the linker cannot discard it. */
	printk("padding[0] = 0x%02x, size = %u\n", padding[0], CONFIG_TEST_BLOAT_SIZE);
#endif
	printk("MCUboot image size check sysbuild test\n");
	return 0;
}
