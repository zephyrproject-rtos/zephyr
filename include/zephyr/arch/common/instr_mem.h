/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_COMMON_INSTR_MEM_H_
#define ZEPHYR_INCLUDE_ARCH_COMMON_INSTR_MEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/sys/util.h>

/**
 * @file
 *
 * @brief Public instruction memory APIs.
 * @defgroup instr_mem Instruction Memory APIs
 * @ingroup arch-interface
 * @{
 */

/**
 * @brief Check if region is in instruction memory
 *
 * Check if the given address range is in instruction memory. For
 * non-Harvard architectures, this function will always return true.
 * For Harvard architectures, if it cannot be determined whether the
 * region is in instruction memory, e.g. due to missing Devicetree
 * information, this function should return false.
 *
 * @param addr Starting address of the region
 * @param len Length of the region in bytes
 * @returns true if architecture non-Harvard or region in instruction memory, false otherwise
 */
bool arch_is_instr_mem(const void *addr, size_t len);

/**
 * @brief Memset buffer in instruction memory
 *
 * Memset buffer in instruction memory.
 *
 * @param buf Buffer to memset
 * @param c Value to set
 * @param n Number of bytes to set
 * @returns the provided buffer pointer, or `NULL` on error
 */
void *arch_memset_instr(void *buf, int c, size_t n);

/**
 * @brief Memcpy buffer from data memory to instruction memory
 *
 * Memcpy used to copy buffer from data memory to instruction memory.
 *
 * @param d Destination buffer
 * @param s Source buffer
 * @param n Number of bytes to copy
 * @returns the provided destination buffer pointer, or `NULL` on error
 */
void *arch_memcpy_to_instr(void *d, const void *s, size_t n);

/**
 * @brief Memcpy buffer from instruction memory to data memory
 *
 * Memcpy used to copy buffer from instruction memory to data memory.
 *
 * @param d Destination buffer
 * @param s Source buffer
 * @param n Number of bytes to copy
 * @returns the provided destination buffer pointer, or `NULL` on error
 */
void *arch_memcpy_from_instr(void *d, const void *s, size_t n);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_COMMON_INSTR_MEM_H_ */
