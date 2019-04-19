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
 * @file  vl53l1_api_debug.h
 *
 * @brief EwokPlus25 low level API function definitions
 */

#ifndef _VL53L1_API_DEBUG_H_
#define _VL53L1_API_DEBUG_H_

#include "vl53l1_platform.h"

#ifdef __cplusplus
extern "C" {
#endif



/* Start Patch_AdditionalDebugData_11823 */

/**
 * @brief Gets the current LL Driver configuration parameters and the last
 *        set of histogram data for debug
 *
 * @param[in]   Dev    	  : Device Handle
 * @param[out]  pdata     : pointer to VL53L1_additional_data_t data structure
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_additional_data(
	VL53L1_DEV                Dev,
	VL53L1_additional_data_t *pdata);

/* End Patch_AdditionalDebugData_11823 */


#ifdef VL53L1_LOG_ENABLE

/**
 * @brief Implements an sprintf function for  signed fixed point numbers
 *
 * @param[in]   fp_value   : input signed fixed point number
 * @param[in]   frac_bits  : number of fixed point fractional bits
 * @param[in]   buf_size   : size of supplied text buffer
 * @param[out]  pbuffer    : pointer to text buffer
 *
 */

void  VL53L1_signed_fixed_point_sprintf(
	int32_t    fp_value,
	uint8_t    frac_bits,
	uint16_t   buf_size,
	char      *pbuffer);


/**
 * @brief   Convenience function to print out VL53L1_static_nvm_managed_t for debug
 *
 * @param[in]  pdata      : pointer to VL53L1_static_nvm_managed_t
 * @param[in]  pprefix     : pointer to name prefix string
 * @param[in]  trace_flags : logging module enable bit flags
 */

void VL53L1_print_static_nvm_managed(
	VL53L1_static_nvm_managed_t   *pdata,
	char                          *pprefix,
	uint32_t                       trace_flags);


/**
 * @brief   Convenience function to print out VL53L1_customer_nvm_managed_t for debug
 *
 * @param[in]  pdata       : pointer to VL53L1_customer_nvm_managed_t
 * @param[in]  pprefix     : pointer to name prefix string
 * @param[in]  trace_flags : logging module enable bit flags
 */

void VL53L1_print_customer_nvm_managed(
	VL53L1_customer_nvm_managed_t *pdata,
	char                          *pprefix,
	uint32_t                       trace_flags);


/**
 * @brief   Convenience function to print out VL53L1_nvm_copy_data_t for debug
 *
 * @param[in]  pdata    : pointer to VL53L1_nvm_copy_data_t
 * @param[in]  pprefix     : pointer to name prefix string
 * @param[in]  trace_flags : logging module enable bit flags
 */

void VL53L1_print_nvm_copy_data(
	VL53L1_nvm_copy_data_t        *pdata,
	char                          *pprefix,
	uint32_t                       trace_flags);


/**
 * @brief Convenience function to print out the contents of
 *        the Range Results structure for debug
 *
 * @param[in]  pdata       : pointer to a VL53L1_range_results_t structure
 * @param[in]  pprefix     : pointer to name prefix string
 * @param[in]  trace_flags : logging module enable bit flags
 */

void VL53L1_print_range_results(
	VL53L1_range_results_t *pdata,
	char                   *pprefix,
	uint32_t                trace_flags);

/**
 * @brief Convenience function to print out the contents of
 *        the  Range Data structure for debug
 *
 * @param[in]  pdata       : pointer to a VL53L1_range_data_t structure
 * @param[in]  pprefix     : pointer to name prefix string
 * @param[in]  trace_flags : logging module enable bit flags
 */

void VL53L1_print_range_data(
	VL53L1_range_data_t *pdata,
	char                *pprefix,
	uint32_t             trace_flags);

/**
 * @brief Convenience function to print out the contents of
 *        the offset range results structure for debug
 *
 * @param[in]  pdata       : pointer to a VL53L1_offset_range_results_t structure
 * @param[in]  pprefix     : pointer to name prefix string
 * @param[in]  trace_flags : logging module enable bit flags
 */

void VL53L1_print_offset_range_results(
	VL53L1_offset_range_results_t *pdata,
	char                          *pprefix,
	uint32_t                       trace_flags);

/**
 * @brief Convenience function to print out the contents of
 *        the offset range data structure for debug
 *
 * @param[in]  pdata       : pointer to a VL53L1_offset_range_data_t structure
 * @param[in]  pprefix     : pointer to name prefix string
 * @param[in]  trace_flags : logging module enable bit flags
 */

void VL53L1_print_offset_range_data(
	VL53L1_offset_range_data_t *pdata,
	char                       *pprefix,
	uint32_t                    trace_flags);


/**
 * @brief Convenience function to print out the contents of
 *        the peak rate map calibration data structure
 *
 * @param[in]  pdata       : pointer to a VL53L1_cal_peak_rate_map_t structure
 * @param[in]  pprefix     : pointer to name prefix string
 * @param[in]  trace_flags : logging module enable bit flags
 */

void VL53L1_print_cal_peak_rate_map(
	VL53L1_cal_peak_rate_map_t *pdata,
	char                       *pprefix,
	uint32_t                    trace_flags);


/**
 * @brief Convenience function to print out the contents of
 *        the additional offset calibration data structure
 *
 * @param[in]  pdata       : pointer to a VL53L1_additional_offset_cal_data_t structure
 * @param[in]  pprefix     : pointer to name prefix string
 * @param[in]  trace_flags : logging module enable bit flags
 */

void VL53L1_print_additional_offset_cal_data(
	VL53L1_additional_offset_cal_data_t *pdata,
	char                                *pprefix,
	uint32_t                             trace_flags);

/**
 * @brief Convenience function to print out the contents of
 *        the additional  data structure
 *
 * @param[in]  pdata       : pointer to a VL53L1_additional_data_t structure
 * @param[in]  pprefix     : pointer to name prefix string
 * @param[in]  trace_flags : logging module enable bit flags
 */

void VL53L1_print_additional_data(
	VL53L1_additional_data_t *pdata,
	char                     *pprefix,
	uint32_t                 trace_flags);


/**
 * @brief Convenience function to print out the contents of
 *        the LL driver gain calibration data structure
 *
 * @param[in]  pdata       : pointer to a VL53L1_gain_calibration_data_t structure
 * @param[in]  pprefix     : pointer to name prefix string
 * @param[in]  trace_flags : logging module enable bit flags
 */

void VL53L1_print_gain_calibration_data(
	VL53L1_gain_calibration_data_t *pdata,
	char                           *pprefix,
	uint32_t                        trace_flags);


/**
 * @brief Convenience function to print out the contents of
 *        the xtalk configuration data for debug
 *
 * @param[in]  pdata       : pointer to a VL53L1_xtalk_config_t Structure
 * @param[in]  pprefix     : pointer to name prefix string
 * @param[in]  trace_flags : logging module enable bit flags
 */

void VL53L1_print_xtalk_config(
	VL53L1_xtalk_config_t *pdata,
	char                  *pprefix,
	uint32_t               trace_flags);

/**
 * @brief Convenience function to print out the contents of
 *        the Optical Centre structure for debug
 *
 * @param[in]  pdata       : pointer to a VL53L1_optical_centre_t structure
 * @param[in]  pprefix     : pointer to name prefix string
 * @param[in]  trace_flags : logging module enable bit flags
 */

void VL53L1_print_optical_centre(
	VL53L1_optical_centre_t   *pdata,
	char                      *pprefix,
	uint32_t                   trace_flags);


/**
 * @brief Convenience function to print out the contents of
 *        the User Zone (ROI) structure for debug
 *
 * @param[in]  pdata       : pointer to a VL53L1_user_zone_t structure
 * @param[in]  pprefix     : pointer to name prefix string
 * @param[in]  trace_flags : logging module enable bit flags
 */

void VL53L1_print_user_zone(
	VL53L1_user_zone_t   *pdata,
	char                 *pprefix,
	uint32_t              trace_flags);

/**
 * @brief Convenience function for printing out VL53L1_spad_rate_data_t
 *
 * @param[in]  pspad_rates : pointer to VL53L1_spad_rate_data_t
 * @param[in]  pprefix     : pointer to name prefix string
 * @param[in]  trace_flags : logging module enable bit flags
 */

void VL53L1_print_spad_rate_data(
	VL53L1_spad_rate_data_t  *pspad_rates,
	char                     *pprefix,
	uint32_t                  trace_flags);


/**
 * @brief Convenience function for printing out VL53L1_spad_rate_map_t
 *
 * @param[in]  pspad_rates  : pointer to VL53L1_spad_rate_map_t
 * @param[in]  pprefix     : pointer to name prefix string
 * @param[in]  trace_flags : logging module enable bit flags
 */

void VL53L1_print_spad_rate_map(
	VL53L1_spad_rate_data_t  *pspad_rates,
	char                     *pprefix,
	uint32_t                  trace_flags);


#endif /* VL53L1_LOG_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* _VL53L1_API_DEBUG_H_ */
