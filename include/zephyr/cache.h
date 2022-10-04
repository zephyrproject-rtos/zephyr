/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CACHE_H_
#define ZEPHYR_INCLUDE_CACHE_H_

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Common operations for the caches
 *
 * WB means write-back and intends to transfer dirty cache lines to memory in a
 * copy-back cache policy. May be a no-op in write-back cache policy.
 *
 * INVD means invalidate and will mark cache lines as not valid. A future
 * access to the associated address is guaranteed to generate a memory fetch.
 */

#define K_CACHE_WB	BIT(0)
#define K_CACHE_INVD	BIT(1)
#define K_CACHE_WB_INVD	(K_CACHE_WB | K_CACHE_INVD)

#if defined(CONFIG_HAS_EXTERNAL_CACHE)

/* Driver interface mirrored in include/drivers/cache.h */

/* Enable d-cache */
extern void cache_data_enable(void);

/* Disable d-cache */
extern void cache_data_disable(void);

/* Enable i-cache */
extern void cache_instr_enable(void);

/* Disable i-cache */
extern void cache_instr_disable(void);

/* Write-back / Invalidate / Write-back + Invalidate all d-cache */
extern int cache_data_all(int op);

/* Write-back / Invalidate / Write-back + Invalidate d-cache lines */
extern int cache_data_range(void *addr, size_t size, int op);

/* Write-back / Invalidate / Write-back + Invalidate all i-cache */
extern int cache_instr_all(int op);

/* Write-back / Invalidate / Write-back + Invalidate i-cache lines */
extern int cache_instr_range(void *addr, size_t size, int op);

#else

/* Hooks into arch code */

#define cache_data_enable			arch_dcache_enable
#define cache_data_disable			arch_dcache_disable
#define cache_instr_enable			arch_icache_enable
#define cache_instr_disable			arch_icache_disable
#define cache_data_all(op)			arch_dcache_all(op)
#define cache_data_range(addr, size, op)	arch_dcache_range(addr, size, op)
#define cache_instr_all(op)			arch_icache_all(op)
#define cache_instr_range(addr, size, op)	arch_icache_range(addr, size, op)
#define cache_data_line_size_get		arch_dcache_line_size_get
#define cache_instr_line_size_get		arch_icache_line_size_get

#endif

__syscall int sys_cache_data_all(int op);
static inline int z_impl_sys_cache_data_all(int op)
{
#if defined(CONFIG_CACHE_MANAGEMENT)
	return cache_data_all(op);
#endif
	ARG_UNUSED(op);

	return -ENOTSUP;
}

__syscall int sys_cache_data_range(void *addr, size_t size, int op);
static inline int z_impl_sys_cache_data_range(void *addr, size_t size, int op)
{
#if defined(CONFIG_CACHE_MANAGEMENT)
	return cache_data_range(addr, size, op);
#endif
	ARG_UNUSED(addr);
	ARG_UNUSED(size);
	ARG_UNUSED(op);

	return -ENOTSUP;
}

__syscall int sys_cache_instr_all(int op);
static inline int z_impl_sys_cache_instr_all(int op)
{
#if defined(CONFIG_CACHE_MANAGEMENT)
	return cache_instr_all(op);
#endif
	ARG_UNUSED(op);

	return -ENOTSUP;
}

__syscall int sys_cache_instr_range(void *addr, size_t size, int op);
static inline int z_impl_sys_cache_instr_range(void *addr, size_t size, int op)
{
#if defined(CONFIG_CACHE_MANAGEMENT)
	return cache_instr_range(addr, size, op);
#endif
	ARG_UNUSED(addr);
	ARG_UNUSED(size);
	ARG_UNUSED(op);

	return -ENOTSUP;
}

#ifdef CONFIG_LIBMETAL
static inline void sys_cache_flush(void *addr, size_t size)
{
	sys_cache_data_range(addr, size, K_CACHE_WB);
}
#endif

#define CPU DT_PATH(cpus, cpu_0)

/**
 *
 * @brief Get the d-cache line size.
 *
 * The API is provided to get the d-cache line size.
 *
 * @return size of the d-cache line or 0 if the d-cache is not enabled.
 */
static inline size_t sys_cache_data_line_size_get(void)
{
#ifdef CONFIG_DCACHE_LINE_SIZE_DETECT
	return cache_data_line_size_get();
#elif (CONFIG_DCACHE_LINE_SIZE != 0)
	return CONFIG_DCACHE_LINE_SIZE;
#else
	return DT_PROP_OR(CPU, d_cache_line_size, 0);
#endif
}

/*
 *
 * @brief Get the i-cache line size.
 *
 * The API is provided to get the i-cache line size.
 *
 * @return size of the i-cache line or 0 if the i-cache is not enabled.
 */
static inline size_t sys_cache_instr_line_size_get(void)
{
#ifdef CONFIG_ICACHE_LINE_SIZE_DETECT
	return cache_instr_line_size_get();
#elif (CONFIG_ICACHE_LINE_SIZE != 0)
	return CONFIG_ICACHE_LINE_SIZE;
#else
	return DT_PROP_OR(CPU, i_cache_line_size, 0);
#endif
}

#include <syscalls/cache.h>
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CACHE_H_ */
