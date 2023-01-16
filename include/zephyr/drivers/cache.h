/*
 * Copyright 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Public APIs for external cache controller drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CACHE_H_
#define ZEPHYR_INCLUDE_DRIVERS_CACHE_H_

/**
 * @brief External Cache Controller Interface
 * @defgroup cache_external_interface External Cache Controller Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/cache.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_DCACHE)

/**
 * @brief Enable the d-cache
 *
 * Enable the data cache.
 */
void cache_data_enable(void);

/**
 * @brief Disable the d-cache
 *
 * Disable the data cache.
 */
void cache_data_disable(void);

/**
 * @brief Flush the d-cache
 *
 * Flush the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int cache_data_flush_all(void);

/**
 * @brief Invalidate the d-cache
 *
 * Invalidate the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int cache_data_invd_all(void);

/**
 * @brief Flush and Invalidate the d-cache
 *
 * Flush and Invalidate the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int cache_data_flush_and_invd_all(void);

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
int cache_data_flush_range(void *addr, size_t size);

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
int cache_data_invd_range(void *addr, size_t size);

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
int cache_data_flush_and_invd_range(void *addr, size_t size);

#if defined(CONFIG_DCACHE_LINE_SIZE_DETECT)
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
size_t cache_data_line_size_get(void);

#endif /* CONFIG_DCACHE_LINE_SIZE_DETECT */

#endif /* CONFIG_DCACHE */

#if defined(CONFIG_ICACHE)

/**
 * @brief Enable the i-cache
 *
 * Enable the instruction cache.
 */
void cache_instr_enable(void);

/**
 * @brief Disable the i-cache
 *
 * Disable the instruction cache.
 */
void cache_instr_disable(void);

/**
 * @brief Flush the i-cache
 *
 * Flush the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int cache_instr_flush_all(void);

/**
 * @brief Invalidate the i-cache
 *
 * Invalidate the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int cache_instr_invd_all(void);

/**
 * @brief Flush and Invalidate the i-cache
 *
 * Flush and Invalidate the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int cache_instr_flush_and_invd_all(void);

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
int cache_instr_flush_range(void *addr, size_t size);

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
int cache_instr_invd_range(void *addr, size_t size);

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
int cache_instr_flush_and_invd_range(void *addr, size_t size);

#ifdef CONFIG_ICACHE_LINE_SIZE_DETECT
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
size_t cache_instr_line_size_get(void);

#endif /* CONFIG_ICACHE_LINE_SIZE_DETECT */

#endif /* CONFIG_ICACHE */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CACHE_H_ */
