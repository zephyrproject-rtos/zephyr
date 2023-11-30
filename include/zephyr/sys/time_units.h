/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TIME_UNITS_H_
#define ZEPHYR_INCLUDE_TIME_UNITS_H_

#include <zephyr/toolchain.h>
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
#define SYS_TIMEOUT_MS(ms) Z_TIMEOUT_TICKS((ms) == SYS_FOREVER_MS ? \
					   K_TICKS_FOREVER : Z_TIMEOUT_MS_TICKS(ms))

/* Exhaustively enumerated, highly optimized time unit conversion API */

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
__syscall int sys_clock_hw_cycles_per_sec_runtime_get(void);

static inline int z_impl_sys_clock_hw_cycles_per_sec_runtime_get(void)
{
	extern int z_clock_hw_cycles_per_sec;

	return z_clock_hw_cycles_per_sec;
}
#endif /* CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME */

#if defined(__cplusplus) && __cplusplus >= 201402L
  #if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
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
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
#define sys_clock_hw_cycles_per_sec() sys_clock_hw_cycles_per_sec_runtime_get()
#else
#define sys_clock_hw_cycles_per_sec() CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
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
 * appropriately in a user-facing API.  The boolean arguments are:
 *
 *    const_hz  - The hz arguments are known to be compile-time
 *                constants (because otherwise the modulus test would
 *                have to be done at runtime)
 *    result32  - The result will be truncated to 32 bits on use
 *    round_up  - Return the ceiling of the resulting fraction
 *    round_off - Return the nearest value to the resulting fraction
 *                (pass both round_up/off as false to get "round_down")
 *
 * All of this must be implemented as expressions so that, when constant,
 * the results may be used to initialize global variables.
 */

/* true if the conversion is the identity */
#define z_tmcvt_is_identity(__from_hz, __to_hz) \
	((__to_hz) == (__from_hz))

/* true if the conversion requires a simple integer multiply */
#define z_tmcvt_is_int_mul(__from_hz, __to_hz) \
	((__to_hz) > (__from_hz) && (__to_hz) % (__from_hz) == 0U)

/* true if the conversion requires a simple integer division */
#define z_tmcvt_is_int_div(__from_hz, __to_hz) \
	((__from_hz) > (__to_hz) && (__from_hz) % (__to_hz) == 0U)

/*
 * Compute the offset needed to round the result correctly when
 * the conversion requires a simple integer division
 */
#define z_tmcvt_off_div(__from_hz, __to_hz, __round_up, __round_off)	\
	((__round_off) ? ((__from_hz) / (__to_hz)) / 2 :		\
	 (__round_up) ? ((__from_hz) / (__to_hz)) - 1 :			\
	 0)

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
#define z_tmcvt_divisor(a, b) ((a)/(b) ? (a)/(b) : 01u)

/*
 * Compute the offset needed to round the result correctly when
 * the conversion requires a full mul/div
 */
#define z_tmcvt_off_gen(__from_hz, __to_hz, __round_up, __round_off)	\
	((__round_off) ? (__from_hz) / 2 :				\
	 (__round_up) ? (__from_hz) - 1 :				\
	 0)

/* Integer division 32-bit conversion */
#define z_tmcvt_int_div_32(__t, __from_hz, __to_hz, __round_up, __round_off) \
	((uint64_t) (__t) <= 0xffffffffU -				\
	 z_tmcvt_off_div(__from_hz, __to_hz, __round_up, __round_off) ?	\
	 ((uint32_t)((__t) +						\
		     z_tmcvt_off_div(__from_hz, __to_hz,		\
				     __round_up, __round_off)) /	\
	  z_tmcvt_divisor(__from_hz, __to_hz))				\
	 :								\
	 (uint32_t) (((uint64_t) (__t) +				\
		      z_tmcvt_off_div(__from_hz, __to_hz,		\
				      __round_up, __round_off)) /	\
		     z_tmcvt_divisor(__from_hz, __to_hz))		\
		)

/* Integer multiplication 32-bit conversion */
#define z_tmcvt_int_mul_32(__t, __from_hz, __to_hz)	\
	(uint32_t) (__t)*((__to_hz) / (__from_hz))

/* General 32-bit conversion */
#define z_tmcvt_gen_32(__t, __from_hz, __to_hz, __round_up, __round_off) \
	((uint32_t) (((uint64_t) (__t)*(__to_hz) +			\
		      z_tmcvt_off_gen(__from_hz, __to_hz, __round_up, __round_off)) / (__from_hz)))

/* Integer division 64-bit conversion */
#define z_tmcvt_int_div_64(__t, __from_hz, __to_hz, __round_up, __round_off) \
	(((uint64_t) (__t) + z_tmcvt_off_div(__from_hz, __to_hz,	\
					     __round_up, __round_off)) / \
	z_tmcvt_divisor(__from_hz, __to_hz))

/* Integer multiplcation 64-bit conversion */
#define z_tmcvt_int_mul_64(__t, __from_hz, __to_hz)	\
	(uint64_t) (__t)*((__to_hz) / (__from_hz))

/* Fast 64-bit conversion. This relies on the multiply not overflowing */
#define z_tmcvt_gen_64_fast(__t, __from_hz, __to_hz, __round_up, __round_off) \
	(((uint64_t) (__t)*(__to_hz) + \
	  z_tmcvt_off_gen(__from_hz, __to_hz, __round_up, __round_off)) / (__from_hz))

/* Slow 64-bit conversion. This avoids overflowing the multiply */
#define z_tmcvt_gen_64_slow(__t, __from_hz, __to_hz, __round_up, __round_off) \
	(((uint64_t) (__t) / (__from_hz))*(__to_hz) +			\
	 (((uint64_t) (__t) % (__from_hz))*(__to_hz) +		\
	  z_tmcvt_off_gen(__from_hz, __to_hz, __round_up, __round_off)) / (__from_hz))

/* General 64-bit conversion. Uses one of the two above macros */
#define z_tmcvt_gen_64(__t, __from_hz, __to_hz, __round_up, __round_off) \
	(z_tmcvt_use_fast_algo(__from_hz, __to_hz) ?			\
	 z_tmcvt_gen_64_fast(__t, __from_hz, __to_hz, __round_up, __round_off) : \
	 z_tmcvt_gen_64_slow(__t, __from_hz, __to_hz, __round_up, __round_off))

/* Convert, generating a 32-bit result */
#define z_tmcvt_32(__t, __from_hz, __to_hz, __const_hz, __round_up, __round_off) \
	((__const_hz) ?							\
	 (								\
		 z_tmcvt_is_identity(__from_hz, __to_hz) ?		\
		 (uint32_t) (__t)					\
		 :							\
		 z_tmcvt_is_int_div(__from_hz, __to_hz) ?		\
		 z_tmcvt_int_div_32(__t, __from_hz, __to_hz, __round_up, __round_off) \
		 :							\
		 z_tmcvt_is_int_mul(__from_hz, __to_hz) ?		\
		 z_tmcvt_int_mul_32(__t, __from_hz, __to_hz)		\
		 :							\
		 z_tmcvt_gen_32(__t, __from_hz, __to_hz, __round_up, __round_off) \
		 )							\
	 :								\
	 z_tmcvt_gen_32(__t, __from_hz, __to_hz, __round_up, __round_off) \
		)

/* Convert, generating a 64-bit result */
#define z_tmcvt_64(__t, __from_hz, __to_hz, __const_hz, __round_up, __round_off) \
	((__const_hz) ?							\
	 (								\
		 z_tmcvt_is_identity(__from_hz, __to_hz) ?		\
		 (uint64_t) (__t)					\
		 :							\
		 z_tmcvt_is_int_div(__from_hz, __to_hz) ?		\
		 z_tmcvt_int_div_64(__t, __from_hz, __to_hz, __round_up, __round_off) \
		 :							\
		 z_tmcvt_is_int_mul(__from_hz, __to_hz) ?		\
		 z_tmcvt_int_mul_64(__t, __from_hz, __to_hz)		\
		 :							\
		 z_tmcvt_gen_64(__t, __from_hz, __to_hz, __round_up, __round_off) \
		 )							\
	 :								\
	 z_tmcvt_gen_64_slow(__t, __from_hz, __to_hz, __round_up, __round_off) \
		)

#define z_tmcvt(__t, __from_hz, __to_hz, __const_hz, __result32, __round_up, __round_off) \
	((__result32) ?							\
	 z_tmcvt_32(__t, __from_hz, __to_hz, __const_hz, __round_up, __round_off) : \
	 z_tmcvt_64(__t, __from_hz, __to_hz, __const_hz, __round_up, __round_off))

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
 * my %human = ("ms" => "milliseconds",
 *              "us" => "microseconds",
 *              "ns" => "nanoseconds",
 *              "cyc" => "hardware cycles",
 *              "ticks" => "ticks");
 * my %human_round = ("ceil" => "Rounds up",
 *		   "near" => "Round nearest",
 *		   "floor" => "Truncates");
 *
 * sub big { return $_[0] eq "us" || $_[0] eq "ns"; }
 * sub prefix { return $_[0] eq "ms" || $_[0] eq "us" || $_[0] eq "ns"; }
 *
 * for my $from_unit ("ms", "us", "ns", "cyc", "ticks") {
 *     for my $to_unit ("ms", "us", "ns", "cyc", "ticks") {
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
 *		my $hround = $human_round{$round};
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
 *		print " * \@param t Source time in $hfrom. uint64_t\n";
 *		print " *\n";
 *                 print " * \@return The converted time value in $hto. $type\n";
 *                 print " *", "/\n";
 *
 *                 print "/", "* Generated.  Do not edit.  See above. *", "/\n";
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
#define Z_HZ_ms 1000
#define Z_HZ_us 1000000
#define Z_HZ_ns 1000000000
#define Z_HZ_cyc sys_clock_hw_cycles_per_sec()
#define Z_HZ_ticks CONFIG_SYS_CLOCK_TICKS_PER_SEC
#define Z_CCYC (!IS_ENABLED(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME))

/** @brief Convert milliseconds to hardware cycles. 32 bits. Truncates.
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ms_to_cyc_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert milliseconds to hardware cycles. 64 bits. Truncates.
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ms_to_cyc_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert milliseconds to hardware cycles. 32 bits. Round nearest.
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ms_to_cyc_near32(t) \
	z_tmcvt_32(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert milliseconds to hardware cycles. 64 bits. Round nearest.
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ms_to_cyc_near64(t) \
	z_tmcvt_64(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert milliseconds to hardware cycles. 32 bits. Rounds up.
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ms_to_cyc_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, true, false)


/** @brief Convert milliseconds to hardware cycles. 64 bits. Rounds up.
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ms_to_cyc_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, true, false)


/** @brief Convert milliseconds to ticks. 32 bits. Truncates.
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ms_to_ticks_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ms, Z_HZ_ticks, true, false, false)


/** @brief Convert milliseconds to ticks. 64 bits. Truncates.
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ms_to_ticks_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ms, Z_HZ_ticks, true, false, false)


/** @brief Convert milliseconds to ticks. 32 bits. Round nearest.
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ms_to_ticks_near32(t) \
	z_tmcvt_32(t, Z_HZ_ms, Z_HZ_ticks, true, false, true)


/** @brief Convert milliseconds to ticks. 64 bits. Round nearest.
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ms_to_ticks_near64(t) \
	z_tmcvt_64(t, Z_HZ_ms, Z_HZ_ticks, true, false, true)


/** @brief Convert milliseconds to ticks. 32 bits. Rounds up.
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ms_to_ticks_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ms, Z_HZ_ticks, true, true, false)


/** @brief Convert milliseconds to ticks. 64 bits. Rounds up.
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in milliseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ms_to_ticks_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ms, Z_HZ_ticks, true, true, false)


/** @brief Convert microseconds to hardware cycles. 32 bits. Truncates.
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_us_to_cyc_floor32(t) \
	z_tmcvt_32(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert microseconds to hardware cycles. 64 bits. Truncates.
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_us_to_cyc_floor64(t) \
	z_tmcvt_64(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert microseconds to hardware cycles. 32 bits. Round nearest.
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_us_to_cyc_near32(t) \
	z_tmcvt_32(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert microseconds to hardware cycles. 64 bits. Round nearest.
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_us_to_cyc_near64(t) \
	z_tmcvt_64(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert microseconds to hardware cycles. 32 bits. Rounds up.
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_us_to_cyc_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, true, false)


/** @brief Convert microseconds to hardware cycles. 64 bits. Rounds up.
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_us_to_cyc_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, true, false)


/** @brief Convert microseconds to ticks. 32 bits. Truncates.
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_us_to_ticks_floor32(t) \
	z_tmcvt_32(t, Z_HZ_us, Z_HZ_ticks, true, false, false)


/** @brief Convert microseconds to ticks. 64 bits. Truncates.
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_us_to_ticks_floor64(t) \
	z_tmcvt_64(t, Z_HZ_us, Z_HZ_ticks, true, false, false)


/** @brief Convert microseconds to ticks. 32 bits. Round nearest.
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_us_to_ticks_near32(t) \
	z_tmcvt_32(t, Z_HZ_us, Z_HZ_ticks, true, false, true)


/** @brief Convert microseconds to ticks. 64 bits. Round nearest.
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_us_to_ticks_near64(t) \
	z_tmcvt_64(t, Z_HZ_us, Z_HZ_ticks, true, false, true)


/** @brief Convert microseconds to ticks. 32 bits. Rounds up.
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_us_to_ticks_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_us, Z_HZ_ticks, true, true, false)


/** @brief Convert microseconds to ticks. 64 bits. Rounds up.
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in microseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_us_to_ticks_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_us, Z_HZ_ticks, true, true, false)


/** @brief Convert nanoseconds to hardware cycles. 32 bits. Truncates.
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ns_to_cyc_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert nanoseconds to hardware cycles. 64 bits. Truncates.
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ns_to_cyc_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert nanoseconds to hardware cycles. 32 bits. Round nearest.
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ns_to_cyc_near32(t) \
	z_tmcvt_32(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert nanoseconds to hardware cycles. 64 bits. Round nearest.
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ns_to_cyc_near64(t) \
	z_tmcvt_64(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert nanoseconds to hardware cycles. 32 bits. Rounds up.
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ns_to_cyc_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, true, false)


/** @brief Convert nanoseconds to hardware cycles. 64 bits. Rounds up.
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ns_to_cyc_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, true, false)


/** @brief Convert nanoseconds to ticks. 32 bits. Truncates.
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ns_to_ticks_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ns, Z_HZ_ticks, true, false, false)


/** @brief Convert nanoseconds to ticks. 64 bits. Truncates.
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ns_to_ticks_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ns, Z_HZ_ticks, true, false, false)


/** @brief Convert nanoseconds to ticks. 32 bits. Round nearest.
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ns_to_ticks_near32(t) \
	z_tmcvt_32(t, Z_HZ_ns, Z_HZ_ticks, true, false, true)


/** @brief Convert nanoseconds to ticks. 64 bits. Round nearest.
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ns_to_ticks_near64(t) \
	z_tmcvt_64(t, Z_HZ_ns, Z_HZ_ticks, true, false, true)


/** @brief Convert nanoseconds to ticks. 32 bits. Rounds up.
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ns_to_ticks_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ns, Z_HZ_ticks, true, true, false)


/** @brief Convert nanoseconds to ticks. 64 bits. Rounds up.
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in nanoseconds. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ns_to_ticks_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ns, Z_HZ_ticks, true, true, false)


/** @brief Convert hardware cycles to milliseconds. 32 bits. Truncates.
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in milliseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ms_floor32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, false, false)


/** @brief Convert hardware cycles to milliseconds. 64 bits. Truncates.
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in milliseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ms_floor64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, false, false)


/** @brief Convert hardware cycles to milliseconds. 32 bits. Round nearest.
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in milliseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ms_near32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, false, true)


/** @brief Convert hardware cycles to milliseconds. 64 bits. Round nearest.
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in milliseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ms_near64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, false, true)


/** @brief Convert hardware cycles to milliseconds. 32 bits. Rounds up.
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in milliseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ms_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, true, false)


/** @brief Convert hardware cycles to milliseconds. 64 bits. Rounds up.
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in milliseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ms_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, true, false)


/** @brief Convert hardware cycles to microseconds. 32 bits. Truncates.
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in microseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_us_floor32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, false, false)


/** @brief Convert hardware cycles to microseconds. 64 bits. Truncates.
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in microseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_us_floor64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, false, false)


/** @brief Convert hardware cycles to microseconds. 32 bits. Round nearest.
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in microseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_us_near32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, false, true)


/** @brief Convert hardware cycles to microseconds. 64 bits. Round nearest.
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in microseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_us_near64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, false, true)


/** @brief Convert hardware cycles to microseconds. 32 bits. Rounds up.
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in microseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_us_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, true, false)


/** @brief Convert hardware cycles to microseconds. 64 bits. Rounds up.
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in microseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_us_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, true, false)


/** @brief Convert hardware cycles to nanoseconds. 32 bits. Truncates.
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in nanoseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ns_floor32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, false, false)


/** @brief Convert hardware cycles to nanoseconds. 64 bits. Truncates.
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in nanoseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ns_floor64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, false, false)


/** @brief Convert hardware cycles to nanoseconds. 32 bits. Round nearest.
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in nanoseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ns_near32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, false, true)


/** @brief Convert hardware cycles to nanoseconds. 64 bits. Round nearest.
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in nanoseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ns_near64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, false, true)


/** @brief Convert hardware cycles to nanoseconds. 32 bits. Rounds up.
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in nanoseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ns_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, true, false)


/** @brief Convert hardware cycles to nanoseconds. 64 bits. Rounds up.
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in nanoseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ns_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, true, false)


/** @brief Convert hardware cycles to ticks. 32 bits. Truncates.
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ticks_floor32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, false, false)


/** @brief Convert hardware cycles to ticks. 64 bits. Truncates.
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ticks_floor64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, false, false)


/** @brief Convert hardware cycles to ticks. 32 bits. Round nearest.
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ticks_near32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, false, true)


/** @brief Convert hardware cycles to ticks. 64 bits. Round nearest.
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ticks_near64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, false, true)


/** @brief Convert hardware cycles to ticks. 32 bits. Rounds up.
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in ticks. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ticks_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, true, false)


/** @brief Convert hardware cycles to ticks. 64 bits. Rounds up.
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in hardware cycles. uint64_t
 *
 * @return The converted time value in ticks. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_cyc_to_ticks_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, true, false)


/** @brief Convert ticks to milliseconds. 32 bits. Truncates.
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in milliseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_ms_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_ms, true, false, false)


/** @brief Convert ticks to milliseconds. 64 bits. Truncates.
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in milliseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_ms_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_ms, true, false, false)


/** @brief Convert ticks to milliseconds. 32 bits. Round nearest.
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in milliseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_ms_near32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_ms, true, false, true)


/** @brief Convert ticks to milliseconds. 64 bits. Round nearest.
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in milliseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_ms_near64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_ms, true, false, true)


/** @brief Convert ticks to milliseconds. 32 bits. Rounds up.
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in milliseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_ms_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_ms, true, true, false)


/** @brief Convert ticks to milliseconds. 64 bits. Rounds up.
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in milliseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_ms_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_ms, true, true, false)


/** @brief Convert ticks to microseconds. 32 bits. Truncates.
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in microseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_us_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_us, true, false, false)


/** @brief Convert ticks to microseconds. 64 bits. Truncates.
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in microseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_us_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_us, true, false, false)


/** @brief Convert ticks to microseconds. 32 bits. Round nearest.
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in microseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_us_near32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_us, true, false, true)


/** @brief Convert ticks to microseconds. 64 bits. Round nearest.
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in microseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_us_near64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_us, true, false, true)


/** @brief Convert ticks to microseconds. 32 bits. Rounds up.
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in microseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_us_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_us, true, true, false)


/** @brief Convert ticks to microseconds. 64 bits. Rounds up.
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in microseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_us_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_us, true, true, false)


/** @brief Convert ticks to nanoseconds. 32 bits. Truncates.
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in nanoseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_ns_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_ns, true, false, false)


/** @brief Convert ticks to nanoseconds. 64 bits. Truncates.
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in nanoseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_ns_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_ns, true, false, false)


/** @brief Convert ticks to nanoseconds. 32 bits. Round nearest.
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in nanoseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_ns_near32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_ns, true, false, true)


/** @brief Convert ticks to nanoseconds. 64 bits. Round nearest.
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in nanoseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_ns_near64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_ns, true, false, true)


/** @brief Convert ticks to nanoseconds. 32 bits. Rounds up.
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in nanoseconds. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_ns_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_ns, true, true, false)


/** @brief Convert ticks to nanoseconds. 64 bits. Rounds up.
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in nanoseconds. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_ns_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_ns, true, true, false)


/** @brief Convert ticks to hardware cycles. 32 bits. Truncates.
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_cyc_floor32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert ticks to hardware cycles. 64 bits. Truncates.
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_cyc_floor64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, false, false)


/** @brief Convert ticks to hardware cycles. 32 bits. Round nearest.
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_cyc_near32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert ticks to hardware cycles. 64 bits. Round nearest.
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_cyc_near64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, false, true)


/** @brief Convert ticks to hardware cycles. 32 bits. Rounds up.
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in hardware cycles. uint32_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_cyc_ceil32(t) \
	z_tmcvt_32(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, true, false)


/** @brief Convert ticks to hardware cycles. 64 bits. Rounds up.
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @param t Source time in ticks. uint64_t
 *
 * @return The converted time value in hardware cycles. uint64_t
 */
/* Generated.  Do not edit.  See above. */
#define k_ticks_to_cyc_ceil64(t) \
	z_tmcvt_64(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, true, false)

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
#include <syscalls/time_units.h>
#endif

#undef TIME_CONSTEXPR

/**
 * @}
 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_INCLUDE_TIME_UNITS_H_ */
