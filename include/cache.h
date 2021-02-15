/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CACHE_H_
#define ZEPHYR_INCLUDE_CACHE_H_

#include <kernel.h>

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

/**
 *
 * @brief Enable d-cache
 *
 * Enable the d-cache.
 *
 * @return N/A
 */
void arch_dcache_enable(void);

/**
 *
 * @brief Disable d-cache
 *
 * Disable the d-cache.
 *
 * @return N/A
 */
void arch_dcache_disable(void);

/**
 *
 * @brief Enable i-cache
 *
 * Enable the i-cache.
 *
 * @return N/A
 */
void arch_icache_enable(void);

/**
 *
 * @brief Disable i-cache
 *
 * Disable the i-cache.
 *
 * @return N/A
 */
void arch_icache_disable(void);

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
int arch_dcache_all(int op);

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
int arch_dcache_range(void *addr, size_t size, int op);

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
int arch_icache_all(int op);

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
int arch_icache_range(void *addr, size_t size, int op);

__syscall int sys_dcache_all(int op);
static inline int z_impl_sys_dcache_all(int op)
{
	if (IS_ENABLED(CONFIG_CACHE_MANAGEMENT)) {
		return arch_dcache_all(op);
	}
	return -ENOTSUP;
}

__syscall int sys_dcache_range(void *addr, size_t size, int op);
static inline int z_impl_sys_dcache_range(void *addr, size_t size, int op)
{
	if (IS_ENABLED(CONFIG_CACHE_MANAGEMENT)) {
		return arch_dcache_range(addr, size, op);
	}
	return -ENOTSUP;
}

__syscall int sys_icache_all(int op);
static inline int z_impl_sys_icache_all(int op)
{
	if (IS_ENABLED(CONFIG_CACHE_MANAGEMENT)) {
		return arch_icache_all(op);
	}
	return -ENOTSUP;
}

__syscall int sys_icache_range(void *addr, size_t size, int op);
static inline int z_impl_sys_icache_range(void *addr, size_t size, int op)
{
	if (IS_ENABLED(CONFIG_CACHE_MANAGEMENT)) {
		return arch_icache_range(addr, size, op);
	}
	return -ENOTSUP;
}

#ifdef CONFIG_LIBMETAL
static inline void sys_cache_flush(void *addr, size_t size)
{
	sys_dcache_range(addr, size, K_CACHE_WB);
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
static inline size_t sys_dcache_line_size_get(void)
{
#ifdef CONFIG_DCACHE_LINE_SIZE_DETECT
	return arch_dcache_line_size_get();
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
static inline size_t sys_icache_line_size_get(void)
{
#ifdef CONFIG_ICACHE_LINE_SIZE_DETECT
	return arch_icache_line_size_get();
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
