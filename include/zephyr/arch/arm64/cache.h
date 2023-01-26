/*
 * Copyright 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_CACHE_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_CACHE_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/cpu.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define K_CACHE_WB		BIT(0)
#define K_CACHE_INVD		BIT(1)
#define K_CACHE_WB_INVD		(K_CACHE_WB | K_CACHE_INVD)

#if defined(CONFIG_DCACHE)

extern int arm64_dcache_range(void *addr, size_t size, int op);
extern int arm64_dcache_all(int op);

extern size_t arch_dcache_line_size_get(void);

int ALWAYS_INLINE arch_dcache_flush_all(void)
{
	return arm64_dcache_all(K_CACHE_WB);
}

int ALWAYS_INLINE arch_dcache_invd_all(void)
{
	return arm64_dcache_all(K_CACHE_INVD);
}

int ALWAYS_INLINE arch_dcache_flush_and_invd_all(void)
{
	return arm64_dcache_all(K_CACHE_WB_INVD);
}

int ALWAYS_INLINE arch_dcache_flush_range(void *addr, size_t size)
{
	return arm64_dcache_range(addr, size, K_CACHE_WB);
}

int ALWAYS_INLINE arch_dcache_invd_range(void *addr, size_t size)
{
	return arm64_dcache_range(addr, size, K_CACHE_INVD);
}

int ALWAYS_INLINE arch_dcache_flush_and_invd_range(void *addr, size_t size)
{
	return arm64_dcache_range(addr, size, K_CACHE_WB_INVD);
}

void ALWAYS_INLINE arch_dcache_enable(void)
{
	/* nothing */
}

void ALWAYS_INLINE arch_dcache_disable(void)
{
	/* nothing */
}

#endif /* CONFIG_DCACHE */

#if defined(CONFIG_ICACHE)

size_t arch_icache_line_size_get(void)
{
	return -ENOTSUP;
}

int ALWAYS_INLINE arch_icache_flush_all(void)
{
	return -ENOTSUP;
}

int ALWAYS_INLINE arch_icache_invd_all(void)
{
	return -ENOTSUP;
}

int ALWAYS_INLINE arch_icache_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

int ALWAYS_INLINE arch_icache_flush_range(void *addr, size_t size)
{
	return -ENOTSUP;
}

int ALWAYS_INLINE arch_icache_invd_range(void *addr, size_t size)
{
	return -ENOTSUP;
}

int ALWAYS_INLINE arch_icache_flush_and_invd_range(void *addr, size_t size)
{
	return -ENOTSUP;
}

void ALWAYS_INLINE arch_icache_enable(void)
{
	/* nothing */
}

void ALWAYS_INLINE arch_icache_disable(void)
{
	/* nothing */
}

#endif /* CONFIG_ICACHE */
#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_CACHE_H_ */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
