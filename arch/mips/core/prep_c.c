/*
 * Copyright (c) 2020 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 */

#include <zephyr/irq.h>
#include <mips/mipsregs.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/arch/cache.h>
#include <zephyr/arch/common/xip.h>
#include <zephyr/arch/common/init.h>

/*
 * Line size encoded in CP0 Config1: 0 means no cache, otherwise 2 << n bytes.
 */
static unsigned long cache_line_size(uint32_t field)
{
	return (field == 0U) ? 0UL : (2UL << field);
}

/*
 * The vector is written through the data cache but fetched through the
 * instruction cache, so push it out to memory and drop any stale
 * instructions before the first exception.
 */
static void sync_icache(unsigned long addr, unsigned long size)
{
	uint32_t config1 = read_c0_config1();
	unsigned long dline = cache_line_size((config1 & CONF1_DL_MASK) >> CONF1_DL_SHIFT);
	unsigned long iline = cache_line_size((config1 & CONF1_IL_MASK) >> CONF1_IL_SHIFT);
	unsigned long end = addr + size;
	unsigned long p;

	if (dline != 0UL) {
		for (p = addr & ~(dline - 1UL); p < end; p += dline) {
			/* Hit_Writeback_Inv_D */
			__asm__ volatile("cache 0x15, 0(%0)" : : "r" (p) : "memory");
		}
		__asm__ volatile("sync" : : : "memory");
	}

	if (iline != 0UL) {
		for (p = addr & ~(iline - 1UL); p < end; p += iline) {
			/* Hit_Invalidate_I */
			__asm__ volatile("cache 0x10, 0(%0)" : : "r" (p) : "memory");
		}
		__asm__ volatile("sync\n\tehb" : : : "memory");
	}
}

static void interrupt_init(void)
{
	extern char __isr_vec[];
	extern uint32_t mips_cp0_status_int_mask;
	unsigned long ebase;

	irq_lock();

	mips_cp0_status_int_mask = 0;

	ebase = 0x80000000;

	memcpy((void *)(ebase + 0x180), __isr_vec, 0x80);
	sync_icache(ebase + 0x180, 0x80);

	/*
	 * Disable boot exception vector in BOOTROM,
	 * use exception vector in RAM.
	 */
	write_c0_status(read_c0_status() & ~(ST0_BEV));
}

/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * @return N/A
 */

FUNC_NORETURN void z_prep_c(void)
{
	soc_prep_hook();

	arch_bss_zero();

	interrupt_init();
#if CONFIG_ARCH_CACHE
	arch_cache_init();
#endif

	z_cstart();
	CODE_UNREACHABLE;
}
