/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Cache management using the RISC-V Zicbom (cbo.clean, cbo.flush, cbo.inval)
 * and Zicboz (cbo.zero) standard extensions.
 *
 * cbo.inval  - invalidate a cache block (Zicbom)
 * cbo.clean  - write back a dirty cache block without invalidating (Zicbom)
 * cbo.flush  - write back and invalidate a cache block (Zicbom)
 */

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/toolchain.h>

#if defined(CONFIG_DCACHE) && defined(CONFIG_CACHE_MANAGEMENT)

void __weak arch_dcache_enable(void)
{
	/* Not controllable via standard CBO extensions */
}

void __weak arch_dcache_disable(void)
{
	/* Not controllable via standard CBO extensions */
}

static ALWAYS_INLINE void zicbo_inval_line(uintptr_t addr)
{
#if HAS_BUILTIN(__builtin_riscv_zicbom_cbo_inval)
	__builtin_riscv_zicbom_cbo_inval((void *)addr);
#else
	__asm__ volatile("cbo.inval (%0)" : : "r"(addr) : "memory");
#endif
}

static ALWAYS_INLINE void zicbo_clean_line(uintptr_t addr)
{
#if HAS_BUILTIN(__builtin_riscv_zicbom_cbo_clean)
	__builtin_riscv_zicbom_cbo_clean((void *)addr);
#else
	__asm__ volatile("cbo.clean (%0)" : : "r"(addr) : "memory");
#endif
}

static ALWAYS_INLINE void zicbo_flush_line(uintptr_t addr)
{
#if HAS_BUILTIN(__builtin_riscv_zicbom_cbo_flush)
	__builtin_riscv_zicbom_cbo_flush((void *)addr);
#else
	__asm__ volatile("cbo.flush (%0)" : : "r"(addr) : "memory");
#endif
}

int arch_dcache_invd_range(void *addr, size_t size)
{
	uintptr_t start = ROUND_DOWN((uintptr_t)addr, CONFIG_DCACHE_LINE_SIZE);
	uintptr_t end = (uintptr_t)addr + size;

	for (uintptr_t i = start; i < end; i += CONFIG_DCACHE_LINE_SIZE) {
		zicbo_inval_line(i);
	}

	return 0;
}

int arch_dcache_invd_all(void)
{
	/* Zicbom has no "invalidate all" instruction; not supported */
	return -ENOTSUP;
}

int arch_dcache_flush_range(void *addr, size_t size)
{
	uintptr_t start = ROUND_DOWN((uintptr_t)addr, CONFIG_DCACHE_LINE_SIZE);
	uintptr_t end = (uintptr_t)addr + size;

	for (uintptr_t i = start; i < end; i += CONFIG_DCACHE_LINE_SIZE) {
		zicbo_clean_line(i);
	}

	return 0;
}

int arch_dcache_flush_all(void)
{
	/* Zicbom has no "clean all" instruction; not supported */
	return -ENOTSUP;
}

int arch_dcache_flush_and_invd_range(void *addr, size_t size)
{
	uintptr_t start = ROUND_DOWN((uintptr_t)addr, CONFIG_DCACHE_LINE_SIZE);
	uintptr_t end = (uintptr_t)addr + size;

	for (uintptr_t i = start; i < end; i += CONFIG_DCACHE_LINE_SIZE) {
		zicbo_flush_line(i);
	}

	return 0;
}

int arch_dcache_flush_and_invd_all(void)
{
	/* Zicbom has no "flush all" instruction; not supported */
	return -ENOTSUP;
}

void __weak arch_cache_init(void)
{
	/* Nothing to initialise for standard CBO extensions */
}

#endif /* CONFIG_DCACHE && CONFIG_CACHE_MANAGEMENT */
