/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/div64.h>

/* n >> s for s from 0 to 31, without libgcc's 64-bit shift */
static ALWAYS_INLINE uint64_t div64_shr(uint64_t n, uint32_t s)
{
	uint32_t lo = (uint32_t)n;
	uint32_t hi = (uint32_t)(n >> 32);

	lo = (lo >> s) | ((hi << 1) << (31U - s));
	hi >>= s;

	return ((uint64_t)hi << 32) | lo;
}

uint64_t z_div64_u32_recip(uint64_t n, uint64_t m, uint32_t flags)
{
	return div64_shr(z_div64_mulhi(n, m, (flags & Z_DIV64_FLAG_BIAS) != 0U, false),
			 flags & Z_DIV64_SHIFT_MASK);
}
