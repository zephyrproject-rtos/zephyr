/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MISC_SPECULATION_H
#define ZEPHYR_MISC_SPECULATION_H

#include <zephyr/types.h>

/**
 * Sanitize an array index against bounds check bypass attacks aka the
 * Spectre V1 vulnerability.
 *
 * CPUs with speculative execution may speculate past any size checks and
 * leak confidential data due to analysis of micro-architectural properties.
 * This will unconditionally truncate any out-of-bounds indexes to
 * zero in the speculative execution path using bit twiddling instead of
 * any branch instructions.
 *
 * Example usage:
 *
 * if (index < size) {
 *     index = k_array_index_sanitize(index, size);
 *     data = array[index];
 * }
 *
 * @param index Untrusted array index which has been validated, but not used
 * @param array_size Size of the array
 * @return The original index value if < size, or 0
 */
static inline uint32_t k_array_index_sanitize(uint32_t index, uint32_t array_size)
{
#ifdef CONFIG_BOUNDS_CHECK_BYPASS_MITIGATION
	return (index < array_size) ? index : 0U;
#else
	ARG_UNUSED(array_size);

	return index;
#endif /* CONFIG_BOUNDS_CHECK_BYPASS_MITIGATION */
}
#endif /* ZEPHYR_MISC_SPECULATION_H */
