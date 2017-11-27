/*******************************************************************************
Copyright © 2016, STMicroelectronics International N.V.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of STMicroelectronics nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
NON-INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS ARE DISCLAIMED.
IN NO EVENT SHALL STMICROELECTRONICS INTERNATIONAL N.V. BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#ifndef VL53L0X_API_STRINGS_H_
#define VL53L0X_API_STRINGS_H_

#include "vl53l0x_def.h"
#include "vl53l0x_platform.h"

#ifdef __cplusplus
extern "C" {
#endif


VL53L0X_Error VL53L0X_get_device_info(VL53L0X_DEV Dev,
			VL53L0X_DeviceInfo_t *pVL53L0X_DeviceInfo);

VL53L0X_Error VL53L0X_get_device_error_string(VL53L0X_DeviceError ErrorCode,
		char *pDeviceErrorString);

VL53L0X_Error VL53L0X_get_range_status_string(uint8_t RangeStatus,
		char *pRangeStatusString);

VL53L0X_Error VL53L0X_get_pal_error_string(VL53L0X_Error PalErrorCode,
		char *pPalErrorString);

VL53L0X_Error VL53L0X_get_pal_state_string(VL53L0X_State PalStateCode,
		char *pPalStateString);

VL53L0X_Error VL53L0X_get_sequence_steps_info(
		VL53L0X_SequenceStepId SequenceStepId,
		char *pSequenceStepsString);

VL53L0X_Error VL53L0X_get_limit_check_info(VL53L0X_DEV Dev, uint16_t LimitCheckId,
	char *pLimitCheckString);


#ifdef USE_EMPTY_STRING
	#define  VL53L0X_STRING_DEVICE_INFO_NAME                             ""
	#define  VL53L0X_STRING_DEVICE_INFO_NAME_TS0                         ""
	#define  VL53L0X_STRING_DEVICE_INFO_NAME_TS1                         ""
	#define  VL53L0X_STRING_DEVICE_INFO_NAME_TS2                         ""
	#define  VL53L0X_STRING_DEVICE_INFO_NAME_ES1                         ""
	#define  VL53L0X_STRING_DEVICE_INFO_TYPE                             ""

	/* PAL ERROR strings */
	#define  VL53L0X_STRING_ERROR_NONE                                   ""
	#define  VL53L0X_STRING_ERROR_CALIBRATION_WARNING                    ""
	#define  VL53L0X_STRING_ERROR_MIN_CLIPPED                            ""
	#define  VL53L0X_STRING_ERROR_UNDEFINED                              ""
	#define  VL53L0X_STRING_ERROR_INVALID_PARAMS                         ""
	#define  VL53L0X_STRING_ERROR_NOT_SUPPORTED                          ""
	#define  VL53L0X_STRING_ERROR_RANGE_ERROR                            ""
	#define  VL53L0X_STRING_ERROR_TIME_OUT                               ""
	#define  VL53L0X_STRING_ERROR_MODE_NOT_SUPPORTED                     ""
	#define  VL53L0X_STRING_ERROR_BUFFER_TOO_SMALL                       ""
	#define  VL53L0X_STRING_ERROR_GPIO_NOT_EXISTING                      ""
	#define  VL53L0X_STRING_ERROR_GPIO_FUNCTIONALITY_NOT_SUPPORTED       ""
	#define  VL53L0X_STRING_ERROR_CONTROL_INTERFACE                      ""
	#define  VL53L0X_STRING_ERROR_INVALID_COMMAND                        ""
	#define  VL53L0X_STRING_ERROR_DIVISION_BY_ZERO                       ""
	#define  VL53L0X_STRING_ERROR_REF_SPAD_INIT                          ""
	#define  VL53L0X_STRING_ERROR_NOT_IMPLEMENTED                        ""

	#define  VL53L0X_STRING_UNKNOW_ERROR_CODE                            ""



	/* Range Status */
	#define  VL53L0X_STRING_RANGESTATUS_NONE                             ""
	#define  VL53L0X_STRING_RANGESTATUS_RANGEVALID                       ""
	#define  VL53L0X_STRING_RANGESTATUS_SIGMA                            ""
	#define  VL53L0X_STRING_RANGESTATUS_SIGNAL                           ""
	#define  VL53L0X_STRING_RANGESTATUS_MINRANGE                         ""
	#define  VL53L0X_STRING_RANGESTATUS_PHASE                            ""
	#define  VL53L0X_STRING_RANGESTATUS_HW                               ""


	/* Range Status */
	#define  VL53L0X_STRING_STATE_POWERDOWN                              ""
	#define  VL53L0X_STRING_STATE_WAIT_STATICINIT                        ""
	#define  VL53L0X_STRING_STATE_STANDBY                                ""
	#define  VL53L0X_STRING_STATE_IDLE                                   ""
	#define  VL53L0X_STRING_STATE_RUNNING                                ""
	#define  VL53L0X_STRING_STATE_UNKNOWN                                ""
	#define  VL53L0X_STRING_STATE_ERROR                                  ""


	/* Device Specific */
	#define  VL53L0X_STRING_DEVICEERROR_NONE                             ""
	#define  VL53L0X_STRING_DEVICEERROR_VCSELCONTINUITYTESTFAILURE       ""
	#define  VL53L0X_STRING_DEVICEERROR_VCSELWATCHDOGTESTFAILURE         ""
	#define  VL53L0X_STRING_DEVICEERROR_NOVHVVALUEFOUND                  ""
	#define  VL53L0X_STRING_DEVICEERROR_MSRCNOTARGET                     ""
	#define  VL53L0X_STRING_DEVICEERROR_SNRCHECK                         ""
	#define  VL53L0X_STRING_DEVICEERROR_RANGEPHASECHECK                  ""
	#define  VL53L0X_STRING_DEVICEERROR_SIGMATHRESHOLDCHECK              ""
	#define  VL53L0X_STRING_DEVICEERROR_TCC                              ""
	#define  VL53L0X_STRING_DEVICEERROR_PHASECONSISTENCY                 ""
	#define  VL53L0X_STRING_DEVICEERROR_MINCLIP                          ""
	#define  VL53L0X_STRING_DEVICEERROR_RANGECOMPLETE                    ""
	#define  VL53L0X_STRING_DEVICEERROR_ALGOUNDERFLOW                    ""
	#define  VL53L0X_STRING_DEVICEERROR_ALGOOVERFLOW                     ""
	#define  VL53L0X_STRING_DEVICEERROR_RANGEIGNORETHRESHOLD             ""
	#define  VL53L0X_STRING_DEVICEERROR_UNKNOWN                          ""

	/* Check Enable */
	#define  VL53L0X_STRING_CHECKENABLE_SIGMA_FINAL_RANGE                ""
	#define  VL53L0X_STRING_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE          ""
	#define  VL53L0X_STRING_CHECKENABLE_SIGNAL_REF_CLIP                  ""
	#define  VL53L0X_STRING_CHECKENABLE_RANGE_IGNORE_THRESHOLD           ""

	/* Sequence Step */
	#define  VL53L0X_STRING_SEQUENCESTEP_TCC                             ""
	#define  VL53L0X_STRING_SEQUENCESTEP_DSS                             ""
	#define  VL53L0X_STRING_SEQUENCESTEP_MSRC                            ""
	#define  VL53L0X_STRING_SEQUENCESTEP_PRE_RANGE                       ""
	#define  VL53L0X_STRING_SEQUENCESTEP_FINAL_RANGE                     ""
#else
	#define  VL53L0X_STRING_DEVICE_INFO_NAME          "VL53L0X cut1.0"
	#define  VL53L0X_STRING_DEVICE_INFO_NAME_TS0      "VL53L0X TS0"
	#define  VL53L0X_STRING_DEVICE_INFO_NAME_TS1      "VL53L0X TS1"
	#define  VL53L0X_STRING_DEVICE_INFO_NAME_TS2      "VL53L0X TS2"
	#define  VL53L0X_STRING_DEVICE_INFO_NAME_ES1      "VL53L0X ES1 or later"
	#define  VL53L0X_STRING_DEVICE_INFO_TYPE          "VL53L0X"

	/* PAL ERROR strings */
	#define  VL53L0X_STRING_ERROR_NONE \
			"No Error"
	#define  VL53L0X_STRING_ERROR_CALIBRATION_WARNING \
			"Calibration Warning Error"
	#define  VL53L0X_STRING_ERROR_MIN_CLIPPED \
			"Min clipped error"
	#define  VL53L0X_STRING_ERROR_UNDEFINED \
			"Undefined error"
	#define  VL53L0X_STRING_ERROR_INVALID_PARAMS \
			"Invalid parameters error"
	#define  VL53L0X_STRING_ERROR_NOT_SUPPORTED \
			"Not supported error"
	#define  VL53L0X_STRING_ERROR_RANGE_ERROR \
			"Range error"
	#define  VL53L0X_STRING_ERROR_TIME_OUT \
			"Time out error"
	#define  VL53L0X_STRING_ERROR_MODE_NOT_SUPPORTED \
			"Mode not supported error"
	#define  VL53L0X_STRING_ERROR_BUFFER_TOO_SMALL \
			"Buffer too small"
	#define  VL53L0X_STRING_ERROR_GPIO_NOT_EXISTING \
			"GPIO not existing"
	#define  VL53L0X_STRING_ERROR_GPIO_FUNCTIONALITY_NOT_SUPPORTED \
			"GPIO funct not supported"
	#define  VL53L0X_STRING_ERROR_INTERRUPT_NOT_CLEARED \
			"Interrupt not Cleared"
	#define  VL53L0X_STRING_ERROR_CONTROL_INTERFACE \
			"Control Interface Error"
	#define  VL53L0X_STRING_ERROR_INVALID_COMMAND \
			"Invalid Command Error"
	#define  VL53L0X_STRING_ERROR_DIVISION_BY_ZERO \
			"Division by zero Error"
	#define  VL53L0X_STRING_ERROR_REF_SPAD_INIT \
			"Reference Spad Init Error"
	#define  VL53L0X_STRING_ERROR_NOT_IMPLEMENTED \
			"Not implemented error"

	#define  VL53L0X_STRING_UNKNOW_ERROR_CODE \
			"Unknown Error Code"



	/* Range Status */
	#define  VL53L0X_STRING_RANGESTATUS_NONE                 "No Update"
	#define  VL53L0X_STRING_RANGESTATUS_RANGEVALID           "Range Valid"
	#define  VL53L0X_STRING_RANGESTATUS_SIGMA                "Sigma Fail"
	#define  VL53L0X_STRING_RANGESTATUS_SIGNAL               "Signal Fail"
	#define  VL53L0X_STRING_RANGESTATUS_MINRANGE             "Min Range Fail"
	#define  VL53L0X_STRING_RANGESTATUS_PHASE                "Phase Fail"
	#define  VL53L0X_STRING_RANGESTATUS_HW                   "Hardware Fail"


	/* Range Status */
	#define  VL53L0X_STRING_STATE_POWERDOWN               "POWERDOWN State"
	#define  VL53L0X_STRING_STATE_WAIT_STATICINIT \
			"Wait for staticinit State"
	#define  VL53L0X_STRING_STATE_STANDBY                 "STANDBY State"
	#define  VL53L0X_STRING_STATE_IDLE                    "IDLE State"
	#define  VL53L0X_STRING_STATE_RUNNING                 "RUNNING State"
	#define  VL53L0X_STRING_STATE_UNKNOWN                 "UNKNOWN State"
	#define  VL53L0X_STRING_STATE_ERROR                   "ERROR State"


	/* Device Specific */
	#define  VL53L0X_STRING_DEVICEERROR_NONE                   "No Update"
	#define  VL53L0X_STRING_DEVICEERROR_VCSELCONTINUITYTESTFAILURE \
			"VCSEL Continuity Test Failure"
	#define  VL53L0X_STRING_DEVICEERROR_VCSELWATCHDOGTESTFAILURE \
			"VCSEL Watchdog Test Failure"
	#define  VL53L0X_STRING_DEVICEERROR_NOVHVVALUEFOUND \
			"No VHV Value found"
	#define  VL53L0X_STRING_DEVICEERROR_MSRCNOTARGET \
			"MSRC No Target Error"
	#define  VL53L0X_STRING_DEVICEERROR_SNRCHECK \
			"SNR Check Exit"
	#define  VL53L0X_STRING_DEVICEERROR_RANGEPHASECHECK \
			"Range Phase Check Error"
	#define  VL53L0X_STRING_DEVICEERROR_SIGMATHRESHOLDCHECK \
			"Sigma Threshold Check Error"
	#define  VL53L0X_STRING_DEVICEERROR_TCC \
			"TCC Error"
	#define  VL53L0X_STRING_DEVICEERROR_PHASECONSISTENCY \
			"Phase Consistency Error"
	#define  VL53L0X_STRING_DEVICEERROR_MINCLIP \
			"Min Clip Error"
	#define  VL53L0X_STRING_DEVICEERROR_RANGECOMPLETE \
			"Range Complete"
	#define  VL53L0X_STRING_DEVICEERROR_ALGOUNDERFLOW \
			"Range Algo Underflow Error"
	#define  VL53L0X_STRING_DEVICEERROR_ALGOOVERFLOW \
			"Range Algo Overlow Error"
	#define  VL53L0X_STRING_DEVICEERROR_RANGEIGNORETHRESHOLD \
			"Range Ignore Threshold Error"
	#define  VL53L0X_STRING_DEVICEERROR_UNKNOWN \
			"Unknown error code"

	/* Check Enable */
	#define  VL53L0X_STRING_CHECKENABLE_SIGMA_FINAL_RANGE \
			"SIGMA FINAL RANGE"
	#define  VL53L0X_STRING_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE \
			"SIGNAL RATE FINAL RANGE"
	#define  VL53L0X_STRING_CHECKENABLE_SIGNAL_REF_CLIP \
			"SIGNAL REF CLIP"
	#define  VL53L0X_STRING_CHECKENABLE_RANGE_IGNORE_THRESHOLD \
			"RANGE IGNORE THRESHOLD"
	#define  VL53L0X_STRING_CHECKENABLE_SIGNAL_RATE_MSRC \
			"SIGNAL RATE MSRC"
	#define  VL53L0X_STRING_CHECKENABLE_SIGNAL_RATE_PRE_RANGE \
			"SIGNAL RATE PRE RANGE"

	/* Sequence Step */
	#define  VL53L0X_STRING_SEQUENCESTEP_TCC                   "TCC"
	#define  VL53L0X_STRING_SEQUENCESTEP_DSS                   "DSS"
	#define  VL53L0X_STRING_SEQUENCESTEP_MSRC                  "MSRC"
	#define  VL53L0X_STRING_SEQUENCESTEP_PRE_RANGE             "PRE RANGE"
	#define  VL53L0X_STRING_SEQUENCESTEP_FINAL_RANGE           "FINAL RANGE"
#endif /* USE_EMPTY_STRING */


#ifdef __cplusplus
}
#endif

#endif

