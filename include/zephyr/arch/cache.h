/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Public APIs for architectural cache controller drivers
 */

#ifndef ZEPHYR_INCLUDE_ARCH_CACHE_H_
#define ZEPHYR_INCLUDE_ARCH_CACHE_H_

/**
 * @brief Cache Controller Interface
 * @defgroup cache_arch_interface Cache Controller Interface
 * @ingroup io_interfaces
 * @{
 */

#if defined(CONFIG_ARM64)
#include <zephyr/arch/arm64/cache.h>
#elif defined(CONFIG_XTENSA)
#include <zephyr/arch/xtensa/cache.h>
#endif

#if defined(CONFIG_DCACHE) || defined(__DOXYGEN__)

/**
 * @brief Enable the d-cache
 *
 * Enable the data cache.
 */
void arch_dcache_enable(void);

#define cache_data_enable arch_dcache_enable

/**
 * @brief Disable the d-cache
 *
 * Disable the data cache.
 */
void arch_dcache_disable(void);

#define cache_data_disable arch_dcache_disable

/**
 * @brief Flush the d-cache
 *
 * Flush the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_flush_all(void);

#define cache_data_flush_all arch_dcache_flush_all

/**
 * @brief Invalidate the d-cache
 *
 * Invalidate the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_invd_all(void);

#define cache_data_invd_all arch_dcache_invd_all

/**
 * @brief Flush and Invalidate the d-cache
 *
 * Flush and Invalidate the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_flush_and_invd_all(void);

#define cache_data_flush_and_invd_all arch_dcache_flush_and_invd_all

/**
 * @brief Flush an address range in the d-cache
 *
 * Flush the specified address range of the data cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being flushed, all the portions of the
 *       data structures sharing the same line will be flushed. This is usually
 *       not a problem because writing back is a non-destructive process that
 *       could be triggered by hardware at any time, so having an aligned
 *       @p addr or a padded @p size is not strictly necessary.
 *
 * @param addr Starting address to flush.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_flush_range(void *addr, size_t size);

#define cache_data_flush_range(addr, size) arch_dcache_flush_range(addr, size)

/**
 * @brief Invalidate an address range in the d-cache
 *
 * Invalidate the specified address range of the data cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being invalidated, all the portions of the
 *       non-read-only data structures sharing the same line will be
 *       invalidated as well. This is a destructive process that could lead to
 *       data loss and/or corruption. When @p addr is not aligned to the cache
 *       line and/or @p size is not a multiple of the cache line size the
 *       behaviour is undefined.
 *
 * @param addr Starting address to invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_invd_range(void *addr, size_t size);

#define cache_data_invd_range(addr, size) arch_dcache_invd_range(addr, size)

/**
 * @brief Flush and Invalidate an address range in the d-cache
 *
 * Flush and Invalidate the specified address range of the data cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being flushed, all the portions of the
 *       data structures sharing the same line will be flushed before being
 *       invalidated. This is usually not a problem because writing back is a
 *       non-destructive process that could be triggered by hardware at any
 *       time, so having an aligned @p addr or a padded @p size is not strictly
 *       necessary.
 *
 * @param addr Starting address to flush and invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */

int arch_dcache_flush_and_invd_range(void *addr, size_t size);

#define cache_data_flush_and_invd_range(addr, size) \
	arch_dcache_flush_and_invd_range(addr, size)

#if defined(CONFIG_DCACHE_LINE_SIZE_DETECT) || defined(__DOXYGEN__)

/**
 *
 * @brief Get the d-cache line size.
 *
 * The API is provided to dynamically detect the data cache line size at run
 * time.
 *
 * The function must be implemented only when CONFIG_DCACHE_LINE_SIZE_DETECT is
 * defined.
 *
 * @retval size Size of the d-cache line.
 * @retval 0 If the d-cache is not enabled.
 */
size_t arch_dcache_line_size_get(void);

#define cache_data_line_size_get arch_dcache_line_size_get

#endif /* CONFIG_DCACHE_LINE_SIZE_DETECT || __DOXYGEN__ */

#endif /* CONFIG_DCACHE || __DOXYGEN__ */

#if defined(CONFIG_ICACHE) || defined(__DOXYGEN__)

/**
 * @brief Enable the i-cache
 *
 * Enable the instruction cache.
 */
void arch_icache_enable(void);

#define cache_instr_enable arch_icache_enable

/**
 * @brief Disable the i-cache
 *
 * Disable the instruction cache.
 */
void arch_icache_disable(void);

#define cache_instr_disable arch_icache_disable

/**
 * @brief Flush the i-cache
 *
 * Flush the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_flush_all(void);

#define cache_instr_flush_all arch_icache_flush_all

/**
 * @brief Invalidate the i-cache
 *
 * Invalidate the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_invd_all(void);

#define cache_instr_invd_all arch_icache_invd_all

/**
 * @brief Flush and Invalidate the i-cache
 *
 * Flush and Invalidate the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_flush_and_invd_all(void);

#define cache_instr_flush_and_invd_all arch_icache_flush_and_invd_all

/**
 * @brief Flush an address range in the i-cache
 *
 * Flush the specified address range of the instruction cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being flushed, all the portions of the
 *       data structures sharing the same line will be flushed. This is usually
 *       not a problem because writing back is a non-destructive process that
 *       could be triggered by hardware at any time, so having an aligned
 *       @p addr or a padded @p size is not strictly necessary.
 *
 * @param addr Starting address to flush.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_flush_range(void *addr, size_t size);

#define cache_instr_flush_range(addr, size) arch_icache_flush_range(addr, size)

/**
 * @brief Invalidate an address range in the i-cache
 *
 * Invalidate the specified address range of the instruction cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being invalidated, all the portions of the
 *       non-read-only data structures sharing the same line will be
 *       invalidated as well. This is a destructive process that could lead to
 *       data loss and/or corruption. When @p addr is not aligned to the cache
 *       line and/or @p size is not a multiple of the cache line size the
 *       behaviour is undefined.
 *
 * @param addr Starting address to invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_invd_range(void *addr, size_t size);

#define cache_instr_invd_range(addr, size) arch_icache_invd_range(addr, size)

/**
 * @brief Flush and Invalidate an address range in the i-cache
 *
 * Flush and Invalidate the specified address range of the instruction cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being flushed, all the portions of the
 *       data structures sharing the same line will be flushed before being
 *       invalidated. This is usually not a problem because writing back is a
 *       non-destructive process that could be triggered by hardware at any
 *       time, so having an aligned @p addr or a padded @p size is not strictly
 *       necessary.
 *
 * @param addr Starting address to flush and invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_flush_and_invd_range(void *addr, size_t size);

#define cache_instr_flush_and_invd_range(addr, size) \
	arch_icache_flush_and_invd_range(addr, size)

#if defined(CONFIG_ICACHE_LINE_SIZE_DETECT) || defined(__DOXYGEN__)

/**
 *
 * @brief Get the i-cache line size.
 *
 * The API is provided to dynamically detect the instruction cache line size at
 * run time.
 *
 * The function must be implemented only when CONFIG_ICACHE_LINE_SIZE_DETECT is
 * defined.
 *
 * @retval size Size of the d-cache line.
 * @retval 0 If the d-cache is not enabled.
 */

size_t arch_icache_line_size_get(void);

#define cache_instr_line_size_get arch_icache_line_size_get

#endif /* CONFIG_ICACHE_LINE_SIZE_DETECT || __DOXYGEN__ */

#endif /* CONFIG_ICACHE || __DOXYGEN__ */

#if CONFIG_CACHE_DOUBLEMAP  || __DOXYGEN__
bool arch_cache_is_ptr_cached(void *ptr);
#define cache_is_ptr_cached(ptr) arch_cache_is_ptr_cached(ptr)

bool arch_cache_is_ptr_uncached(void *ptr);
#define cache_is_ptr_uncached(ptr) arch_cache_is_ptr_uncached(ptr)

void __sparse_cache *arch_cache_cached_ptr_get(void *ptr);
#define cache_cached_ptr(ptr) arch_cache_cached_ptr_get(ptr)

void *arch_cache_uncached_ptr_get(void __sparse_cache *ptr);
#define cache_uncached_ptr(ptr) arch_cache_uncached_ptr_get(ptr)
#endif /* CONFIG_CACHE_DOUBLEMAP */


void arch_cache_init(void);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_ARCH_CACHE_H_ */
