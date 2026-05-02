/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/util.h>
#include <zephyr/sys/math_extras.h>

int bitmask_find_gap(uint32_t mask, size_t num_bits, size_t total_bits, bool first_match)
{
	uint32_t max = UINT32_MAX;
	int max_loc = -1;

	if (total_bits < 32) {
		mask |= ~BIT_MASK(total_bits);
	}

	mask = ~mask;
	while (mask != 0U) {
		uint32_t block_size;
		uint32_t loc;
		int nidx;
		uint32_t idx = 31 - u32_count_leading_zeros(mask);
		uint32_t rmask = ~BIT_MASK(idx);

		rmask |= mask;
		rmask = ~rmask;
		if (rmask != 0U) {
			nidx = 31 - u32_count_leading_zeros(rmask);
			block_size = idx - nidx;
			loc = nidx + 1;
			mask &= BIT_MASK(nidx);
		} else {
			mask = 0;
			block_size = idx + 1;
			loc = 0;
		}

		if ((block_size == num_bits) || (first_match && block_size > num_bits)) {
			max_loc = loc;
			max = block_size;
			break;
		} else if (block_size >= num_bits && block_size < max) {
			max_loc = loc;
			max = block_size;
		}
	}

	return max_loc;
}
