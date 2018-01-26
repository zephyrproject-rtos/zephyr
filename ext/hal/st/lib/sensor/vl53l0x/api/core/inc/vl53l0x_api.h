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
 *****************************************************************************/

#ifndef _VL53L0X_API_H_
#define _VL53L0X_API_H_

#include "vl53l0x_api_strings.h"
#include "vl53l0x_def.h"
#include "vl53l0x_platform.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _MSC_VER
#   ifdef VL53L0X_API_EXPORTS
#       define VL53L0X_API  __declspec(dllexport)
#   else
#       define VL53L0X_API
#   endif
#else
#   define VL53L0X_API
#endif

/** @defgroup VL53L0X_cut11_group VL53L0X cut1.1 Function Definition
 *  @brief    VL53L0X cut1.1 Function Definition
 *  @{
 */

/** @defgroup VL53L0X_general_group VL53L0X General Functions
 *  @brief    General functions and definitions
 *  @{
 */

/**
 * @brief Return the VL53L0X PAL Implementation Version
 *
 * @note This function doesn't access to the device
 *
 * @param   pVersion              Pointer to current PAL Implementation Version
 * @return  VL53L0X_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetVersion(VL53L0X_Version_t *pVersion);

/**
 * @brief Return the PAL Specification Version used for the current
 * implementation.
 *
 * @note This function doesn't access to the device
 *
 * @param   pPalSpecVersion       Pointer to current PAL Specification Version
 * @return  VL53L0X_ERROR_NONE        Success
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetPalSpecVersion(
	VL53L0X_Version_t *pPalSpecVersion);

/**
 * @brief Reads the Product Revision for a for given Device
 * This function can be used to distinguish cut1.0 from cut1.1.
 *
 * @note This function Access to the device
 *
 * @param   Dev                 Device Handle
 * @param   pProductRevisionMajor  Pointer to Product Revision Major
 * for a given Device
 * @param   pProductRevisionMinor  Pointer to Product Revision Minor
 * for a given Device
 * @return  VL53L0X_ERROR_NONE      Success
 * @return  "Other error code"  See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetProductRevision(VL53L0X_DEV Dev,
	uint8_t *pProductRevisionMajor, uint8_t *pProductRevisionMinor);

/**
 * @brief Reads the Device information for given Device
 *
 * @note This function Access to the device
 *
 * @param   Dev                 Device Handle
 * @param   pVL53L0X_DeviceInfo  Pointer to current device info for a given
 *  Device
 * @return  VL53L0X_ERROR_NONE   Success
 * @return  "Other error code"  See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetDeviceInfo(VL53L0X_DEV Dev,
	VL53L0X_DeviceInfo_t *pVL53L0X_DeviceInfo);

/**
 * @brief Read current status of the error register for the selected device
 *
 * @note This function Access to the device
 *
 * @param   Dev                   Device Handle
 * @param   pDeviceErrorStatus    Pointer to current error code of the device
 * @return  VL53L0X_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetDeviceErrorStatus(VL53L0X_DEV Dev,
	VL53L0X_DeviceError *pDeviceErrorStatus);

/**
 * @brief Human readable Range Status string for a given RangeStatus
 *
 * @note This function doesn't access to the device
 *
 * @param   RangeStatus         The RangeStatus code as stored on
 * @a VL53L0X_RangingMeasurementData_t
 * @param   pRangeStatusString  The returned RangeStatus string.
 * @return  VL53L0X_ERROR_NONE   Success
 * @return  "Other error code"  See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetRangeStatusString(uint8_t RangeStatus,
	char *pRangeStatusString);

/**
 * @brief Human readable error string for a given Error Code
 *
 * @note This function doesn't access to the device
 *
 * @param   ErrorCode           The error code as stored on ::VL53L0X_DeviceError
 * @param   pDeviceErrorString  The error string corresponding to the ErrorCode
 * @return  VL53L0X_ERROR_NONE   Success
 * @return  "Other error code"  See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetDeviceErrorString(
	VL53L0X_DeviceError ErrorCode, char *pDeviceErrorString);

/**
 * @brief Human readable error string for current PAL error status
 *
 * @note This function doesn't access to the device
 *
 * @param   PalErrorCode       The error code as stored on @a VL53L0X_Error
 * @param   pPalErrorString    The error string corresponding to the
 * PalErrorCode
 * @return  VL53L0X_ERROR_NONE  Success
 * @return  "Other error code" See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetPalErrorString(VL53L0X_Error PalErrorCode,
	char *pPalErrorString);

/**
 * @brief Human readable PAL State string
 *
 * @note This function doesn't access to the device
 *
 * @param   PalStateCode          The State code as stored on @a VL53L0X_State
 * @param   pPalStateString       The State string corresponding to the
 * PalStateCode
 * @return  VL53L0X_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetPalStateString(VL53L0X_State PalStateCode,
	char *pPalStateString);

/**
 * @brief Reads the internal state of the PAL for a given Device
 *
 * @note This function doesn't access to the device
 *
 * @param   Dev                   Device Handle
 * @param   pPalState             Pointer to current state of the PAL for a
 * given Device
 * @return  VL53L0X_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetPalState(VL53L0X_DEV Dev,
	VL53L0X_State *pPalState);

/**
 * @brief Set the power mode for a given Device
 * The power mode can be Standby or Idle. Different level of both Standby and
 * Idle can exists.
 * This function should not be used when device is in Ranging state.
 *
 * @note This function Access to the device
 *
 * @param   Dev                   Device Handle
 * @param   PowerMode             The value of the power mode to set.
 * see ::VL53L0X_PowerModes
 *                                Valid values are:
 *                                VL53L0X_POWERMODE_STANDBY_LEVEL1,
 *                                VL53L0X_POWERMODE_IDLE_LEVEL1
 * @return  VL53L0X_ERROR_NONE                  Success
 * @return  VL53L0X_ERROR_MODE_NOT_SUPPORTED    This error occurs when PowerMode
 * is not in the supported list
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetPowerMode(VL53L0X_DEV Dev,
	VL53L0X_PowerModes PowerMode);

/**
 * @brief Get the power mode for a given Device
 *
 * @note This function Access to the device
 *
 * @param   Dev                   Device Handle
 * @param   pPowerMode            Pointer to the current value of the power
 * mode. see ::VL53L0X_PowerModes
 *                                Valid values are:
 *                                VL53L0X_POWERMODE_STANDBY_LEVEL1,
 *                                VL53L0X_POWERMODE_IDLE_LEVEL1
 * @return  VL53L0X_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetPowerMode(VL53L0X_DEV Dev,
	VL53L0X_PowerModes *pPowerMode);

/**
 * Set or over-hide part to part calibration offset
 * \sa VL53L0X_DataInit()   VL53L0X_GetOffsetCalibrationDataMicroMeter()
 *
 * @note This function Access to the device
 *
 * @param   Dev                                Device Handle
 * @param   OffsetCalibrationDataMicroMeter    Offset (microns)
 * @return  VL53L0X_ERROR_NONE                  Success
 * @return  "Other error code"                 See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetOffsetCalibrationDataMicroMeter(
	VL53L0X_DEV Dev, int32_t OffsetCalibrationDataMicroMeter);

/**
 * @brief Get part to part calibration offset
 *
 * @par Function Description
 * Should only be used after a successful call to @a VL53L0X_DataInit to backup
 * device NVM value
 *
 * @note This function Access to the device
 *
 * @param   Dev                                Device Handle
 * @param   pOffsetCalibrationDataMicroMeter   Return part to part
 * calibration offset from device (microns)
 * @return  VL53L0X_ERROR_NONE                  Success
 * @return  "Other error code"                 See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetOffsetCalibrationDataMicroMeter(
	VL53L0X_DEV Dev, int32_t *pOffsetCalibrationDataMicroMeter);

/**
 * Set the linearity corrective gain
 *
 * @note This function Access to the device
 *
 * @param   Dev                                Device Handle
 * @param   LinearityCorrectiveGain            Linearity corrective
 * gain in x1000
 * if value is 1000 then no modification is applied.
 * @return  VL53L0X_ERROR_NONE                  Success
 * @return  "Other error code"                 See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetLinearityCorrectiveGain(VL53L0X_DEV Dev,
	int16_t LinearityCorrectiveGain);

/**
 * @brief Get the linearity corrective gain
 *
 * @par Function Description
 * Should only be used after a successful call to @a VL53L0X_DataInit to backup
 * device NVM value
 *
 * @note This function Access to the device
 *
 * @param   Dev                                Device Handle
 * @param   pLinearityCorrectiveGain           Pointer to the linearity
 * corrective gain in x1000
 * if value is 1000 then no modification is applied.
 * @return  VL53L0X_ERROR_NONE                  Success
 * @return  "Other error code"                 See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetLinearityCorrectiveGain(VL53L0X_DEV Dev,
	uint16_t *pLinearityCorrectiveGain);

/**
 * Set Group parameter Hold state
 *
 * @par Function Description
 * Set or remove device internal group parameter hold
 *
 * @note This function is not Implemented
 *
 * @param   Dev      Device Handle
 * @param   GroupParamHold   Group parameter Hold state to be set (on/off)
 * @return  VL53L0X_ERROR_NOT_IMPLEMENTED        Not implemented
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetGroupParamHold(VL53L0X_DEV Dev,
	uint8_t GroupParamHold);

/**
 * @brief Get the maximal distance for actual setup
 * @par Function Description
 * Device must be initialized through @a VL53L0X_SetParameters() prior calling
 * this function.
 *
 * Any range value more than the value returned is to be considered as
 * "no target detected" or
 * "no target in detectable range"\n
 * @warning The maximal distance depends on the setup
 *
 * @note This function is not Implemented
 *
 * @param   Dev      Device Handle
 * @param   pUpperLimitMilliMeter   The maximal range limit for actual setup
 * (in millimeter)
 * @return  VL53L0X_ERROR_NOT_IMPLEMENTED        Not implemented
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetUpperLimitMilliMeter(VL53L0X_DEV Dev,
	uint16_t *pUpperLimitMilliMeter);


/**
 * @brief Get the Total Signal Rate
 * @par Function Description
 * This function will return the Total Signal Rate after a good ranging is done.
 *
 * @note This function access to Device
 *
 * @param   Dev      Device Handle
 * @param   pTotalSignalRate   Total Signal Rate value in Mega count per second
 * @return  VL53L0X_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_Error VL53L0X_GetTotalSignalRate(VL53L0X_DEV Dev,
	FixPoint1616_t *pTotalSignalRate);

/** @} VL53L0X_general_group */

/** @defgroup VL53L0X_init_group VL53L0X Init Functions
 *  @brief    VL53L0X Init Functions
 *  @{
 */

/**
 * @brief Set new device address
 *
 * After completion the device will answer to the new address programmed.
 * This function should be called when several devices are used in parallel
 * before start programming the sensor.
 * When a single device us used, there is no need to call this function.
 *
 * @note This function Access to the device
 *
 * @param   Dev                   Device Handle
 * @param   DeviceAddress         The new Device address
 * @return  VL53L0X_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetDeviceAddress(VL53L0X_DEV Dev,
	uint8_t DeviceAddress);

/**
 *
 * @brief One time device initialization
 *
 * To be called once and only once after device is brought out of reset
 * (Chip enable) and booted see @a VL53L0X_WaitDeviceBooted()
 *
 * @par Function Description
 * When not used after a fresh device "power up" or reset, it may return
 * @a #VL53L0X_ERROR_CALIBRATION_WARNING meaning wrong calibration data
 * may have been fetched from device that can result in ranging offset error\n
 * If application cannot execute device reset or need to run VL53L0X_DataInit
 * multiple time then it  must ensure proper offset calibration saving and
 * restore on its own by using @a VL53L0X_GetOffsetCalibrationData() on first
 * power up and then @a VL53L0X_SetOffsetCalibrationData() in all subsequent init
 * This function will change the VL53L0X_State from VL53L0X_STATE_POWERDOWN to
 * VL53L0X_STATE_WAIT_STATICINIT.
 *
 * @note This function Access to the device
 *
 * @param   Dev                   Device Handle
 * @return  VL53L0X_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_DataInit(VL53L0X_DEV Dev);

/**
 * @brief Set the tuning settings pointer
 *
 * This function is used to specify the Tuning settings buffer to be used
 * for a given device. The buffer contains all the necessary data to permit
 * the API to write tuning settings.
 * This function permit to force the usage of either external or internal
 * tuning settings.
 *
 * @note This function Access to the device
 *
 * @param   Dev                             Device Handle
 * @param   pTuningSettingBuffer            Pointer to tuning settings buffer.
 * @param   UseInternalTuningSettings       Use internal tuning settings value.
 * @return  VL53L0X_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetTuningSettingBuffer(VL53L0X_DEV Dev,
	uint8_t *pTuningSettingBuffer, uint8_t UseInternalTuningSettings);

/**
 * @brief Get the tuning settings pointer and the internal external switch
 * value.
 *
 * This function is used to get the Tuning settings buffer pointer and the
 * value.
 * of the switch to select either external or internal tuning settings.
 *
 * @note This function Access to the device
 *
 * @param   Dev                        Device Handle
 * @param   ppTuningSettingBuffer      Pointer to tuning settings buffer.
 * @param   pUseInternalTuningSettings Pointer to store Use internal tuning
 *                                     settings value.
 * @return  VL53L0X_ERROR_NONE          Success
 * @return  "Other error code"         See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetTuningSettingBuffer(VL53L0X_DEV Dev,
	uint8_t **ppTuningSettingBuffer, uint8_t *pUseInternalTuningSettings);

/**
 * @brief Do basic device init (and eventually patch loading)
 * This function will change the VL53L0X_State from
 * VL53L0X_STATE_WAIT_STATICINIT to VL53L0X_STATE_IDLE.
 * In this stage all default setting will be applied.
 *
 * @note This function Access to the device
 *
 * @param   Dev                   Device Handle
 * @return  VL53L0X_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_StaticInit(VL53L0X_DEV Dev);

/**
 * @brief Wait for device booted after chip enable (hardware standby)
 * This function can be run only when VL53L0X_State is VL53L0X_STATE_POWERDOWN.
 *
 * @note This function is not Implemented
 *
 * @param   Dev      Device Handle
 * @return  VL53L0X_ERROR_NOT_IMPLEMENTED Not implemented
 *
 */
VL53L0X_API VL53L0X_Error VL53L0X_WaitDeviceBooted(VL53L0X_DEV Dev);

/**
 * @brief Do an hard reset or soft reset (depending on implementation) of the
 * device \nAfter call of this function, device must be in same state as right
 * after a power-up sequence.This function will change the VL53L0X_State to
 * VL53L0X_STATE_POWERDOWN.
 *
 * @note This function Access to the device
 *
 * @param   Dev                   Device Handle
 * @return  VL53L0X_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_ResetDevice(VL53L0X_DEV Dev);

/** @} VL53L0X_init_group */

/** @defgroup VL53L0X_parameters_group VL53L0X Parameters Functions
 *  @brief    Functions used to prepare and setup the device
 *  @{
 */

/**
 * @brief  Prepare device for operation
 * @par Function Description
 * Update device with provided parameters
 * @li Then start ranging operation.
 *
 * @note This function Access to the device
 *
 * @param   Dev                   Device Handle
 * @param   pDeviceParameters     Pointer to store current device parameters.
 * @return  VL53L0X_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetDeviceParameters(VL53L0X_DEV Dev,
	const VL53L0X_DeviceParameters_t *pDeviceParameters);

/**
 * @brief  Retrieve current device parameters
 * @par Function Description
 * Get actual parameters of the device
 * @li Then start ranging operation.
 *
 * @note This function Access to the device
 *
 * @param   Dev                   Device Handle
 * @param   pDeviceParameters     Pointer to store current device parameters.
 * @return  VL53L0X_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetDeviceParameters(VL53L0X_DEV Dev,
	VL53L0X_DeviceParameters_t *pDeviceParameters);

/**
 * @brief  Set a new device mode
 * @par Function Description
 * Set device to a new mode (ranging, histogram ...)
 *
 * @note This function doesn't Access to the device
 *
 * @param   Dev                   Device Handle
 * @param   DeviceMode            New device mode to apply
 *                                Valid values are:
 *                                VL53L0X_DEVICEMODE_SINGLE_RANGING
 *                                VL53L0X_DEVICEMODE_CONTINUOUS_RANGING
 *                                VL53L0X_DEVICEMODE_CONTINUOUS_TIMED_RANGING
 *                                VL53L0X_DEVICEMODE_SINGLE_HISTOGRAM
 *                                VL53L0X_HISTOGRAMMODE_REFERENCE_ONLY
 *                                VL53L0X_HISTOGRAMMODE_RETURN_ONLY
 *                                VL53L0X_HISTOGRAMMODE_BOTH
 *
 *
 * @return  VL53L0X_ERROR_NONE               Success
 * @return  VL53L0X_ERROR_MODE_NOT_SUPPORTED This error occurs when DeviceMode is
 *                                          not in the supported list
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetDeviceMode(VL53L0X_DEV Dev,
	VL53L0X_DeviceModes DeviceMode);

/**
 * @brief  Get current new device mode
 * @par Function Description
 * Get actual mode of the device(ranging, histogram ...)
 *
 * @note This function doesn't Access to the device
 *
 * @param   Dev                   Device Handle
 * @param   pDeviceMode           Pointer to current apply mode value
 *                                Valid values are:
 *                                VL53L0X_DEVICEMODE_SINGLE_RANGING
 *                                VL53L0X_DEVICEMODE_CONTINUOUS_RANGING
 *                                VL53L0X_DEVICEMODE_CONTINUOUS_TIMED_RANGING
 *                                VL53L0X_DEVICEMODE_SINGLE_HISTOGRAM
 *                                VL53L0X_HISTOGRAMMODE_REFERENCE_ONLY
 *                                VL53L0X_HISTOGRAMMODE_RETURN_ONLY
 *                                VL53L0X_HISTOGRAMMODE_BOTH
 *
 * @return  VL53L0X_ERROR_NONE                   Success
 * @return  VL53L0X_ERROR_MODE_NOT_SUPPORTED     This error occurs when
 * DeviceMode is not in the supported list
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetDeviceMode(VL53L0X_DEV Dev,
	VL53L0X_DeviceModes *pDeviceMode);

/**
 * @brief  Sets the resolution of range measurements.
 * @par Function Description
 * Set resolution of range measurements to either 0.25mm if
 * fraction enabled or 1mm if not enabled.
 *
 * @note This function Accesses the device
 *
 * @param   Dev               Device Handle
 * @param   Enable            Enable high resolution
 *
 * @return  VL53L0X_ERROR_NONE               Success
 * @return  "Other error code"              See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetRangeFractionEnable(VL53L0X_DEV Dev,
	uint8_t Enable);

/**
 * @brief  Gets the fraction enable parameter indicating the resolution of
 * range measurements.
 *
 * @par Function Description
 * Gets the fraction enable state, which translates to the resolution of
 * range measurements as follows :Enabled:=0.25mm resolution,
 * Not Enabled:=1mm resolution.
 *
 * @note This function Accesses the device
 *
 * @param   Dev               Device Handle
 * @param   pEnable           Output Parameter reporting the fraction enable state.
 *
 * @return  VL53L0X_ERROR_NONE                   Success
 * @return  "Other error code"                  See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetFractionEnable(VL53L0X_DEV Dev,
	uint8_t *pEnable);

/**
 * @brief  Set a new Histogram mode
 * @par Function Description
 * Set device to a new Histogram mode
 *
 * @note This function doesn't Access to the device
 *
 * @param   Dev                   Device Handle
 * @param   HistogramMode         New device mode to apply
 *                                Valid values are:
 *                                VL53L0X_HISTOGRAMMODE_DISABLED
 *                                VL53L0X_DEVICEMODE_SINGLE_HISTOGRAM
 *                                VL53L0X_HISTOGRAMMODE_REFERENCE_ONLY
 *                                VL53L0X_HISTOGRAMMODE_RETURN_ONLY
 *                                VL53L0X_HISTOGRAMMODE_BOTH
 *
 * @return  VL53L0X_ERROR_NONE                   Success
 * @return  VL53L0X_ERROR_MODE_NOT_SUPPORTED     This error occurs when
 * HistogramMode is not in the supported list
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetHistogramMode(VL53L0X_DEV Dev,
	VL53L0X_HistogramModes HistogramMode);

/**
 * @brief  Get current new device mode
 * @par Function Description
 * Get current Histogram mode of a Device
 *
 * @note This function doesn't Access to the device
 *
 * @param   Dev                   Device Handle
 * @param   pHistogramMode        Pointer to current Histogram Mode value
 *                                Valid values are:
 *                                VL53L0X_HISTOGRAMMODE_DISABLED
 *                                VL53L0X_DEVICEMODE_SINGLE_HISTOGRAM
 *                                VL53L0X_HISTOGRAMMODE_REFERENCE_ONLY
 *                                VL53L0X_HISTOGRAMMODE_RETURN_ONLY
 *                                VL53L0X_HISTOGRAMMODE_BOTH
 * @return  VL53L0X_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetHistogramMode(VL53L0X_DEV Dev,
	VL53L0X_HistogramModes *pHistogramMode);

/**
 * @brief Set Ranging Timing Budget in microseconds
 *
 * @par Function Description
 * Defines the maximum time allowed by the user to the device to run a
 * full ranging sequence for the current mode (ranging, histogram, ASL ...)
 *
 * @note This function Access to the device
 *
 * @param   Dev                                Device Handle
 * @param MeasurementTimingBudgetMicroSeconds  Max measurement time in
 * microseconds.
 *                                   Valid values are:
 *                                   >= 17000 microsecs when wraparound enabled
 *                                   >= 12000 microsecs when wraparound disabled
 * @return  VL53L0X_ERROR_NONE             Success
 * @return  VL53L0X_ERROR_INVALID_PARAMS   This error is returned if
 MeasurementTimingBudgetMicroSeconds out of range
 * @return  "Other error code"            See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetMeasurementTimingBudgetMicroSeconds(
	VL53L0X_DEV Dev, uint32_t MeasurementTimingBudgetMicroSeconds);

/**
 * @brief Get Ranging Timing Budget in microseconds
 *
 * @par Function Description
 * Returns the programmed the maximum time allowed by the user to the
 * device to run a full ranging sequence for the current mode
 * (ranging, histogram, ASL ...)
 *
 * @note This function Access to the device
 *
 * @param   Dev                                    Device Handle
 * @param   pMeasurementTimingBudgetMicroSeconds   Max measurement time in
 * microseconds.
 *                                   Valid values are:
 *                                   >= 17000 microsecs when wraparound enabled
 *                                   >= 12000 microsecs when wraparound disabled
 * @return  VL53L0X_ERROR_NONE                      Success
 * @return  "Other error code"                     See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetMeasurementTimingBudgetMicroSeconds(
	VL53L0X_DEV Dev, uint32_t *pMeasurementTimingBudgetMicroSeconds);

/**
 * @brief Gets the VCSEL pulse period.
 *
 * @par Function Description
 * This function retrieves the VCSEL pulse period for the given period type.
 *
 * @note This function Accesses the device
 *
 * @param   Dev                      Device Handle
 * @param   VcselPeriodType          VCSEL period identifier (pre-range|final).
 * @param   pVCSELPulsePeriod        Pointer to VCSEL period value.
 * @return  VL53L0X_ERROR_NONE        Success
 * @return  VL53L0X_ERROR_INVALID_PARAMS  Error VcselPeriodType parameter not
 *                                       supported.
 * @return  "Other error code"           See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetVcselPulsePeriod(VL53L0X_DEV Dev,
	VL53L0X_VcselPeriod VcselPeriodType, uint8_t *pVCSELPulsePeriod);

/**
 * @brief Sets the VCSEL pulse period.
 *
 * @par Function Description
 * This function retrieves the VCSEL pulse period for the given period type.
 *
 * @note This function Accesses the device
 *
 * @param   Dev                       Device Handle
 * @param   VcselPeriodType	      VCSEL period identifier (pre-range|final).
 * @param   VCSELPulsePeriod          VCSEL period value
 * @return  VL53L0X_ERROR_NONE            Success
 * @return  VL53L0X_ERROR_INVALID_PARAMS  Error VcselPeriodType parameter not
 *                                       supported.
 * @return  "Other error code"           See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetVcselPulsePeriod(VL53L0X_DEV Dev,
	VL53L0X_VcselPeriod VcselPeriodType, uint8_t VCSELPulsePeriod);

/**
 * @brief Sets the (on/off) state of a requested sequence step.
 *
 * @par Function Description
 * This function enables/disables a requested sequence step.
 *
 * @note This function Accesses the device
 *
 * @param   Dev                          Device Handle
 * @param   SequenceStepId	         Sequence step identifier.
 * @param   SequenceStepEnabled          Demanded state {0=Off,1=On}
 *                                       is enabled.
 * @return  VL53L0X_ERROR_NONE            Success
 * @return  VL53L0X_ERROR_INVALID_PARAMS  Error SequenceStepId parameter not
 *                                       supported.
 * @return  "Other error code"           See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetSequenceStepEnable(VL53L0X_DEV Dev,
	VL53L0X_SequenceStepId SequenceStepId, uint8_t SequenceStepEnabled);

/**
 * @brief Gets the (on/off) state of a requested sequence step.
 *
 * @par Function Description
 * This function retrieves the state of a requested sequence step, i.e. on/off.
 *
 * @note This function Accesses the device
 *
 * @param   Dev                    Device Handle
 * @param   SequenceStepId         Sequence step identifier.
 * @param   pSequenceStepEnabled   Out parameter reporting if the sequence step
 *                                 is enabled {0=Off,1=On}.
 * @return  VL53L0X_ERROR_NONE            Success
 * @return  VL53L0X_ERROR_INVALID_PARAMS  Error SequenceStepId parameter not
 *                                       supported.
 * @return  "Other error code"           See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetSequenceStepEnable(VL53L0X_DEV Dev,
	VL53L0X_SequenceStepId SequenceStepId, uint8_t *pSequenceStepEnabled);

/**
 * @brief Gets the (on/off) state of all sequence steps.
 *
 * @par Function Description
 * This function retrieves the state of all sequence step in the scheduler.
 *
 * @note This function Accesses the device
 *
 * @param   Dev                          Device Handle
 * @param   pSchedulerSequenceSteps      Pointer to struct containing result.
 * @return  VL53L0X_ERROR_NONE            Success
 * @return  "Other error code"           See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetSequenceStepEnables(VL53L0X_DEV Dev,
	VL53L0X_SchedulerSequenceSteps_t *pSchedulerSequenceSteps);

/**
 * @brief Sets the timeout of a requested sequence step.
 *
 * @par Function Description
 * This function sets the timeout of a requested sequence step.
 *
 * @note This function Accesses the device
 *
 * @param   Dev                          Device Handle
 * @param   SequenceStepId               Sequence step identifier.
 * @param   TimeOutMilliSecs             Demanded timeout
 * @return  VL53L0X_ERROR_NONE            Success
 * @return  VL53L0X_ERROR_INVALID_PARAMS  Error SequenceStepId parameter not
 *                                       supported.
 * @return  "Other error code"           See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetSequenceStepTimeout(VL53L0X_DEV Dev,
	VL53L0X_SequenceStepId SequenceStepId, FixPoint1616_t TimeOutMilliSecs);

/**
 * @brief Gets the timeout of a requested sequence step.
 *
 * @par Function Description
 * This function retrieves the timeout of a requested sequence step.
 *
 * @note This function Accesses the device
 *
 * @param   Dev                          Device Handle
 * @param   SequenceStepId               Sequence step identifier.
 * @param   pTimeOutMilliSecs            Timeout value.
 * @return  VL53L0X_ERROR_NONE            Success
 * @return  VL53L0X_ERROR_INVALID_PARAMS  Error SequenceStepId parameter not
 *                                       supported.
 * @return  "Other error code"           See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetSequenceStepTimeout(VL53L0X_DEV Dev,
	VL53L0X_SequenceStepId SequenceStepId,
	FixPoint1616_t *pTimeOutMilliSecs);

/**
 * @brief Gets number of sequence steps managed by the API.
 *
 * @par Function Description
 * This function retrieves the number of sequence steps currently managed
 * by the API
 *
 * @note This function Accesses the device
 *
 * @param   Dev                          Device Handle
 * @param   pNumberOfSequenceSteps       Out parameter reporting the number of
 *                                       sequence steps.
 * @return  VL53L0X_ERROR_NONE            Success
 * @return  "Other error code"           See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetNumberOfSequenceSteps(VL53L0X_DEV Dev,
	uint8_t *pNumberOfSequenceSteps);

/**
 * @brief Gets the name of a given sequence step.
 *
 * @par Function Description
 * This function retrieves the name of sequence steps corresponding to
 * SequenceStepId.
 *
 * @note This function doesn't Accesses the device
 *
 * @param   SequenceStepId               Sequence step identifier.
 * @param   pSequenceStepsString         Pointer to Info string
 *
 * @return  VL53L0X_ERROR_NONE            Success
 * @return  "Other error code"           See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetSequenceStepsInfo(
	VL53L0X_SequenceStepId SequenceStepId, char *pSequenceStepsString);

/**
 * Program continuous mode Inter-Measurement period in milliseconds
 *
 * @par Function Description
 * When trying to set too short time return  INVALID_PARAMS minimal value
 *
 * @note This function Access to the device
 *
 * @param   Dev                                  Device Handle
 * @param   InterMeasurementPeriodMilliSeconds   Inter-Measurement Period in ms.
 * @return  VL53L0X_ERROR_NONE                    Success
 * @return  "Other error code"                   See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetInterMeasurementPeriodMilliSeconds(
	VL53L0X_DEV Dev, uint32_t InterMeasurementPeriodMilliSeconds);

/**
 * Get continuous mode Inter-Measurement period in milliseconds
 *
 * @par Function Description
 * When trying to set too short time return  INVALID_PARAMS minimal value
 *
 * @note This function Access to the device
 *
 * @param   Dev                                  Device Handle
 * @param   pInterMeasurementPeriodMilliSeconds  Pointer to programmed
 *  Inter-Measurement Period in milliseconds.
 * @return  VL53L0X_ERROR_NONE                    Success
 * @return  "Other error code"                   See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetInterMeasurementPeriodMilliSeconds(
	VL53L0X_DEV Dev, uint32_t *pInterMeasurementPeriodMilliSeconds);

/**
 * @brief Enable/Disable Cross talk compensation feature
 *
 * @note This function is not Implemented.
 * Enable/Disable Cross Talk by set to zero the Cross Talk value
 * by using @a VL53L0X_SetXTalkCompensationRateMegaCps().
 *
 * @param   Dev                       Device Handle
 * @param   XTalkCompensationEnable   Cross talk compensation
 *  to be set 0=disabled else = enabled
 * @return  VL53L0X_ERROR_NOT_IMPLEMENTED   Not implemented
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetXTalkCompensationEnable(VL53L0X_DEV Dev,
	uint8_t XTalkCompensationEnable);

/**
 * @brief Get Cross talk compensation rate
 *
 * @note This function is not Implemented.
 * Enable/Disable Cross Talk by set to zero the Cross Talk value by
 * using @a VL53L0X_SetXTalkCompensationRateMegaCps().
 *
 * @param   Dev                        Device Handle
 * @param   pXTalkCompensationEnable   Pointer to the Cross talk compensation
 *  state 0=disabled or 1 = enabled
 * @return  VL53L0X_ERROR_NOT_IMPLEMENTED   Not implemented
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetXTalkCompensationEnable(VL53L0X_DEV Dev,
	uint8_t *pXTalkCompensationEnable);

/**
 * @brief Set Cross talk compensation rate
 *
 * @par Function Description
 * Set Cross talk compensation rate.
 *
 * @note This function Access to the device
 *
 * @param   Dev                            Device Handle
 * @param   XTalkCompensationRateMegaCps   Compensation rate in
 *  Mega counts per second (16.16 fix point) see datasheet for details
 * @return  VL53L0X_ERROR_NONE              Success
 * @return  "Other error code"             See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetXTalkCompensationRateMegaCps(VL53L0X_DEV Dev,
	FixPoint1616_t XTalkCompensationRateMegaCps);

/**
 * @brief Get Cross talk compensation rate
 *
 * @par Function Description
 * Get Cross talk compensation rate.
 *
 * @note This function Access to the device
 *
 * @param   Dev                            Device Handle
 * @param   pXTalkCompensationRateMegaCps  Pointer to Compensation rate
 in Mega counts per second (16.16 fix point) see datasheet for details
 * @return  VL53L0X_ERROR_NONE              Success
 * @return  "Other error code"             See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetXTalkCompensationRateMegaCps(VL53L0X_DEV Dev,
	FixPoint1616_t *pXTalkCompensationRateMegaCps);

/**
 * @brief Set Reference Calibration Parameters
 *
 * @par Function Description
 * Set Reference Calibration Parameters.
 *
 * @note This function Access to the device
 *
 * @param   Dev                            Device Handle
 * @param   VhvSettings                    Parameter for VHV
 * @param   PhaseCal                       Parameter for PhaseCal
 * @return  VL53L0X_ERROR_NONE              Success
 * @return  "Other error code"             See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetRefCalibration(VL53L0X_DEV Dev,
	uint8_t VhvSettings, uint8_t PhaseCal);

/**
 * @brief Get Reference Calibration Parameters
 *
 * @par Function Description
 * Get Reference Calibration Parameters.
 *
 * @note This function Access to the device
 *
 * @param   Dev                            Device Handle
 * @param   pVhvSettings                   Pointer to VHV parameter
 * @param   pPhaseCal                      Pointer to PhaseCal Parameter
 * @return  VL53L0X_ERROR_NONE              Success
 * @return  "Other error code"             See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetRefCalibration(VL53L0X_DEV Dev,
	uint8_t *pVhvSettings, uint8_t *pPhaseCal);

/**
 * @brief  Get the number of the check limit managed by a given Device
 *
 * @par Function Description
 * This function give the number of the check limit managed by the Device
 *
 * @note This function doesn't Access to the device
 *
 * @param   pNumberOfLimitCheck           Pointer to the number of check limit.
 * @return  VL53L0X_ERROR_NONE             Success
 * @return  "Other error code"            See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetNumberOfLimitCheck(
	uint16_t *pNumberOfLimitCheck);

/**
 * @brief  Return a description string for a given limit check number
 *
 * @par Function Description
 * This function returns a description string for a given limit check number.
 * The limit check is identified with the LimitCheckId.
 *
 * @note This function doesn't Access to the device
 *
 * @param   Dev                           Device Handle
 * @param   LimitCheckId                  Limit Check ID
 (0<= LimitCheckId < VL53L0X_GetNumberOfLimitCheck() ).
 * @param   pLimitCheckString             Pointer to the
 description string of the given check limit.
 * @return  VL53L0X_ERROR_NONE             Success
 * @return  VL53L0X_ERROR_INVALID_PARAMS   This error is
 returned when LimitCheckId value is out of range.
 * @return  "Other error code"            See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetLimitCheckInfo(VL53L0X_DEV Dev,
	uint16_t LimitCheckId, char *pLimitCheckString);

/**
 * @brief  Return a the Status of the specified check limit
 *
 * @par Function Description
 * This function returns the Status of the specified check limit.
 * The value indicate if the check is fail or not.
 * The limit check is identified with the LimitCheckId.
 *
 * @note This function doesn't Access to the device
 *
 * @param   Dev                           Device Handle
 * @param   LimitCheckId                  Limit Check ID
 (0<= LimitCheckId < VL53L0X_GetNumberOfLimitCheck() ).
 * @param   pLimitCheckStatus             Pointer to the
 Limit Check Status of the given check limit.
 * LimitCheckStatus :
 * 0 the check is not fail
 * 1 the check if fail or not enabled
 *
 * @return  VL53L0X_ERROR_NONE             Success
 * @return  VL53L0X_ERROR_INVALID_PARAMS   This error is
 returned when LimitCheckId value is out of range.
 * @return  "Other error code"            See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetLimitCheckStatus(VL53L0X_DEV Dev,
	uint16_t LimitCheckId, uint8_t *pLimitCheckStatus);

/**
 * @brief  Enable/Disable a specific limit check
 *
 * @par Function Description
 * This function Enable/Disable a specific limit check.
 * The limit check is identified with the LimitCheckId.
 *
 * @note This function doesn't Access to the device
 *
 * @param   Dev                           Device Handle
 * @param   LimitCheckId                  Limit Check ID
 *  (0<= LimitCheckId < VL53L0X_GetNumberOfLimitCheck() ).
 * @param   LimitCheckEnable              if 1 the check limit
 *  corresponding to LimitCheckId is Enabled
 *                                        if 0 the check limit
 *  corresponding to LimitCheckId is disabled
 * @return  VL53L0X_ERROR_NONE             Success
 * @return  VL53L0X_ERROR_INVALID_PARAMS   This error is returned
 *  when LimitCheckId value is out of range.
 * @return  "Other error code"            See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetLimitCheckEnable(VL53L0X_DEV Dev,
	uint16_t LimitCheckId, uint8_t LimitCheckEnable);

/**
 * @brief  Get specific limit check enable state
 *
 * @par Function Description
 * This function get the enable state of a specific limit check.
 * The limit check is identified with the LimitCheckId.
 *
 * @note This function Access to the device
 *
 * @param   Dev                           Device Handle
 * @param   LimitCheckId                  Limit Check ID
 *  (0<= LimitCheckId < VL53L0X_GetNumberOfLimitCheck() ).
 * @param   pLimitCheckEnable             Pointer to the check limit enable
 * value.
 *  if 1 the check limit
 *        corresponding to LimitCheckId is Enabled
 *  if 0 the check limit
 *        corresponding to LimitCheckId is disabled
 * @return  VL53L0X_ERROR_NONE             Success
 * @return  VL53L0X_ERROR_INVALID_PARAMS   This error is returned
 *  when LimitCheckId value is out of range.
 * @return  "Other error code"            See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetLimitCheckEnable(VL53L0X_DEV Dev,
	uint16_t LimitCheckId, uint8_t *pLimitCheckEnable);

/**
 * @brief  Set a specific limit check value
 *
 * @par Function Description
 * This function set a specific limit check value.
 * The limit check is identified with the LimitCheckId.
 *
 * @note This function Access to the device
 *
 * @param   Dev                           Device Handle
 * @param   LimitCheckId                  Limit Check ID
 *  (0<= LimitCheckId < VL53L0X_GetNumberOfLimitCheck() ).
 * @param   LimitCheckValue               Limit check Value for a given
 * LimitCheckId
 * @return  VL53L0X_ERROR_NONE             Success
 * @return  VL53L0X_ERROR_INVALID_PARAMS   This error is returned when either
 *  LimitCheckId or LimitCheckValue value is out of range.
 * @return  "Other error code"            See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetLimitCheckValue(VL53L0X_DEV Dev,
	uint16_t LimitCheckId, FixPoint1616_t LimitCheckValue);

/**
 * @brief  Get a specific limit check value
 *
 * @par Function Description
 * This function get a specific limit check value from device then it updates
 * internal values and check enables.
 * The limit check is identified with the LimitCheckId.
 *
 * @note This function Access to the device
 *
 * @param   Dev                           Device Handle
 * @param   LimitCheckId                  Limit Check ID
 *  (0<= LimitCheckId < VL53L0X_GetNumberOfLimitCheck() ).
 * @param   pLimitCheckValue              Pointer to Limit
 *  check Value for a given LimitCheckId.
 * @return  VL53L0X_ERROR_NONE             Success
 * @return  VL53L0X_ERROR_INVALID_PARAMS   This error is returned
 *  when LimitCheckId value is out of range.
 * @return  "Other error code"            See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetLimitCheckValue(VL53L0X_DEV Dev,
	uint16_t LimitCheckId, FixPoint1616_t *pLimitCheckValue);

/**
 * @brief  Get the current value of the signal used for the limit check
 *
 * @par Function Description
 * This function get a the current value of the signal used for the limit check.
 * To obtain the latest value you should run a ranging before.
 * The value reported is linked to the limit check identified with the
 * LimitCheckId.
 *
 * @note This function Access to the device
 *
 * @param   Dev                           Device Handle
 * @param   LimitCheckId                  Limit Check ID
 *  (0<= LimitCheckId < VL53L0X_GetNumberOfLimitCheck() ).
 * @param   pLimitCheckCurrent            Pointer to current Value for a
 * given LimitCheckId.
 * @return  VL53L0X_ERROR_NONE             Success
 * @return  VL53L0X_ERROR_INVALID_PARAMS   This error is returned when
 * LimitCheckId value is out of range.
 * @return  "Other error code"            See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetLimitCheckCurrent(VL53L0X_DEV Dev,
	uint16_t LimitCheckId, FixPoint1616_t *pLimitCheckCurrent);

/**
 * @brief  Enable (or disable) Wrap around Check
 *
 * @note This function Access to the device
 *
 * @param   Dev                    Device Handle
 * @param   WrapAroundCheckEnable  Wrap around Check to be set
 *                                 0=disabled, other = enabled
 * @return  VL53L0X_ERROR_NONE      Success
 * @return  "Other error code"     See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetWrapAroundCheckEnable(VL53L0X_DEV Dev,
		uint8_t WrapAroundCheckEnable);

/**
 * @brief  Get setup of Wrap around Check
 *
 * @par Function Description
 * This function get the wrapAround check enable parameters
 *
 * @note This function Access to the device
 *
 * @param   Dev                     Device Handle
 * @param   pWrapAroundCheckEnable  Pointer to the Wrap around Check state
 *                                  0=disabled or 1 = enabled
 * @return  VL53L0X_ERROR_NONE       Success
 * @return  "Other error code"      See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetWrapAroundCheckEnable(VL53L0X_DEV Dev,
		uint8_t *pWrapAroundCheckEnable);

/**
 * @brief   Set Dmax Calibration Parameters for a given device
 * When one of the parameter is zero, this function will get parameter
 * from NVM.
 * @note This function doesn't Access to the device
 *
 * @param   Dev                    Device Handle
 * @param   RangeMilliMeter        Calibration Distance
 * @param   SignalRateRtnMegaCps   Signal rate return read at CalDistance
 * @return  VL53L0X_ERROR_NONE      Success
 * @return  "Other error code"     See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetDmaxCalParameters(VL53L0X_DEV Dev,
		uint16_t RangeMilliMeter, FixPoint1616_t SignalRateRtnMegaCps);

/**
 * @brief  Get Dmax Calibration Parameters for a given device
 *
 *
 * @note This function Access to the device
 *
 * @param   Dev                     Device Handle
 * @param   pRangeMilliMeter        Pointer to Calibration Distance
 * @param   pSignalRateRtnMegaCps   Pointer to Signal rate return
 * @return  VL53L0X_ERROR_NONE       Success
 * @return  "Other error code"      See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetDmaxCalParameters(VL53L0X_DEV Dev,
	uint16_t *pRangeMilliMeter, FixPoint1616_t *pSignalRateRtnMegaCps);

/** @} VL53L0X_parameters_group */

/** @defgroup VL53L0X_measurement_group VL53L0X Measurement Functions
 *  @brief    Functions used for the measurements
 *  @{
 */

/**
 * @brief Single shot measurement.
 *
 * @par Function Description
 * Perform simple measurement sequence (Start measure, Wait measure to end,
 * and returns when measurement is done).
 * Once function returns, user can get valid data by calling
 * VL53L0X_GetRangingMeasurement or VL53L0X_GetHistogramMeasurement
 * depending on defined measurement mode
 * User should Clear the interrupt in case this are enabled by using the
 * function VL53L0X_ClearInterruptMask().
 *
 * @warning This function is a blocking function
 *
 * @note This function Access to the device
 *
 * @param   Dev                  Device Handle
 * @return  VL53L0X_ERROR_NONE    Success
 * @return  "Other error code"   See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_PerformSingleMeasurement(VL53L0X_DEV Dev);

/**
 * @brief Perform Reference Calibration
 *
 * @details Perform a reference calibration of the Device.
 * This function should be run from time to time before doing
 * a ranging measurement.
 * This function will launch a special ranging measurement, so
 * if interrupt are enable an interrupt will be done.
 * This function will clear the interrupt generated automatically.
 *
 * @warning This function is a blocking function
 *
 * @note This function Access to the device
 *
 * @param   Dev                  Device Handle
 * @param   pVhvSettings         Pointer to vhv settings parameter.
 * @param   pPhaseCal            Pointer to PhaseCal parameter.
 * @return  VL53L0X_ERROR_NONE    Success
 * @return  "Other error code"   See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_PerformRefCalibration(VL53L0X_DEV Dev,
	uint8_t *pVhvSettings, uint8_t *pPhaseCal);

/**
 * @brief Perform XTalk Measurement
 *
 * @details Measures the current cross talk from glass in front
 * of the sensor.
 * This functions performs a histogram measurement and uses the results
 * to measure the crosstalk. For the function to be successful, there
 * must be no target in front of the sensor.
 *
 * @warning This function is a blocking function
 *
 * @warning This function is not supported when the final range
 * vcsel clock period is set below 10 PCLKS.
 *
 * @note This function Access to the device
 *
 * @param   Dev                  Device Handle
 * @param   TimeoutMs            Histogram measurement duration.
 * @param   pXtalkPerSpad        Output parameter containing the crosstalk
 * measurement result, in MCPS/Spad. Format fixpoint 16:16.
 * @param   pAmbientTooHigh      Output parameter which indicate that
 * pXtalkPerSpad is not good if the Ambient is too high.
 * @return  VL53L0X_ERROR_NONE    Success
 * @return  VL53L0X_ERROR_INVALID_PARAMS vcsel clock period not supported
 * for this operation. Must not be less than 10PCLKS.
 * @return  "Other error code"   See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_PerformXTalkMeasurement(VL53L0X_DEV Dev,
	uint32_t TimeoutMs, FixPoint1616_t *pXtalkPerSpad,
	uint8_t *pAmbientTooHigh);

/**
 * @brief Perform XTalk Calibration
 *
 * @details Perform a XTalk calibration of the Device.
 * This function will launch a ranging measurement, if interrupts
 * are enabled an interrupt will be done.
 * This function will clear the interrupt generated automatically.
 * This function will program a new value for the XTalk compensation
 * and it will enable the cross talk before exit.
 * This function will disable the VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD.
 *
 * @warning This function is a blocking function
 *
 * @note This function Access to the device
 *
 * @note This function change the device mode to
 * VL53L0X_DEVICEMODE_SINGLE_RANGING
 *
 * @param   Dev                  Device Handle
 * @param   XTalkCalDistance     XTalkCalDistance value used for the XTalk
 * computation.
 * @param   pXTalkCompensationRateMegaCps  Pointer to new
 * XTalkCompensation value.
 * @return  VL53L0X_ERROR_NONE    Success
 * @return  "Other error code"   See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_PerformXTalkCalibration(VL53L0X_DEV Dev,
	FixPoint1616_t XTalkCalDistance,
	FixPoint1616_t *pXTalkCompensationRateMegaCps);

/**
 * @brief Perform Offset Calibration
 *
 * @details Perform a Offset calibration of the Device.
 * This function will launch a ranging measurement, if interrupts are
 * enabled an interrupt will be done.
 * This function will clear the interrupt generated automatically.
 * This function will program a new value for the Offset calibration value
 * This function will disable the VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD.
 *
 * @warning This function is a blocking function
 *
 * @note This function Access to the device
 *
 * @note This function does not change the device mode.
 *
 * @param   Dev                  Device Handle
 * @param   CalDistanceMilliMeter     Calibration distance value used for the
 * offset compensation.
 * @param   pOffsetMicroMeter  Pointer to new Offset value computed by the
 * function.
 *
 * @return  VL53L0X_ERROR_NONE    Success
 * @return  "Other error code"   See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_PerformOffsetCalibration(VL53L0X_DEV Dev,
	FixPoint1616_t CalDistanceMilliMeter, int32_t *pOffsetMicroMeter);

/**
 * @brief Start device measurement
 *
 * @details Started measurement will depend on device parameters set through
 * @a VL53L0X_SetParameters()
 * This is a non-blocking function.
 * This function will change the VL53L0X_State from VL53L0X_STATE_IDLE to
 * VL53L0X_STATE_RUNNING.
 *
 * @note This function Access to the device
 *

 * @param   Dev                  Device Handle
 * @return  VL53L0X_ERROR_NONE                  Success
 * @return  VL53L0X_ERROR_MODE_NOT_SUPPORTED    This error occurs when
 * DeviceMode programmed with @a VL53L0X_SetDeviceMode is not in the supported
 * list:
 *                                   Supported mode are:
 *                                   VL53L0X_DEVICEMODE_SINGLE_RANGING,
 *                                   VL53L0X_DEVICEMODE_CONTINUOUS_RANGING,
 *                                   VL53L0X_DEVICEMODE_CONTINUOUS_TIMED_RANGING
 * @return  VL53L0X_ERROR_TIME_OUT    Time out on start measurement
 * @return  "Other error code"   See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_StartMeasurement(VL53L0X_DEV Dev);

/**
 * @brief Stop device measurement
 *
 * @details Will set the device in standby mode at end of current measurement\n
 *          Not necessary in single mode as device shall return automatically
 *          in standby mode at end of measurement.
 *          This function will change the VL53L0X_State from VL53L0X_STATE_RUNNING
 *          to VL53L0X_STATE_IDLE.
 *
 * @note This function Access to the device
 *
 * @param   Dev                  Device Handle
 * @return  VL53L0X_ERROR_NONE    Success
 * @return  "Other error code"   See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_StopMeasurement(VL53L0X_DEV Dev);

/**
 * @brief Return Measurement Data Ready
 *
 * @par Function Description
 * This function indicate that a measurement data is ready.
 * This function check if interrupt mode is used then check is done accordingly.
 * If perform function clear the interrupt, this function will not work,
 * like in case of @a VL53L0X_PerformSingleRangingMeasurement().
 * The previous function is blocking function, VL53L0X_GetMeasurementDataReady
 * is used for non-blocking capture.
 *
 * @note This function Access to the device
 *
 * @param   Dev                    Device Handle
 * @param   pMeasurementDataReady  Pointer to Measurement Data Ready.
 *  0=data not ready, 1 = data ready
 * @return  VL53L0X_ERROR_NONE      Success
 * @return  "Other error code"     See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetMeasurementDataReady(VL53L0X_DEV Dev,
	uint8_t *pMeasurementDataReady);

/**
 * @brief Wait for device ready for a new measurement command.
 * Blocking function.
 *
 * @note This function is not Implemented
 *
 * @param   Dev      Device Handle
 * @param   MaxLoop    Max Number of polling loop (timeout).
 * @return  VL53L0X_ERROR_NOT_IMPLEMENTED   Not implemented
 */
VL53L0X_API VL53L0X_Error VL53L0X_WaitDeviceReadyForNewMeasurement(VL53L0X_DEV Dev,
	uint32_t MaxLoop);

/**
 * @brief Retrieve the Reference Signal after a measurements
 *
 * @par Function Description
 * Get Reference Signal from last successful Ranging measurement
 * This function return a valid value after that you call the
 * @a VL53L0X_GetRangingMeasurementData().
 *
 * @note This function Access to the device
 *
 * @param   Dev                      Device Handle
 * @param   pMeasurementRefSignal    Pointer to the Ref Signal to fill up.
 * @return  VL53L0X_ERROR_NONE        Success
 * @return  "Other error code"       See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetMeasurementRefSignal(VL53L0X_DEV Dev,
	FixPoint1616_t *pMeasurementRefSignal);

/**
 * @brief Retrieve the measurements from device for a given setup
 *
 * @par Function Description
 * Get data from last successful Ranging measurement
 * @warning USER should take care about  @a VL53L0X_GetNumberOfROIZones()
 * before get data.
 * PAL will fill a NumberOfROIZones times the corresponding data
 * structure used in the measurement function.
 *
 * @note This function Access to the device
 *
 * @param   Dev                      Device Handle
 * @param   pRangingMeasurementData  Pointer to the data structure to fill up.
 * @return  VL53L0X_ERROR_NONE        Success
 * @return  "Other error code"       See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetRangingMeasurementData(VL53L0X_DEV Dev,
	VL53L0X_RangingMeasurementData_t *pRangingMeasurementData);

/**
 * @brief Retrieve the measurements from device for a given setup
 *
 * @par Function Description
 * Get data from last successful Histogram measurement
 * @warning USER should take care about  @a VL53L0X_GetNumberOfROIZones()
 * before get data.
 * PAL will fill a NumberOfROIZones times the corresponding data structure
 * used in the measurement function.
 *
 * @note This function is not Implemented
 *
 * @param   Dev                         Device Handle
 * @param   pHistogramMeasurementData   Pointer to the histogram data structure.
 * @return  VL53L0X_ERROR_NOT_IMPLEMENTED   Not implemented
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetHistogramMeasurementData(VL53L0X_DEV Dev,
	VL53L0X_HistogramMeasurementData_t *pHistogramMeasurementData);

/**
 * @brief Performs a single ranging measurement and retrieve the ranging
 * measurement data
 *
 * @par Function Description
 * This function will change the device mode to VL53L0X_DEVICEMODE_SINGLE_RANGING
 * with @a VL53L0X_SetDeviceMode(),
 * It performs measurement with @a VL53L0X_PerformSingleMeasurement()
 * It get data from last successful Ranging measurement with
 * @a VL53L0X_GetRangingMeasurementData.
 * Finally it clear the interrupt with @a VL53L0X_ClearInterruptMask().
 *
 * @note This function Access to the device
 *
 * @note This function change the device mode to
 * VL53L0X_DEVICEMODE_SINGLE_RANGING
 *
 * @param   Dev                       Device Handle
 * @param   pRangingMeasurementData   Pointer to the data structure to fill up.
 * @return  VL53L0X_ERROR_NONE         Success
 * @return  "Other error code"        See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_PerformSingleRangingMeasurement(VL53L0X_DEV Dev,
	VL53L0X_RangingMeasurementData_t *pRangingMeasurementData);

/**
 * @brief Performs a single histogram measurement and retrieve the histogram
 * measurement data
 *   Is equivalent to VL53L0X_PerformSingleMeasurement +
 *   VL53L0X_GetHistogramMeasurementData
 *
 * @par Function Description
 * Get data from last successful Ranging measurement.
 * This function will clear the interrupt in case of these are enabled.
 *
 * @note This function is not Implemented
 *
 * @param   Dev                        Device Handle
 * @param   pHistogramMeasurementData  Pointer to the data structure to fill up.
 * @return  VL53L0X_ERROR_NOT_IMPLEMENTED   Not implemented
 */
VL53L0X_API VL53L0X_Error VL53L0X_PerformSingleHistogramMeasurement(VL53L0X_DEV Dev,
	VL53L0X_HistogramMeasurementData_t *pHistogramMeasurementData);

/**
 * @brief Set the number of ROI Zones to be used for a specific Device
 *
 * @par Function Description
 * Set the number of ROI Zones to be used for a specific Device.
 * The programmed value should be less than the max number of ROI Zones given
 * with @a VL53L0X_GetMaxNumberOfROIZones().
 * This version of API manage only one zone.
 *
 * @param   Dev                           Device Handle
 * @param   NumberOfROIZones              Number of ROI Zones to be used for a
 *  specific Device.
 * @return  VL53L0X_ERROR_NONE             Success
 * @return  VL53L0X_ERROR_INVALID_PARAMS   This error is returned if
 * NumberOfROIZones != 1
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetNumberOfROIZones(VL53L0X_DEV Dev,
	uint8_t NumberOfROIZones);

/**
 * @brief Get the number of ROI Zones managed by the Device
 *
 * @par Function Description
 * Get number of ROI Zones managed by the Device
 * USER should take care about  @a VL53L0X_GetNumberOfROIZones()
 * before get data after a perform measurement.
 * PAL will fill a NumberOfROIZones times the corresponding data
 * structure used in the measurement function.
 *
 * @note This function doesn't Access to the device
 *
 * @param   Dev                   Device Handle
 * @param   pNumberOfROIZones     Pointer to the Number of ROI Zones value.
 * @return  VL53L0X_ERROR_NONE     Success
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetNumberOfROIZones(VL53L0X_DEV Dev,
	uint8_t *pNumberOfROIZones);

/**
 * @brief Get the Maximum number of ROI Zones managed by the Device
 *
 * @par Function Description
 * Get Maximum number of ROI Zones managed by the Device.
 *
 * @note This function doesn't Access to the device
 *
 * @param   Dev                    Device Handle
 * @param   pMaxNumberOfROIZones   Pointer to the Maximum Number
 *  of ROI Zones value.
 * @return  VL53L0X_ERROR_NONE      Success
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetMaxNumberOfROIZones(VL53L0X_DEV Dev,
	uint8_t *pMaxNumberOfROIZones);

/** @} VL53L0X_measurement_group */

/** @defgroup VL53L0X_interrupt_group VL53L0X Interrupt Functions
 *  @brief    Functions used for interrupt managements
 *  @{
 */

/**
 * @brief Set the configuration of GPIO pin for a given device
 *
 * @note This function Access to the device
 *
 * @param   Dev                   Device Handle
 * @param   Pin                   ID of the GPIO Pin
 * @param   Functionality         Select Pin functionality.
 *  Refer to ::VL53L0X_GpioFunctionality
 * @param   DeviceMode            Device Mode associated to the Gpio.
 * @param   Polarity              Set interrupt polarity. Active high
 *   or active low see ::VL53L0X_InterruptPolarity
 * @return  VL53L0X_ERROR_NONE                            Success
 * @return  VL53L0X_ERROR_GPIO_NOT_EXISTING               Only Pin=0 is accepted.
 * @return  VL53L0X_ERROR_GPIO_FUNCTIONALITY_NOT_SUPPORTED    This error occurs
 * when Functionality programmed is not in the supported list:
 *                             Supported value are:
 *                             VL53L0X_GPIOFUNCTIONALITY_OFF,
 *                             VL53L0X_GPIOFUNCTIONALITY_THRESHOLD_CROSSED_LOW,
 *                             VL53L0X_GPIOFUNCTIONALITY_THRESHOLD_CROSSED_HIGH,
 VL53L0X_GPIOFUNCTIONALITY_THRESHOLD_CROSSED_OUT,
 *                               VL53L0X_GPIOFUNCTIONALITY_NEW_MEASURE_READY
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetGpioConfig(VL53L0X_DEV Dev, uint8_t Pin,
	VL53L0X_DeviceModes DeviceMode, VL53L0X_GpioFunctionality Functionality,
	VL53L0X_InterruptPolarity Polarity);

/**
 * @brief Get current configuration for GPIO pin for a given device
 *
 * @note This function Access to the device
 *
 * @param   Dev                   Device Handle
 * @param   Pin                   ID of the GPIO Pin
 * @param   pDeviceMode           Pointer to Device Mode associated to the Gpio.
 * @param   pFunctionality        Pointer to Pin functionality.
 *  Refer to ::VL53L0X_GpioFunctionality
 * @param   pPolarity             Pointer to interrupt polarity.
 *  Active high or active low see ::VL53L0X_InterruptPolarity
 * @return  VL53L0X_ERROR_NONE                            Success
 * @return  VL53L0X_ERROR_GPIO_NOT_EXISTING               Only Pin=0 is accepted.
 * @return  VL53L0X_ERROR_GPIO_FUNCTIONALITY_NOT_SUPPORTED   This error occurs
 * when Functionality programmed is not in the supported list:
 *                      Supported value are:
 *                      VL53L0X_GPIOFUNCTIONALITY_OFF,
 *                      VL53L0X_GPIOFUNCTIONALITY_THRESHOLD_CROSSED_LOW,
 *                      VL53L0X_GPIOFUNCTIONALITY_THRESHOLD_CROSSED_HIGH,
 *                      VL53L0X_GPIOFUNCTIONALITY_THRESHOLD_CROSSED_OUT,
 *                      VL53L0X_GPIOFUNCTIONALITY_NEW_MEASURE_READY
 * @return  "Other error code"    See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetGpioConfig(VL53L0X_DEV Dev, uint8_t Pin,
	VL53L0X_DeviceModes *pDeviceMode,
	VL53L0X_GpioFunctionality *pFunctionality,
	VL53L0X_InterruptPolarity *pPolarity);

/**
 * @brief Set low and high Interrupt thresholds for a given mode
 * (ranging, ALS, ...) for a given device
 *
 * @par Function Description
 * Set low and high Interrupt thresholds for a given mode (ranging, ALS, ...)
 * for a given device
 *
 * @note This function Access to the device
 *
 * @note DeviceMode is ignored for the current device
 *
 * @param   Dev              Device Handle
 * @param   DeviceMode       Device Mode for which change thresholds
 * @param   ThresholdLow     Low threshold (mm, lux ..., depending on the mode)
 * @param   ThresholdHigh    High threshold (mm, lux ..., depending on the mode)
 * @return  VL53L0X_ERROR_NONE    Success
 * @return  "Other error code"   See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetInterruptThresholds(VL53L0X_DEV Dev,
	VL53L0X_DeviceModes DeviceMode, FixPoint1616_t ThresholdLow,
	FixPoint1616_t ThresholdHigh);

/**
 * @brief  Get high and low Interrupt thresholds for a given mode
 *  (ranging, ALS, ...) for a given device
 *
 * @par Function Description
 * Get high and low Interrupt thresholds for a given mode (ranging, ALS, ...)
 * for a given device
 *
 * @note This function Access to the device
 *
 * @note DeviceMode is ignored for the current device
 *
 * @param   Dev              Device Handle
 * @param   DeviceMode       Device Mode from which read thresholds
 * @param   pThresholdLow    Low threshold (mm, lux ..., depending on the mode)
 * @param   pThresholdHigh   High threshold (mm, lux ..., depending on the mode)
 * @return  VL53L0X_ERROR_NONE   Success
 * @return  "Other error code"  See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetInterruptThresholds(VL53L0X_DEV Dev,
	VL53L0X_DeviceModes DeviceMode, FixPoint1616_t *pThresholdLow,
	FixPoint1616_t *pThresholdHigh);

/**
 * @brief Return device stop completion status
 *
 * @par Function Description
 * Returns stop completiob status.
 * User shall call this function after a stop command
 *
 * @note This function Access to the device
 *
 * @param   Dev                    Device Handle
 * @param   pStopStatus            Pointer to status variable to update
 * @return  VL53L0X_ERROR_NONE      Success
 * @return  "Other error code"     See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetStopCompletedStatus(VL53L0X_DEV Dev,
	uint32_t *pStopStatus);


/**
 * @brief Clear given system interrupt condition
 *
 * @par Function Description
 * Clear given interrupt(s).
 *
 * @note This function Access to the device
 *
 * @param   Dev                  Device Handle
 * @param   InterruptMask        Mask of interrupts to clear
 * @return  VL53L0X_ERROR_NONE    Success
 * @return  VL53L0X_ERROR_INTERRUPT_NOT_CLEARED    Cannot clear interrupts
 *
 * @return  "Other error code"   See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_ClearInterruptMask(VL53L0X_DEV Dev,
	uint32_t InterruptMask);

/**
 * @brief Return device interrupt status
 *
 * @par Function Description
 * Returns currently raised interrupts by the device.
 * User shall be able to activate/deactivate interrupts through
 * @a VL53L0X_SetGpioConfig()
 *
 * @note This function Access to the device
 *
 * @param   Dev                    Device Handle
 * @param   pInterruptMaskStatus   Pointer to status variable to update
 * @return  VL53L0X_ERROR_NONE      Success
 * @return  "Other error code"     See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetInterruptMaskStatus(VL53L0X_DEV Dev,
	uint32_t *pInterruptMaskStatus);

/**
 * @brief Configure ranging interrupt reported to system
 *
 * @note This function is not Implemented
 *
 * @param   Dev                  Device Handle
 * @param   InterruptMask         Mask of interrupt to Enable/disable
 *  (0:interrupt disabled or 1: interrupt enabled)
 * @return  VL53L0X_ERROR_NOT_IMPLEMENTED   Not implemented
 */
VL53L0X_API VL53L0X_Error VL53L0X_EnableInterruptMask(VL53L0X_DEV Dev,
	uint32_t InterruptMask);

/** @} VL53L0X_interrupt_group */

/** @defgroup VL53L0X_SPADfunctions_group VL53L0X SPAD Functions
 *  @brief    Functions used for SPAD managements
 *  @{
 */

/**
 * @brief  Set the SPAD Ambient Damper Threshold value
 *
 * @par Function Description
 * This function set the SPAD Ambient Damper Threshold value
 *
 * @note This function Access to the device
 *
 * @param   Dev                           Device Handle
 * @param   SpadAmbientDamperThreshold    SPAD Ambient Damper Threshold value
 * @return  VL53L0X_ERROR_NONE             Success
 * @return  "Other error code"            See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetSpadAmbientDamperThreshold(VL53L0X_DEV Dev,
	uint16_t SpadAmbientDamperThreshold);

/**
 * @brief  Get the current SPAD Ambient Damper Threshold value
 *
 * @par Function Description
 * This function get the SPAD Ambient Damper Threshold value
 *
 * @note This function Access to the device
 *
 * @param   Dev                           Device Handle
 * @param   pSpadAmbientDamperThreshold   Pointer to programmed
 *                                        SPAD Ambient Damper Threshold value
 * @return  VL53L0X_ERROR_NONE             Success
 * @return  "Other error code"            See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetSpadAmbientDamperThreshold(VL53L0X_DEV Dev,
	uint16_t *pSpadAmbientDamperThreshold);

/**
 * @brief  Set the SPAD Ambient Damper Factor value
 *
 * @par Function Description
 * This function set the SPAD Ambient Damper Factor value
 *
 * @note This function Access to the device
 *
 * @param   Dev                           Device Handle
 * @param   SpadAmbientDamperFactor       SPAD Ambient Damper Factor value
 * @return  VL53L0X_ERROR_NONE             Success
 * @return  "Other error code"            See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetSpadAmbientDamperFactor(VL53L0X_DEV Dev,
	uint16_t SpadAmbientDamperFactor);

/**
 * @brief  Get the current SPAD Ambient Damper Factor value
 *
 * @par Function Description
 * This function get the SPAD Ambient Damper Factor value
 *
 * @note This function Access to the device
 *
 * @param   Dev                           Device Handle
 * @param   pSpadAmbientDamperFactor      Pointer to programmed SPAD Ambient
 * Damper Factor value
 * @return  VL53L0X_ERROR_NONE             Success
 * @return  "Other error code"            See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetSpadAmbientDamperFactor(VL53L0X_DEV Dev,
	uint16_t *pSpadAmbientDamperFactor);

/**
 * @brief Performs Reference Spad Management
 *
 * @par Function Description
 * The reference SPAD initialization procedure determines the minimum amount
 * of reference spads to be enables to achieve a target reference signal rate
 * and should be performed once during initialization.
 *
 * @note This function Access to the device
 *
 * @note This function change the device mode to
 * VL53L0X_DEVICEMODE_SINGLE_RANGING
 *
 * @param   Dev                          Device Handle
 * @param   refSpadCount                 Reports ref Spad Count
 * @param   isApertureSpads              Reports if spads are of type
 *                                       aperture or non-aperture.
 *                                       1:=aperture, 0:=Non-Aperture
 * @return  VL53L0X_ERROR_NONE            Success
 * @return  VL53L0X_ERROR_REF_SPAD_INIT   Error in the Ref Spad procedure.
 * @return  "Other error code"           See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_PerformRefSpadManagement(VL53L0X_DEV Dev,
	uint32_t *refSpadCount, uint8_t *isApertureSpads);

/**
 * @brief Applies Reference SPAD configuration
 *
 * @par Function Description
 * This function applies a given number of reference spads, identified as
 * either Aperture or Non-Aperture.
 * The requested spad count and type are stored within the device specific
 * parameters data for access by the host.
 *
 * @note This function Access to the device
 *
 * @param   Dev                          Device Handle
 * @param   refSpadCount                 Number of ref spads.
 * @param   isApertureSpads              Defines if spads are of type
 *                                       aperture or non-aperture.
 *                                       1:=aperture, 0:=Non-Aperture
 * @return  VL53L0X_ERROR_NONE            Success
 * @return  VL53L0X_ERROR_REF_SPAD_INIT   Error in the in the reference
 *                                       spad configuration.
 * @return  "Other error code"           See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_SetReferenceSpads(VL53L0X_DEV Dev,
	uint32_t refSpadCount, uint8_t isApertureSpads);

/**
 * @brief Retrieves SPAD configuration
 *
 * @par Function Description
 * This function retrieves the current number of applied reference spads
 * and also their type : Aperture or Non-Aperture.
 *
 * @note This function Access to the device
 *
 * @param   Dev                          Device Handle
 * @param   refSpadCount                 Number ref Spad Count
 * @param   isApertureSpads              Reports if spads are of type
 *                                       aperture or non-aperture.
 *                                       1:=aperture, 0:=Non-Aperture
 * @return  VL53L0X_ERROR_NONE            Success
 * @return  VL53L0X_ERROR_REF_SPAD_INIT   Error in the in the reference
 *                                       spad configuration.
 * @return  "Other error code"           See ::VL53L0X_Error
 */
VL53L0X_API VL53L0X_Error VL53L0X_GetReferenceSpads(VL53L0X_DEV Dev,
	uint32_t *refSpadCount, uint8_t *isApertureSpads);

/** @} VL53L0X_SPADfunctions_group */

/** @} VL53L0X_cut11_group */

#ifdef __cplusplus
}
#endif

#endif /* _VL53L0X_API_H_ */
