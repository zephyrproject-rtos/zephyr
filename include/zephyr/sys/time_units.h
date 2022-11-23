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
#define SYS_TIMEOUT_MS(ms) ((ms) == SYS_FOREVER_MS ? K_FOREVER : K_MSEC(ms))

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

static TIME_CONSTEXPR inline int sys_clock_hw_cycles_per_sec(void)
{
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
	return sys_clock_hw_cycles_per_sec_runtime_get();
#else
	return CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
#endif
}

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
#define Z_TMCVT_USE_FAST_ALGO(from_hz, to_hz) \
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
 */
static TIME_CONSTEXPR ALWAYS_INLINE uint64_t z_tmcvt(uint64_t t, uint32_t from_hz,
						  uint32_t to_hz, bool const_hz,
						  bool result32, bool round_up,
						  bool round_off)
{
	bool mul_ratio = const_hz &&
		(to_hz > from_hz) && ((to_hz % from_hz) == 0U);
	bool div_ratio = const_hz &&
		(from_hz > to_hz) && ((from_hz % to_hz) == 0U);

	if (from_hz == to_hz) {
		return result32 ? ((uint32_t)t) : t;
	}

	uint64_t off = 0;

	if (!mul_ratio) {
		uint32_t rdivisor = div_ratio ? (from_hz / to_hz) : from_hz;

		if (round_up) {
			off = rdivisor - 1U;
		}
		if (round_off) {
			off = rdivisor / 2U;
		}
	}

	/* Select (at build time!) between three different expressions for
	 * the same mathematical relationship, each expressed with and
	 * without truncation to 32 bits (I couldn't find a way to make
	 * the compiler correctly guess at the 32 bit result otherwise).
	 */
	if (div_ratio) {
		t += off;
		if (result32 && (t < BIT64(32))) {
			return ((uint32_t)t) / (from_hz / to_hz);
		} else {
			return t / ((uint64_t)from_hz / to_hz);
		}
	} else if (mul_ratio) {
		if (result32) {
			return ((uint32_t)t) * (to_hz / from_hz);
		} else {
			return t * ((uint64_t)to_hz / from_hz);
		}
	} else {
		if (result32) {
			return (uint32_t)((t * to_hz + off) / from_hz);
		} else if (const_hz && Z_TMCVT_USE_FAST_ALGO(from_hz, to_hz)) {
			/* Faster algorithm but source is first multiplied by target frequency
			 * and it can overflow even though final result would not overflow.
			 * Kconfig option shall prevent use of this algorithm when there is a
			 * risk of overflow.
			 */
			return ((t * to_hz + off) / from_hz);
		} else {
			/* Slower algorithm but input is first divided before being multiplied
			 * which prevents overflow of intermediate value.
			 */
			return (t / from_hz) * to_hz + ((t % from_hz) * to_hz + off) / from_hz;
		}
	}
}

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
 *                 my $type = "u${sz}_t";
 *                 my $const_hz = ($from_unit eq "cyc" || $to_unit eq "cyc")
 *                     ? "Z_CCYC" : "true";
 *                 my $ret32 = $big ? "false" : "true";
 *                 my $rup = $round eq "ceil" ? "true" : "false";
 *                 my $roff = $round eq "near" ? "true" : "false";
 *
 *                 my $hfrom = $human{$from_unit};
 *                 my $hto = $human{$to_unit};
 *                 print "/", "** \@brief Convert $hfrom to $hto\n";
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
 *                 print " * \@return The converted time value\n";
 *                 print " *", "/\n";
 *
 *                 print "static TIME_CONSTEXPR inline $type $sym($type t)\n{\n\t";
 *                 print "/", "* Generated.  Do not edit.  See above. *", "/\n\t";
 *                 print "return z_tmcvt(t, Z_HZ_$from_unit, Z_HZ_$to_unit,";
 *                 print " $const_hz, $ret32, $rup, $roff);\n";
 *                 print "}\n\n";
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

/** @brief Convert milliseconds to hardware cycles
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ms_to_cyc_floor32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, true, false, false);
}

/** @brief Convert milliseconds to hardware cycles
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ms_to_cyc_floor64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, false, false, false);
}

/** @brief Convert milliseconds to hardware cycles
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ms_to_cyc_near32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, true, false, true);
}

/** @brief Convert milliseconds to hardware cycles
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ms_to_cyc_near64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, false, false, true);
}

/** @brief Convert milliseconds to hardware cycles
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ms_to_cyc_ceil32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, true, true, false);
}

/** @brief Convert milliseconds to hardware cycles
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ms_to_cyc_ceil64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ms, Z_HZ_cyc, Z_CCYC, false, true, false);
}

/** @brief Convert milliseconds to ticks
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ms_to_ticks_floor32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ms, Z_HZ_ticks, true, true, false, false);
}

/** @brief Convert milliseconds to ticks
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ms_to_ticks_floor64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ms, Z_HZ_ticks, true, false, false, false);
}

/** @brief Convert milliseconds to ticks
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ms_to_ticks_near32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ms, Z_HZ_ticks, true, true, false, true);
}

/** @brief Convert milliseconds to ticks
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ms_to_ticks_near64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ms, Z_HZ_ticks, true, false, false, true);
}

/** @brief Convert milliseconds to ticks
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ms_to_ticks_ceil32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ms, Z_HZ_ticks, true, true, true, false);
}

/** @brief Convert milliseconds to ticks
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ms_to_ticks_ceil64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ms, Z_HZ_ticks, true, false, true, false);
}

/** @brief Convert microseconds to hardware cycles
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_us_to_cyc_floor32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, true, false, false);
}

/** @brief Convert microseconds to hardware cycles
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_us_to_cyc_floor64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, false, false, false);
}

/** @brief Convert microseconds to hardware cycles
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_us_to_cyc_near32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, true, false, true);
}

/** @brief Convert microseconds to hardware cycles
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_us_to_cyc_near64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, false, false, true);
}

/** @brief Convert microseconds to hardware cycles
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_us_to_cyc_ceil32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, true, true, false);
}

/** @brief Convert microseconds to hardware cycles
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_us_to_cyc_ceil64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_us, Z_HZ_cyc, Z_CCYC, false, true, false);
}

/** @brief Convert microseconds to ticks
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_us_to_ticks_floor32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_us, Z_HZ_ticks, true, true, false, false);
}

/** @brief Convert microseconds to ticks
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_us_to_ticks_floor64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_us, Z_HZ_ticks, true, false, false, false);
}

/** @brief Convert microseconds to ticks
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_us_to_ticks_near32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_us, Z_HZ_ticks, true, true, false, true);
}

/** @brief Convert microseconds to ticks
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_us_to_ticks_near64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_us, Z_HZ_ticks, true, false, false, true);
}

/** @brief Convert microseconds to ticks
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_us_to_ticks_ceil32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_us, Z_HZ_ticks, true, true, true, false);
}

/** @brief Convert microseconds to ticks
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_us_to_ticks_ceil64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_us, Z_HZ_ticks, true, false, true, false);
}

/** @brief Convert nanoseconds to hardware cycles
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ns_to_cyc_floor32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, true, false, false);
}

/** @brief Convert nanoseconds to hardware cycles
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ns_to_cyc_floor64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, false, false, false);
}

/** @brief Convert nanoseconds to hardware cycles
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ns_to_cyc_near32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, true, false, true);
}

/** @brief Convert nanoseconds to hardware cycles
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ns_to_cyc_near64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, false, false, true);
}

/** @brief Convert nanoseconds to hardware cycles
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ns_to_cyc_ceil32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, true, true, false);
}

/** @brief Convert nanoseconds to hardware cycles
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ns_to_cyc_ceil64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ns, Z_HZ_cyc, Z_CCYC, false, true, false);
}

/** @brief Convert nanoseconds to ticks
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ns_to_ticks_floor32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ns, Z_HZ_ticks, true, true, false, false);
}

/** @brief Convert nanoseconds to ticks
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ns_to_ticks_floor64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ns, Z_HZ_ticks, true, false, false, false);
}

/** @brief Convert nanoseconds to ticks
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ns_to_ticks_near32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ns, Z_HZ_ticks, true, true, false, true);
}

/** @brief Convert nanoseconds to ticks
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ns_to_ticks_near64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ns, Z_HZ_ticks, true, false, false, true);
}

/** @brief Convert nanoseconds to ticks
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ns_to_ticks_ceil32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ns, Z_HZ_ticks, true, true, true, false);
}

/** @brief Convert nanoseconds to ticks
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ns_to_ticks_ceil64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ns, Z_HZ_ticks, true, false, true, false);
}

/** @brief Convert hardware cycles to milliseconds
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_cyc_to_ms_floor32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, true, false, false);
}

/** @brief Convert hardware cycles to milliseconds
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_cyc_to_ms_floor64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, false, false, false);
}

/** @brief Convert hardware cycles to milliseconds
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_cyc_to_ms_near32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, true, false, true);
}

/** @brief Convert hardware cycles to milliseconds
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_cyc_to_ms_near64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, false, false, true);
}

/** @brief Convert hardware cycles to milliseconds
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_cyc_to_ms_ceil32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, true, true, false);
}

/** @brief Convert hardware cycles to milliseconds
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_cyc_to_ms_ceil64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ms, Z_CCYC, false, true, false);
}

/** @brief Convert hardware cycles to microseconds
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_cyc_to_us_floor32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, true, false, false);
}

/** @brief Convert hardware cycles to microseconds
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_cyc_to_us_floor64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, false, false, false);
}

/** @brief Convert hardware cycles to microseconds
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_cyc_to_us_near32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, true, false, true);
}

/** @brief Convert hardware cycles to microseconds
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_cyc_to_us_near64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, false, false, true);
}

/** @brief Convert hardware cycles to microseconds
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_cyc_to_us_ceil32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, true, true, false);
}

/** @brief Convert hardware cycles to microseconds
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_cyc_to_us_ceil64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_us, Z_CCYC, false, true, false);
}

/** @brief Convert hardware cycles to nanoseconds
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_cyc_to_ns_floor32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, true, false, false);
}

/** @brief Convert hardware cycles to nanoseconds
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_cyc_to_ns_floor64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, false, false, false);
}

/** @brief Convert hardware cycles to nanoseconds
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_cyc_to_ns_near32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, true, false, true);
}

/** @brief Convert hardware cycles to nanoseconds
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_cyc_to_ns_near64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, false, false, true);
}

/** @brief Convert hardware cycles to nanoseconds
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_cyc_to_ns_ceil32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, true, true, false);
}

/** @brief Convert hardware cycles to nanoseconds
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_cyc_to_ns_ceil64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ns, Z_CCYC, false, true, false);
}

/** @brief Convert hardware cycles to ticks
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_cyc_to_ticks_floor32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, true, false, false);
}

/** @brief Convert hardware cycles to ticks
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_cyc_to_ticks_floor64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, false, false, false);
}

/** @brief Convert hardware cycles to ticks
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_cyc_to_ticks_near32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, true, false, true);
}

/** @brief Convert hardware cycles to ticks
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_cyc_to_ticks_near64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, false, false, true);
}

/** @brief Convert hardware cycles to ticks
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_cyc_to_ticks_ceil32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, true, true, false);
}

/** @brief Convert hardware cycles to ticks
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_cyc_to_ticks_ceil64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_cyc, Z_HZ_ticks, Z_CCYC, false, true, false);
}

/** @brief Convert ticks to milliseconds
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ticks_to_ms_floor32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_ms, true, true, false, false);
}

/** @brief Convert ticks to milliseconds
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ticks_to_ms_floor64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_ms, true, false, false, false);
}

/** @brief Convert ticks to milliseconds
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ticks_to_ms_near32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_ms, true, true, false, true);
}

/** @brief Convert ticks to milliseconds
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ticks_to_ms_near64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_ms, true, false, false, true);
}

/** @brief Convert ticks to milliseconds
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ticks_to_ms_ceil32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_ms, true, true, true, false);
}

/** @brief Convert ticks to milliseconds
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ticks_to_ms_ceil64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_ms, true, false, true, false);
}

/** @brief Convert ticks to microseconds
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ticks_to_us_floor32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_us, true, true, false, false);
}

/** @brief Convert ticks to microseconds
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ticks_to_us_floor64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_us, true, false, false, false);
}

/** @brief Convert ticks to microseconds
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ticks_to_us_near32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_us, true, true, false, true);
}

/** @brief Convert ticks to microseconds
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ticks_to_us_near64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_us, true, false, false, true);
}

/** @brief Convert ticks to microseconds
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ticks_to_us_ceil32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_us, true, true, true, false);
}

/** @brief Convert ticks to microseconds
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ticks_to_us_ceil64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_us, true, false, true, false);
}

/** @brief Convert ticks to nanoseconds
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ticks_to_ns_floor32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_ns, true, true, false, false);
}

/** @brief Convert ticks to nanoseconds
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ticks_to_ns_floor64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_ns, true, false, false, false);
}

/** @brief Convert ticks to nanoseconds
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ticks_to_ns_near32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_ns, true, true, false, true);
}

/** @brief Convert ticks to nanoseconds
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ticks_to_ns_near64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_ns, true, false, false, true);
}

/** @brief Convert ticks to nanoseconds
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ticks_to_ns_ceil32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_ns, true, true, true, false);
}

/** @brief Convert ticks to nanoseconds
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ticks_to_ns_ceil64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_ns, true, false, true, false);
}

/** @brief Convert ticks to hardware cycles
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ticks_to_cyc_floor32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, true, false, false);
}

/** @brief Convert ticks to hardware cycles
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ticks_to_cyc_floor64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, false, false, false);
}

/** @brief Convert ticks to hardware cycles
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ticks_to_cyc_near32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, true, false, true);
}

/** @brief Convert ticks to hardware cycles
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ticks_to_cyc_near64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, false, false, true);
}

/** @brief Convert ticks to hardware cycles
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint32_t k_ticks_to_cyc_ceil32(uint32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, true, true, false);
}

/** @brief Convert ticks to hardware cycles
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static TIME_CONSTEXPR inline uint64_t k_ticks_to_cyc_ceil64(uint64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return z_tmcvt(t, Z_HZ_ticks, Z_HZ_cyc, Z_CCYC, false, true, false);
}

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
#include <syscalls/time_units.h>
#endif

#undef TIME_CONSTEXPR

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_INCLUDE_TIME_UNITS_H_ */
