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
 * @file vl53l1_error_codes.h
 *
 * @brief Error Code definitions for VL53L1 API.
 *
 */

#ifndef _VL53L1_ERROR_CODES_H_
#define _VL53L1_ERROR_CODES_H_

#include "vl53l1_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/****************************************
 * PRIVATE define do not edit
 ****************************************/

/** @defgroup VL53L1_define_Error_group Error and Warning code returned by API
 *  The following DEFINE are used to identify the PAL ERROR
 *  @{
 */

typedef int8_t VL53L1_Error;

#define VL53L1_ERROR_NONE                              ((VL53L1_Error)  0)
#define VL53L1_ERROR_CALIBRATION_WARNING               ((VL53L1_Error) - 1)
	/*!< Warning invalid calibration data may be in used
		\a  VL53L1_InitData()
		\a VL53L1_GetOffsetCalibrationData
		\a VL53L1_SetOffsetCalibrationData */
#define VL53L1_ERROR_MIN_CLIPPED                       ((VL53L1_Error) - 2)
	/*!< Warning parameter passed was clipped to min before to be applied */

#define VL53L1_ERROR_UNDEFINED                         ((VL53L1_Error) - 3)
	/*!< Unqualified error */
#define VL53L1_ERROR_INVALID_PARAMS                    ((VL53L1_Error) - 4)
	/*!< Parameter passed is invalid or out of range */
#define VL53L1_ERROR_NOT_SUPPORTED                     ((VL53L1_Error) - 5)
	/*!< Function is not supported in current mode or configuration */
#define VL53L1_ERROR_RANGE_ERROR                       ((VL53L1_Error) - 6)
	/*!< Device report a ranging error interrupt status */
#define VL53L1_ERROR_TIME_OUT                          ((VL53L1_Error) - 7)
	/*!< Aborted due to time out */
#define VL53L1_ERROR_MODE_NOT_SUPPORTED                ((VL53L1_Error) - 8)
	/*!< Asked mode is not supported by the device */
#define VL53L1_ERROR_BUFFER_TOO_SMALL                  ((VL53L1_Error) - 9)
	/*!< ... */
#define VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL            ((VL53L1_Error) - 10)
	/*!< Supplied buffer is larger than I2C supports */
#define VL53L1_ERROR_GPIO_NOT_EXISTING                 ((VL53L1_Error) - 11)
	/*!< User tried to setup a non-existing GPIO pin */
#define VL53L1_ERROR_GPIO_FUNCTIONALITY_NOT_SUPPORTED  ((VL53L1_Error) - 12)
	/*!< unsupported GPIO functionality */
#define VL53L1_ERROR_CONTROL_INTERFACE                 ((VL53L1_Error) - 13)
	/*!< error reported from IO functions */
#define VL53L1_ERROR_INVALID_COMMAND                   ((VL53L1_Error) - 14)
	/*!< The command is not allowed in the current device state
	 *  (power down) */
#define VL53L1_ERROR_DIVISION_BY_ZERO                  ((VL53L1_Error) - 15)
	/*!< In the function a division by zero occurs */
#define VL53L1_ERROR_REF_SPAD_INIT                     ((VL53L1_Error) - 16)
	/*!< Error during reference SPAD initialization */
#define VL53L1_ERROR_GPH_SYNC_CHECK_FAIL               ((VL53L1_Error) - 17)
	/*!<  GPH sync interrupt check fail - API out of sync with device*/
#define VL53L1_ERROR_STREAM_COUNT_CHECK_FAIL           ((VL53L1_Error) - 18)
	/*!<  Stream count check fail - API out of sync with device */
#define VL53L1_ERROR_GPH_ID_CHECK_FAIL                 ((VL53L1_Error) - 19)
	/*!<  GPH ID check fail - API out of sync with device */
#define VL53L1_ERROR_ZONE_STREAM_COUNT_CHECK_FAIL      ((VL53L1_Error) - 20)
	/*!<  Zone dynamic config stream count check failed - API out of sync */
#define VL53L1_ERROR_ZONE_GPH_ID_CHECK_FAIL            ((VL53L1_Error) - 21)
	/*!<  Zone dynamic config GPH ID check failed - API out of sync */

#define VL53L1_ERROR_XTALK_EXTRACTION_NO_SAMPLE_FAIL   ((VL53L1_Error) - 22)
	/*!<  Thrown when run_xtalk_extraction fn has 0 succesful samples
	 * when using the full array to sample the xtalk. In this case there is
	 * not enough information to generate new Xtalk parm info. The function
	 * will exit and leave the current xtalk parameters unaltered */
#define VL53L1_ERROR_XTALK_EXTRACTION_SIGMA_LIMIT_FAIL ((VL53L1_Error) - 23)
	/*!<  Thrown when run_xtalk_extraction fn has found that the
	 * avg sigma estimate of the full array xtalk sample is > than the
	 * maximal limit allowed. In this case the xtalk sample is too noisy for
	 * measurement. The function will exit and leave the current xtalk parameters
	 * unaltered. */


#define VL53L1_ERROR_OFFSET_CAL_NO_SAMPLE_FAIL           ((VL53L1_Error) - 24)
	/*!<  Thrown if there one of stages has no valid offset calibration
	 *    samples. A fatal error calibration not valid */
#define VL53L1_ERROR_OFFSET_CAL_NO_SPADS_ENABLED_FAIL    ((VL53L1_Error) - 25)
	/*!<  Thrown if there one of stages has zero effective SPADS
	 *    Traps the case when MM1 SPADs is zero.
	 *    A fatal error calibration not valid */
#define VL53L1_ERROR_ZONE_CAL_NO_SAMPLE_FAIL             ((VL53L1_Error) - 26)
	/*!<  Thrown if then some of the zones have no valid samples
	 *    A fatal error calibration not valid */

#define VL53L1_ERROR_TUNING_PARM_KEY_MISMATCH             ((VL53L1_Error) - 27)
	/*!<  Thrown if the tuning file key table version does not match with
	 * expected value. The driver expects the key table version to match
	 * the compiled default version number in the define
	 * #VL53L1_TUNINGPARM_KEY_TABLE_VERSION_DEFAULT
	 * */

#define VL53L1_WARNING_REF_SPAD_CHAR_NOT_ENOUGH_SPADS   ((VL53L1_Error) - 28)
	/*!<  Thrown if there are less than 5 good SPADs are available. */
#define VL53L1_WARNING_REF_SPAD_CHAR_RATE_TOO_HIGH      ((VL53L1_Error) - 29)
	/*!<  Thrown if the final reference rate is greater than
	      the upper reference rate limit - default is 40 Mcps.
	      Implies a minimum Q3 (x10) SPAD (5) selected */
#define VL53L1_WARNING_REF_SPAD_CHAR_RATE_TOO_LOW       ((VL53L1_Error) - 30)
	/*!<  Thrown if the final reference rate is less than
	      the lower reference rate limit - default is 10 Mcps.
	      Implies maximum Q1 (x1) SPADs selected */


#define VL53L1_WARNING_OFFSET_CAL_MISSING_SAMPLES       ((VL53L1_Error) - 31)
	/*!<  Thrown if there is less than the requested number of
	 *    valid samples. */
#define VL53L1_WARNING_OFFSET_CAL_SIGMA_TOO_HIGH        ((VL53L1_Error) - 32)
	/*!<  Thrown if the offset calibration range sigma estimate is greater
	 *    than 8.0 mm. This is the recommended min value to yield a stable
	 *    offset measurement */
#define VL53L1_WARNING_OFFSET_CAL_RATE_TOO_HIGH         ((VL53L1_Error) - 33)
	/*!< Thrown when VL53L1_run_offset_calibration()  peak rate is greater
	     than that 50.0Mcps. This is the recommended  max rate to avoid
	     pile-up influencing the offset measurement */
#define VL53L1_WARNING_OFFSET_CAL_SPAD_COUNT_TOO_LOW    ((VL53L1_Error) - 34)
	/*!< Thrown when VL53L1_run_offset_calibration() when one of stages
	     range has less that 5.0 effective SPADS. This is the recommended
	     min value to yield a stable offset */


#define VL53L1_WARNING_ZONE_CAL_MISSING_SAMPLES       ((VL53L1_Error) - 35)
	/*!<  Thrown if one of more of the zones have less than
	      the requested number of valid samples */
#define VL53L1_WARNING_ZONE_CAL_SIGMA_TOO_HIGH        ((VL53L1_Error) - 36)
	/*!<  Thrown if one or more zones have sigma estimate value greater
	 *    than 8.0 mm. This is the recommended min value to yield a stable
	 *    offset measurement */
#define VL53L1_WARNING_ZONE_CAL_RATE_TOO_HIGH         ((VL53L1_Error) - 37)
	/*!< Thrown if one of more zones have  peak rate higher than
	      that 50.0Mcps. This is the recommended  max rate to avoid
	     pile-up influencing the offset measurement */


#define VL53L1_WARNING_XTALK_MISSING_SAMPLES             ((VL53L1_Error) - 38)
	/*!< Thrown to notify that some of the xtalk samples did not yield
	 * valid ranging pulse data while attempting to measure
	 * the xtalk signal in vl53l1_run_xtalk_extract(). This can signify any of
	 * the zones are missing samples, for further debug information the
	 * xtalk_results struct should be referred to. This warning is for
	 * notification only, the xtalk pulse and shape have still been generated
	 */
#define VL53L1_WARNING_XTALK_NO_SAMPLES_FOR_GRADIENT     ((VL53L1_Error) - 39)
	/*!< Thrown to notify that some of teh xtalk samples used for gradient
	 * generation did not yield valid ranging pulse data while attempting to
	 * measure the xtalk signal in vl53l1_run_xtalk_extract(). This can signify
	 * that any one of the zones 0-3 yielded no successful samples. The
	 * xtalk_results struct should be referred to for further debug info.
	 * This warning is for notification only, the xtalk pulse and shape
	 * have still been generated.
	 */
#define VL53L1_WARNING_XTALK_SIGMA_LIMIT_FOR_GRADIENT    ((VL53L1_Error) - 40)
/*!< Thrown to notify that some of the xtalk samples used for gradient
	 * generation did not pass the sigma limit check  while attempting to
	 * measure the xtalk signal in vl53l1_run_xtalk_extract(). This can signify
	 * that any one of the zones 0-3 yielded an avg sigma_mm value > the limit.
	 * The xtalk_results struct should be referred to for further debug info.
	 * This warning is for notification only, the xtalk pulse and shape
	 * have still been generated.
	 */

#define VL53L1_ERROR_NOT_IMPLEMENTED                   ((VL53L1_Error) - 41)
	/*!< Tells requested functionality has not been implemented yet or
	 * not compatible with the device */
#define VL53L1_ERROR_PLATFORM_SPECIFIC_START           ((VL53L1_Error) - 60)
	/*!< Tells the starting code for platform */
/** @} VL53L1_define_Error_group */


#ifdef __cplusplus
}
#endif


#endif /* _VL53L1_ERROR_CODES_H_ */
