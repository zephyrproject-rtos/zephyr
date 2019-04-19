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
 * @file   vl53l1_error_strings.h
 * @brief  VL53L1 function declarations for decoding error codes to a
 *         text strings
 */


#ifndef VL53L1_ERROR_STRINGS_H_
#define VL53L1_ERROR_STRINGS_H_

#include "vl53l1_error_codes.h"

#ifdef __cplusplus
extern "C" {
#endif


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


#ifndef VL53L1_USE_EMPTY_STRING

	/* PAL ERROR strings */
	#define  VL53L1_STRING_ERROR_NONE \
			"No Error"
	#define  VL53L1_STRING_ERROR_CALIBRATION_WARNING \
			"Calibration Warning Error"
	#define  VL53L1_STRING_ERROR_MIN_CLIPPED \
			"Min clipped error"
	#define  VL53L1_STRING_ERROR_UNDEFINED \
			"Undefined error"
	#define  VL53L1_STRING_ERROR_INVALID_PARAMS \
			"Invalid parameters error"
	#define  VL53L1_STRING_ERROR_NOT_SUPPORTED \
			"Not supported error"
	#define  VL53L1_STRING_ERROR_RANGE_ERROR \
			"Range error"
	#define  VL53L1_STRING_ERROR_TIME_OUT \
			"Time out error"
	#define  VL53L1_STRING_ERROR_MODE_NOT_SUPPORTED \
			"Mode not supported error"
	#define  VL53L1_STRING_ERROR_BUFFER_TOO_SMALL \
			"Buffer too small"
	#define  VL53L1_STRING_ERROR_COMMS_BUFFER_TOO_SMALL \
			"Comms Buffer too small"
	#define  VL53L1_STRING_ERROR_GPIO_NOT_EXISTING \
			"GPIO not existing"
	#define  VL53L1_STRING_ERROR_GPIO_FUNCTIONALITY_NOT_SUPPORTED \
			"GPIO funct not supported"
	#define  VL53L1_STRING_ERROR_CONTROL_INTERFACE \
			"Control Interface Error"
	#define  VL53L1_STRING_ERROR_INVALID_COMMAND \
			"Invalid Command Error"
	#define  VL53L1_STRING_ERROR_DIVISION_BY_ZERO \
			"Division by zero Error"
	#define  VL53L1_STRING_ERROR_REF_SPAD_INIT \
			"Reference Spad Init Error"
	#define  VL53L1_STRING_ERROR_GPH_SYNC_CHECK_FAIL \
			"GPH Sync Check Fail - API out of sync"
	#define  VL53L1_STRING_ERROR_STREAM_COUNT_CHECK_FAIL \
			"Stream Count Check Fail - API out of sync"
	#define  VL53L1_STRING_ERROR_GPH_ID_CHECK_FAIL \
			"GPH ID Check Fail - API out of sync"
	#define  VL53L1_STRING_ERROR_ZONE_STREAM_COUNT_CHECK_FAIL \
			"Zone Stream Count Check Fail - API out of sync"
	#define  VL53L1_STRING_ERROR_ZONE_GPH_ID_CHECK_FAIL \
			"Zone GPH ID Check Fail - API out of sync"

	#define  VL53L1_STRING_ERROR_XTALK_EXTRACTION_NO_SAMPLES_FAIL \
			"No Xtalk using full array - Xtalk Extract Fail"
	#define  VL53L1_STRING_ERROR_XTALK_EXTRACTION_SIGMA_LIMIT_FAIL \
			"Xtalk does not meet required sigma limit - Xtalk Extract Fail"

	#define  VL53L1_STRING_ERROR_OFFSET_CAL_NO_SAMPLE_FAIL \
		"Offset Cal - one of more stages with no valid samples - fatal"
	#define  VL53L1_STRING_ERROR_OFFSET_CAL_NO_SPADS_ENABLED_FAIL \
		"Offset Cal - one of more stages with no SPADS enables - fatal"
	#define  VL53L1_STRING_ERROR_ZONE_CAL_NO_SAMPLE_FAIL \
		"Zone Cal - one of more zones with no valid samples - fatal"

	#define  VL53L1_STRING_WARNING_REF_SPAD_CHAR_NOT_ENOUGH_SPADS \
		"Ref SPAD Char - Not Enough Good SPADs"
	#define  VL53L1_STRING_WARNING_REF_SPAD_CHAR_RATE_TOO_HIGH \
		"Ref SPAD Char - Final Ref Rate too high"
	#define  VL53L1_STRING_WARNING_REF_SPAD_CHAR_RATE_TOO_LOW \
		"Ref SPAD Char - Final Ref Rate too low"

	#define  VL53L1_STRING_WARNING_OFFSET_CAL_MISSING_SAMPLES \
		"Offset Cal - Less than the requested number of valid samples"
	#define  VL53L1_STRING_WARNING_OFFSET_CAL_SIGMA_TOO_HIGH \
		"Offset Cal - Sigma estimate value too high - offset not stable"
	#define  VL53L1_STRING_WARNING_OFFSET_CAL_RATE_TOO_HIGH \
		"Offset Cal - Rate too high - in pile up"
	#define  VL53L1_STRING_WARNING_OFFSET_CAL_SPAD_COUNT_TOO_LOW \
		"Offset Cal - Insufficient SPADs - offset may not be stable"

	#define  VL53L1_STRING_WARNING_ZONE_CAL_MISSING_SAMPLES \
		"Zone Cal - One or more zone with less than requested valid samples"
	#define  VL53L1_STRING_WARNING_ZONE_CAL_SIGMA_TOO_HIGH \
		"Zone Cal - One of more zones the sigma estimate too high"
	#define  VL53L1_STRING_WARNING_ZONE_CAL_RATE_TOO_HIGH \
		"Zone Cal - One of more zones with rate too high - in pile up"

	#define  VL53L1_STRING_WARNING_XTALK_NO_SAMPLES_FOR_GRADIENT \
			"Xtalk - Gradient sample num = 0"
	#define  VL53L1_STRING_WARNING_XTALK_SIGMA_LIMIT_FOR_GRADIENT \
			"Xtalk - Gradient Sigma > Limit"
	#define  VL53L1_STRING_WARNING_XTALK_MISSING_SAMPLES \
			"Xtalk - Some missing and invalid samples"

	#define  VL53L1_STRING_ERROR_DEVICE_FIRMWARE_TOO_OLD \
			"Device Firmware too old"
	#define  VL53L1_STRING_ERROR_DEVICE_FIRMWARE_TOO_NEW \
			"Device Firmware too new"
	#define  VL53L1_STRING_ERROR_UNIT_TEST_FAIL \
			"Unit Test Fail"
	#define  VL53L1_STRING_ERROR_FILE_READ_FAIL \
			"File Read Fail"
	#define  VL53L1_STRING_ERROR_FILE_WRITE_FAIL \
			"File Write Fail"

	#define  VL53L1_STRING_ERROR_NOT_IMPLEMENTED \
			"Not implemented error"
	#define  VL53L1_STRING_UNKNOW_ERROR_CODE \
			"Unknown Error Code"

#endif /* VL53L1_USE_EMPTY_STRING */


#ifdef __cplusplus
}
#endif

#endif

