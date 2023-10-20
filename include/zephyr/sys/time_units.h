/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TIME_UNITS_H_
#define ZEPHYR_INCLUDE_TIME_UNITS_H_

#include <zephyr/sys_time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Types needed to represent and convert system clock time.
 *
 * @defgroup timeutil_sys_clock_apis System Clock Time Unit Helpers
 * @ingroup timeutil_apis
 *
 * @brief Various helper APIs for representing and converting between system
 * clock time units.
 *
 * @{
 */

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
__syscall int sys_clock_hw_cycles_per_sec_runtime_get(void);

static inline int z_impl_sys_clock_hw_cycles_per_sec_runtime_get(void)
{
	extern int z_clock_hw_cycles_per_sec;

	return z_clock_hw_cycles_per_sec;
}
#endif /* CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME */

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
#define sys_clock_hw_cycles_per_sec() sys_clock_hw_cycles_per_sec_runtime_get()
#else
/**
 * @brief Get the system timer frequency.
 * @return system timer frequency in Hz
 */
#define sys_clock_hw_cycles_per_sec() CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#endif

/* Some more concise declarations to simplify the generator script and
 * save bytes below.
 */
#define Z_HZ_ms    MSEC_PER_SEC
#define Z_HZ_us    USEC_PER_SEC
#define Z_HZ_ns    NSEC_PER_SEC
#define Z_HZ_cyc   sys_clock_hw_cycles_per_sec()
#define Z_HZ_ticks CONFIG_SYS_CLOCK_TICKS_PER_SEC
#define Z_CCYC     (!IS_ENABLED(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME))

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

/* Exhaustively enumerated, highly optimized time unit conversion API for the
 * system clock.
 */

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

/**
 * @}
 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_INCLUDE_TIME_UNITS_H_ */
