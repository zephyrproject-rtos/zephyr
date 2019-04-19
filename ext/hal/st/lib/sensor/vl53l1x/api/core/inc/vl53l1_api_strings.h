
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
 * @file   vl53l1_api_strings.h
 * @brief  VL53L1 API function declarations for decoding error codes to a
 *         text strings
 */


#ifndef VL53L1_API_STRINGS_H_
#define VL53L1_API_STRINGS_H_

#include "vl53l1_def.h"

#ifdef __cplusplus
extern "C" {
#endif



/**
 * @brief Generates a string for the input device range status code
 *
 * @param[in]   RangeStatus           : Device Range AStatus Code
 * @param[out]  pRangeStatusString    : pointer to character buffer
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_range_status_string(
	uint8_t   RangeStatus,
	char    *pRangeStatusString);

/**
 * @brief Generates an error string for the input PAL error code
 *
 * @param[in]   PalErrorCode         : PAL Error Code
 * @param[out]  pPalErrorString      : pointer to character buffer
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_pal_error_string(
	VL53L1_Error   PalErrorCode,
	char         *pPalErrorString);

/**
 * @brief Generates a string for the input PAL State code
 *
 * @param[in]   PalStateCode         : PAL State Code
 * @param[out]  pPalStateString      : pointer to character buffer
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_pal_state_string(
	VL53L1_State   PalStateCode,
	char         *pPalStateString);


/**
 * @brief Generates a string for the sequence step Id
 *
 * @param[in]   SequenceStepId            : Sequence Step Id
 * @param[out]  pSequenceStepsString      : pointer to character buffer
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */
VL53L1_Error VL53L1_get_sequence_steps_info(
		VL53L1_SequenceStepId SequenceStepId,
		char *pSequenceStepsString);

/**
 * @brief Generates a string for the limit check Id
 *
 * @param[in]   LimitCheckId            : Limit check Id
 * @param[out]  pLimitCheckString       : pointer to character buffer
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */
VL53L1_Error VL53L1_get_limit_check_info(uint16_t LimitCheckId,
	char *pLimitCheckString);

#ifndef VL53L1_USE_EMPTY_STRING
	#define  VL53L1_STRING_DEVICE_INFO_NAME0          "VL53L1 cut1.0"
	#define  VL53L1_STRING_DEVICE_INFO_NAME1          "VL53L1 cut1.1"
	#define  VL53L1_STRING_DEVICE_INFO_TYPE          "VL53L1"

	/* Range Status */
	#define  VL53L1_STRING_RANGESTATUS_NONE                 "No Update"
	#define  VL53L1_STRING_RANGESTATUS_RANGEVALID           "Range Valid"
	#define  VL53L1_STRING_RANGESTATUS_SIGMA                "Sigma Fail"
	#define  VL53L1_STRING_RANGESTATUS_SIGNAL               "Signal Fail"
	#define  VL53L1_STRING_RANGESTATUS_MINRANGE             "Min Range Fail"
	#define  VL53L1_STRING_RANGESTATUS_PHASE                "Phase Fail"
	#define  VL53L1_STRING_RANGESTATUS_HW                   "Hardware Fail"


	/* Range Status */
	#define  VL53L1_STRING_STATE_POWERDOWN               "POWERDOWN State"
	#define  VL53L1_STRING_STATE_WAIT_STATICINIT \
			"Wait for staticinit State"
	#define  VL53L1_STRING_STATE_STANDBY                 "STANDBY State"
	#define  VL53L1_STRING_STATE_IDLE                    "IDLE State"
	#define  VL53L1_STRING_STATE_RUNNING                 "RUNNING State"
	#define  VL53L1_STRING_STATE_RESET                   "RESET State"
	#define  VL53L1_STRING_STATE_UNKNOWN                 "UNKNOWN State"
	#define  VL53L1_STRING_STATE_ERROR                   "ERROR State"



	/* Check Enable */
	#define  VL53L1_STRING_CHECKENABLE_SIGMA_FINAL_RANGE \
			"SIGMA FINAL RANGE"
	#define  VL53L1_STRING_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE \
			"SIGNAL RATE FINAL RANGE"
	#define  VL53L1_STRING_CHECKENABLE_SIGNAL_MIN_CLIP \
			"SIGNAL MIN CLIP"
	#define  VL53L1_STRING_CHECKENABLE_RANGE_IGNORE_THRESHOLD \
			"RANGE IGNORE THRESHOLD"
	#define  VL53L1_STRING_CHECKENABLE_RANGE_PHASE_HIGH \
			"RANGE PHASE HIGH"
	#define  VL53L1_STRING_CHECKENABLE_RANGE_PHASE_LOW \
			"RANGE PHASE LOW"
	#define  VL53L1_STRING_CHECKENABLE_RANGE_PHASE_CONSISTENCY \
			"RANGE PHASE CONSISTENCY"

	/* Sequence Step */
	#define  VL53L1_STRING_SEQUENCESTEP_VHV         "VHV"
	#define  VL53L1_STRING_SEQUENCESTEP_PHASECAL    "PHASE CAL"
	#define  VL53L1_STRING_SEQUENCESTEP_REFPHASE    "REF PHASE"
	#define  VL53L1_STRING_SEQUENCESTEP_DSS1        "DSS1"
	#define  VL53L1_STRING_SEQUENCESTEP_DSS2        "DSS2"
	#define  VL53L1_STRING_SEQUENCESTEP_MM1         "MM1"
	#define  VL53L1_STRING_SEQUENCESTEP_MM2         "MM2"
	#define  VL53L1_STRING_SEQUENCESTEP_RANGE       "RANGE"
#endif /* VL53L1_USE_EMPTY_STRING */


#ifdef __cplusplus
}
#endif

#endif

