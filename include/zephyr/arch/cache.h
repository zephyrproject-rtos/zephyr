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
#endif

#if defined(CONFIG_DCACHE)

/**
 * @brief Enable the d-cache
 *
 * Enable the data cache.
 */
extern void arch_dcache_enable(void);

#define cache_data_enable arch_dcache_enable

/**
 * @brief Disable the d-cache
 *
 * Disable the data cache.
 */
extern void arch_dcache_disable(void);

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
extern int arch_dcache_flush_all(void);

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
extern int arch_dcache_invd_all(void);

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
extern int arch_dcache_flush_and_invd_all(void);

#define cache_data_flush_and_invd_all arch_dcache_flush_and_invd_all

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
extern int arch_dcache_flush_range(void *addr, size_t size);

#define cache_data_flush_range(addr, size) arch_dcache_flush_range(addr, size)

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
extern int arch_dcache_invd_range(void *addr, size_t size);

#define cache_data_invd_range(addr, size) arch_dcache_invd_range(addr, size)

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

extern int arch_dcache_flush_and_invd_range(void *addr, size_t size);

#define cache_data_flush_and_invd_range(addr, size) \
	arch_dcache_flush_and_invd_range(addr, size)

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
extern size_t arch_dcache_line_size_get(void);

#define cache_data_line_size_get arch_dcache_line_size_get

#endif /* CONFIG_DCACHE_LINE_SIZE_DETECT */

#endif /* CONFIG_DCACHE */

#if defined(CONFIG_ICACHE)

/**
 * @brief Enable the i-cache
 *
 * Enable the instruction cache.
 */
extern void arch_icache_enable(void);

#define cache_instr_enable arch_icache_enable

/**
 * @brief Disable the i-cache
 *
 * Disable the instruction cache.
 */
extern void arch_icache_disable(void);

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
extern int arch_icache_flush_all(void);

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
extern int arch_icache_invd_all(void);

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
extern int arch_icache_flush_and_invd_all(void);

#define cache_instr_flush_and_invd_all arch_icache_flush_and_invd_all

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
extern int arch_icache_flush_range(void *addr, size_t size);

#define cache_instr_flush_range(addr, size) arch_icache_flush_range(addr, size)

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
extern int arch_icache_invd_range(void *addr, size_t size);

#define cache_instr_invd_range(addr, size) arch_icache_invd_range(addr, size)

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
extern int arch_icache_flush_and_invd_range(void *addr, size_t size);

#define cache_instr_flush_and_invd_range(addr, size) \
	arch_icache_flush_and_invd_range(addr, size)

#if defined(CONFIG_ICACHE_LINE_SIZE_DETECT)

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

extern size_t arch_icache_line_size_get(void);

#define cache_instr_line_size_get arch_icache_line_size_get

#endif /* CONFIG_ICACHE_LINE_SIZE_DETECT */

#endif /* CONFIG_ICACHE */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_ARCH_CACHE_H_ */
