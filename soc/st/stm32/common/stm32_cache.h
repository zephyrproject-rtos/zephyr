/*
 * Copyright (c) 2025 Henrik Lindblom <henrik.lindblom@vaisala.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef STM32_CACHE_H_
#define STM32_CACHE_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <zephyr/toolchain.h>

#if defined(CONFIG_DCACHE)
/**
 * @brief Check if the given buffer resides in nocache memory
 *
 * Always returns true if CONFIG_DCACHE is not set.
 *
 * @param buf Buffer
 * @param len_bytes Buffer size in bytes
 * @return true if buf resides completely in nocache memory
 */
bool stm32_buf_in_nocache(uintptr_t buf, size_t len_bytes);

/**
 * @brief Check if a DMA transfer from or to the given buffer needs data cache maintenance
 *
 * Always returns false if CONFIG_DCACHE is not set.
 *
 * @param buf Buffer
 * @param len_bytes Buffer size in bytes
 * @retval true buf is not empty and does not reside completely in nocache memory
 * @retval false buf is empty or resides completely in nocache memory
 */
bool stm32_buf_is_cached(uintptr_t buf, size_t len_bytes);

/**
 * @brief Check if the given buffer owns whole data cache lines
 *
 * Invalidating the data cache over a buffer also discards the CPU's writes to other data
 * sharing the first or the last cache line of the buffer. A DMA may write to a buffer in
 * cacheable memory only if the buffer owns whole cache lines: its start and its length are
 * both multiples of the data cache line size.
 *
 * Always returns true if CONFIG_DCACHE is not set.
 *
 * @param buf Buffer
 * @param len_bytes Buffer size in bytes
 * @retval true buf and len_bytes are both multiples of the data cache line size
 * @retval false buf or len_bytes is not a multiple of the data cache line size
 */
bool stm32_buf_owns_cache_lines(uintptr_t buf, size_t len_bytes);

#else /* !CONFIG_DCACHE */

static inline bool stm32_buf_in_nocache(uintptr_t buf, size_t len_bytes)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len_bytes);

	/* Whole memory is nocache if DCACHE is disabled */
	return true;
}

static inline bool stm32_buf_is_cached(uintptr_t buf, size_t len_bytes)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len_bytes);

	/* Whole memory is nocache if DCACHE is disabled */
	return false;
}

static inline bool stm32_buf_owns_cache_lines(uintptr_t buf, size_t len_bytes)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len_bytes);

	/* There are no cache lines to share if DCACHE is disabled */
	return true;
}

#endif /* CONFIG_DCACHE */

#endif /* STM32_CACHE_H_ */
