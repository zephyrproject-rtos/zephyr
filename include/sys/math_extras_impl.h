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

#include <toolchain.h>

/*
 * Force the use of portable C code (no builtins) by defining
 * PORTABLE_MISC_MATH_EXTRAS before including <misc/math_extras.h>.
 * This is primarily for use by tests.
 *
 * We'll #undef use_builtin again at the end of the file.
 */
#ifdef PORTABLE_MISC_MATH_EXTRAS
#define use_builtin(x) 0
#else
#define use_builtin(x) HAS_BUILTIN(x)
#endif

#if use_builtin(__builtin_add_overflow)
static inline bool u32_add_overflow(u32_t a, u32_t b, u32_t *result)
{
	return __builtin_add_overflow(a, b, result);
}

static inline bool u64_add_overflow(u64_t a, u64_t b, u64_t *result)
{
	return __builtin_add_overflow(a, b, result);
}

static inline bool size_add_overflow(size_t a, size_t b, size_t *result)
{
	return __builtin_add_overflow(a, b, result);
}
#else /* !use_builtin(__builtin_add_overflow) */
static inline bool u32_add_overflow(u32_t a, u32_t b, u32_t *result)
{
	u32_t c = a + b;

	*result = c;

	return c < a;
}

static inline bool u64_add_overflow(u64_t a, u64_t b, u64_t *result)
{
	u64_t c = a + b;

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
static inline bool u32_mul_overflow(u32_t a, u32_t b, u32_t *result)
{
	return __builtin_mul_overflow(a, b, result);
}

static inline bool u64_mul_overflow(u64_t a, u64_t b, u64_t *result)
{
	return __builtin_mul_overflow(a, b, result);
}

static inline bool size_mul_overflow(size_t a, size_t b, size_t *result)
{
	return __builtin_mul_overflow(a, b, result);
}
#else /* !use_builtin(__builtin_mul_overflow) */
static inline bool u32_mul_overflow(u32_t a, u32_t b, u32_t *result)
{
	u32_t c = a * b;

	*result = c;

	return a != 0 && (c / a) != b;
}

static inline bool u64_mul_overflow(u64_t a, u64_t b, u64_t *result)
{
	u64_t c = a * b;

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
 *		struct k_thread *t = NULL;
 *		sys_dlist_t *l =
 *			&pq->queues[u32_count_trailing_zeros(pq->bitmask)];
 *
 *		...
 *
 * The compiler will often be able to eliminate the redundant x == 0
 * check after inlining the call to u32_count_trailing_zeros().
 */

#if use_builtin(__builtin_clz)
static inline int u32_count_leading_zeros(u32_t x)
{
	return x == 0 ? 32 : __builtin_clz(x);
}
#else /* !use_builtin(__builtin_clz) */
static inline int u32_count_leading_zeros(u32_t x)
{
	int b;

	for (b = 0; b < 32 && (x >> 31) == 0; b++) {
		x <<= 1;
	}

	return b;
}
#endif /* use_builtin(__builtin_clz) */

#if use_builtin(__builtin_clzll)
static inline int u64_count_leading_zeros(u64_t x)
{
	return x == 0 ? 64 : __builtin_clzll(x);
}
#else /* !use_builtin(__builtin_clzll) */
static inline int u64_count_leading_zeros(u64_t x)
{
	if (x == (u32_t)x) {
		return 32 + u32_count_leading_zeros((u32_t)x);
	} else {
		return u32_count_leading_zeros(x >> 32);
	}
}
#endif /* use_builtin(__builtin_clzll) */

#if use_builtin(__builtin_ctz)
static inline int u32_count_trailing_zeros(u32_t x)
{
	return x == 0 ? 32 : __builtin_ctz(x);
}
#else /* !use_builtin(__builtin_ctz) */
static inline int u32_count_trailing_zeros(u32_t x)
{
	int b;

	for (b = 0; b < 32 && (x & 1) == 0; b++) {
		x >>= 1;
	}

	return b;
}
#endif /* use_builtin(__builtin_ctz) */

#if use_builtin(__builtin_ctzll)
static inline int u64_count_trailing_zeros(u64_t x)
{
	return x == 0 ? 64 : __builtin_ctzll(x);
}
#else /* !use_builtin(__builtin_ctzll) */
static inline int u64_count_trailing_zeros(u64_t x)
{
	if ((u32_t)x) {
		return u32_count_trailing_zeros((u32_t)x);
	} else {
		return 32 + u32_count_trailing_zeros(x >> 32);
	}
}
#endif /* use_builtin(__builtin_ctzll) */

#undef use_builtin
