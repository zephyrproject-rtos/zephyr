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

#else /* !CONFIG_DCACHE */

static inline bool stm32_buf_in_nocache(uintptr_t buf, size_t len_bytes)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len_bytes);

	/* Whole memory is nocache if DCACHE is disabled */
	return true;
}

#endif /* CONFIG_DCACHE */

#endif /* STM32_CACHE_H_ */
