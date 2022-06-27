/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/debug/coredump.h>

void func_3(uint32_t *addr)
{
#if defined(CONFIG_BOARD_M2GL025_MIV) || \
	defined(CONFIG_BOARD_HIFIVE1) || \
	defined(CONFIG_BOARD_LONGAN_NANO) || \
	defined(CONFIG_BOARD_LONGAN_NANO_LITE)
	ARG_UNUSED(addr);
	/* Call k_panic() directly so Renode doesn't pause execution */
	k_panic();
#elif !defined(CONFIG_CPU_CORTEX_M)
	/* For null pointer reference */
	*addr = 0;
#else
	ARG_UNUSED(addr);
	/* Dereferencing null-pointer in TrustZone-enabled
	 * builds may crash the system, so use, instead an
	 * undefined instruction to trigger a CPU fault.
	 */
	__asm__ volatile("udf #0" : : : );
#endif
}

void func_2(uint32_t *addr)
{
	func_3(addr);
}

void func_1(uint32_t *addr)
{
	func_2(addr);
}

void main(void)
{
	printk("Coredump: %s\n", CONFIG_BOARD);

	func_1(0);
}
