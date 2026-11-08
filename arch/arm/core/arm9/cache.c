/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv5 cache maintenance
 *
 * ARMv5 predates the set/way encoding used from ARMv7 on; all maintenance is
 * done through the CP15 c7 operations, which take either no argument
 * ("entire cache") or an MVA. Range operations are therefore a simple loop
 * over cache lines.
 *
 * Note that "clean" by MVA is an ARMv5 *TE* addition.
 */

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/arch/cache.h>
#include <zephyr/arch/arm/arm9/lib_helpers.h>
#include <zephyr/sys/util.h>

#define LINE_SIZE CONFIG_DCACHE_LINE_SIZE

static ALWAYS_INLINE void dcache_op_range(void *start_addr, size_t size, int op)
{
	uintptr_t addr = ROUND_DOWN((uintptr_t)start_addr, LINE_SIZE);
	uintptr_t end = (uintptr_t)start_addr + size;

	for (; addr < end; addr += LINE_SIZE) {
		switch (op) {
		case 0: /* clean */
			MCR15(c7, 0, c10, 1, addr);
			break;
		case 1: /* invalidate */
			MCR15(c7, 0, c6, 1, addr);
			break;
		default: /* clean and invalidate */
			MCR15(c7, 0, c14, 1, addr);
			break;
		}
	}

	__DSB();
}

size_t arch_dcache_line_size_get(void)
{
	return LINE_SIZE;
}

void arch_dcache_enable(void)
{
	uint32_t sctlr = __get_SCTLR();

	if ((sctlr & SCTLR_C_BIT) != 0U) {
		return;
	}

	/* Invalidate before enabling so that reset garbage is not used. */
	MCR15(c7, 0, c6, 0, 0);
	__set_SCTLR(sctlr | SCTLR_C_BIT | SCTLR_W_BIT);
	__ISB();
}

void arch_dcache_disable(void)
{
	uint32_t sctlr = __get_SCTLR();

	arch_dcache_flush_and_invd_all();
	__set_SCTLR(sctlr & ~SCTLR_C_BIT);
	__ISB();
}

int arch_dcache_flush_all(void)
{
	/*
	 * ARMv5 has no "clean entire data cache" operation that works for a
	 * write-back cache without walking it, so use the test-clean loop
	 * (CP15 c7 c10 3).
	 */
	__asm__ volatile("1:	mrc	p15, 0, r15, c7, c10, 3\n\t"
			 "	bne	1b"
			 : /* no outputs */
			 : /* no inputs */
			 : "cc", "memory");
	__DSB();

	return 0;
}

int arch_dcache_invd_all(void)
{
	MCR15(c7, 0, c6, 0, 0);
	__DSB();

	return 0;
}

int arch_dcache_flush_and_invd_all(void)
{
	__asm__ volatile("1:	mrc	p15, 0, r15, c7, c14, 3\n\t"
			 "	bne	1b"
			 : /* no outputs */
			 : /* no inputs */
			 : "cc", "memory");
	__DSB();

	return 0;
}

int arch_dcache_flush_range(void *start_addr, size_t size)
{
	dcache_op_range(start_addr, size, 0);

	return 0;
}

int arch_dcache_invd_range(void *start_addr, size_t size)
{
	dcache_op_range(start_addr, size, 1);

	return 0;
}

int arch_dcache_flush_and_invd_range(void *start_addr, size_t size)
{
	dcache_op_range(start_addr, size, 2);

	return 0;
}

size_t arch_icache_line_size_get(void)
{
	return CONFIG_ICACHE_LINE_SIZE;
}

void arch_icache_enable(void)
{
	uint32_t sctlr = __get_SCTLR();

	MCR15(c7, 0, c5, 0, 0);
	__set_SCTLR(sctlr | SCTLR_I_BIT);
	__ISB();
}

void arch_icache_disable(void)
{
	__set_SCTLR(__get_SCTLR() & ~SCTLR_I_BIT);
	__ISB();
}

int arch_icache_flush_all(void)
{
	return -ENOTSUP;
}

int arch_icache_invd_all(void)
{
	MCR15(c7, 0, c5, 0, 0);
	__ISB();

	return 0;
}

int arch_icache_flush_and_invd_all(void)
{
	return arch_icache_invd_all();
}

int arch_icache_flush_range(void *start_addr, size_t size)
{
	ARG_UNUSED(start_addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

int arch_icache_invd_range(void *start_addr, size_t size)
{
	uintptr_t addr = ROUND_DOWN((uintptr_t)start_addr, CONFIG_ICACHE_LINE_SIZE);
	uintptr_t end = (uintptr_t)start_addr + size;

	for (; addr < end; addr += CONFIG_ICACHE_LINE_SIZE) {
		MCR15(c7, 0, c5, 1, addr);
	}
	__ISB();

	return 0;
}

int arch_icache_flush_and_invd_range(void *start_addr, size_t size)
{
	return arch_icache_invd_range(start_addr, size);
}

void arch_cache_init(void)
{
	/*
	 * Caches are only usable once the MMU provides the cacheable
	 * attribute, so leave them off unless the MMU is in.
	 */
#ifdef CONFIG_ARMV5_FLAT_MMU
	if (IS_ENABLED(CONFIG_ICACHE)) {
		arch_icache_enable();
	}
	if (IS_ENABLED(CONFIG_DCACHE)) {
		arch_dcache_enable();
	}
#endif
}
