/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TriCore L1 cache management
 *
 * The program and data caches are bypassed after reset (PCON0.PCBYP,
 * DCON0.DCBYP). The data cache is write-back and not coherent with other
 * bus masters. Line operations use CACHEA.* on an address; whole-cache
 * operations use CACHEI.* on every index and way, which take the way in
 * address bit 0 and the index in bits 12:5 of a cacheable address
 * (TC3xx UM v2.0.0, 5.3.6.1.3; TC4Dx UM v1.1, 5.3.4.2).
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#define CON0_BYPASS BIT(1)
#define PCON1_PCINV BIT(0)

#define CACHE_INDEX_POS 5
#define CACHE_INDICES   256
#define CACHE_WAYS      2

/* Cacheable address (segment 8) for the CACHEI.* index operations */
#define CACHE_INDEX_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_flash))

BUILD_ASSERT((CACHE_INDEX_BASE >> 28) == 0x8, "CACHEI.* needs a cached segment 8 address");

unsigned int z_tricore_cpu_endinit_unlock(void);
void z_tricore_cpu_endinit_lock(unsigned int key);

/* DCON0 and PCON0 are CPU ENDINIT protected on TC3x */
__weak unsigned int z_tricore_cpu_endinit_unlock(void)
{
	return 0;
}

__weak void z_tricore_cpu_endinit_lock(unsigned int key)
{
	ARG_UNUSED(key);
}

#define CACHE_LINE_OP(insn, start_addr, size)                                                      \
	do {                                                                                       \
		uintptr_t addr = ROUND_DOWN(POINTER_TO_UINT(start_addr), CONFIG_DCACHE_LINE_SIZE); \
		uintptr_t end = POINTER_TO_UINT(start_addr) + (size);                              \
                                                                                                   \
		for (; addr < end; addr += CONFIG_DCACHE_LINE_SIZE) {                              \
			__asm__ volatile(insn " [%0]0" : : "a"(addr) : "memory");                  \
		}                                                                                  \
		__asm__ volatile("dsync" : : : "memory");                                          \
	} while (false)

#define CACHE_INDEX_OP(insn)                                                                       \
	do {                                                                                       \
		for (uint32_t index = 0; index < CACHE_INDICES; index++) {                         \
			for (uint32_t way = 0; way < CACHE_WAYS; way++) {                          \
				uintptr_t addr =                                                   \
					CACHE_INDEX_BASE | (index << CACHE_INDEX_POS) | way;       \
                                                                                                   \
				__asm__ volatile(insn " [%0]0" : : "a"(addr) : "memory");          \
			}                                                                          \
		}                                                                                  \
		__asm__ volatile("dsync" : : : "memory");                                          \
	} while (false)

void arch_cache_init(void)
{
}

void arch_dcache_enable(void)
{
	unsigned int key;

	CACHE_INDEX_OP("cachei.i");

	key = z_tricore_cpu_endinit_unlock();
	cr_write(TRICORE_DCON0, cr_read(TRICORE_DCON0) & ~CON0_BYPASS);
	z_tricore_cpu_endinit_lock(key);
}

void arch_dcache_disable(void)
{
	unsigned int key;

	/* Write back the dirty lines before the cache is bypassed */
	CACHE_INDEX_OP("cachei.wi");

	key = z_tricore_cpu_endinit_unlock();
	cr_write(TRICORE_DCON0, cr_read(TRICORE_DCON0) | CON0_BYPASS);
	z_tricore_cpu_endinit_lock(key);
}

int arch_dcache_flush_all(void)
{
	CACHE_INDEX_OP("cachei.w");

	return 0;
}

int arch_dcache_invd_all(void)
{
	CACHE_INDEX_OP("cachei.i");

	return 0;
}

int arch_dcache_flush_and_invd_all(void)
{
	CACHE_INDEX_OP("cachei.wi");

	return 0;
}

int arch_dcache_flush_range(void *start_addr, size_t size)
{
	CACHE_LINE_OP("cachea.w", start_addr, size);

	return 0;
}

int arch_dcache_invd_range(void *start_addr, size_t size)
{
	CACHE_LINE_OP("cachea.i", start_addr, size);

	return 0;
}

int arch_dcache_flush_and_invd_range(void *start_addr, size_t size)
{
	CACHE_LINE_OP("cachea.wi", start_addr, size);

	return 0;
}

void arch_icache_enable(void)
{
	unsigned int key;

	(void)arch_icache_invd_all();

	key = z_tricore_cpu_endinit_unlock();
	cr_write(TRICORE_PCON0, cr_read(TRICORE_PCON0) & ~CON0_BYPASS);
	z_tricore_cpu_endinit_lock(key);
}

void arch_icache_disable(void)
{
	unsigned int key = z_tricore_cpu_endinit_unlock();

	cr_write(TRICORE_PCON0, cr_read(TRICORE_PCON0) | CON0_BYPASS);
	z_tricore_cpu_endinit_lock(key);
}

/* The program cache is never dirty */
int arch_icache_flush_all(void)
{
	return -ENOTSUP;
}

/* PCON1.PCINV invalidates all lines within 64 cycles (TC3xx UM, PMI) */
int arch_icache_invd_all(void)
{
	cr_write(TRICORE_PCON1, PCON1_PCINV);

	while ((cr_read(TRICORE_PCON1) & PCON1_PCINV) != 0U) {
	}

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

/* There is no line invalidation for the program cache */
int arch_icache_invd_range(void *start_addr, size_t size)
{
	ARG_UNUSED(start_addr);
	ARG_UNUSED(size);

	return arch_icache_invd_all();
}

int arch_icache_flush_and_invd_range(void *start_addr, size_t size)
{
	return arch_icache_invd_range(start_addr, size);
}
