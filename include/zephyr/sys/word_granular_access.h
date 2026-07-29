/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_WORD_GRANULAR_H_
#define ZEPHYR_INCLUDE_SYS_WORD_GRANULAR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/sys/util.h>

/**
 * @file
 *
 * @brief Public word granular access memory APIs.
 * @defgroup word_granular_access Word Granular Access Memory APIs
 * @ingroup memory_management
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 */

#ifndef __sys_mem_word_t_defined
#define __sys_mem_word_t_defined

/**
 * @typedef sys_mem_word_t
 *
 * @brief Access type for word granular access memory.
 *
 * Should match the optimal memory access word width
 * on the target platform. Here we default it to uintptr_t.
 */
typedef uintptr_t __attribute__((may_alias)) sys_mem_word_t;

#define Z_MEM_WORD_T_WIDTH __INTPTR_WIDTH__
#endif /* __sys_mem_word_t_defined */

BUILD_ASSERT(Z_MEM_WORD_T_WIDTH == 32, "Unsupported word width for access to "
				       "word granular access memory");

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Memset buffer in word granular access memory
 *
 * Memset buffer in word granular access memory.
 *
 * Note that if the start of the buffer being written to is not word-aligned, or the
 * length is not a multiple of the word size, bytes outside of the requested range may
 * need to be written to. The data in these bytes will remain unchanged.
 *
 * @param buf Buffer to memset
 * @param c Value to set
 * @param n Number of bytes to set
 * @returns the provided buffer pointer, or `NULL` on error
 */
void *sys_memset_word_granular_access(void *buf, int c, size_t n);

/**
 * @brief Memcpy buffer into word granular access memory
 *
 * Memcpy used to copy buffer into word granular access memory.
 *
 * Note that if the start of the buffer being written to or read from is not
 * word-aligned, or the length is not a multiple of the word size, bytes outside
 * of the requested range may need to be read from or written to. The data in these
 * bytes will remain unchanged.
 *
 * @param d Destination buffer
 * @param s Source buffer
 * @param n Number of bytes to copy
 * @returns the provided destination buffer pointer, or `NULL` on error
 */
void *sys_memcpy_to_word_granular_access(void *d, const void *s, size_t n);

/**
 * @brief Memcpy buffer out of word granular access memory
 *
 * Memcpy used to copy buffer out of word granular access memory.
 *
 * Note that if the start of the buffer being written to or read from is not
 * word-aligned, or the length is not a multiple of the word size, bytes outside
 * of the requested range may need to be read from or written to. The data in these
 * bytes will remain unchanged.
 *
 * @param d Destination buffer
 * @param s Source buffer
 * @param n Number of bytes to copy
 * @returns the provided destination buffer pointer, or `NULL` on error
 */
void *sys_memcpy_from_word_granular_access(void *d, const void *s, size_t n);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_WORD_GRANULAR_H_ */
