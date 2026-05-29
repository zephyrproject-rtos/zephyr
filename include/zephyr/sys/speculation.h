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
	int32_t signed_index = index, signed_array_size = array_size;

	/* Take the difference between index and max.
	 * A proper value will result in a negative result. We also AND in
	 * the complement of index, so that we automatically reject any large
	 * indexes which would wrap around the difference calculation.
	 *
	 * Sign-extend just the sign bit to produce a mask of all 1s (accept)
	 * or all 0s (truncate).
	 */
	uint32_t mask = ((signed_index - signed_array_size) & ~signed_index) >> 31;

	return index & mask;
#else
	ARG_UNUSED(array_size);

	return index;
#endif /* CONFIG_BOUNDS_CHECK_BYPASS_MITIGATION */
}
#endif /* ZEPHYR_MISC_SPECULATION_H */
