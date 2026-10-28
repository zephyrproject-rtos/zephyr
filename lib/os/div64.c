/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/__assert.h>
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
	if ((flags & Z_DIV64_FLAG_SHIFT) == 0U) {
		n = z_div64_mulhi(n, m, (flags & Z_DIV64_FLAG_BIAS) != 0U, false);
	}

	return div64_shr(n, flags & Z_DIV64_SHIFT_MASK);
}

/* The derivation of Z_DIV64_M() and Z_DIV64_FLAGS(), for a run-time divisor. */
void sys_div64_recip_init(struct sys_div64_recip *r, uint32_t d)
{
	uint32_t k = Z_DIV64_LOG2(d);
	uint32_t p = 1U << k;
	uint32_t s, dn, dh, dl, rem;
	uint64_t m = 0U;
	uint32_t zeros;
	bool bias;

	__ASSERT(d != 0U, "division by zero");

	r->div = d;

	if (IS_POWER_OF_TWO(d)) {
		r->m = 0U;
		r->flags = Z_DIV64_ILOG2(d) | Z_DIV64_FLAG_SHIFT;
		return;
	}

	/*
	 * m = 2^(64 + k) / d = 2^(64 + k + s) / dn, with dn = d << s normalized,
	 * in four 16-bit digits, as Knuth's algorithm D. rem starts as the top 32
	 * bits of the dividend. A digit estimated from dn's top half is checked
	 * against its bottom half, which makes it exact.
	 */
	s = 31U - k;
	dn = d << s;
	dh = dn >> 16;
	dl = dn & 0xffffU;
	rem = 1U << 31;
	for (uint32_t i = 0U; i < 4U; i++) {
		uint32_t q = rem / dh;
		uint32_t rh = rem - (q * dh);

		while ((q > 0xffffU) || ((q * dl) > (rh << 16))) {
			q--;
			rh += dh;
			if (rh > 0xffffU) {
				break;
			}
		}
		rem = (rem << 16) - (q * dn);
		m = (m << 16) | q;
	}
	rem >>= s;

	bias = (rem != 0U) && (rem < (d - p));
	if ((rem != 0U) && !bias) {
		m++;
	}

	zeros = __builtin_ctz((uint32_t)m | p);
	r->m = div64_shr(m, zeros);
	r->flags = (k - zeros) | (bias ? Z_DIV64_FLAG_BIAS : 0U);
}
