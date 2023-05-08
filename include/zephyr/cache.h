/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CACHE_H_
#define ZEPHYR_INCLUDE_CACHE_H_

/**
 * @file
 * @brief cache API interface
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_EXTERNAL_CACHE)
#include <zephyr/drivers/cache.h>

#elif defined(CONFIG_ARCH_CACHE)
#include <zephyr/arch/cache.h>

#endif

/**
 * @defgroup cache_interface Cache Interface
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 *
 */

#define _CPU DT_PATH(cpus, cpu_0)

/** @endcond */

/**
 * @brief Enable the d-cache
 *
 * Enable the data cache
 *
 */
static ALWAYS_INLINE void sys_cache_data_enable(void)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	cache_data_enable();
#endif
}

/**
 * @brief Disable the d-cache
 *
 * Disable the data cache
 *
 */
static ALWAYS_INLINE void sys_cache_data_disable(void)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	cache_data_disable();
#endif
}

/**
 * @brief Enable the i-cache
 *
 * Enable the instruction cache
 *
 */
static ALWAYS_INLINE void sys_cache_instr_enable(void)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_ICACHE)
	cache_instr_enable();
#endif
}

/**
 * @brief Disable the i-cache
 *
 * Disable the instruction cache
 *
 */
static ALWAYS_INLINE void sys_cache_instr_disable(void)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_ICACHE)
	cache_instr_disable();
#endif
}

/**
 * @brief Flush the d-cache
 *
 * Flush the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
static ALWAYS_INLINE int sys_cache_data_flush_all(void)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	return cache_data_flush_all();
#endif
	return -ENOTSUP;
}

/**
 * @brief Flush the i-cache
 *
 * Flush the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
static ALWAYS_INLINE int sys_cache_instr_flush_all(void)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_ICACHE)
	return cache_instr_flush_all();
#endif
	return -ENOTSUP;
}

/**
 * @brief Invalidate the d-cache
 *
 * Invalidate the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
static ALWAYS_INLINE int sys_cache_data_invd_all(void)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	return cache_data_invd_all();
#endif
	return -ENOTSUP;
}

/**
 * @brief Invalidate the i-cache
 *
 * Invalidate the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
static ALWAYS_INLINE int sys_cache_instr_invd_all(void)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_ICACHE)
	return cache_instr_invd_all();
#endif
	return -ENOTSUP;
}

/**
 * @brief Flush and Invalidate the d-cache
 *
 * Flush and Invalidate the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
static ALWAYS_INLINE int sys_cache_data_flush_and_invd_all(void)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	return cache_data_flush_and_invd_all();
#endif
	return -ENOTSUP;
}

/**
 * @brief Flush and Invalidate the i-cache
 *
 * Flush and Invalidate the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
static ALWAYS_INLINE int sys_cache_instr_flush_and_invd_all(void)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_ICACHE)
	return cache_instr_flush_and_invd_all();
#endif
	return -ENOTSUP;
}

/**
 * @brief Flush an address range in the d-cache
 *
 * Flush the specified address range of the data cache.
 *
 * @param addr Starting address to flush.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
__syscall_always_inline int sys_cache_data_flush_range(void *addr, size_t size);

static ALWAYS_INLINE int z_impl_sys_cache_data_flush_range(void *addr, size_t size)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	return cache_data_flush_range(addr, size);
#endif
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

/**
 * @brief Flush an address range in the i-cache
 *
 * Flush the specified address range of the instruction cache.
 *
 * @param addr Starting address to flush.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
static ALWAYS_INLINE int sys_cache_instr_flush_range(void *addr, size_t size)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_ICACHE)
	return cache_instr_flush_range(addr, size);
#endif
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

/**
 * @brief Invalidate an address range in the d-cache
 *
 * Invalidate the specified address range of the data cache.
 *
 * @param addr Starting address to invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
__syscall_always_inline int sys_cache_data_invd_range(void *addr, size_t size);

static ALWAYS_INLINE int z_impl_sys_cache_data_invd_range(void *addr, size_t size)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	return cache_data_invd_range(addr, size);
#endif
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

/**
 * @brief Invalidate an address range in the i-cache
 *
 * Invalidate the specified address range of the instruction cache.
 *
 * @param addr Starting address to invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
static ALWAYS_INLINE int sys_cache_instr_invd_range(void *addr, size_t size)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_ICACHE)
	return cache_instr_invd_range(addr, size);
#endif
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

/**
 * @brief Flush and Invalidate an address range in the d-cache
 *
 * Flush and Invalidate the specified address range of the data cache.
 *
 * @param addr Starting address to flush and invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
__syscall_always_inline int sys_cache_data_flush_and_invd_range(void *addr, size_t size);

static ALWAYS_INLINE int z_impl_sys_cache_data_flush_and_invd_range(void *addr, size_t size)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	return cache_data_flush_and_invd_range(addr, size);
#endif
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

/**
 * @brief Flush and Invalidate an address range in the i-cache
 *
 * Flush and Invalidate the specified address range of the instruction cache.
 *
 * @param addr Starting address to flush and invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
static ALWAYS_INLINE int sys_cache_instr_flush_and_invd_range(void *addr, size_t size)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_ICACHE)
	return cache_instr_flush_and_invd_range(addr, size);
#endif
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

/**
 *
 * @brief Get the the d-cache line size.
 *
 * The API is provided to get the data cache line.
 *
 * The cache line size is calculated (in order of priority):
 *
 * - At run-time when @kconfig{CONFIG_DCACHE_LINE_SIZE_DETECT} is set.
 * - At compile time using the value set in @kconfig{CONFIG_DCACHE_LINE_SIZE}.
 * - At compile time using the `d-cache-line-size` CPU0 property of the DT.
 * - 0 otherwise
 *
 * @retval size Size of the d-cache line.
 * @retval 0 If the d-cache is not enabled.
 */
static ALWAYS_INLINE size_t sys_cache_data_line_size_get(void)
{
#ifdef CONFIG_DCACHE_LINE_SIZE_DETECT
	return cache_data_line_size_get();
#elif (CONFIG_DCACHE_LINE_SIZE != 0)
	return CONFIG_DCACHE_LINE_SIZE;
#else
	return DT_PROP_OR(_CPU, d_cache_line_size, 0);
#endif
}

/**
 *
 * @brief Get the the i-cache line size.
 *
 * The API is provided to get the instruction cache line.
 *
 * The cache line size is calculated (in order of priority):
 *
 * - At run-time when @kconfig{CONFIG_ICACHE_LINE_SIZE_DETECT} is set.
 * - At compile time using the value set in @kconfig{CONFIG_ICACHE_LINE_SIZE}.
 * - At compile time using the `i-cache-line-size` CPU0 property of the DT.
 * - 0 otherwise
 *
 * @retval size Size of the d-cache line.
 * @retval 0 If the d-cache is not enabled.
 */
static ALWAYS_INLINE size_t sys_cache_instr_line_size_get(void)
{
#ifdef CONFIG_ICACHE_LINE_SIZE_DETECT
	return cache_instr_line_size_get();
#elif (CONFIG_ICACHE_LINE_SIZE != 0)
	return CONFIG_ICACHE_LINE_SIZE;
#else
	return DT_PROP_OR(_CPU, i_cache_line_size, 0);
#endif
}

#ifdef CONFIG_LIBMETAL
static ALWAYS_INLINE void sys_cache_flush(void *addr, size_t size)
{
	sys_cache_data_flush_range(addr, size);
}
#endif

#include <syscalls/cache.h>
#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_CACHE_H_ */
