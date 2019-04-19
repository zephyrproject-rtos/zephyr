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
 * @file  vl53l1_core_support.h
 *
 * @brief EwokPlus25 core function definitions
 */

#ifndef _VL53L1_CORE_SUPPORT_H_
#define _VL53L1_CORE_SUPPORT_H_

#include "vl53l1_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Calculates the PLL period in [us] from the input
 *        fast_osc_frequency
 *
 * @param[in] fast_osc_frequency : fast oscillator frequency in 4.12 MHz format
 *
 * @return    pll_period_us : PLL period in [us]  8.24 format
 */

uint32_t VL53L1_calc_pll_period_us(
	uint16_t fast_osc_frequency);



#ifdef PAL_EXTENDED
/**
 * @brief Calculates the ranging duration in ns using fixed point maths
 * *
 * Uses a temporary uint64_t variable internally
 *
 * @param[in]  pll_period_us          : PLL Period in [us] 0.25 format
 * @param[in]  vcsel_parm_pclks       : period,  width or WOI window in PLL clocks
 *                                      in 6.4 format.
 * @param[in]  window_vclks           : ranging or ambient window duration in VCSEL clocks
 * @param[in]  periods_elapsed_mclks  : elapsed time in macro clocks
 *
 * @return     duration_us            : uint32_t containing the duration time in us
 */

uint32_t VL53L1_duration_maths(
	uint32_t  pll_period_us,
	uint32_t  vcsel_parm_pclks,
	uint32_t  window_vclks,
	uint32_t  periods_elapsed_mclks);


/**
 * @brief Calculates the square root of the input integer
 *
 * Reference : http://en.wikipedia.org/wiki/Methods_of_computing_square_roots
 *
 * @param[in]  num : input integer
 *
 * @return     res : square root result
 */

uint32_t VL53L1_isqrt(
	uint32_t  num);

/**
 * @brief Calculates the count rate using fixed point maths
 *
 * Uses a temporary  uint64_t variable internally
 * Any negative negative rates are clipped to 0.
 *
 * @param[in]  events          : accumulated SPAD events
 * @param[in]  time_us         : elapsed time in us
 *
 * @return     rate_mcps       : uint16_t count rate in 9.7 format
 */

uint16_t VL53L1_rate_maths(
	int32_t   events,
	uint32_t  time_us);

/**
 * @brief Calculates the Rate per spad
 *
 * Uses a temporary  uint32_t variable internally
 * Any output rates larger than 16 bit are clipped to 0xFFFF.
 *
 * @param[in]  frac_bits        : required output fractional precision - 7 \
 *                                due to inherent resolution of pk_rate
 * @param[in]  peak_count_rate  : peak signal count rate in mcps
 * @param[in]  num_spads        : actual effective spads in 8.8
 * @param[in]  max_output_value : User set value to clip output
 *
 * @return     rate_per_spad    : uint16_t count rate in variable fractional format
 */

uint16_t VL53L1_rate_per_spad_maths(
	uint32_t  frac_bits,
	uint32_t  peak_count_rate,
	uint16_t  num_spads,
	uint32_t  max_output_value);


/**
 * @brief Calculates the range from the phase data
 *
 * Uses a temporary  int64_t variable internally
 *
 * @param[in]  fast_osc_frequency     : Fast oscillator freq [MHz] 4.12 format
 * @param[in]  phase                  : phase in 5.11 format
 * @param[in]  zero_distance_phase    : zero distance phase in 5.11 format
 * @param[in]  fractional_bits        : valid options : 0, 1, 2
 * @param[in]  gain_factor            : gain correction factor 1.11 format
 * @param[in]  range_offset_mm        : range offset [mm] 14.2 format

 * @return     range_mm  : signed range in [mm]
 *                         format depends on fractional_bits input
 */

int32_t VL53L1_range_maths(
	uint16_t  fast_osc_frequency,
	uint16_t  phase,
	uint16_t  zero_distance_phase,
	uint8_t   fractional_bits,
	int32_t   gain_factor,
	int32_t   range_offset_mm);
#endif


/**
 * @brief  Decodes VCSEL period register value into the real period in PLL clocks
 *
 * @param[in]  vcsel_period_reg   : 8 -bit register value
 *
 * @return     vcsel_period_pclks : 8-bit decoded value
 *
 */

uint8_t VL53L1_decode_vcsel_period(
	uint8_t vcsel_period_reg);

/**
 * @brief Decodes the Byte.Bit coord encoding into an (x,y) coord value
 *
 * @param[in]   spad_number     : Coord location in Byte.Bit format
 * @param[out]  prow            : Decoded row
 * @param[out]  pcol            : Decoded column
 *
 */

void VL53L1_decode_row_col(
	uint8_t   spad_number,
	uint8_t  *prow,
	uint8_t  *pcol);

#ifdef __cplusplus
}
#endif

#endif /* _VL53L1_CORE_SUPPORT_H_ */
