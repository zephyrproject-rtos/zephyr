/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_TIME_UNITS_H_
#define ZEPHYR_INCLUDE_SYS_TIME_UNITS_H_

#include <zephyr/toolchain.h>
#include <zephyr/sys/div64.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @defgroup timeutil_unit_apis Time Units Helpers
 * @ingroup timeutil_apis
 *
 * @brief Various helper APIs for converting between time units.
 * @{
 */

/** @brief System-wide macro to denote "forever" in milliseconds
 *
 *  Usage of this macro is limited to APIs that want to expose a timeout value
 *  that can optionally be unlimited, or "forever".
 *  This macro can not be fed into kernel functions or macros directly. Use
 *  @ref SYS_TIMEOUT_MS instead.
 */
#define SYS_FOREVER_MS (-1)

/** @brief System-wide macro to denote "forever" in microseconds
 *
 * See @ref SYS_FOREVER_MS.
 */
#define SYS_FOREVER_US (-1)

/** @brief System-wide macro to convert milliseconds to kernel timeouts
 */
#define SYS_TIMEOUT_MS(ms) \
	Z_TIMEOUT_TICKS((ms) == SYS_FOREVER_MS ? \
	K_TICKS_FOREVER : Z_TIMEOUT_MS_TICKS(ms))

/* Exhaustively enumerated, highly optimized time unit conversion API */

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || \
	defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
/**
 * @brief Get the current system timer frequency.
 *
 * Returns the system timer frequency in Hz as a runtime value. This is used by
 * sys_clock_hw_cycles_per_sec() and by time conversion helpers, and may change
 * after boot on some platforms.
 */
__syscall unsigned int sys_clock_hw_cycles_per_sec_runtime_get(void);

/** @cond INTERNAL_HIDDEN */

/* z_sys_clock_cyc_recip holds the reciprocal of the system timer frequency,
 * with the frequency in .div. z_sys_clock_hw_cycles_per_sec_set() updates
 * it under a spinlock. Readers of more than .div take the same lock.
 */
extern struct sys_div64_recip z_sys_clock_cyc_recip;

static inline unsigned int z_sys_clock_hw_cycles_per_sec_get(void)
{
	return z_sys_clock_cyc_recip.div;
}

/* The reciprocal of the system timer frequency, coherent with itself,
 * copied to buf. For time conversions on 32-bit targets.
 */
__syscall const struct sys_div64_recip *z_sys_clock_cyc_recip_get(struct sys_div64_recip *buf);

#if defined(Z_DIV64_PLAIN_RECIP)
/* A plain reciprocal is the frequency alone, read without the lock. */
static inline const struct sys_div64_recip *
z_impl_z_sys_clock_cyc_recip_get(struct sys_div64_recip *buf)
{
	buf->div = z_sys_clock_hw_cycles_per_sec_get();
	return buf;
}
#endif

/* The system timer frequency, read only. z_sys_clock_hw_cycles_per_sec_set()
 * changes it.
 */
#define z_clock_hw_cycles_per_sec z_sys_clock_hw_cycles_per_sec_get()

/** @endcond */

static inline unsigned int z_impl_sys_clock_hw_cycles_per_sec_runtime_get(void)
{
	return z_clock_hw_cycles_per_sec;
}
#endif /* CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME */

#if defined(__cplusplus) && (__cplusplus >= 201402L)
	#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || \
		defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
    #define TIME_CONSTEXPR
  #else
    #define TIME_CONSTEXPR constexpr
  #endif
#else
  #define TIME_CONSTEXPR
#endif

/**
 * @brief Get the system timer frequency.
 * @return system timer frequency in Hz
 */
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || \
	defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
#define sys_clock_hw_cycles_per_sec() sys_clock_hw_cycles_per_sec_runtime_get()
#else
#define sys_clock_hw_cycles_per_sec() (uint32_t)CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#endif

/** @internal
 * Macro determines if fast conversion algorithm can be used. It checks if
 * maximum timeout represented in source frequency domain and multiplied by
 * target frequency fits in 64 bits.
 *
 * @param from_hz Source frequency.
 * @param to_hz Target frequency.
 *
 * @retval true Use faster algorithm.
 * @retval false Use algorithm preventing overflow of intermediate value.
 */
#define z_tmcvt_use_fast_algo(from_hz, to_hz) \
	((DIV_ROUND_UP(CONFIG_SYS_CLOCK_MAX_TIMEOUT_DAYS * 24ULL * 3600ULL * from_hz, \
			   UINT32_MAX) * to_hz) <= UINT32_MAX)

/* Time converter generator gadget.  Selects from one of three
 * conversion algorithms: ones that take advantage when the
 * frequencies are an integer ratio (in either direction), or a full
 * precision conversion.  Clever use of extra arguments causes all the
 * selection logic to be optimized out, and the generated code even
 * reduces to 32 bit only if a ratio conversion is available and the
 * result is 32 bits.
 *
 * This isn't intended to be used directly, instead being wrapped
 * appropriately in a user-facing API.  The other arguments are:
 *
 *    const_hz  - true if the hz arguments are known to be compile-time
 *                constants (because otherwise the modulus test would
 *                have to be done at runtime), Z_TMCVT_RT_CYC if one of
 *                them is the cycle rate set at run time, else false
 *    result32  - The result will be truncated to 32 bits on use
 *    round_up  - Return the ceiling of the resulting fraction
 *    round_off - Return the nearest value to the resulting fraction
 *                (pass both round_up/off as false to get "round_down")
 *
 * A constant t with constant rates takes z_tmcvt_const(), an expression
 * that the compiler folds, so the result may initialize a global
 * variable. Otherwise, constant rates take z_tmcvt_fixed() and other rates
 * take z_tmcvt_rt(), which both evaluate t once.
 */

/*
 * All users of this macro MUST ensure its output is never used when a/b
 * is zero because it incorrectly but by design never returns zero.
 *
 * Some compiler versions emit a divide-by-zero warning for this code:
 * "false ? 42/0 : 43". Dealing with (generated) dead code is hard:
 * https://github.com/zephyrproject-rtos/zephyr/issues/63564
 * https://blog.llvm.org/2011/05/what-every-c-programmer-should-know_21.html
 *
 * To silence such divide-by-zero warnings, "cheat" and never return
 * zero.  Return 1 instead. Use octal "01u" as a breadcrumb to ease a
 * little bit the huge pain of "reverse-engineering" pre-processor
 * output.
 *
 * The "Elvis" operator "a/b ?: 1" is tempting because it avoids
 * evaluating the same expression twice. However: 1. it's a non-standard
 * GNU extension; 2. everything in this file is designed to be computed
 * at compile time anyway.
 */
#define z_tmcvt_divisor(a, b) ((a)/(b) ? (a)/(b) : 1U)

/* 64-bit n / d and n % d, through the div64 helpers when d is constant */
#define z_tmcvt_r_div(__n, __d)                                                                    \
	z_div64_u32_const((__n), (__d), IS_ENABLED(CONFIG_SPEED_OPTIMIZATIONS))

/* The offset added before dividing by d, for rounding */
#define z_tmcvt_off(__d, __round_up, __round_off)                                                  \
	((__round_off) ? ((__d) / 2U) : ((__round_up) ? ((__d) - 1U) : 0U))

/* (t * to_hz + off) / from_hz, exactly, without overflowing t * to_hz. For a
 * constant t, which the compiler folds.
 */
#define z_tmcvt_const(__t, __from_hz, __to_hz, __round_up, __round_off)                           \
	((((uint64_t)(__t) / (uint32_t)(__from_hz)) * (uint32_t)(__to_hz)) +                       \
	 (((((uint64_t)(__t) % (uint32_t)(__from_hz)) * (uint32_t)(__to_hz)) +                     \
	   z_tmcvt_off((uint32_t)(__from_hz), __round_up, __round_off)) /                          \
	  (uint32_t)(__from_hz)))

/* const_hz for a conversion to or from the cycle rate set at run time */
#define Z_TMCVT_RT_CYC 2U

/* Whether x is an integer constant expression */
#define z_tmcvt_is_const(__x) (Z_DIV64_IF_CONST(__x, 1, 0) != 0)

/* The cast keeps g++ from warning that a bool is compared with 2 */
#define z_tmcvt_const_hz_is(__const_hz, __val) ((uint32_t)(__const_hz) == (__val))

#if !defined(CONFIG_64BIT) &&                                                                      \
	(defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) ||                                   \
	 defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE))
/* z_tmcvt_cyc() modes */
#define Z_TMCVT_CYC_UP   1U
#define Z_TMCVT_CYC_NEAR 2U
#define Z_TMCVT_CYC_32   4U

/* (t * to_hz + rounding) / the cycle rate, through its reciprocal. A 64-bit
 * result takes two steps, as z_tmcvt_gen_64_slow(), with one copy of the
 * reciprocal for both divisions.
 */
static ALWAYS_INLINE uint64_t z_tmcvt_cyc_inline(uint64_t t, uint32_t to_hz, uint32_t mode)
{
	struct sys_div64_recip buf;
	const struct sys_div64_recip *r = z_sys_clock_cyc_recip_get(&buf);
	uint32_t from_hz = r->div;
	uint32_t off = ((mode & Z_TMCVT_CYC_NEAR) != 0U) ? (from_hz / 2U) :
		       (((mode & Z_TMCVT_CYC_UP) != 0U) ? (from_hz - 1U) : 0U);
	uint64_t q;
	uint32_t rem;

	if ((mode & Z_TMCVT_CYC_32) != 0U) {
		return (uint32_t)sys_div64_recip_div((t * to_hz) + off, r);
	}

	q = sys_div64_recip_div(t, r);
	rem = (uint32_t)t - ((uint32_t)q * from_hz);

	return (q * to_hz) + sys_div64_recip_div(((uint64_t)rem * to_hz) + off, r);
}

uint64_t z_tmcvt_cyc_out(uint64_t t, uint32_t to_hz, uint32_t mode);

static ALWAYS_INLINE uint64_t z_tmcvt_cyc(uint64_t t, uint32_t to_hz, bool round_up,
					  bool round_off, bool result32)
{
	uint32_t mode = (round_off ? Z_TMCVT_CYC_NEAR : (round_up ? Z_TMCVT_CYC_UP : 0U)) |
			(result32 ? Z_TMCVT_CYC_32 : 0U);

	if (IS_ENABLED(CONFIG_SPEED_OPTIMIZATIONS)) {
		return z_tmcvt_cyc_inline(t, to_hz, mode);
	}

	return z_tmcvt_cyc_out(t, to_hz, mode);
}

#endif

/* Euclid's algorithm, unrolled so that it folds for constants. Two 32-bit
 * values take at most 46 steps: 45 for the worst pair, consecutive
 * Fibonacci numbers, and one more when b > a, as the first step swaps them.
 */
#define Z_TMCVT_GCD_STEPS 46
#define Z_TMCVT_GCD_STEP(i, ...)                                                                   \
	if (b != 0U) {                                                                             \
		r = a % b;                                                                         \
		a = b;                                                                             \
		b = r;                                                                             \
	}

static ALWAYS_INLINE uint32_t z_tmcvt_gcd(uint32_t a, uint32_t b)
{
	uint32_t r;

	LISTIFY(Z_TMCVT_GCD_STEPS, Z_TMCVT_GCD_STEP, ())

	return a;
}

/* t / from_hz * to_hz + (t % from_hz * to_hz + off) / from_hz, which does not
 * overflow t * to_hz
 */
static ALWAYS_INLINE uint64_t z_tmcvt_slow(uint64_t t, uint32_t from_hz, uint32_t to_hz,
					   uint32_t off)
{
	uint64_t q = z_tmcvt_r_div(t, from_hz);
	uint32_t rem = (uint32_t)t - ((uint32_t)q * from_hz);

	return (q * to_hz) + z_tmcvt_r_div(((uint64_t)rem * to_hz) + off, from_hz);
}

/* (t * to_hz + off) / from_hz for constant rates. The optimizer computes
 * everything that depends on the rates alone.
 */
static ALWAYS_INLINE uint64_t z_tmcvt_fixed(uint64_t t, uint32_t from_hz, uint32_t to_hz,
					    bool result32, bool round_up, bool round_off)
{
	uint32_t den = from_hz;
	uint32_t num = to_hz;
	uint32_t off;

	/* (t * (to_hz / g)) / (from_hz / g) is the same, with smaller operands.
	 * With den or num then 1, what follows is a plain multiply or divide.
	 */
	if (z_tmcvt_is_const(from_hz) && z_tmcvt_is_const(to_hz)) {
		uint32_t g = z_tmcvt_gcd(from_hz, to_hz);

		den /= g;
		num /= g;
	}

	off = z_tmcvt_off(den, round_up, round_off);

	/* All in 32 bits when t * num + off fits. Testing the upper half of t
	 * on its own lets the optimizer drop that test for a 32-bit t.
	 */
	if (result32 && (den != 1U) && ((t >> 32U) == 0U) &&
	    ((uint32_t)t <= ((UINT32_MAX - off) / num))) {
		return (((uint32_t)t * num) + off) / den;
	}

	/* The reduced rates spare work when num is 1, when den is a power of
	 * two, or when a 64-bit result then takes one division instead of two.
	 * Otherwise the division costs the same, and from_hz may have a shared
	 * reciprocal, see z_div64_shared(). The maximum timeout is counted at
	 * from_hz, so t * num fits in 64 bits when it does at from_hz.
	 */
	if ((num != 1U) && !IS_POWER_OF_TWO(den) &&
	    (result32 ||
	     (z_tmcvt_use_fast_algo(from_hz, num) == z_tmcvt_use_fast_algo(from_hz, to_hz)))) {
		den = from_hz;
		num = to_hz;
		off = z_tmcvt_off(den, round_up, round_off);
	}

	if (result32 || z_tmcvt_use_fast_algo(from_hz, num)) {
		return z_tmcvt_r_div((t * num) + off, den);
	}

	return z_tmcvt_slow(t, den, num, off);
}

/* (t * to_hz + off) / from_hz for rates not known at build time */
static ALWAYS_INLINE uint64_t z_tmcvt_var(uint64_t t, uint32_t from_hz, uint32_t to_hz,
					  bool result32, bool round_up, bool round_off)
{
	uint32_t off = z_tmcvt_off(from_hz, round_up, round_off);

	if (result32) {
		return z_tmcvt_r_div((t * to_hz) + off, from_hz);
	}

	return z_tmcvt_slow(t, from_hz, to_hz, off);
}

#if !defined(CONFIG_64BIT) &&                                                                      \
	(defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) ||                                   \
	 defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE))
/* With Z_TMCVT_RT_CYC, a source rate that is not constant is the cycle rate,
 * which z_tmcvt_cyc() takes from its reciprocal. 0 stands for it, so that the
 * rate is not read for nothing.
 */
#define z_tmcvt_rt_from(__from_hz, __const_hz)                                                     \
	((z_tmcvt_const_hz_is(__const_hz, Z_TMCVT_RT_CYC) &&                                       \
	  !z_tmcvt_is_const(__from_hz))                                                            \
		 ? 0U                                                                              \
		 : (uint32_t)(__from_hz))
#else
#define z_tmcvt_rt_from(__from_hz, __const_hz) (__from_hz)
#endif

/* Rates not all known at build time */
static ALWAYS_INLINE uint64_t z_tmcvt_rt(uint64_t t, uint32_t from_hz, uint32_t to_hz,
					 bool rt_cyc, bool result32, bool round_up, bool round_off)
{
#if !defined(CONFIG_64BIT) &&                                                                      \
	(defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) ||                                   \
	 defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE))
	if (rt_cyc && (from_hz == 0U)) {
		return z_tmcvt_cyc(t, to_hz, round_up, round_off, result32);
	}
#else
	ARG_UNUSED(rt_cyc);
#endif

	return z_tmcvt_var(t, from_hz, to_hz, result32, round_up, round_off);
}

#define z_tmcvt(__t, __from_hz, __to_hz, __const_hz, __result32, __round_up, __round_off)          \
	(!z_tmcvt_const_hz_is(__const_hz, 1U)                                                      \
		 ? z_tmcvt_rt((__t), z_tmcvt_rt_from(__from_hz, __const_hz), (__to_hz),            \
			      z_tmcvt_const_hz_is(__const_hz, Z_TMCVT_RT_CYC), (__result32),       \
			      (__round_up), (__round_off))                                         \
	 : z_tmcvt_is_const(__t)                                                                   \
		 ? (z_tmcvt_const(__t, __from_hz, __to_hz, __round_up, __round_off) &              \
		    ((__result32) ? 0xffffffffULL : UINT64_MAX))                                   \
		 : z_tmcvt_fixed((__t), (__from_hz), (__to_hz), (__result32), (__round_up),        \
				 (__round_off)))

#define z_tmcvt_32(__t, __from_hz, __to_hz, __const_hz, __round_up, __round_off)                   \
	((uint32_t)z_tmcvt(__t, __from_hz, __to_hz, __const_hz, true, __round_up, __round_off))

#define z_tmcvt_64(__t, __from_hz, __to_hz, __const_hz, __round_up, __round_off)                   \
	((uint64_t)z_tmcvt(__t, __from_hz, __to_hz, __const_hz, false, __round_up, __round_off))

/* The following code is programmatically generated using this perl
 * code, which enumerates all possible combinations of units, rounding
 * modes and precision.  Do not edit directly.
 *
 * Note that nano/microsecond conversions are only defined with 64 bit
 * precision.  These units conversions were not available in 32 bit
 * variants historically, and doing 32 bit math with units that small
 * has precision traps that we probably don't want to support in an
 * official API.
 *
 * #!/usr/bin/perl -w
 * use strict;
 *
 * my %human = ("sec" => "seconds",
 *              "ms" => "milliseconds",
 *              "us" => "microseconds",
 *              "ns" => "nanoseconds",
 *              "cyc" => "hardware cycles",
 *              "ticks" => "ticks");
 * my %human_round = ("ceil" => "Rounds up",
 *                    "near" => "Round nearest",
 *                    "floor" => "Truncates");
 *
 * sub big { return $_[0] eq "us" || $_[0] eq "ns"; }
 * sub prefix { return $_[0] eq "sec" || $_[0] eq "ms" || $_[0] eq "us" || $_[0] eq "ns"; }
 *
 * for my $from_unit ("sec", "ms", "us", "ns", "cyc", "ticks") {
 *     for my $to_unit ("sec", "ms", "us", "ns", "cyc", "ticks") {
 *         next if $from_unit eq $to_unit;
 *         next if prefix($from_unit) && prefix($to_unit);
 *         for my $round ("floor", "near", "ceil") {
 *             for(my $big=0; $big <= 1; $big++) {
 *                 my $sz = $big ? 64 : 32;
 *                 my $sym = "k_${from_unit}_to_${to_unit}_$round$sz";
 *                 my $type = "uint${sz}_t";
 *                 my $const_hz = ($from_unit eq "cyc" || $to_unit eq "cyc")
 *                     ? "Z_CCYC" : "true";
 *                 my $ret32 = $big ? "64" : "32";
 *                 my $rup = $round eq "ceil" ? "true" : "false";
 *                 my $roff = $round eq "near" ? "true" : "false";
 *
 *                 my $hfrom = $human{$from_unit};
 *                 my $hto = $human{$to_unit};
 *                 my $hround = $human_round{$round};
 *                 print "/", "** \@brief Convert $hfrom to $hto. $ret32 bits. $hround.\n";
 *                 print " *\n";
 *                 print " * Converts time values in $hfrom to $hto.\n";
 *                 print " * Computes result in $sz bit precision.\n";
 *                 if ($round eq "ceil") {
 *                     print " * Rounds up to the next highest output unit.\n";
 *                 } elsif ($round eq "near") {
 *                     print " * Rounds to the nearest output unit.\n";
 *                 } else {
 *                     print " * Truncates to the next lowest output unit.\n";
 *                 }
 *                 print " *\n";
 *                 print " * \@warning Generated. Do not edit. See above.\n";
 *                 print " *\n";
 *                 print " * \@param t Source time in $hfrom. uint64_t\n";
 *                 print " *\n";
 *                 print " * \@return The converted time value in $hto. $type\n";
 *                 print " *", "/\n";
 *                 print "#define $sym(t) \\\n";
 *                 print "\tz_tmcvt_$ret32(t, Z_HZ_$from_unit, Z_HZ_$to_unit,";
 *                 print " $const_hz, $rup, $roff)\n";
 *                 print "\n\n";
 *             }
 *         }
 *     }
 * }
 */

/* Some more concise declarations to simplify the generator script and
 * save bytes below
 */
#define Z_HZ_sec 1
#define Z_HZ_ms 1000
#define Z_HZ_us 1000000
#define Z_HZ_ns 1000000000
#define Z_HZ_cyc sys_clock_hw_cycles_per_sec()
#define Z_HZ_ticks CONFIG_SYS_CLOCK_TICKS_PER_SEC
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || \
	defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
#define Z_CCYC Z_TMCVT_RT_CYC
#else
#define Z_CCYC true
#endif

/** @brief Convert seconds to hardware cycles. 32 bits. Truncates.
 *
 * Converts time values in seconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in seconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
#define k_sec_to_cyc_floor32(t) \
	z_tmcvt_32(t, Z_HZ_sec, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert seconds to hardware cycles. 64 bits. Truncates.
 *
 * Converts time values in seconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in seconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
#define k_sec_to_cyc_floor64(t) \
	z_tmcvt_64(t, Z_HZ_sec, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert seconds to hardware cycles. 32 bits. Round nearest.
 *
 * Converts time values in seconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in seconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
#define k_sec_to_cyc_near32(t) \
	z_tmcvt_32(t, Z_HZ_sec, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert seconds to hardware cycles. 64 bits. Round nearest.
 *
 * Converts time values in seconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in seconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
#define k_sec_to_cyc_near64(t) \
	z_tmcvt_64(t, Z_HZ_sec, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert seconds to hardware cycles. 32 bits. Rounds up.
 *
 * Converts time values in seconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in seconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
#define k_sec_to_cyc_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_sec, Z_HZ_cyc, Z_CCYC, true, false)


/** @brief Convert seconds to hardware cycles. 64 bits. Rounds up.
 *
 * Converts time values in seconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in seconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
#define k_sec_to_cyc_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_sec, Z_HZ_cyc, Z_CCYC, true, false)


/** @brief Convert seconds to ticks. 32 bits. Truncates.
 *
 * Converts time values in seconds to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in seconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
#define k_sec_to_ticks_floor32(t) \
	z_tmcvt_32(t, Z_HZ_sec, Z_HZ_ticks, true, false, false)


/** @brief Convert seconds to ticks. 64 bits. Truncates.
 *
 * Converts time values in seconds to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in seconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
#define k_sec_to_ticks_floor64(t) \
	z_tmcvt_64(t, Z_HZ_sec, Z_HZ_ticks, true, false, false)


/** @brief Convert seconds to ticks. 32 bits. Round nearest.
 *
 * Converts time values in seconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in seconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
#define k_sec_to_ticks_near32(t) \
	z_tmcvt_32(t, Z_HZ_sec, Z_HZ_ticks, true, false, true)


/** @brief Convert seconds to ticks. 64 bits. Round nearest.
 *
 * Converts time values in seconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in seconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
#define k_sec_to_ticks_near64(t) \
	z_tmcvt_64(t, Z_HZ_sec, Z_HZ_ticks, true, false, true)


/** @brief Convert seconds to ticks. 32 bits. Rounds up.
 *
 * Converts time values in seconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in seconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
#define k_sec_to_ticks_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_sec, Z_HZ_ticks, true, true, false)


/** @brief Convert seconds to ticks. 64 bits. Rounds up.
 *
 * Converts time values in seconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in seconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
#define k_sec_to_ticks_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_sec, Z_HZ_ticks, true, true, false)


/** @brief Convert milliseconds to hardware cycles. 32 bits. Truncates.
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
#define k_ms_to_cyc_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert milliseconds to hardware cycles. 64 bits. Truncates.
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
#define k_ms_to_cyc_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert milliseconds to hardware cycles. 32 bits. Round nearest.
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
#define k_ms_to_cyc_near32(t) \
	z_tmcvt_32(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert milliseconds to hardware cycles. 64 bits. Round nearest.
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
#define k_ms_to_cyc_near64(t) \
	z_tmcvt_64(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert milliseconds to hardware cycles. 32 bits. Rounds up.
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
#define k_ms_to_cyc_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, true, false)


/** @brief Convert milliseconds to hardware cycles. 64 bits. Rounds up.
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
#define k_ms_to_cyc_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, true, false)


/** @brief Convert milliseconds to ticks. 32 bits. Truncates.
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
#define k_ms_to_ticks_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ms, Z_HZ_ticks, true, false, false)


/** @brief Convert milliseconds to ticks. 64 bits. Truncates.
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
#define k_ms_to_ticks_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ms, Z_HZ_ticks, true, false, false)


/** @brief Convert milliseconds to ticks. 32 bits. Round nearest.
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
#define k_ms_to_ticks_near32(t) \
	z_tmcvt_32(t, Z_HZ_ms, Z_HZ_ticks, true, false, true)


/** @brief Convert milliseconds to ticks. 64 bits. Round nearest.
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
#define k_ms_to_ticks_near64(t) \
	z_tmcvt_64(t, Z_HZ_ms, Z_HZ_ticks, true, false, true)


/** @brief Convert milliseconds to ticks. 32 bits. Rounds up.
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
#define k_ms_to_ticks_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ms, Z_HZ_ticks, true, true, false)


/** @brief Convert milliseconds to ticks. 64 bits. Rounds up.
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
#define k_ms_to_ticks_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ms, Z_HZ_ticks, true, true, false)


/** @brief Convert microseconds to hardware cycles. 32 bits. Truncates.
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
#define k_us_to_cyc_floor32(t) \
	z_tmcvt_32(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert microseconds to hardware cycles. 64 bits. Truncates.
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
#define k_us_to_cyc_floor64(t) \
	z_tmcvt_64(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert microseconds to hardware cycles. 32 bits. Round nearest.
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
#define k_us_to_cyc_near32(t) \
	z_tmcvt_32(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert microseconds to hardware cycles. 64 bits. Round nearest.
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
#define k_us_to_cyc_near64(t) \
	z_tmcvt_64(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert microseconds to hardware cycles. 32 bits. Rounds up.
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
#define k_us_to_cyc_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, true, false)


/** @brief Convert microseconds to hardware cycles. 64 bits. Rounds up.
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
#define k_us_to_cyc_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, true, false)


/** @brief Convert microseconds to ticks. 32 bits. Truncates.
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
#define k_us_to_ticks_floor32(t) \
	z_tmcvt_32(t, Z_HZ_us, Z_HZ_ticks, true, false, false)


/** @brief Convert microseconds to ticks. 64 bits. Truncates.
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
#define k_us_to_ticks_floor64(t) \
	z_tmcvt_64(t, Z_HZ_us, Z_HZ_ticks, true, false, false)


/** @brief Convert microseconds to ticks. 32 bits. Round nearest.
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
#define k_us_to_ticks_near32(t) \
	z_tmcvt_32(t, Z_HZ_us, Z_HZ_ticks, true, false, true)


/** @brief Convert microseconds to ticks. 64 bits. Round nearest.
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
#define k_us_to_ticks_near64(t) \
	z_tmcvt_64(t, Z_HZ_us, Z_HZ_ticks, true, false, true)


/** @brief Convert microseconds to ticks. 32 bits. Rounds up.
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
#define k_us_to_ticks_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_us, Z_HZ_ticks, true, true, false)


/** @brief Convert microseconds to ticks. 64 bits. Rounds up.
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
#define k_us_to_ticks_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_us, Z_HZ_ticks, true, true, false)


/** @brief Convert nanoseconds to hardware cycles. 32 bits. Truncates.
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
#define k_ns_to_cyc_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert nanoseconds to hardware cycles. 64 bits. Truncates.
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
#define k_ns_to_cyc_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert nanoseconds to hardware cycles. 32 bits. Round nearest.
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
#define k_ns_to_cyc_near32(t) \
	z_tmcvt_32(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert nanoseconds to hardware cycles. 64 bits. Round nearest.
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
#define k_ns_to_cyc_near64(t) \
	z_tmcvt_64(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert nanoseconds to hardware cycles. 32 bits. Rounds up.
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
#define k_ns_to_cyc_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, true, false)


/** @brief Convert nanoseconds to hardware cycles. 64 bits. Rounds up.
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
#define k_ns_to_cyc_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, true, false)


/** @brief Convert nanoseconds to cycles at a given frequency. 32 bits. Truncates.
 *
 * Converts time values in nanoseconds to cycles at the specified frequency.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 * @param freq Target frequency in Hz. uint32_t
 *
 * @return The converted time value in cycles. uint32_t
 */
#define ns_to_cyc_at_hz_floor32(t, freq) \
	z_tmcvt_32(t, Z_HZ_ns, freq, false, false, false)


/** @brief Convert nanoseconds to cycles at a given frequency. 64 bits. Truncates.
 *
 * Converts time values in nanoseconds to cycles at the specified frequency.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 * @param freq Target frequency in Hz. uint32_t
 *
 * @return The converted time value in cycles. uint64_t
 */
#define ns_to_cyc_at_hz_floor64(t, freq) \
	z_tmcvt_64(t, Z_HZ_ns, freq, false, false, false)


/** @brief Convert nanoseconds to cycles at a given frequency. 32 bits. Round nearest.
 *
 * Converts time values in nanoseconds to cycles at the specified frequency.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 * @param freq Target frequency in Hz. uint32_t
 *
 * @return The converted time value in cycles. uint32_t
 */
#define ns_to_cyc_at_hz_near32(t, freq) \
	z_tmcvt_32(t, Z_HZ_ns, freq, false, false, true)


/** @brief Convert nanoseconds to cycles at a given frequency. 64 bits. Round nearest.
 *
 * Converts time values in nanoseconds to cycles at the specified frequency.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 * @param freq Target frequency in Hz. uint32_t
 *
 * @return The converted time value in cycles. uint64_t
 */
#define ns_to_cyc_at_hz_near64(t, freq) \
	z_tmcvt_64(t, Z_HZ_ns, freq, false, false, true)


/** @brief Convert nanoseconds to cycles at a given frequency. 32 bits. Rounds up.
 *
 * Converts time values in nanoseconds to cycles at the specified frequency.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 * @param freq Target frequency in Hz. uint32_t
 *
 * @return The converted time value in cycles. uint32_t
 */
#define ns_to_cyc_at_hz_ceil32(t, freq) \
	z_tmcvt_32(t, Z_HZ_ns, freq, false, true, false)


/** @brief Convert nanoseconds to cycles at a given frequency. 64 bits. Rounds up.
 *
 * Converts time values in nanoseconds to cycles at the specified frequency.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 * @param freq Target frequency in Hz. uint32_t
 *
 * @return The converted time value in cycles. uint64_t
 */
#define ns_to_cyc_at_hz_ceil64(t, freq) \
	z_tmcvt_64(t, Z_HZ_ns, freq, false, true, false)


/** @brief Convert nanoseconds to ticks. 32 bits. Truncates.
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
#define k_ns_to_ticks_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ns, Z_HZ_ticks, true, false, false)


/** @brief Convert nanoseconds to ticks. 64 bits. Truncates.
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
#define k_ns_to_ticks_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ns, Z_HZ_ticks, true, false, false)


/** @brief Convert nanoseconds to ticks. 32 bits. Round nearest.
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
#define k_ns_to_ticks_near32(t) \
	z_tmcvt_32(t, Z_HZ_ns, Z_HZ_ticks, true, false, true)


/** @brief Convert nanoseconds to ticks. 64 bits. Round nearest.
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
#define k_ns_to_ticks_near64(t) \
	z_tmcvt_64(t, Z_HZ_ns, Z_HZ_ticks, true, false, true)


/** @brief Convert nanoseconds to ticks. 32 bits. Rounds up.
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
#define k_ns_to_ticks_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ns, Z_HZ_ticks, true, true, false)


/** @brief Convert nanoseconds to ticks. 64 bits. Rounds up.
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
#define k_ns_to_ticks_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ns, Z_HZ_ticks, true, true, false)


/** @brief Convert hardware cycles to seconds. 32 bits. Truncates.
 *
 * Converts time values in hardware cycles to seconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in seconds. uint32_t
 */
#define k_cyc_to_sec_floor32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_sec, Z_CCYC, false, false)


/** @brief Convert hardware cycles to seconds. 64 bits. Truncates.
 *
 * Converts time values in hardware cycles to seconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in seconds. uint64_t
 */
#define k_cyc_to_sec_floor64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_sec, Z_CCYC, false, false)


/** @brief Convert hardware cycles to seconds. 32 bits. Round nearest.
 *
 * Converts time values in hardware cycles to seconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in seconds. uint32_t
 */
#define k_cyc_to_sec_near32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_sec, Z_CCYC, false, true)


/** @brief Convert hardware cycles to seconds. 64 bits. Round nearest.
 *
 * Converts time values in hardware cycles to seconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in seconds. uint64_t
 */
#define k_cyc_to_sec_near64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_sec, Z_CCYC, false, true)


/** @brief Convert hardware cycles to seconds. 32 bits. Rounds up.
 *
 * Converts time values in hardware cycles to seconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in seconds. uint32_t
 */
#define k_cyc_to_sec_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_sec, Z_CCYC, true, false)


/** @brief Convert hardware cycles to seconds. 64 bits. Rounds up.
 *
 * Converts time values in hardware cycles to seconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in seconds. uint64_t
 */
#define k_cyc_to_sec_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_sec, Z_CCYC, true, false)


/** @brief Convert hardware cycles to milliseconds. 32 bits. Truncates.
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in milliseconds. uint32_t
 */
#define k_cyc_to_ms_floor32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, false, false)


/** @brief Convert hardware cycles to milliseconds. 64 bits. Truncates.
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in milliseconds. uint64_t
 */
#define k_cyc_to_ms_floor64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, false, false)


/** @brief Convert hardware cycles to milliseconds. 32 bits. Round nearest.
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in milliseconds. uint32_t
 */
#define k_cyc_to_ms_near32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, false, true)


/** @brief Convert hardware cycles to milliseconds. 64 bits. Round nearest.
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in milliseconds. uint64_t
 */
#define k_cyc_to_ms_near64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, false, true)


/** @brief Convert hardware cycles to milliseconds. 32 bits. Rounds up.
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in milliseconds. uint32_t
 */
#define k_cyc_to_ms_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, true, false)


/** @brief Convert hardware cycles to milliseconds. 64 bits. Rounds up.
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in milliseconds. uint64_t
 */
#define k_cyc_to_ms_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, true, false)


/** @brief Convert hardware cycles to microseconds. 32 bits. Truncates.
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in microseconds. uint32_t
 */
#define k_cyc_to_us_floor32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, false, false)


/** @brief Convert hardware cycles to microseconds. 64 bits. Truncates.
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in microseconds. uint64_t
 */
#define k_cyc_to_us_floor64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, false, false)


/** @brief Convert hardware cycles to microseconds. 32 bits. Round nearest.
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in microseconds. uint32_t
 */
#define k_cyc_to_us_near32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, false, true)


/** @brief Convert hardware cycles to microseconds. 64 bits. Round nearest.
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in microseconds. uint64_t
 */
#define k_cyc_to_us_near64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, false, true)


/** @brief Convert hardware cycles to microseconds. 32 bits. Rounds up.
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in microseconds. uint32_t
 */
#define k_cyc_to_us_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, true, false)


/** @brief Convert hardware cycles to microseconds. 64 bits. Rounds up.
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in microseconds. uint64_t
 */
#define k_cyc_to_us_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, true, false)


/** @brief Convert hardware cycles to nanoseconds. 32 bits. Truncates.
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in nanoseconds. uint32_t
 */
#define k_cyc_to_ns_floor32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, false, false)


/** @brief Convert hardware cycles to nanoseconds. 64 bits. Truncates.
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in nanoseconds. uint64_t
 */
#define k_cyc_to_ns_floor64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, false, false)


/** @brief Convert hardware cycles to nanoseconds. 32 bits. Round nearest.
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in nanoseconds. uint32_t
 */
#define k_cyc_to_ns_near32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, false, true)


/** @brief Convert hardware cycles to nanoseconds. 64 bits. Round nearest.
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in nanoseconds. uint64_t
 */
#define k_cyc_to_ns_near64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, false, true)


/** @brief Convert hardware cycles to nanoseconds. 32 bits. Rounds up.
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in nanoseconds. uint32_t
 */
#define k_cyc_to_ns_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, true, false)


/** @brief Convert hardware cycles to nanoseconds. 64 bits. Rounds up.
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in nanoseconds. uint64_t
 */
#define k_cyc_to_ns_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, true, false)


/** @brief Convert hardware cycles to ticks. 32 bits. Truncates.
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
#define k_cyc_to_ticks_floor32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, false, false)


/** @brief Convert hardware cycles to ticks. 64 bits. Truncates.
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
#define k_cyc_to_ticks_floor64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, false, false)


/** @brief Convert hardware cycles to ticks. 32 bits. Round nearest.
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
#define k_cyc_to_ticks_near32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, false, true)


/** @brief Convert hardware cycles to ticks. 64 bits. Round nearest.
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
#define k_cyc_to_ticks_near64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, false, true)


/** @brief Convert hardware cycles to ticks. 32 bits. Rounds up.
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
#define k_cyc_to_ticks_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, true, false)


/** @brief Convert hardware cycles to ticks. 64 bits. Rounds up.
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
#define k_cyc_to_ticks_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, true, false)


/** @brief Convert ticks to seconds. 32 bits. Truncates.
 *
 * Converts time values in ticks to seconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in seconds. uint32_t
 */
#define k_ticks_to_sec_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_sec, true, false, false)


/** @brief Convert ticks to seconds. 64 bits. Truncates.
 *
 * Converts time values in ticks to seconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in seconds. uint64_t
 */
#define k_ticks_to_sec_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_sec, true, false, false)


/** @brief Convert ticks to seconds. 32 bits. Round nearest.
 *
 * Converts time values in ticks to seconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in seconds. uint32_t
 */
#define k_ticks_to_sec_near32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_sec, true, false, true)


/** @brief Convert ticks to seconds. 64 bits. Round nearest.
 *
 * Converts time values in ticks to seconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in seconds. uint64_t
 */
#define k_ticks_to_sec_near64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_sec, true, false, true)


/** @brief Convert ticks to seconds. 32 bits. Rounds up.
 *
 * Converts time values in ticks to seconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in seconds. uint32_t
 */
#define k_ticks_to_sec_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_sec, true, true, false)


/** @brief Convert ticks to seconds. 64 bits. Rounds up.
 *
 * Converts time values in ticks to seconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in seconds. uint64_t
 */
#define k_ticks_to_sec_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_sec, true, true, false)


/** @brief Convert ticks to milliseconds. 32 bits. Truncates.
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in milliseconds. uint32_t
 */
#define k_ticks_to_ms_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_ms, true, false, false)


/** @brief Convert ticks to milliseconds. 64 bits. Truncates.
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in milliseconds. uint64_t
 */
#define k_ticks_to_ms_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_ms, true, false, false)


/** @brief Convert ticks to milliseconds. 32 bits. Round nearest.
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in milliseconds. uint32_t
 */
#define k_ticks_to_ms_near32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_ms, true, false, true)


/** @brief Convert ticks to milliseconds. 64 bits. Round nearest.
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in milliseconds. uint64_t
 */
#define k_ticks_to_ms_near64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_ms, true, false, true)


/** @brief Convert ticks to milliseconds. 32 bits. Rounds up.
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in milliseconds. uint32_t
 */
#define k_ticks_to_ms_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_ms, true, true, false)


/** @brief Convert ticks to milliseconds. 64 bits. Rounds up.
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in milliseconds. uint64_t
 */
#define k_ticks_to_ms_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_ms, true, true, false)


/** @brief Convert ticks to microseconds. 32 bits. Truncates.
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in microseconds. uint32_t
 */
#define k_ticks_to_us_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_us, true, false, false)


/** @brief Convert ticks to microseconds. 64 bits. Truncates.
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in microseconds. uint64_t
 */
#define k_ticks_to_us_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_us, true, false, false)


/** @brief Convert ticks to microseconds. 32 bits. Round nearest.
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in microseconds. uint32_t
 */
#define k_ticks_to_us_near32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_us, true, false, true)


/** @brief Convert ticks to microseconds. 64 bits. Round nearest.
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in microseconds. uint64_t
 */
#define k_ticks_to_us_near64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_us, true, false, true)


/** @brief Convert ticks to microseconds. 32 bits. Rounds up.
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in microseconds. uint32_t
 */
#define k_ticks_to_us_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_us, true, true, false)


/** @brief Convert ticks to microseconds. 64 bits. Rounds up.
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in microseconds. uint64_t
 */
#define k_ticks_to_us_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_us, true, true, false)


/** @brief Convert ticks to nanoseconds. 32 bits. Truncates.
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in nanoseconds. uint32_t
 */
#define k_ticks_to_ns_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_ns, true, false, false)


/** @brief Convert ticks to nanoseconds. 64 bits. Truncates.
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in nanoseconds. uint64_t
 */
#define k_ticks_to_ns_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_ns, true, false, false)


/** @brief Convert ticks to nanoseconds. 32 bits. Round nearest.
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in nanoseconds. uint32_t
 */
#define k_ticks_to_ns_near32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_ns, true, false, true)


/** @brief Convert ticks to nanoseconds. 64 bits. Round nearest.
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in nanoseconds. uint64_t
 */
#define k_ticks_to_ns_near64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_ns, true, false, true)


/** @brief Convert ticks to nanoseconds. 32 bits. Rounds up.
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in nanoseconds. uint32_t
 */
#define k_ticks_to_ns_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_ns, true, true, false)


/** @brief Convert ticks to nanoseconds. 64 bits. Rounds up.
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in nanoseconds. uint64_t
 */
#define k_ticks_to_ns_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_ns, true, true, false)


/** @brief Convert ticks to hardware cycles. 32 bits. Truncates.
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
#define k_ticks_to_cyc_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert ticks to hardware cycles. 64 bits. Truncates.
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
#define k_ticks_to_cyc_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert ticks to hardware cycles. 32 bits. Round nearest.
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
#define k_ticks_to_cyc_near32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert ticks to hardware cycles. 64 bits. Round nearest.
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
#define k_ticks_to_cyc_near64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert ticks to hardware cycles. 32 bits. Rounds up.
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
#define k_ticks_to_cyc_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, true, false)


/** @brief Convert ticks to hardware cycles. 64 bits. Rounds up.
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @warning Generated. Do not edit. See above.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
#define k_ticks_to_cyc_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, true, false)

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || \
	defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
#include <zephyr/syscalls/time_units.h>
#endif

#undef TIME_CONSTEXPR

/**
 * @}
 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_INCLUDE_SYS_TIME_UNITS_H_ */
