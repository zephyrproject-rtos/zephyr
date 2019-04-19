/*
* Copyright (c) 2017, STMicroelectronics - All Rights Reserved
*
* This file is part of VL53L1 Core and is dual licensed,
* either 'STMicroelectronics
* Proprietary license'
* or 'BSD 3-clause "New" or "Revised" License' , at your option.
*
********************************************************************************
*
* 'STMicroelectronics Proprietary license'
*
********************************************************************************
*
* License terms: STMicroelectronics Proprietary in accordance with licensing
* terms at www.st.com/sla0081
*
* STMicroelectronics confidential
* Reproduction and Communication of this document is strictly prohibited unless
* specifically authorized in writing by STMicroelectronics.
*
*
********************************************************************************
*
* Alternatively, VL53L1 Core may be distributed under the terms of
* 'BSD 3-clause "New" or "Revised" License', in which case the following
* provisions apply instead of the ones mentioned above :
*
********************************************************************************
*
* License terms: BSD 3-clause "New" or "Revised" License.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors
* may be used to endorse or promote products derived from this software
* without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*
********************************************************************************
*
*/

/**
 * @file  vl53l1_core_support.c
 *
 * @brief EwokPlus25 core function definition
 */

#include "vl53l1_ll_def.h"
#include "vl53l1_ll_device.h"
#include "vl53l1_platform_log.h"
#include "vl53l1_core_support.h"
#include "vl53l1_platform_user_data.h"
#include "vl53l1_platform_user_defines.h"

#ifdef VL53L1_LOGGING
#include "vl53l1_debug.h"
#include "vl53l1_register_debug.h"
#endif

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L1_TRACE_MODULE_CORE, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L1_TRACE_MODULE_CORE, status, ##__VA_ARGS__)
#define LOG_FUNCTION_END_FMT(status, fmt, ...) \
	_LOG_FUNCTION_END_FMT(VL53L1_TRACE_MODULE_CORE, \
		status, fmt, ##__VA_ARGS__)

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L1_TRACE_MODULE_CORE, \
	level, VL53L1_TRACE_FUNCTION_NONE, ##__VA_ARGS__)


uint32_t VL53L1_calc_pll_period_us(
	uint16_t  fast_osc_frequency)
{
	/*  Calculates PLL frequency using NVM fast_osc_frequency
	 *  Fast osc frequency fixed point format = unsigned 4.12
	 *
	 *  PLL period fixed point format = unsigned 0.24
	 *  Min input fast osc frequency  = 1 MHz
	 *  PLL Multiplier = 64 (fixed)
	 *  Min PLL freq = 64.0MHz
	 *  -> max PLL period = 1/ 64
	 *  ->  only the 18 LS bits are used
	 *
	 *  2^30 = (2^24) (1.0us) * 4096 (2^12) / 64 (PLL Multiplier)
	 */

	uint32_t  pll_period_us        = 0;

	LOG_FUNCTION_START("");

	pll_period_us = (0x01 << 30) / fast_osc_frequency;

#ifdef VL53L1_LOGGING
	trace_print(VL53L1_TRACE_LEVEL_DEBUG,
			"    %-48s : %10u\n", "pll_period_us",
			pll_period_us);
#endif

	LOG_FUNCTION_END(0);

	return pll_period_us;
}


#ifdef PAL_EXTENDED
uint32_t  VL53L1_duration_maths(
	uint32_t  pll_period_us,
	uint32_t  vcsel_parm_pclks,
	uint32_t  window_vclks,
	uint32_t  elapsed_mclks)
{
	/*
	 * Generates the ranging duration in us
	 *
	 * duration_us = elapsed_mclks * vcsel_perm_pclks *
	 *                        window_vclks * pll_period_us
	 *
	 * returned value in [us] with no fraction bits
	 */

	uint64_t  tmp_long_int = 0;
	uint32_t  duration_us  = 0;

	/* PLL period us =  0.24  18 LS bits used
	 * window_vclks  =  12.0  (2304 max)
	 * output 30b (6.24)
	 */
	duration_us = window_vclks * pll_period_us;

	/* down shift by 12
	 * output 18b (6.12)
	 */
	duration_us = duration_us >> 12;

	/* Save first part of the calc (#1) */
	tmp_long_int = (uint64_t)duration_us;

	/* Multiply elapsed macro periods (22-bit)
	 *      by VCSEL parameter 6.4  (max 63.9999)
	 * output 32b (28.4)
	 */
	duration_us = elapsed_mclks * vcsel_parm_pclks;

	/* down shift by 4 to remove fractional bits (#2)
	 * output 28b (28.0)
	 */
	duration_us = duration_us >> 4;

	/* Multiply #1 18b (6.12) by #2  28b (28.0)
	 * output 46b (34.12)
	 */
	tmp_long_int = tmp_long_int * (uint64_t)duration_us;

	/* Remove fractional part
	 * output 34b (34.0)
	 */
	tmp_long_int = tmp_long_int >> 12;

	/* Clip to 32-bits */
	if (tmp_long_int > 0xFFFFFFFF) {
		tmp_long_int = 0xFFFFFFFF;
	}

	duration_us  = (uint32_t)tmp_long_int;

	return duration_us;
}


uint32_t VL53L1_isqrt(uint32_t num)
{

	/*
	 * Implements an integer square root
	 *
	 * From: http://en.wikipedia.org/wiki/Methods_of_computing_square_roots
	 */

	uint32_t  res = 0;
	uint32_t  bit = 1 << 30; /* The second-to-top bit is set: 1 << 14 for 16-bits, 1 << 30 for 32 bits */

	/* "bit" starts at the highest power of four <= the argument. */
	while (bit > num) {
		bit >>= 2;
	}

	while (bit != 0) {
		if (num >= res + bit)  {
			num -= res + bit;
			res = (res >> 1) + bit;
		} else {
			res >>= 1;
		}
		bit >>= 2;
	}

	return res;
}


uint16_t VL53L1_rate_maths(
	int32_t   events,
	uint32_t  time_us)
{
	/*
	 * Converts events into count rate
	 *
	 * Max events = 512 Mcps * 1sec
	 *            = 512,000,000 events
	 *            = 29b
	 *
	 * If events >  2^24 use  3-bit fractional bits is used internally
	 * otherwise  7-bit fractional bits are used
	 */

	uint32_t  tmp_int   = 0;
	uint32_t  frac_bits = 7;
	uint16_t  rate_mcps = 0; /* 9.7 format */

	/*
	 *  Clip input event range
	 */

	if (events > VL53L1_SPAD_TOTAL_COUNT_MAX) {
		tmp_int = VL53L1_SPAD_TOTAL_COUNT_MAX;
	} else if (events > 0) {
		tmp_int = (uint32_t)events;
	}

	/*
	 * if events > VL53L1_SPAD_TOTAL_COUNT_RES_THRES use 3 rather
	 *  than 7 fractional bits internal to function
	 */

	if (events > VL53L1_SPAD_TOTAL_COUNT_RES_THRES) {
		frac_bits = 3;
	} else {
		frac_bits = 7;
	}

	/*
	 * Create 3 or 7 fractional bits
	 * output 32b (29.3 or 25.7)
	 * Divide by range duration in [us] - no fractional bits
	 */
	if (time_us > 0) {
		tmp_int = ((tmp_int << frac_bits) + (time_us / 2)) / time_us;
	}

	/*
	 * Re align if reduced resolution
	 */
	if (events > VL53L1_SPAD_TOTAL_COUNT_RES_THRES) {
		tmp_int = tmp_int << 4;
	}

	/*
	 * Firmware internal count is 17.7 (24b) but it this
	 * case clip to 16-bit value for reporting
	 */

	if (tmp_int > 0xFFFF) {
		tmp_int = 0xFFFF;
	}

	rate_mcps =  (uint16_t)tmp_int;

	return rate_mcps;
}

uint16_t VL53L1_rate_per_spad_maths(
	uint32_t  frac_bits,
	uint32_t  peak_count_rate,
	uint16_t  num_spads,
	uint32_t  max_output_value)
{

	uint32_t  tmp_int   = 0;

	/* rate_per_spad Format varies with prog frac_bits */
	uint16_t  rate_per_spad = 0;

	/* Calculate rate per spad with variable fractional bits */

	/* Frac_bits should be programmed as final frac_bits - 7 as
	 * the pk_rate contains an inherent 7 bit resolution
	 */

	if (num_spads > 0) {
		tmp_int = (peak_count_rate << 8) << frac_bits;
		tmp_int = (tmp_int + ((uint32_t)num_spads / 2)) / (uint32_t)num_spads;
	} else {
		tmp_int = ((peak_count_rate) << frac_bits);
	}

	/* Clip in case of overwrap - special code */

	if (tmp_int > max_output_value) {
		tmp_int = max_output_value;
	}

	rate_per_spad = (uint16_t)tmp_int;

	return rate_per_spad;
}

int32_t VL53L1_range_maths(
	uint16_t  fast_osc_frequency,
	uint16_t  phase,
	uint16_t  zero_distance_phase,
	uint8_t   fractional_bits,
	int32_t   gain_factor,
	int32_t   range_offset_mm)
{
	/*
	 * Converts phase information into distance in [mm]
	 */

	uint32_t    pll_period_us = 0; /* 0.24 format */
	int64_t     tmp_long_int  = 0;
	int32_t     range_mm      = 0;

	/* Calculate PLL period in [ps] */

	pll_period_us  = VL53L1_calc_pll_period_us(fast_osc_frequency);

	/* Raw range in [mm]
	 *
	 * calculate the phase difference between return and reference phases
	 *
	 * phases 16b (5.11)
	 * output 17b including sign bit
	 */

	tmp_long_int = (int64_t)phase - (int64_t)zero_distance_phase;

	/*
	 * multiply by the PLL period
	 *
	 * PLL period 24bit (0.24) but only 18 LS bits used
	 *
	 * Output  35b (0.35) (17b + 18b)
	 */

	tmp_long_int =  tmp_long_int * (int64_t)pll_period_us;

	/*
	 * Down shift by 9 - Output 26b (0.26)
	 */

	tmp_long_int =  tmp_long_int / (0x01 << 9);

	/*
	 *  multiply by speed of light in air divided by 8
	 *  Factor of 8 includes 2 for the round trip and 4 scaling
	 *
	 *  VL53L1_SPEED_OF_LIGHT_IN_AIR_DIV_8 = 16b (16.2)
	 *
	 *  Output 42b (18.24) (16b + 26b)
	 */

	tmp_long_int =  tmp_long_int * VL53L1_SPEED_OF_LIGHT_IN_AIR_DIV_8;

	/*
	 * Down shift by 22 - Output 20b (18.2)
	 */

	tmp_long_int =  tmp_long_int / (0x01 << 22);

	/* Add range offset */
	range_mm  = (int32_t)tmp_long_int + range_offset_mm;

	/* apply correction gain */
	range_mm *= gain_factor;
	range_mm += 0x0400;
	range_mm /= 0x0800;

	/* Remove fractional bits */
	if (fractional_bits == 0)
		range_mm = range_mm / (0x01 << 2);
	else if (fractional_bits == 1)
		range_mm = range_mm / (0x01 << 1);

	return range_mm;
}
#endif

uint8_t VL53L1_decode_vcsel_period(uint8_t vcsel_period_reg)
{
	/*
	 * Converts the encoded VCSEL period register value into
	 * the real period in PLL clocks
	 */

	uint8_t vcsel_period_pclks = 0;

	vcsel_period_pclks = (vcsel_period_reg + 1) << 1;

	return vcsel_period_pclks;
}


void VL53L1_decode_row_col(
	uint8_t  spad_number,
	uint8_t  *prow,
	uint8_t  *pcol)
{

	/**
	 *  Decodes the array (row,col) location from
	 *  the input SPAD number
	 */

	if (spad_number > 127) {
		*prow = 8 + ((255-spad_number) & 0x07);
		*pcol = (spad_number-128) >> 3;
	} else {
		*prow = spad_number & 0x07;
		*pcol = (127-spad_number) >> 3;
	}
}

