/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/div64.h>

/* Never defined: a value that does not fold to a constant fails the link. */
void div64_not_folded(void);

#define FOLD_K 12345678901ULL

#define CHECK_FOLDED(x)                                                                            \
	do {                                                                                       \
		uint64_t v = (uint64_t)(x);                                                        \
                                                                                                   \
		if (!__builtin_constant_p(v)) {                                                    \
			div64_not_folded();                                                        \
		}                                                                                  \
	} while (false)

/* The dividend becomes constant only once inlined. */
static ALWAYS_INLINE uint64_t fold_inline(uint64_t n)
{
	return sys_div64_u32_const(n, 641) + sys_div64_u32_const(n, 1000) +
	       sys_mod64_u32_const(n, 1000000) + sys_div64_u32_const_inline(n, 3) +
	       (uint64_t)sys_sdiv64_u32_const(-(int64_t)n, 400);
}

/* Conversions with a constant value and constant rates fold. */
static ALWAYS_INLINE void check_folded(void)
{
	CHECK_FOLDED(fold_inline(FOLD_K));
	CHECK_FOLDED(k_ms_to_ticks_ceil64(FOLD_K));
	CHECK_FOLDED(k_ticks_to_ms_floor64(FOLD_K));
	CHECK_FOLDED(k_us_to_ticks_near64(FOLD_K));
	CHECK_FOLDED(k_ticks_to_ns_floor64(FOLD_K));
#if !defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
	CHECK_FOLDED(k_ms_to_cyc_ceil64(FOLD_K));
	CHECK_FOLDED(k_cyc_to_ms_floor64(FOLD_K));
	CHECK_FOLDED(k_ns_to_cyc_near64(FOLD_K));
	CHECK_FOLDED(k_cyc_to_ns_ceil64(FOLD_K));
#endif
}
