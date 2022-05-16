/*
 * Copyright 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CACHE_H_
#define ZEPHYR_INCLUDE_DRIVERS_CACHE_H_

#include <zephyr/cache.h>

/**
 * @brief Enable d-cache
 *
 * Enable the d-cache.
 */
void cache_data_enable(void);

/**
 * @brief Disable d-cache
 *
 * Disable the d-cache.
 */
void cache_data_disable(void);

/**
 * @brief Enable i-cache
 *
 * Enable the i-cache.
 */
void cache_instr_enable(void);

/**
 * @brief Disable i-cache
 *
 * Disable the i-cache.
 */
void cache_instr_disable(void);

/**
 *
 * @brief Write-back / Invalidate / Write-back + Invalidate all d-cache
 *
 * Write-back, Invalidate or Write-back + Invalidate the whole d-cache.
 *
 * @param op	Operation to perform (one of the K_CACHE_* operations)
 *
 * @retval 0		On success
 * @retval -ENOTSUP	If the operation is not supported
 */
int cache_data_all(int op);

/**
 *
 * @brief Write-back / Invalidate / Write-back + Invalidate d-cache lines
 *
 * No alignment is required for either addr or size, but since
 * arch_dcache_range() iterates on the d-cache lines, a d-cache line alignment
 * for both is optimal.
 *
 * The d-cache line size is specified either via the CONFIG_DCACHE_LINE_SIZE
 * kconfig option or it is detected at runtime.
 *
 * @param addr	The pointer to start the multi-line operation
 * @param size	The number of bytes that are to be acted on
 * @param op	Operation to perform (one of the K_CACHE_* operations)
 *
 * @retval 0		On success
 * @retval -ENOTSUP	If the operation is not supported
 */
int cache_data_range(void *addr, size_t size, int op);

/**
 *
 * @brief Write-back / Invalidate / Write-back + Invalidate all i-cache
 *
 * Write-back, Invalidate or Write-back + Invalidate the whole i-cache.
 *
 * @param op	Operation to perform (one of the K_CACHE_* operations)
 *
 * @retval 0		On success
 * @retval -ENOTSUP	If the operation is not supported
 */
int cache_instr_all(int op);

/**
 *
 * @brief Write-back / Invalidate / Write-back + Invalidate i-cache lines
 *
 * No alignment is required for either addr or size, but since
 * arch_icache_range() iterates on the i-cache lines, an i-cache line alignment
 * for both is optimal.
 *
 * The i-cache line size is specified either via the CONFIG_ICACHE_LINE_SIZE
 * kconfig option or it is detected at runtime.
 *
 * @param addr	The pointer to start the multi-line operation
 * @param size	The number of bytes that are to be acted on
 * @param op	Operation to perform (one of the K_CACHE_* operations)
 *
 * @retval 0		On success
 * @retval -ENOTSUP	If the operation is not supported
 */
int cache_instr_range(void *addr, size_t size, int op);

#ifdef CONFIG_DCACHE_LINE_SIZE_DETECT
/**
 *
 * @brief Get the i-cache line size.
 *
 * The API is provided to get the i-cache line size.
 *
 * @return size of the i-cache line or 0 if the i-cache is not enabled.
 */
size_t cache_data_line_size_get(void);

#endif /* CONFIG_DCACHE_LINE_SIZE_DETECT */

#ifdef CONFIG_ICACHE_LINE_SIZE_DETECT
/**
 *
 * @brief Get the i-cache line size.
 *
 * The API is provided to get the i-cache line size.
 *
 * @return size of the i-cache line or 0 if the i-cache is not enabled.
 */
size_t cache_instr_line_size_get(void);

#endif /* CONFIG_ICACHE_LINE_SIZE_DETECT */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CACHE_H_ */
