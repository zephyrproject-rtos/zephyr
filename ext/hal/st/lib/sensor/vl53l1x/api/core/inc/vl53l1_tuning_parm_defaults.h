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
 * @file vl53l1_tuning_parm_defaults.h
 *
 * @brief Define defaults for tuning parm list
 *
 * - Ensures coherence of internal defaults to tuning table
 * - Allows reduction of tuning parm list to only changes from the
 * standard settings defined below
 *
 */


#ifndef _VL53L1_TUNING_PARM_DEFAULTS_H_
#define _VL53L1_TUNING_PARM_DEFAULTS_H_


#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup VL53L1_tuningparmdefault_group VL53L1 Defines
 *  @brief    VL53L1 Tuning Parm Default Values
 *  @{
 */

#define VL53L1_TUNINGPARM_VERSION_DEFAULT \
((uint16_t) 32771)
#define VL53L1_TUNINGPARM_KEY_TABLE_VERSION_DEFAULT \
((uint16_t) 32769)
#define VL53L1_TUNINGPARM_LLD_VERSION_DEFAULT \
((uint16_t) 32833)
#define VL53L1_TUNINGPARM_CONSISTENCY_LITE_PHASE_TOLERANCE_DEFAULT \
((uint8_t) 2)
#define VL53L1_TUNINGPARM_PHASECAL_TARGET_DEFAULT \
((uint8_t) 33)
#define VL53L1_TUNINGPARM_LITE_CAL_REPEAT_RATE_DEFAULT \
((uint16_t) 0)
#define VL53L1_TUNINGPARM_LITE_RANGING_GAIN_FACTOR_DEFAULT \
((uint16_t) 2011)
#define VL53L1_TUNINGPARM_LITE_MIN_CLIP_MM_DEFAULT \
((uint8_t) 0)
#define VL53L1_TUNINGPARM_LITE_LONG_SIGMA_THRESH_MM_DEFAULT \
((uint16_t) 360)
#define VL53L1_TUNINGPARM_LITE_MED_SIGMA_THRESH_MM_DEFAULT \
((uint16_t) 360)
#define VL53L1_TUNINGPARM_LITE_SHORT_SIGMA_THRESH_MM_DEFAULT \
((uint16_t) 360)
#define VL53L1_TUNINGPARM_LITE_LONG_MIN_COUNT_RATE_RTN_MCPS_DEFAULT \
((uint16_t) 192)
#define VL53L1_TUNINGPARM_LITE_MED_MIN_COUNT_RATE_RTN_MCPS_DEFAULT \
((uint16_t) 192)
#define VL53L1_TUNINGPARM_LITE_SHORT_MIN_COUNT_RATE_RTN_MCPS_DEFAULT \
((uint16_t) 192)
#define VL53L1_TUNINGPARM_LITE_SIGMA_EST_PULSE_WIDTH_DEFAULT \
((uint8_t) 8)
#define VL53L1_TUNINGPARM_LITE_SIGMA_EST_AMB_WIDTH_NS_DEFAULT \
((uint8_t) 16)
#define VL53L1_TUNINGPARM_LITE_SIGMA_REF_MM_DEFAULT \
((uint8_t) 1)
#define VL53L1_TUNINGPARM_LITE_RIT_MULT_DEFAULT \
((uint8_t) 64)
#define VL53L1_TUNINGPARM_LITE_SEED_CONFIG_DEFAULT \
((uint8_t) 2)
#define VL53L1_TUNINGPARM_LITE_QUANTIFIER_DEFAULT \
((uint8_t) 2)
#define VL53L1_TUNINGPARM_LITE_FIRST_ORDER_SELECT_DEFAULT \
((uint8_t) 0)
#define VL53L1_TUNINGPARM_LITE_XTALK_MARGIN_KCPS_DEFAULT \
((int16_t) 0)
#define VL53L1_TUNINGPARM_INITIAL_PHASE_RTN_LITE_LONG_RANGE_DEFAULT \
((uint8_t) 14)
#define VL53L1_TUNINGPARM_INITIAL_PHASE_RTN_LITE_MED_RANGE_DEFAULT \
((uint8_t) 10)
#define VL53L1_TUNINGPARM_INITIAL_PHASE_RTN_LITE_SHORT_RANGE_DEFAULT \
((uint8_t) 6)
#define VL53L1_TUNINGPARM_INITIAL_PHASE_REF_LITE_LONG_RANGE_DEFAULT \
((uint8_t) 14)
#define VL53L1_TUNINGPARM_INITIAL_PHASE_REF_LITE_MED_RANGE_DEFAULT \
((uint8_t) 10)
#define VL53L1_TUNINGPARM_INITIAL_PHASE_REF_LITE_SHORT_RANGE_DEFAULT \
((uint8_t) 6)
#define VL53L1_TUNINGPARM_TIMED_SEED_CONFIG_DEFAULT \
((uint8_t) 1)
#define VL53L1_TUNINGPARM_VHV_LOOPBOUND_DEFAULT \
((uint8_t) 32)
#define VL53L1_TUNINGPARM_REFSPADCHAR_DEVICE_TEST_MODE_DEFAULT \
((uint8_t) 8)
#define VL53L1_TUNINGPARM_REFSPADCHAR_VCSEL_PERIOD_DEFAULT \
((uint8_t) 11)
#define VL53L1_TUNINGPARM_REFSPADCHAR_PHASECAL_TIMEOUT_US_DEFAULT \
((uint32_t) 1000)
#define VL53L1_TUNINGPARM_REFSPADCHAR_TARGET_COUNT_RATE_MCPS_DEFAULT \
((uint16_t) 2560)
#define VL53L1_TUNINGPARM_REFSPADCHAR_MIN_COUNTRATE_LIMIT_MCPS_DEFAULT \
((uint16_t) 1280)
#define VL53L1_TUNINGPARM_REFSPADCHAR_MAX_COUNTRATE_LIMIT_MCPS_DEFAULT \
((uint16_t) 5120)
#define VL53L1_TUNINGPARM_OFFSET_CAL_DSS_RATE_MCPS_DEFAULT \
((uint16_t) 2560)
#define VL53L1_TUNINGPARM_OFFSET_CAL_PHASECAL_TIMEOUT_US_DEFAULT \
((uint32_t) 1000)
#define VL53L1_TUNINGPARM_OFFSET_CAL_MM_TIMEOUT_US_DEFAULT \
((uint32_t) 13000)
#define VL53L1_TUNINGPARM_OFFSET_CAL_RANGE_TIMEOUT_US_DEFAULT \
((uint32_t) 13000)
#define VL53L1_TUNINGPARM_OFFSET_CAL_PRE_SAMPLES_DEFAULT \
((uint8_t) 8)
#define VL53L1_TUNINGPARM_OFFSET_CAL_MM1_SAMPLES_DEFAULT \
((uint8_t) 40)
#define VL53L1_TUNINGPARM_OFFSET_CAL_MM2_SAMPLES_DEFAULT \
((uint8_t) 9)
#define VL53L1_TUNINGPARM_SPADMAP_VCSEL_PERIOD_DEFAULT \
((uint8_t) 18)
#define VL53L1_TUNINGPARM_SPADMAP_VCSEL_START_DEFAULT \
((uint8_t) 15)
#define VL53L1_TUNINGPARM_SPADMAP_RATE_LIMIT_MCPS_DEFAULT \
((uint16_t) 12)
#define VL53L1_TUNINGPARM_LITE_DSS_CONFIG_TARGET_TOTAL_RATE_MCPS_DEFAULT \
((uint16_t) 2560)
#define VL53L1_TUNINGPARM_TIMED_DSS_CONFIG_TARGET_TOTAL_RATE_MCPS_DEFAULT \
((uint16_t) 2560)
#define VL53L1_TUNINGPARM_LITE_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT \
((uint32_t) 1000)
#define VL53L1_TUNINGPARM_TIMED_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT \
((uint32_t) 1000)
#define VL53L1_TUNINGPARM_LITE_MM_CONFIG_TIMEOUT_US_DEFAULT \
((uint32_t) 2000)
#define VL53L1_TUNINGPARM_TIMED_MM_CONFIG_TIMEOUT_US_DEFAULT \
((uint32_t) 2000)
#define VL53L1_TUNINGPARM_LITE_RANGE_CONFIG_TIMEOUT_US_DEFAULT \
((uint32_t) 63000)
#define VL53L1_TUNINGPARM_TIMED_RANGE_CONFIG_TIMEOUT_US_DEFAULT \
((uint32_t) 13000)
#define VL53L1_TUNINGPARM_LOWPOWERAUTO_VHV_LOOP_BOUND_DEFAULT \
((uint8_t) 3)
#define VL53L1_TUNINGPARM_LOWPOWERAUTO_MM_CONFIG_TIMEOUT_US_DEFAULT \
((uint32_t) 1)
#define VL53L1_TUNINGPARM_LOWPOWERAUTO_RANGE_CONFIG_TIMEOUT_US_DEFAULT \
((uint32_t) 8000)


#ifdef __cplusplus
}
#endif

#endif /* _VL53L1_LL_DEF_H_ */


