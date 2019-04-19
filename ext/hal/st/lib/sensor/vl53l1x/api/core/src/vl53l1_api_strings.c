
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
 * @file   vl53l1_api_strings.c
 * @brief  VL53L1 API functions for decoding error codes to a text string
 */

#include "vl53l1_api_core.h"
#include "vl53l1_api_strings.h"
#include "vl53l1_error_codes.h"
#include "vl53l1_error_strings.h"

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L1_TRACE_MODULE_API, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L1_TRACE_MODULE_API, status, ##__VA_ARGS__)
#define LOG_FUNCTION_END_FMT(status, fmt, ...) \
	_LOG_FUNCTION_END_FMT(VL53L1_TRACE_MODULE_API, status, fmt, \
			##__VA_ARGS__)


VL53L1_Error VL53L1_get_range_status_string(
	uint8_t   RangeStatus,
	char    *pRangeStatusString)
{
	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

#ifdef VL53L1_USE_EMPTY_STRING
	VL53L1_COPYSTRING(pRangeStatusString, "");
#else
	switch (RangeStatus) {
	case 0:
		VL53L1_COPYSTRING(pRangeStatusString,
			VL53L1_STRING_RANGESTATUS_RANGEVALID);
	break;
	case 1:
		VL53L1_COPYSTRING(pRangeStatusString,
			VL53L1_STRING_RANGESTATUS_SIGMA);
	break;
	case 2:
		VL53L1_COPYSTRING(pRangeStatusString,
			VL53L1_STRING_RANGESTATUS_SIGNAL);
	break;
	case 3:
		VL53L1_COPYSTRING(pRangeStatusString,
			VL53L1_STRING_RANGESTATUS_MINRANGE);
	break;
	case 4:
		VL53L1_COPYSTRING(pRangeStatusString,
			VL53L1_STRING_RANGESTATUS_PHASE);
	break;
	case 5:
		VL53L1_COPYSTRING(pRangeStatusString,
			VL53L1_STRING_RANGESTATUS_HW);
	break;

	default: /**/
		VL53L1_COPYSTRING(pRangeStatusString,
			VL53L1_STRING_RANGESTATUS_NONE);
	}
#endif

	LOG_FUNCTION_END(status);
	return status;
}


VL53L1_Error VL53L1_get_pal_state_string(
	VL53L1_State PalStateCode,
	char *pPalStateString)
{
	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

#ifdef VL53L1_USE_EMPTY_STRING
	VL53L1_COPYSTRING(pPalStateString, "");
#else
	switch (PalStateCode) {
	case VL53L1_STATE_POWERDOWN:
		VL53L1_COPYSTRING(pPalStateString,
			VL53L1_STRING_STATE_POWERDOWN);
	break;
	case VL53L1_STATE_WAIT_STATICINIT:
		VL53L1_COPYSTRING(pPalStateString,
			VL53L1_STRING_STATE_WAIT_STATICINIT);
	break;
	case VL53L1_STATE_STANDBY:
		VL53L1_COPYSTRING(pPalStateString,
			VL53L1_STRING_STATE_STANDBY);
	break;
	case VL53L1_STATE_IDLE:
		VL53L1_COPYSTRING(pPalStateString,
			VL53L1_STRING_STATE_IDLE);
	break;
	case VL53L1_STATE_RUNNING:
		VL53L1_COPYSTRING(pPalStateString,
			VL53L1_STRING_STATE_RUNNING);
	break;
	case VL53L1_STATE_RESET:
		VL53L1_COPYSTRING(pPalStateString,
			VL53L1_STRING_STATE_RESET);
	break;
	case VL53L1_STATE_UNKNOWN:
		VL53L1_COPYSTRING(pPalStateString,
			VL53L1_STRING_STATE_UNKNOWN);
	break;
	case VL53L1_STATE_ERROR:
		VL53L1_COPYSTRING(pPalStateString,
			VL53L1_STRING_STATE_ERROR);
	break;

	default:
		VL53L1_COPYSTRING(pPalStateString,
			VL53L1_STRING_STATE_UNKNOWN);
	}
#endif

	LOG_FUNCTION_END(status);
	return status;
}

VL53L1_Error VL53L1_get_sequence_steps_info(
		VL53L1_SequenceStepId SequenceStepId,
		char *pSequenceStepsString)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

#ifdef VL53L1_USE_EMPTY_STRING
	VL53L1_COPYSTRING(pSequenceStepsString, "");
#else
	switch (SequenceStepId) {
	case VL53L1_SEQUENCESTEP_VHV:
		VL53L1_COPYSTRING(pSequenceStepsString,
				VL53L1_STRING_SEQUENCESTEP_VHV);
	break;
	case VL53L1_SEQUENCESTEP_PHASECAL:
		VL53L1_COPYSTRING(pSequenceStepsString,
				VL53L1_STRING_SEQUENCESTEP_PHASECAL);
	break;
	case VL53L1_SEQUENCESTEP_REFPHASE:
		VL53L1_COPYSTRING(pSequenceStepsString,
				VL53L1_STRING_SEQUENCESTEP_DSS1);
	break;
	case VL53L1_SEQUENCESTEP_DSS1:
		VL53L1_COPYSTRING(pSequenceStepsString,
				VL53L1_STRING_SEQUENCESTEP_DSS1);
	break;
	case VL53L1_SEQUENCESTEP_DSS2:
		VL53L1_COPYSTRING(pSequenceStepsString,
				VL53L1_STRING_SEQUENCESTEP_DSS2);
	break;
	case VL53L1_SEQUENCESTEP_MM1:
		VL53L1_COPYSTRING(pSequenceStepsString,
				VL53L1_STRING_SEQUENCESTEP_MM1);
	break;
	case VL53L1_SEQUENCESTEP_MM2:
		VL53L1_COPYSTRING(pSequenceStepsString,
				VL53L1_STRING_SEQUENCESTEP_MM2);
	break;
	case VL53L1_SEQUENCESTEP_RANGE:
		VL53L1_COPYSTRING(pSequenceStepsString,
				VL53L1_STRING_SEQUENCESTEP_RANGE);
	break;
	default:
		Status = VL53L1_ERROR_INVALID_PARAMS;
	}
#endif

	LOG_FUNCTION_END(Status);

	return Status;
}

VL53L1_Error VL53L1_get_limit_check_info(uint16_t LimitCheckId,
	char *pLimitCheckString)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

#ifdef VL53L1_USE_EMPTY_STRING
	VL53L1_COPYSTRING(pLimitCheckString, "");
#else
	switch (LimitCheckId) {
	case VL53L1_CHECKENABLE_SIGMA_FINAL_RANGE:
		VL53L1_COPYSTRING(pLimitCheckString,
			VL53L1_STRING_CHECKENABLE_SIGMA_FINAL_RANGE);
	break;
	case VL53L1_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE:
		VL53L1_COPYSTRING(pLimitCheckString,
			VL53L1_STRING_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE);
	break;
	default:
		VL53L1_COPYSTRING(pLimitCheckString,
			VL53L1_STRING_UNKNOW_ERROR_CODE);
	}
#endif

	LOG_FUNCTION_END(Status);
	return Status;
}

