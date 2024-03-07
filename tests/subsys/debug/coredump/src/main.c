/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/debug/coredump.h>

#ifdef CONFIG_COVERAGE_DUMP
#include <zephyr/debug/gcov.h>
#endif


void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	ARG_UNUSED(pEsf);

	printk("%s is expected; reason = %u; halting ...\n", __func__, reason);

#ifdef CONFIG_COVERAGE_DUMP
	gcov_coverage_dump();  /* LCOV_EXCL_LINE */
#endif
	k_fatal_halt(reason);
}

void func_3(uint32_t *addr)
{
#if defined(CONFIG_BOARD_M2GL025_MIV) || \
	defined(CONFIG_BOARD_HIFIVE1) || \
	defined(CONFIG_BOARD_LONGAN_NANO) || \
	defined(CONFIG_BOARD_QEMU_XTENSA) || \
	defined(CONFIG_SOC_FAMILY_INTEL_ADSP)
	ARG_UNUSED(addr);
	/* Call k_panic() directly so Renode doesn't pause execution.
	 * Needed on ADSP as well, since null pointer derefence doesn't
	 * fault as the lowest memory region is writable. SOF uses k_panic
	 * a lot, so it's good to check that it causes a coredump.
	 */
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

int main(void)
{
	printk("Coredump: %s\n", CONFIG_BOARD);

	func_1(0);
	return 0;
}
