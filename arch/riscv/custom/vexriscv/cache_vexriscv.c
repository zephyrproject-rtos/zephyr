/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/cache.h>

#ifdef CONFIG_DCACHE
void arch_dcache_enable(void)
{
	/* Nothing */
}

void arch_dcache_disable(void)
{
	/* Nothing */
}

int arch_dcache_invd_all(void)
{
	/* Invalidate whole data cache instruction: 0x500F
	 * https://github.com/SpinalHDL/VexRiscv?tab=readme-ov-file#dbuscachedplugin
	 */
	__asm__ volatile(".insn 0x500F\n");

	return 0;
}

int arch_dcache_invd_range(void *addr, size_t size)
{
	/* Invalidate cache line instruction: 0x500f | (rs1 << 15)
	 * https://github.com/SpinalHDL/VexRiscv?tab=readme-ov-file#dbuscachedplugin
	 */
	__asm__ volatile(
		"mv a0, %1\n"
		"j 2f\n"
		"3:\n"
		".insn 0x5500F\n" /* 0x500f | (a0 << 15) */
		"add a0, a0, %0\n"
		"2:\n"
		"bltu a0, %2, 3b\n"
		: : "r"(CONFIG_DCACHE_LINE_SIZE),
			"r"((unsigned int)(addr) & ~((CONFIG_DCACHE_LINE_SIZE) - 1UL)),
			"r"((unsigned int)(addr) + (size))
		: "a0");

	return 0;
}


int arch_dcache_flush_all(void)
{
	/* VexRiscv cache is write-through */
	return 0;
}

int arch_dcache_flush_range(void *addr __unused, size_t size __unused)
{
	return 0;
}

int arch_dcache_flush_and_invd_all(void)
{
	return arch_dcache_invd_all();
}

int arch_dcache_flush_and_invd_range(void *addr, size_t size)
{
	return arch_dcache_invd_range(addr, size);
}
#endif /* CONFIG_DCACHE */

#ifdef CONFIG_ICACHE
void arch_icache_enable(void)
{
	/* Nothing */
}

void arch_icache_disable(void)
{
	/* Nothing */
}

int arch_icache_flush_all(void)
{
	__asm__ volatile("fence.i\n");

	return 0;
}

int arch_icache_invd_all(void)
{
	return arch_icache_flush_all();
}

int arch_icache_invd_range(void *addr_in __unused, size_t size __unused)
{
	return arch_icache_flush_all();
}

int arch_icache_flush_and_invd_all(void)
{
	return arch_icache_flush_all();
}

int arch_icache_flush_range(void *addr __unused, size_t size __unused)
{
	return arch_icache_flush_all();
}

int arch_icache_flush_and_invd_range(void *addr __unused, size_t size __unused)
{
	return arch_icache_flush_all();
}
#endif /* CONFIG_ICACHE */

void arch_cache_init(void)
{
	/* Nothing */
}
