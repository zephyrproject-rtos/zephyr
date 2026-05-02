/*
 * Copyright (c) 2019 Facebook.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Inline implementation of functions declared in math_extras.h.
 */

#ifndef ZEPHYR_INCLUDE_SYS_MATH_EXTRAS_H_
#error "please include <sys/math_extras.h> instead of this file"
#endif

#include <zephyr/toolchain.h>

/*
 * Force the use of portable C code (no builtins) by defining
 * PORTABLE_MISC_MATH_EXTRAS before including <misc/math_extras.h>.
 * This is primarily for use by tests.
 *
 * We'll #undef use_builtin again at the end of the file.
 */
#ifdef PORTABLE_MISC_MATH_EXTRAS
#define use_builtin(x) 0
#define __has_type_128 0
#else
#define use_builtin(x) HAS_BUILTIN(x)
#ifdef __SIZEOF_INT128__
	#define __has_type_128 1
#else
	#define __has_type_128 0
#endif
#endif

#if use_builtin(__builtin_add_overflow)
static inline bool u16_add_overflow(uint16_t a, uint16_t b, uint16_t *result)
{
	return __builtin_add_overflow(a, b, result);
}

static inline bool u32_add_overflow(uint32_t a, uint32_t b, uint32_t *result)
{
	return __builtin_add_overflow(a, b, result);
}

static inline bool u64_add_overflow(uint64_t a, uint64_t b, uint64_t *result)
{
	return __builtin_add_overflow(a, b, result);
}

static inline bool size_add_overflow(size_t a, size_t b, size_t *result)
{
	return __builtin_add_overflow(a, b, result);
}
#else /* !use_builtin(__builtin_add_overflow) */
static inline bool u16_add_overflow(uint16_t a, uint16_t b, uint16_t *result)
{
	uint16_t c = a + b;

	*result = c;

	return c < a;
}

static inline bool u32_add_overflow(uint32_t a, uint32_t b, uint32_t *result)
{
	uint32_t c = a + b;

	*result = c;

	return c < a;
}

static inline bool u64_add_overflow(uint64_t a, uint64_t b, uint64_t *result)
{
	uint64_t c = a + b;

	*result = c;

	return c < a;
}

static inline bool size_add_overflow(size_t a, size_t b, size_t *result)
{
	size_t c = a + b;

	*result = c;

	return c < a;
}
#endif /* use_builtin(__builtin_add_overflow) */

#if use_builtin(__builtin_mul_overflow)
static inline bool u16_mul_overflow(uint16_t a, uint16_t b, uint16_t *result)
{
	return __builtin_mul_overflow(a, b, result);
}

static inline bool u32_mul_overflow(uint32_t a, uint32_t b, uint32_t *result)
{
	return __builtin_mul_overflow(a, b, result);
}

static inline bool u64_mul_overflow(uint64_t a, uint64_t b, uint64_t *result)
{
	return __builtin_mul_overflow(a, b, result);
}

static inline bool size_mul_overflow(size_t a, size_t b, size_t *result)
{
	return __builtin_mul_overflow(a, b, result);
}
#else /* !use_builtin(__builtin_mul_overflow) */
static inline bool u16_mul_overflow(uint16_t a, uint16_t b, uint16_t *result)
{
	uint16_t c = a * b;

	*result = c;

	return a != 0 && (c / a) != b;
}

static inline bool u32_mul_overflow(uint32_t a, uint32_t b, uint32_t *result)
{
	uint32_t c = a * b;

	*result = c;

	return a != 0 && (c / a) != b;
}

static inline bool u64_mul_overflow(uint64_t a, uint64_t b, uint64_t *result)
{
	uint64_t c = a * b;

	*result = c;

	return a != 0 && (c / a) != b;
}

static inline bool size_mul_overflow(size_t a, size_t b, size_t *result)
{
	size_t c = a * b;

	*result = c;

	return a != 0 && (c / a) != b;
}
#endif /* use_builtin(__builtin_mul_overflow) */


/*
 * The GCC builtins __builtin_clz(), __builtin_ctz(), and 64-bit
 * variants are described by the GCC documentation as having undefined
 * behavior when the argument is zero. See
 * https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html.
 *
 * The undefined behavior applies to all architectures, regardless of
 * the behavior of the instruction used to implement the builtin.
 *
 * We don't want to expose users of this API to the undefined behavior,
 * so we use a conditional to explicitly provide the correct result when
 * x=0.
 *
 * Most instruction set architectures have a CLZ instruction or similar
 * that already computes the correct result for x=0. Both GCC and Clang
 * know this and simply generate a CLZ instruction, optimizing away the
 * conditional.
 *
 * For x86, and for compilers that fail to eliminate the conditional,
 * there is often another opportunity for optimization since code using
 * these functions tends to contain a zero check already. For example,
 * from kernel/sched.c:
 *
 *	struct k_thread *z_priq_mq_best(struct _priq_mq *pq)
 *	{
 *		if (!pq->bitmask) {
 *			return NULL;
 *		}
 *
 *		struct k_thread *thread = NULL;
 *		sys_dlist_t *l =
 *			&pq->queues[u32_count_trailing_zeros(pq->bitmask)];
 *
 *		...
 *
 * The compiler will often be able to eliminate the redundant x == 0
 * check after inlining the call to u32_count_trailing_zeros().
 */

#if use_builtin(__builtin_clz)
static inline int u32_count_leading_zeros(uint32_t x)
{
	return (x == 0) ? 32 : __builtin_clz(x);
}
#else /* !use_builtin(__builtin_clz) */
static inline int u32_count_leading_zeros(uint32_t x)
{
	int b;

	for (b = 0; b < 32 && (x >> 31) == 0; b++) {
		x <<= 1;
	}

	return b;
}
#endif /* use_builtin(__builtin_clz) */

#if use_builtin(__builtin_clzll)
static inline int u64_count_leading_zeros(uint64_t x)
{
	return (x == 0) ? 64 : __builtin_clzll(x);
}
#else /* !use_builtin(__builtin_clzll) */
static inline int u64_count_leading_zeros(uint64_t x)
{
	if (x == (uint32_t)x) {
		return 32 + u32_count_leading_zeros((uint32_t)x);
	} else {
		return u32_count_leading_zeros(x >> 32);
	}
}
#endif /* use_builtin(__builtin_clzll) */

#if use_builtin(__builtin_ctz)
static inline int u32_count_trailing_zeros(uint32_t x)
{
	return (x == 0) ? 32 : __builtin_ctz(x);
}
#else /* !use_builtin(__builtin_ctz) */
static inline int u32_count_trailing_zeros(uint32_t x)
{
	int b;

	for (b = 0; b < 32 && (x & 1) == 0; b++) {
		x >>= 1;
	}

	return b;
}
#endif /* use_builtin(__builtin_ctz) */

#if use_builtin(__builtin_ctzll)
static inline int u64_count_trailing_zeros(uint64_t x)
{
	return (x == 0) ? 64 : __builtin_ctzll(x);
}
#else /* !use_builtin(__builtin_ctzll) */
static inline int u64_count_trailing_zeros(uint64_t x)
{
	if ((uint32_t)x) {
		return u32_count_trailing_zeros((uint32_t)x);
	} else {
		return 32 + u32_count_trailing_zeros(x >> 32);
	}
}
#endif /* use_builtin(__builtin_ctzll) */

/**
 * @brief Multiply two signed 64-bit integers and store the result in a 128-bit integer.
 *
 * This function performs a full precision multiplication of two signed 64-bit
 * values. The result is guaranteed to fit in a 128-bit representation without overflow.
 *
 * @param a Multiplicand.
 * @param b Multiplier.
 * @param result Pointer to the @ref int128_t structure where the result will be stored.
 */
#if __has_type_128
static inline void i128_multiply_i64_i64(int64_t a, int64_t b, int128_t *result)
{
	__int128 c = (__int128)a * (__int128)b;

	result->low = (uint64_t)c;
	result->high = (uint64_t)(c >> 64);
}
#else
static inline void i128_multiply_i64_i64(int64_t a, int64_t b, int128_t *result)
{
	uint64_t u_a = (a < 0) ? (uint64_t)-a : (uint64_t)a;
	uint64_t u_b = (b < 0) ? (uint64_t)-b : (uint64_t)b;
	int sign = (a < 0) ^ (b < 0);

	/* Split to 32-bit values */
	uint64_t a_lo = u_a & 0xFFFFFFFFULL;
	uint64_t a_hi = u_a >> 32;
	uint64_t b_lo = u_b & 0xFFFFFFFFULL;
	uint64_t b_hi = u_b >> 32;

	/* Calculate product, just like in school */
	uint64_t res_0 = a_lo * b_lo;
	uint64_t res_1 = a_hi * b_lo;
	uint64_t res_2 = a_lo * b_hi;
	uint64_t res_3 = a_hi * b_hi;

	/* Combine values including carry */
	uint64_t carry = 0;
	uint64_t middle = (res_0 >> 32) + (res_1 & 0xFFFFFFFFULL) + (res_2 & 0xFFFFFFFFULL);

	result->low = (res_0 & 0xFFFFFFFFULL) | (middle << 32);

	/* Move the top part */
	carry = (middle >> 32) + (res_1 >> 32) + (res_2 >> 32) + res_3;
	result->high = carry;

	/* Calculate two-complement if sign is minus */
	if (sign) {
		result->low = ~result->low + 1;
		result->high = ~result->high + (result->low == 0 ? 1 : 0);
	}
}
#endif /* __has_type_128 */

#undef use_builtin
