/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief 64-bit by 32-bit division without a 64-bit divide instruction.
 * @ingroup sys_div64
 */

#ifndef ZEPHYR_INCLUDE_SYS_DIV64_H_
#define ZEPHYR_INCLUDE_SYS_DIV64_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 64-bit by 32-bit division
 * @defgroup sys_div64 64-bit division
 * @ingroup utilities
 *
 * On a 32-bit target, n / d with a 64-bit n is a libgcc call that divides bit
 * by bit, even for a constant d. These helpers multiply by a reciprocal of d
 * instead. 64-bit targets divide in hardware.
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/*
 * n / d = ((n * m) >> 64) >> k, with m ~= 2^64 * p / d:
 *
 * 1. p = 2^k is the largest power of two below d, so m uses all 64 bits.
 * 2. With m rounded up, m * d = 2^(64 + k) + e, and n * m >> (64 + k) is
 *    n / d plus n * e / (d * 2^(64 + k)). That is exact for every 64-bit n
 *    if e <= p.
 * 3. Otherwise m is rounded down, so m * d = 2^(64 + k) - (d - e), with
 *    d - e < p, and the product gets m added:
 *    (n * m + m) >> 64 = ((n + 1) * m) >> 64. That is the bias. It is exact
 *    for every 64-bit n.
 * 4. Common factors of two come out of m and p. A smaller m may allow the
 *    short multiply. m has at most k of them, as m * d is within d of
 *    2^(64 + k).
 *
 * In 8 bits, dividing by 10: p = 8, m = ceil(256 * 8 / 10) = 205, e = 2, so
 * n / 10 = (n * 205) >> 11.
 *
 * These are integer constant expressions, so SYS_DIV64_RECIP_INIT() is a
 * static initializer, and the optimizer folds them for a constant d. With
 * 2^64 = q * d + r, m rounded down is q * p + r * p / d, and r * p % d is
 * d - e. A power of two d, 1 included, is a plain shift instead.
 */

#define Z_DIV64_LOG2(d) (31 - __builtin_clz(((uint32_t)(d) - 1U) | 1U))
#define Z_DIV64_P(d)    (1ULL << Z_DIV64_LOG2(d))
#define Z_DIV64_Q(d)    (UINT64_MAX / (d))
#define Z_DIV64_R(d)    ((UINT64_MAX % (d)) + 1ULL)
#define Z_DIV64_REM(d)  ((Z_DIV64_R(d) * Z_DIV64_P(d)) % (d))
#define Z_DIV64_BIAS(d) ((Z_DIV64_REM(d) != 0U) && (Z_DIV64_REM(d) < (d) - Z_DIV64_P(d)))
#define Z_DIV64_M0(d)                                                                              \
	((Z_DIV64_Q(d) * Z_DIV64_P(d)) + ((Z_DIV64_R(d) * Z_DIV64_P(d)) / (d)) +                   \
	 (((Z_DIV64_REM(d) != 0U) && !Z_DIV64_BIAS(d)) ? 1U : 0U))
#define Z_DIV64_ZEROS(d) __builtin_ctz((uint32_t)Z_DIV64_M0(d) | (uint32_t)Z_DIV64_P(d))
#define Z_DIV64_M(d)     (Z_DIV64_M0(d) >> Z_DIV64_ZEROS(d))
#define Z_DIV64_SHIFT(d) ((uint32_t)(Z_DIV64_LOG2(d) - Z_DIV64_ZEROS(d)))
#define Z_DIV64_LO(x)    ((x) & 0xffffffffULL)
#define Z_DIV64_HI(x)    ((x) >> 32)

#define Z_DIV64_FLAG_BIAS  0x80U
#define Z_DIV64_FLAG_SHIFT 0x40U /* n >> shift, for a power of two */
#define Z_DIV64_SHIFT_MASK 0x3fU
#define Z_DIV64_FLAGS(d)   (Z_DIV64_SHIFT(d) | (Z_DIV64_BIAS(d) ? Z_DIV64_FLAG_BIAS : 0U))

/* The short form of z_div64_mulhi() needs room for a carry between m's halves. */
#define Z_DIV64_NO_OVF(d) ((Z_DIV64_HI(Z_DIV64_M(d)) + Z_DIV64_LO(Z_DIV64_M(d))) <= UINT32_MAX)

#define Z_DIV64_ILOG2(d) (31 - __builtin_clz((uint32_t)(d)))

/* (n * m) >> 64, adding m first when @p bias is set. @p bias and @p no_ovf
 * are constant at each call site, so only one form is emitted.
 */
static ALWAYS_INLINE uint64_t z_div64_mulhi(uint64_t n, uint64_t m, bool bias, bool no_ovf)
{
	uint32_t m_lo = (uint32_t)m;
	uint32_t m_hi = (uint32_t)(m >> 32);
	uint32_t n_lo = (uint32_t)n;
	uint32_t n_hi = (uint32_t)(n >> 32);
	uint64_t x;
	uint64_t y;

	if (no_ovf) {
		x = ((uint64_t)m_lo * n_lo) + (bias ? m : 0U);
		x >>= 32;
		x += (uint64_t)m_lo * n_hi;
		x += (uint64_t)m_hi * n_lo;
		x >>= 32;
		x += (uint64_t)m_hi * n_hi;
	} else {
		x = ((uint64_t)m_lo * n_lo) + (bias ? m_lo : 0U);
		y = ((uint64_t)m_lo * n_hi) + (uint32_t)(x >> 32) + (bias ? m_hi : 0U);
		x = ((uint64_t)m_hi * n_hi) + (uint32_t)(y >> 32);
		y = ((uint64_t)m_hi * n_lo) + (uint32_t)y;
		x += (uint32_t)(y >> 32);
	}

	return x;
}

/* n / d from the m and flags of a reciprocal of d. */
__attribute_const__ uint64_t z_div64_u32_recip(uint64_t n, uint64_t m, uint32_t flags);

struct sys_div64_recip;

/* n / d from a shared reciprocal of d. Those never change, so the result
 * depends only on the arguments.
 */
__attribute_const__ uint64_t z_div64_shared_div(uint64_t n, const struct sys_div64_recip *r);

#if defined(CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define Z_DIV64_TICKS ((uint32_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#else
#define Z_DIV64_TICKS 0U
#endif

#if defined(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) &&                                                 \
	!defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
#define Z_DIV64_CYCLES ((uint32_t)CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)
#else
#define Z_DIV64_CYCLES 0U
#endif

/* a / b, or 0 when b is 0 */
#define Z_DIV64_RATIO(a, b) (((b) != 0U) ? ((uint32_t)(a) / (((b) != 0U) ? (b) : 1U)) : 0U)

/* Common divisors: the time units, the tick and cycle rates, and the ratios
 * between them that time conversions divide by. An entry below 2 is unused.
 */
#define Z_DIV64_SHARED_LIST(X)                                                                     \
	X(1000, 1000U)                                                                             \
	X(1000000, 1000000U)                                                                       \
	X(1000000000, 1000000000U)                                                                 \
	X(ticks, Z_DIV64_TICKS)                                                                    \
	X(cycles, Z_DIV64_CYCLES)                                                                  \
	X(ms_per_tick, Z_DIV64_RATIO(1000U, Z_DIV64_TICKS))                                        \
	X(us_per_tick, Z_DIV64_RATIO(1000000U, Z_DIV64_TICKS))                                     \
	X(ns_per_tick, Z_DIV64_RATIO(1000000000U, Z_DIV64_TICKS))                                  \
	X(ticks_per_ms, Z_DIV64_RATIO(Z_DIV64_TICKS, 1000U))                                       \
	X(cycles_per_ms, Z_DIV64_RATIO(Z_DIV64_CYCLES, 1000U))                                     \
	X(cycles_per_us, Z_DIV64_RATIO(Z_DIV64_CYCLES, 1000000U))                                  \
	X(cycles_per_tick, Z_DIV64_RATIO(Z_DIV64_CYCLES, Z_DIV64_TICKS))                           \
	X(ns_per_cycle, Z_DIV64_RATIO(1000000000U, Z_DIV64_CYCLES))

#define Z_DIV64_SHARED_DECLARE(name, v) extern const struct sys_div64_recip z_div64_recip_##name;
Z_DIV64_SHARED_LIST(Z_DIV64_SHARED_DECLARE)

#define Z_DIV64_SHARED_MATCH(name, v)                                                              \
	if (((v) > 1U) && (d == (v))) {                                                            \
		return &z_div64_recip_##name;                                                      \
	}

/* The shared reciprocal of d, or NULL. A call passes a pointer to it instead
 * of loading m and flags.
 */
static ALWAYS_INLINE const struct sys_div64_recip *z_div64_shared(uint64_t d)
{
	Z_DIV64_SHARED_LIST(Z_DIV64_SHARED_MATCH)
	return NULL;
}

/* n / d for a constant d. The derivation folds away once inlined. A
 * non-constant d, or a build without the optimizer, divides plainly.
 */
static ALWAYS_INLINE uint64_t z_div64_u32_const(uint64_t n, uint64_t d, bool inl)
{
#if defined(CONFIG_64BIT) || defined(CONFIG_NO_OPTIMIZATIONS)
	ARG_UNUSED(inl);
	return n / d;
#else
	uint64_t q;

	/* The compiler shifts for a power of two, and handles 0 as plain C does. */
	if (!__builtin_constant_p(d) || (d == 0U) || (d > UINT32_MAX) || IS_POWER_OF_TWO(d)) {
		return n / d;
	}
	if (inl) {
		q = z_div64_mulhi(n, Z_DIV64_M(d), Z_DIV64_BIAS(d), Z_DIV64_NO_OVF(d)) >>
		    Z_DIV64_SHIFT(d);
	} else if (z_div64_shared(d) != NULL) {
		q = z_div64_shared_div(n, z_div64_shared(d));
	} else {
		q = z_div64_u32_recip(n, Z_DIV64_M(d), Z_DIV64_FLAGS(d));
	}

	/* A constant n folds once inlined, and the unused call goes away. Not
	 * branching on it lets the quotient of n / d and n % d be shared.
	 */
	return __builtin_constant_p(n) ? (n / d) : q;
#endif
}

/* a when n is an integer constant expression, as in a static initializer,
 * otherwise b, resolved before the optimizer sees it. g++ lacks
 * __builtin_choose_expr.
 */
#if defined(__cplusplus) && !defined(__clang__)
#define Z_DIV64_IF_CONST(n, a, b) (__builtin_constant_p(n) ? (a) : (b))
#else
#define Z_DIV64_IF_CONST(n, a, b) __builtin_choose_expr(__builtin_constant_p(n), (a), (b))
#endif

#define Z_DIV64_U32_CONST(n, d, inl)                                                               \
	Z_DIV64_IF_CONST(n, (uint64_t)(n) / (d), z_div64_u32_const((n), (d), (inl)))

static ALWAYS_INLINE uint32_t z_mod64_u32_const(uint64_t n, uint64_t d)
{
	return (uint32_t)(n -
			  (z_div64_u32_const(n, d, IS_ENABLED(CONFIG_SPEED_OPTIMIZATIONS)) * d));
}

static ALWAYS_INLINE int64_t z_sdiv64_u32_const(int64_t n, uint64_t d)
{
#if defined(CONFIG_64BIT) || defined(CONFIG_NO_OPTIMIZATIONS)
	return n / (int64_t)d;
#else
	uint64_t q = z_div64_u32_const((n < 0) ? (0U - (uint64_t)n) : (uint64_t)n, d,
				       IS_ENABLED(CONFIG_SPEED_OPTIMIZATIONS));

	return (n < 0) ? (int64_t)(0U - q) : (int64_t)q;
#endif
}

/** @endcond */

/**
 * @brief Divide a 64-bit value by a 32-bit constant.
 *
 * On a 32-bit target this multiplies by a reciprocal of @p d worked out at
 * build time, in a shared out-of-line helper, or inline when optimizing for
 * speed. Without the optimizer, or for a non-constant @p d, it divides
 * plainly. A constant @p n folds to a constant, so the result may initialize
 * a static object.
 *
 * @param n Dividend (uint64_t).
 * @param d Divisor, a constant from 1 to UINT32_MAX.
 *
 * @return @p n / @p d (uint64_t).
 */
#define sys_div64_u32_const(n, d) Z_DIV64_U32_CONST(n, d, IS_ENABLED(CONFIG_SPEED_OPTIMIZATIONS))

/**
 * @brief Divide a 64-bit value by a 32-bit constant, inline.
 *
 * Same as sys_div64_u32_const(), but the multiplication is always inline, for
 * hot paths that are worth a few dozen bytes per call site.
 *
 * @param n Dividend (uint64_t).
 * @param d Divisor, a constant from 1 to UINT32_MAX.
 *
 * @return @p n / @p d (uint64_t).
 */
#define sys_div64_u32_const_inline(n, d) Z_DIV64_U32_CONST(n, d, true)

/**
 * @brief Remainder of a 64-bit value divided by a 32-bit constant.
 *
 * Paired with sys_div64_u32_const() on the same operands, the compiler
 * computes the quotient once.
 *
 * @param n Dividend (uint64_t).
 * @param d Divisor, a constant from 1 to UINT32_MAX.
 *
 * @return @p n % @p d (uint32_t).
 */
#define sys_mod64_u32_const(n, d)                                                                  \
	Z_DIV64_IF_CONST(n, (uint32_t)((uint64_t)(n) % (d)), z_mod64_u32_const((n), (d)))

/**
 * @brief Divide a signed 64-bit value by a 32-bit constant.
 *
 * Rounds toward zero, like the C division operator.
 *
 * @param n Dividend (int64_t).
 * @param d Divisor, a constant from 1 to UINT32_MAX.
 *
 * @return @p n / @p d (int64_t).
 */
#define sys_sdiv64_u32_const(n, d)                                                                 \
	Z_DIV64_IF_CONST(n, (int64_t)(n) / (int64_t)(d), z_sdiv64_u32_const((n), (d)))

/**
 * @brief Reciprocal of a 32-bit divisor known only at run time.
 *
 * Set up by sys_div64_recip_init() or SYS_DIV64_RECIP_INIT(), then used by
 * sys_div64_recip_div() and sys_div64_recip_mod(). The fields are internal.
 */
struct sys_div64_recip {
	/** @cond INTERNAL_HIDDEN */
	uint32_t div;
#if !defined(CONFIG_64BIT)
	uint32_t flags;
	uint64_t m;
#endif
	/** @endcond */
};

/**
 * @brief Static initializer for a reciprocal of a constant divisor.
 *
 * @param d Divisor, an integer constant expression from 1 to UINT32_MAX.
 */
#if defined(CONFIG_64BIT)
#define SYS_DIV64_RECIP_INIT(d) {.div = (d)}
#else
#define SYS_DIV64_RECIP_INIT(d)                                                                    \
	{                                                                                          \
		.div = (d),                                                                        \
		.flags = IS_POWER_OF_TWO(d) ? (Z_DIV64_ILOG2(d) | Z_DIV64_FLAG_SHIFT)              \
					    : Z_DIV64_FLAGS(d),                                    \
		.m = IS_POWER_OF_TWO(d) ? 0U : Z_DIV64_M(d),                                       \
	}
#endif

#if defined(CONFIG_64BIT) || defined(__DOXYGEN__)

/**
 * @brief Set up the reciprocal of a divisor.
 *
 * Takes four 32-bit divisions on a 32-bit target, so do it where the divisor
 * changes, not before each division.
 *
 * @param r Reciprocal to set up.
 * @param d Divisor, from 1 to UINT32_MAX.
 */
static inline void sys_div64_recip_init(struct sys_div64_recip *r, uint32_t d)
{
	r->div = d;
}

#else

void sys_div64_recip_init(struct sys_div64_recip *r, uint32_t d);

#endif /* CONFIG_64BIT || __DOXYGEN__ */

/**
 * @brief Divide a 64-bit value by a reciprocal's divisor.
 *
 * @param n Dividend.
 * @param r Reciprocal of the divisor.
 *
 * @return @p n divided by the divisor of @p r.
 */
static inline uint64_t sys_div64_recip_div(uint64_t n, const struct sys_div64_recip *r)
{
#if defined(CONFIG_64BIT)
	return n / r->div;
#else
	return z_div64_u32_recip(n, r->m, r->flags);
#endif
}

/**
 * @brief Remainder of a 64-bit value divided by a reciprocal's divisor.
 *
 * @param n Dividend.
 * @param r Reciprocal of the divisor.
 *
 * @return @p n modulo the divisor of @p r.
 */
static inline uint32_t sys_div64_recip_mod(uint64_t n, const struct sys_div64_recip *r)
{
#if defined(CONFIG_64BIT)
	return (uint32_t)(n % r->div);
#else
	return (uint32_t)n - ((uint32_t)sys_div64_recip_div(n, r) * r->div);
#endif
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_DIV64_H_ */
