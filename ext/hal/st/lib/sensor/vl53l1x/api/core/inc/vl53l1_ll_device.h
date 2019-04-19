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
 * @file vl53l1_ll_device.h
 *
 * @brief LL Driver Device specific defines. To be adapted by implementer for the
 *        targeted device.
 */

#ifndef _VL53L1_LL_DEVICE_H_
#define _VL53L1_LL_DEVICE_H_

#include "vl53l1_types.h"
#include "vl53l1_platform_user_config.h"

#define   VL53L1_I2C                      0x01
#define   VL53L1_SPI                      0x00


/** @defgroup VL53L1_DevSpecDefines_group VL53L1 Device Specific Defines
 *  @brief VL53L1 cut1.0 Device Specific Defines
 *  @{
 */

/** @defgroup VL53L1_define_WaitMethod_group Defines Wait method used
 *            For example blocking versus non blocking
 *  @{
 */
typedef uint8_t VL53L1_WaitMethod;

#define VL53L1_WAIT_METHOD_BLOCKING               ((VL53L1_WaitMethod)  0)
#define VL53L1_WAIT_METHOD_NON_BLOCKING           ((VL53L1_WaitMethod)  1)

/** @} end of VL53L1_define_WaitMethod_group */

/** @defgroup VL53L1_define_DeviceState_group  Device State
 *
 *  @brief Defines all possible device states for the device
 *  @{
 */
typedef uint8_t VL53L1_DeviceState;

#define VL53L1_DEVICESTATE_POWERDOWN              ((VL53L1_DeviceState)  0)
#define VL53L1_DEVICESTATE_HW_STANDBY             ((VL53L1_DeviceState)  1)
#define VL53L1_DEVICESTATE_FW_COLDBOOT            ((VL53L1_DeviceState)  2)
#define VL53L1_DEVICESTATE_SW_STANDBY             ((VL53L1_DeviceState)  3)
#define VL53L1_DEVICESTATE_RANGING_DSS_AUTO       ((VL53L1_DeviceState)  4)
#define VL53L1_DEVICESTATE_RANGING_DSS_MANUAL     ((VL53L1_DeviceState)  5)
#define VL53L1_DEVICESTATE_RANGING_WAIT_GPH_SYNC  ((VL53L1_DeviceState)  6)
#define VL53L1_DEVICESTATE_RANGING_GATHER_DATA    ((VL53L1_DeviceState)  7)
#define VL53L1_DEVICESTATE_RANGING_OUTPUT_DATA    ((VL53L1_DeviceState)  8)

#define VL53L1_DEVICESTATE_UNKNOWN               ((VL53L1_DeviceState) 98)
#define VL53L1_DEVICESTATE_ERROR                 ((VL53L1_DeviceState) 99)

/** @} end of VL53L1_define_DeviceState_group */


/** @defgroup VL53L1_define_DevicePresetModes_group  Device Preset Modes
 *
 *  @brief Defines all possible device preset modes. The integer values are
 *  kept the same as main EwokPlus driver branch
 *  @{
 */
typedef uint8_t VL53L1_DevicePresetModes;

#define VL53L1_DEVICEPRESETMODE_NONE                             ((VL53L1_DevicePresetModes)  0)
#define VL53L1_DEVICEPRESETMODE_STANDARD_RANGING                 ((VL53L1_DevicePresetModes)  1)
#define VL53L1_DEVICEPRESETMODE_STANDARD_RANGING_SHORT_RANGE     ((VL53L1_DevicePresetModes)  2)
#define VL53L1_DEVICEPRESETMODE_STANDARD_RANGING_LONG_RANGE      ((VL53L1_DevicePresetModes)  3)
#define VL53L1_DEVICEPRESETMODE_STANDARD_RANGING_MM1_CAL         ((VL53L1_DevicePresetModes)  4)
#define VL53L1_DEVICEPRESETMODE_STANDARD_RANGING_MM2_CAL         ((VL53L1_DevicePresetModes)  5)
#define VL53L1_DEVICEPRESETMODE_TIMED_RANGING                    ((VL53L1_DevicePresetModes)  6)
#define VL53L1_DEVICEPRESETMODE_TIMED_RANGING_SHORT_RANGE        ((VL53L1_DevicePresetModes)  7)
#define VL53L1_DEVICEPRESETMODE_TIMED_RANGING_LONG_RANGE         ((VL53L1_DevicePresetModes)  8)
#define VL53L1_DEVICEPRESETMODE_OLT                              ((VL53L1_DevicePresetModes) 17)
#define VL53L1_DEVICEPRESETMODE_SINGLESHOT_RANGING               ((VL53L1_DevicePresetModes) 18)
#define VL53L1_DEVICEPRESETMODE_LOWPOWERAUTO_SHORT_RANGE	 ((VL53L1_DevicePresetModes) 36)
#define VL53L1_DEVICEPRESETMODE_LOWPOWERAUTO_MEDIUM_RANGE	 ((VL53L1_DevicePresetModes) 37)
#define VL53L1_DEVICEPRESETMODE_LOWPOWERAUTO_LONG_RANGE		 ((VL53L1_DevicePresetModes) 38)

/** @} end of VL53L1_define_DevicePresetModes_group */


/** @defgroup VL53L1_define_DeviceMeasurementModes_group Device Measurement Modes
 *
 *  @brief Defines all possible measurement modes for the device
 *  @{
 */
typedef uint8_t VL53L1_DeviceMeasurementModes;

#define VL53L1_DEVICEMEASUREMENTMODE_STOP                        ((VL53L1_DeviceMeasurementModes)  0x00)
#define VL53L1_DEVICEMEASUREMENTMODE_SINGLESHOT                  ((VL53L1_DeviceMeasurementModes)  0x10)
#define VL53L1_DEVICEMEASUREMENTMODE_BACKTOBACK                  ((VL53L1_DeviceMeasurementModes)  0x20)
#define VL53L1_DEVICEMEASUREMENTMODE_TIMED                       ((VL53L1_DeviceMeasurementModes)  0x40)
#define VL53L1_DEVICEMEASUREMENTMODE_ABORT                       ((VL53L1_DeviceMeasurementModes)  0x80)

/** @} VL53L1_define_DeviceMeasurementModes_group */

/** @defgroup VL53L1_define_OffsetCalibrationModes_group Device Offset Calibration Mode
 *
 *  @brief Defines possible offset calibration modes for the device
 *  @{
 */
typedef uint8_t VL53L1_OffsetCalibrationMode;

#define VL53L1_OFFSETCALIBRATIONMODE__NONE                              ((VL53L1_OffsetCalibrationMode)  0)
#define VL53L1_OFFSETCALIBRATIONMODE__MM1_MM2__STANDARD                 ((VL53L1_OffsetCalibrationMode)  1)
#define VL53L1_OFFSETCALIBRATIONMODE__MM1_MM2__HISTOGRAM                ((VL53L1_OffsetCalibrationMode)  2)
#define VL53L1_OFFSETCALIBRATIONMODE__MM1_MM2__STANDARD_PRE_RANGE_ONLY  ((VL53L1_OffsetCalibrationMode)  3)
#define VL53L1_OFFSETCALIBRATIONMODE__MM1_MM2__HISTOGRAM_PRE_RANGE_ONLY ((VL53L1_OffsetCalibrationMode)  4)
#define VL53L1_OFFSETCALIBRATIONMODE__PER_ZONE                          ((VL53L1_OffsetCalibrationMode)  5)

/** @} VL53L1_define_OffsetCalibrationModes_group */


/** @defgroup VL53L1_define_OffsetCalibrationModes_group Device Offset Correction Mode
 *
 *  @brief Defines all possible offset correction modes for the device
 *  @{
 */
typedef uint8_t VL53L1_OffsetCorrectionMode;

#define VL53L1_OFFSETCORRECTIONMODE__NONE               ((VL53L1_OffsetCorrectionMode)  0)
#define VL53L1_OFFSETCORRECTIONMODE__MM1_MM2_OFFSETS    ((VL53L1_OffsetCorrectionMode)  1)
#define VL53L1_OFFSETCORRECTIONMODE__PER_ZONE_OFFSETS   ((VL53L1_OffsetCorrectionMode)  2)

/** @} VL53L1_define_OffsetCalibrationModes_group */



/** @defgroup VL53L1_DeviceSequenceConfig_group Device Sequence Config
 *
 *  @brief  Individual bit enables for each stage in the ranging scheduler
 *          The values below encode the bit shift for each bit
 *  @{
 */
typedef uint8_t VL53L1_DeviceSequenceConfig;

#define VL53L1_DEVICESEQUENCECONFIG_VHV		         ((VL53L1_DeviceSequenceConfig) 0)
#define VL53L1_DEVICESEQUENCECONFIG_PHASECAL         ((VL53L1_DeviceSequenceConfig) 1)
#define VL53L1_DEVICESEQUENCECONFIG_REFERENCE_PHASE  ((VL53L1_DeviceSequenceConfig) 2)
#define VL53L1_DEVICESEQUENCECONFIG_DSS1             ((VL53L1_DeviceSequenceConfig) 3)
#define VL53L1_DEVICESEQUENCECONFIG_DSS2             ((VL53L1_DeviceSequenceConfig) 4)
#define VL53L1_DEVICESEQUENCECONFIG_MM1              ((VL53L1_DeviceSequenceConfig) 5)
#define VL53L1_DEVICESEQUENCECONFIG_MM2              ((VL53L1_DeviceSequenceConfig) 6)
#define VL53L1_DEVICESEQUENCECONFIG_RANGE            ((VL53L1_DeviceSequenceConfig) 7)

/** @} VL53L1_DeviceSequenceConfig_group */


/** @defgroup VL53L1_DeviceInterruptPolarity_group Device Interrupt Polarity
 *
 *  @brief Device Interrupt Polarity
 *  @{
 */
typedef uint8_t VL53L1_DeviceInterruptPolarity;

#define VL53L1_DEVICEINTERRUPTPOLARITY_ACTIVE_HIGH              ((VL53L1_DeviceInterruptPolarity)  0x00)
#define VL53L1_DEVICEINTERRUPTPOLARITY_ACTIVE_LOW               ((VL53L1_DeviceInterruptPolarity)  0x10)
#define VL53L1_DEVICEINTERRUPTPOLARITY_BIT_MASK                 ((VL53L1_DeviceInterruptPolarity)  0x10)
#define VL53L1_DEVICEINTERRUPTPOLARITY_CLEAR_MASK               ((VL53L1_DeviceInterruptPolarity)  0xEF)

/** @} VL53L1_DeviceInterruptPolarity_group */


/** @defgroup VL53L1_DeviceGpioMode_group Device GPIO Mode
 *
 *  @brief Device Gpio Mode
 *  @{
 */
typedef uint8_t VL53L1_DeviceGpioMode;

#define VL53L1_DEVICEGPIOMODE_OUTPUT_CONSTANT_ZERO                    ((VL53L1_DeviceGpioMode)  0x00)
#define VL53L1_DEVICEGPIOMODE_OUTPUT_RANGE_AND_ERROR_INTERRUPTS       ((VL53L1_DeviceGpioMode)  0x01)
#define VL53L1_DEVICEGPIOMODE_OUTPUT_TIMIER_INTERRUPTS                ((VL53L1_DeviceGpioMode)  0x02)
#define VL53L1_DEVICEGPIOMODE_OUTPUT_RANGE_MODE_INTERRUPT_STATUS      ((VL53L1_DeviceGpioMode)  0x03)
#define VL53L1_DEVICEGPIOMODE_OUTPUT_SLOW_OSCILLATOR_CLOCK            ((VL53L1_DeviceGpioMode)  0x04)
#define VL53L1_DEVICEGPIOMODE_BIT_MASK                                ((VL53L1_DeviceGpioMode)  0x0F)
#define VL53L1_DEVICEGPIOMODE_CLEAR_MASK                              ((VL53L1_DeviceGpioMode)  0xF0)

/** @} VL53L1_DeviceGpioMode_group */


/** @defgroup VL53L1_DeviceError_group Device Error
 *
 *  @brief Device Error code in the range status
 *
 *  This enum is Device specific it should be updated in the implementation
 *  Use @a VL53L1_GetStatusErrorString() to get the string.
 *  It is related to Status Register of the Device.
 *  @{
 */
typedef uint8_t VL53L1_DeviceError;

#define VL53L1_DEVICEERROR_NOUPDATE                    ((VL53L1_DeviceError) 0)
	/*!< 0  No Update  */
#define VL53L1_DEVICEERROR_VCSELCONTINUITYTESTFAILURE  ((VL53L1_DeviceError) 1)
#define VL53L1_DEVICEERROR_VCSELWATCHDOGTESTFAILURE    ((VL53L1_DeviceError) 2)
#define VL53L1_DEVICEERROR_NOVHVVALUEFOUND             ((VL53L1_DeviceError) 3)
#define VL53L1_DEVICEERROR_MSRCNOTARGET                ((VL53L1_DeviceError) 4)
#define VL53L1_DEVICEERROR_RANGEPHASECHECK             ((VL53L1_DeviceError) 5)
#define VL53L1_DEVICEERROR_SIGMATHRESHOLDCHECK         ((VL53L1_DeviceError) 6)
#define VL53L1_DEVICEERROR_PHASECONSISTENCY            ((VL53L1_DeviceError) 7)
#define VL53L1_DEVICEERROR_MINCLIP                     ((VL53L1_DeviceError) 8)
#define VL53L1_DEVICEERROR_RANGECOMPLETE               ((VL53L1_DeviceError) 9)
#define VL53L1_DEVICEERROR_ALGOUNDERFLOW               ((VL53L1_DeviceError) 10)
#define VL53L1_DEVICEERROR_ALGOOVERFLOW                ((VL53L1_DeviceError) 11)
#define VL53L1_DEVICEERROR_RANGEIGNORETHRESHOLD        ((VL53L1_DeviceError) 12)
#define VL53L1_DEVICEERROR_USERROICLIP                 ((VL53L1_DeviceError) 13)
#define VL53L1_DEVICEERROR_REFSPADCHARNOTENOUGHDPADS   ((VL53L1_DeviceError) 14)
#define VL53L1_DEVICEERROR_REFSPADCHARMORETHANTARGET   ((VL53L1_DeviceError) 15)
#define VL53L1_DEVICEERROR_REFSPADCHARLESSTHANTARGET   ((VL53L1_DeviceError) 16)
#define VL53L1_DEVICEERROR_MULTCLIPFAIL                ((VL53L1_DeviceError) 17)
#define VL53L1_DEVICEERROR_GPHSTREAMCOUNT0READY        ((VL53L1_DeviceError) 18)
#define VL53L1_DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK ((VL53L1_DeviceError) 19)
#define VL53L1_DEVICEERROR_EVENTCONSISTENCY            ((VL53L1_DeviceError) 20)
#define VL53L1_DEVICEERROR_MINSIGNALEVENTCHECK         ((VL53L1_DeviceError) 21)
#define VL53L1_DEVICEERROR_RANGECOMPLETE_MERGED_PULSE  ((VL53L1_DeviceError) 22)

/* Patch_NewDeviceErrorCodePrevRangeNoTargets_11786 */
#define VL53L1_DEVICEERROR_PREV_RANGE_NO_TARGETS       ((VL53L1_DeviceError) 23)

/** @} end of VL53L1_DeviceError_group */


/** @defgroup VL53L1_DeviceReportStatus_group Device Report Status
 *  @brief Device Report Status code
 *
 *  @{
 */
typedef uint8_t VL53L1_DeviceReportStatus;

#define VL53L1_DEVICEREPORTSTATUS_NOUPDATE                    ((VL53L1_DeviceReportStatus) 0)
	/*!< 0  No Update  */
#define VL53L1_DEVICEREPORTSTATUS_ROI_SETUP                   ((VL53L1_DeviceReportStatus)  1)
#define VL53L1_DEVICEREPORTSTATUS_VHV                         ((VL53L1_DeviceReportStatus)  2)
#define VL53L1_DEVICEREPORTSTATUS_PHASECAL                    ((VL53L1_DeviceReportStatus)  3)
#define VL53L1_DEVICEREPORTSTATUS_REFERENCE_PHASE             ((VL53L1_DeviceReportStatus)  4)
#define VL53L1_DEVICEREPORTSTATUS_DSS1                        ((VL53L1_DeviceReportStatus)  5)
#define VL53L1_DEVICEREPORTSTATUS_DSS2                        ((VL53L1_DeviceReportStatus)  6)
#define VL53L1_DEVICEREPORTSTATUS_MM1                         ((VL53L1_DeviceReportStatus)  7)
#define VL53L1_DEVICEREPORTSTATUS_MM2                         ((VL53L1_DeviceReportStatus)  8)
#define VL53L1_DEVICEREPORTSTATUS_RANGE                       ((VL53L1_DeviceReportStatus)  9)
#define VL53L1_DEVICEREPORTSTATUS_HISTOGRAM                   ((VL53L1_DeviceReportStatus) 10)

/** @} end of VL53L1_DeviceReportStatus_group */

/** @defgroup VL53L1_DeviceDssMode_group Dynamic SPAD Selection Mode
 *  @brief    Selects the device Dynamic SPAD Selection Mode
 *  @{
 */

typedef uint8_t VL53L1_DeviceDssMode;

#define VL53L1_DEVICEDSSMODE__DISABLED \
	((VL53L1_DeviceDssMode) 0)
#define VL53L1_DEVICEDSSMODE__TARGET_RATE \
	((VL53L1_DeviceDssMode) 1)
#define VL53L1_DEVICEDSSMODE__REQUESTED_EFFFECTIVE_SPADS \
	((VL53L1_DeviceDssMode) 2)
#define VL53L1_DEVICEDSSMODE__BLOCK_SELECT \
	((VL53L1_DeviceDssMode) 3)

/** @} end of VL53L1_DeviceDssMode_group */

/** @defgroup VL53L1_DeviceConfigLevel_group Device Config Level
 *
 *  @brief Defines the contents of the config & start range I2C multi byte transaction
 *  @{
 */
typedef uint8_t VL53L1_DeviceConfigLevel;

#define VL53L1_DEVICECONFIGLEVEL_SYSTEM_CONTROL  \
	((VL53L1_DeviceConfigLevel)  0)
	/*!< Configs system control & start range  */
#define VL53L1_DEVICECONFIGLEVEL_DYNAMIC_ONWARDS \
	((VL53L1_DeviceConfigLevel)  1)
	/*!< Dynamic config onwards (dynamic_config, system_control) & start range  */
#define VL53L1_DEVICECONFIGLEVEL_TIMING_ONWARDS \
	((VL53L1_DeviceConfigLevel)  2)
	/*!< Dynamic config onwards (timing config, dynamic_config, system_control) &
		 start range  */
#define VL53L1_DEVICECONFIGLEVEL_GENERAL_ONWARDS \
	((VL53L1_DeviceConfigLevel)  3)
	/*!< General config onwards (general_config, timing config, dynamic_config,
		 system_control) & start range  */
#define VL53L1_DEVICECONFIGLEVEL_STATIC_ONWARDS  \
	((VL53L1_DeviceConfigLevel)  4)
	/*!< Static config onwards  (static_config, general_config, timing_config,
		 dynamic_config, system_control) & start range */
#define VL53L1_DEVICECONFIGLEVEL_CUSTOMER_ONWARDS  \
	((VL53L1_DeviceConfigLevel)  5)
	/*!< Full device config (customer_nvm_managed, static_config, general_config,
		 timing config, dynamic_config, system_control) & start range */
#define VL53L1_DEVICECONFIGLEVEL_FULL  \
	((VL53L1_DeviceConfigLevel)  6)
	/*!< Full device config (static_nvm_managed, customer_nvm_managed, static_config,
		 general_config, timing config, dynamic_config, system_control) & start range */

/** @} end of VL53L1_DeviceConfigLevel_group */


/** @defgroup VL53L1_DeviceResultsLevel_group Device Results Level
 *
 *  @brief Defines the contents of the  read results I2C multi byte transaction
 *  @{
 */
typedef uint8_t VL53L1_DeviceResultsLevel;

#define VL53L1_DEVICERESULTSLEVEL_SYSTEM_RESULTS  \
	((VL53L1_DeviceResultsLevel)  0)
	/*!< Read just system_results  */
#define VL53L1_DEVICERESULTSLEVEL_UPTO_CORE  \
	((VL53L1_DeviceResultsLevel)  1)
	/*!< Read both system and core results */
#define VL53L1_DEVICERESULTSLEVEL_FULL  \
	((VL53L1_DeviceResultsLevel)  2)
	/*!< Read system, core and debug results */

/** @} end of VL53L1_DeviceResultsLevel_group */


/** @defgroup VL53L1_DeviceTestMode_group Device Test Mode
 *
 *  @brief Values below match the the TEST_MODE__CTRL register
 *         do not change
 *  @{
 */

typedef uint8_t VL53L1_DeviceTestMode;

#define VL53L1_DEVICETESTMODE_NONE \
	((VL53L1_DeviceTestMode) 0x00)
	/*!< Idle */
#define VL53L1_DEVICETESTMODE_NVM_ZERO \
	((VL53L1_DeviceTestMode) 0x01)
	/*!< NVM zero */
#define VL53L1_DEVICETESTMODE_NVM_COPY \
	((VL53L1_DeviceTestMode) 0x02)
	/*!< NVM copy */
#define VL53L1_DEVICETESTMODE_PATCH \
	((VL53L1_DeviceTestMode) 0x03)
	/*!< Patch */
#define VL53L1_DEVICETESTMODE_DCR \
	((VL53L1_DeviceTestMode) 0x04)
	/*!< DCR - SPAD Self-Check (Pass if Count Rate is less than Threshold) */
#define VL53L1_DEVICETESTMODE_LCR_VCSEL_OFF \
	((VL53L1_DeviceTestMode) 0x05)
	/*!< LCR - SPAD Self-Check (Pass if Count Rate is greater than Threshold
		 and VCSEL off) */
#define VL53L1_DEVICETESTMODE_LCR_VCSEL_ON \
	((VL53L1_DeviceTestMode) 0x06)
	/*!< LCR - SPAD Self-Check (Pass if Count Rate is greater than Threshold
		 and VCSEL on) */
#define VL53L1_DEVICETESTMODE_SPOT_CENTRE_LOCATE \
	((VL53L1_DeviceTestMode) 0x07)
	/*!< Spot centre locate */
#define VL53L1_DEVICETESTMODE_REF_SPAD_CHAR_WITH_PRE_VHV \
	((VL53L1_DeviceTestMode) 0x08)
	/*!<Reference SPAD Characterisation with pre-VHV */
#define VL53L1_DEVICETESTMODE_REF_SPAD_CHAR_ONLY \
	((VL53L1_DeviceTestMode) 0x09)
	/*!< Reference SPAD Characterisation Only */

/** @} end of VL53L1_DeviceTestMode_group */


/** @defgroup VL53L1_DeviceSscArray_group Device Test Mode
 *
 *  @{
 */

typedef uint8_t VL53L1_DeviceSscArray;

#define VL53L1_DEVICESSCARRAY_RTN ((VL53L1_DeviceSscArray) 0x00)
	/*!<Return Array Rates */
#define VL53L1_DEVICETESTMODE_REF ((VL53L1_DeviceSscArray) 0x01)
	/*!< Reference Array Rates Only */

/** @} end of VL53L1_DeviceSscArray_group */


/** @defgroup VL53L1_SpadArraySelection_group SPAD Array Selection Functionality
 *  @brief SPAD array selection definitions
 *  @{
 */

#define VL53L1_RETURN_ARRAY_ONLY                   0x01
	/*!< Return SPAD Array only */
#define VL53L1_REFERENCE_ARRAY_ONLY                0x10
	/*!< Reference SPAD Array only  */
#define VL53L1_BOTH_RETURN_AND_REFERENCE_ARRAYS    0x11
	/*!< Both Return and Reference SPAD Arrays */
#define VL53L1_NEITHER_RETURN_AND_REFERENCE_ARRAYS 0x00
	/*!< Neither Return or Reference SPAD Array */

/** @} end of VL53L1_SpadArraySelection_group */

/** @defgroup VL53L1_DeviceInterruptLevel_group Interrupt Level Functionality
 *  @brief Interrupt Output Level types
 *  @{
 */

#define VL53L1_DEVICEINTERRUPTLEVEL_ACTIVE_HIGH               0x00
	/*!< Active High Interrupt */
#define VL53L1_DEVICEINTERRUPTLEVEL_ACTIVE_LOW                0x10
	/*!< Active Low Interrupt  */
#define VL53L1_DEVICEINTERRUPTLEVEL_ACTIVE_MASK               0x10
	/*!< Active Bit Mask  */

/** @} end of VL53L1_DeviceInterruptLevel_group */

/** @defgroup VL53L1_ApiCore_group Misc Functionality
 *  @brief API core specific definitions
 *  @{
 */

#define VL53L1_POLLING_DELAY_US                     1000
	/*!< 1000us delay for register polling */
#define VL53L1_SOFTWARE_RESET_DURATION_US            100
	/*!< 100us software reset duration */
#define VL53L1_FIRMWARE_BOOT_TIME_US                1200
	/*!< Duration of firmware boot time for which I2C
	 access is blocked. Real Device 1ms, FPGA 15ms */
#define VL53L1_ENABLE_POWERFORCE_SETTLING_TIME_US    250
	/*!< After enabling power force a delay is required
		 to bring regulator, bandgap, oscillator time
		 to power up and settle */
#define VL53L1_SPAD_ARRAY_WIDTH                       16
	/*!< SPAD array width */
#define VL53L1_SPAD_ARRAY_HEIGHT                      16
	/*!< SPAD array height */
#define VL53L1_NVM_SIZE_IN_BYTES                     512
	/*!< NVM (OTP) size in bytes */
#define VL53L1_NO_OF_SPAD_ENABLES                    256
	/*!< Number of SPADs each SPAD array */
#define VL53L1_RTN_SPAD_BUFFER_SIZE                   32
	/*!< Number of Return SPAD enable registers (bytes) */
#define VL53L1_REF_SPAD_BUFFER_SIZE                    6
	/*!< Number of Reference SPAD enable registers (bytes) */
#define VL53L1_AMBIENT_WINDOW_VCSEL_PERIODS          256
	/*!< Sigma Delta Ambient window in VCSEL Periods */
#define VL53L1_RANGING_WINDOW_VCSEL_PERIODS         2048
	/*!< Sigma Delta Ranging window in VCSEL periods */
#define VL53L1_MACRO_PERIOD_VCSEL_PERIODS \
	(VL53L1_AMBIENT_WINDOW_VCSEL_PERIODS + VL53L1_RANGING_WINDOW_VCSEL_PERIODS)
	/*!< Macro Period in VCSEL periods */
#define VL53L1_MAX_ALLOWED_PHASE                    0xFFFF
	/*!< Maximum Allowed phase 0xFFFF means 31.999 PLL Clocks */

#define VL53L1_RTN_SPAD_UNITY_TRANSMISSION      0x0100
	/*!< SPAD unity transmission value - 1.0 in 8.8 format */
#define VL53L1_RTN_SPAD_APERTURE_TRANSMISSION   0x0038
	/*!< Apertured SPAD transmission value - 8.8 format
	     Nominal:  5x   -> 0.200000 * 256 = 51 = 0x33
	     Measured: 4.6x -> 0.217391 * 256 = 56 = 0x38 */

#define VL53L1_SPAD_TOTAL_COUNT_MAX                 ((0x01 << 29) - 1)
	/*!< Maximum SPAD count - 512Mcps * 1sec = 29bits) */
#define VL53L1_SPAD_TOTAL_COUNT_RES_THRES            (0x01 << 24)
	/*!< SPAD count threshold for reduced 3-bit fractional resolution */
#define VL53L1_COUNT_RATE_INTERNAL_MAX              ((0x01 << 24) - 1)
	/*!< Maximum internal count rate is a 17.7 (24-b) value */
#define VL53L1_SPEED_OF_LIGHT_IN_AIR                299704
	/*!< Speed of light in air in mm/sec */
#define VL53L1_SPEED_OF_LIGHT_IN_AIR_DIV_8          (299704 >> 3)
	/*!< Speed of light in air in divided by 8, 2 for round trip
		 and 4 as an additional scaling factor */

/** @} end of VL53L1_ApiCore_group */

/** @} end of VL53L1_DevSpecDefines_group */

/** @defgroup VL53L1_GPIO_Interrupt_Mode_group Interrupt modes
 *  @brief    Selects between four interrupt modes
 *  @{
 */

typedef uint8_t VL53L1_GPIO_Interrupt_Mode;

#define VL53L1_GPIOINTMODE_LEVEL_LOW \
	((VL53L1_GPIO_Interrupt_Mode) 0)
	/*!< Trigger interupt if value < thresh_low */
#define VL53L1_GPIOINTMODE_LEVEL_HIGH \
	((VL53L1_GPIO_Interrupt_Mode) 1)
	/*!< Trigger interupt if value > thresh_high */
#define VL53L1_GPIOINTMODE_OUT_OF_WINDOW \
	((VL53L1_GPIO_Interrupt_Mode) 2)
	/*!< Trigger interupt if value < thresh_low OR value > thresh_high */
#define VL53L1_GPIOINTMODE_IN_WINDOW \
	((VL53L1_GPIO_Interrupt_Mode) 3)
	/*!< Trigger interupt if value > thresh_low AND value < thresh_high */

/** @} end of VL53L1_GPIO_Interrupt_Mode_group */

/** @defgroup VL53L1_TuningParms_group Tuning Parameters
 *  @brief    Selects specific tuning parameter inputs to get/set \
 *            Added as part of Patch_AddedTuningParms_11761
 *  @{
 */

typedef uint16_t VL53L1_TuningParms;

#define VL53L1_TUNINGPARMS_LLD_PUBLIC_MIN_ADDRESS \
	((VL53L1_TuningParms) VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS)
#define VL53L1_TUNINGPARMS_LLD_PUBLIC_MAX_ADDRESS \
    ((VL53L1_TuningParms) VL53L1_TUNINGPARM_LOWPOWERAUTO_RANGE_CONFIG_TIMEOUT_US)

#define VL53L1_TUNINGPARMS_LLD_PRIVATE_MIN_ADDRESS \
	((VL53L1_TuningParms) VL53L1_TUNINGPARM_PRIVATE_PAGE_BASE_ADDRESS)
#define VL53L1_TUNINGPARMS_LLD_PRIVATE_MAX_ADDRESS \
	((VL53L1_TuningParms) VL53L1_TUNINGPARMS_LLD_PRIVATE_MIN_ADDRESS)

#define VL53L1_TUNINGPARM_VERSION \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 0))
#define VL53L1_TUNINGPARM_KEY_TABLE_VERSION \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 1))
#define VL53L1_TUNINGPARM_LLD_VERSION \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 2))
#define VL53L1_TUNINGPARM_CONSISTENCY_LITE_PHASE_TOLERANCE \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 3))
#define VL53L1_TUNINGPARM_PHASECAL_TARGET \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 4))
#define VL53L1_TUNINGPARM_LITE_CAL_REPEAT_RATE \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 5))
#define VL53L1_TUNINGPARM_LITE_RANGING_GAIN_FACTOR \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 6))
#define VL53L1_TUNINGPARM_LITE_MIN_CLIP_MM \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 7))
#define VL53L1_TUNINGPARM_LITE_LONG_SIGMA_THRESH_MM \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 8))
#define VL53L1_TUNINGPARM_LITE_MED_SIGMA_THRESH_MM \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 9))
#define VL53L1_TUNINGPARM_LITE_SHORT_SIGMA_THRESH_MM \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 10))
#define VL53L1_TUNINGPARM_LITE_LONG_MIN_COUNT_RATE_RTN_MCPS \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 11))
#define VL53L1_TUNINGPARM_LITE_MED_MIN_COUNT_RATE_RTN_MCPS \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 12))
#define VL53L1_TUNINGPARM_LITE_SHORT_MIN_COUNT_RATE_RTN_MCPS \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 13))
#define VL53L1_TUNINGPARM_LITE_SIGMA_EST_PULSE_WIDTH \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 14))
#define VL53L1_TUNINGPARM_LITE_SIGMA_EST_AMB_WIDTH_NS \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 15))
#define VL53L1_TUNINGPARM_LITE_SIGMA_REF_MM \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 16))
#define VL53L1_TUNINGPARM_LITE_RIT_MULT \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 17))
#define VL53L1_TUNINGPARM_LITE_SEED_CONFIG \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 18))
#define VL53L1_TUNINGPARM_LITE_QUANTIFIER \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 19))
#define VL53L1_TUNINGPARM_LITE_FIRST_ORDER_SELECT \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 20))
#define VL53L1_TUNINGPARM_LITE_XTALK_MARGIN_KCPS \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 21))
#define VL53L1_TUNINGPARM_INITIAL_PHASE_RTN_LITE_LONG_RANGE \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 22))
#define VL53L1_TUNINGPARM_INITIAL_PHASE_RTN_LITE_MED_RANGE \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 23))
#define VL53L1_TUNINGPARM_INITIAL_PHASE_RTN_LITE_SHORT_RANGE \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 24))
#define VL53L1_TUNINGPARM_INITIAL_PHASE_REF_LITE_LONG_RANGE \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 25))
#define VL53L1_TUNINGPARM_INITIAL_PHASE_REF_LITE_MED_RANGE \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 26))
#define VL53L1_TUNINGPARM_INITIAL_PHASE_REF_LITE_SHORT_RANGE \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 27))
#define VL53L1_TUNINGPARM_TIMED_SEED_CONFIG \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 28))
#define VL53L1_TUNINGPARM_VHV_LOOPBOUND \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 29))
#define VL53L1_TUNINGPARM_REFSPADCHAR_DEVICE_TEST_MODE \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 30))
#define VL53L1_TUNINGPARM_REFSPADCHAR_VCSEL_PERIOD \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 31))
#define VL53L1_TUNINGPARM_REFSPADCHAR_PHASECAL_TIMEOUT_US \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 32))
#define VL53L1_TUNINGPARM_REFSPADCHAR_TARGET_COUNT_RATE_MCPS \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 33))
#define VL53L1_TUNINGPARM_REFSPADCHAR_MIN_COUNTRATE_LIMIT_MCPS \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 34))
#define VL53L1_TUNINGPARM_REFSPADCHAR_MAX_COUNTRATE_LIMIT_MCPS \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 35))
#define VL53L1_TUNINGPARM_OFFSET_CAL_DSS_RATE_MCPS \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 36))
#define VL53L1_TUNINGPARM_OFFSET_CAL_PHASECAL_TIMEOUT_US \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 37))
#define VL53L1_TUNINGPARM_OFFSET_CAL_MM_TIMEOUT_US \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 38))
#define VL53L1_TUNINGPARM_OFFSET_CAL_RANGE_TIMEOUT_US \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 39))
#define VL53L1_TUNINGPARM_OFFSET_CAL_PRE_SAMPLES \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 40))
#define VL53L1_TUNINGPARM_OFFSET_CAL_MM1_SAMPLES \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 41))
#define VL53L1_TUNINGPARM_OFFSET_CAL_MM2_SAMPLES \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 42))
#define VL53L1_TUNINGPARM_SPADMAP_VCSEL_PERIOD \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 43))
#define VL53L1_TUNINGPARM_SPADMAP_VCSEL_START \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 44))
#define VL53L1_TUNINGPARM_SPADMAP_RATE_LIMIT_MCPS \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 45))
#define VL53L1_TUNINGPARM_LITE_DSS_CONFIG_TARGET_TOTAL_RATE_MCPS \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 46))
#define VL53L1_TUNINGPARM_TIMED_DSS_CONFIG_TARGET_TOTAL_RATE_MCPS \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 47))
#define VL53L1_TUNINGPARM_LITE_PHASECAL_CONFIG_TIMEOUT_US \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 48))
#define VL53L1_TUNINGPARM_TIMED_PHASECAL_CONFIG_TIMEOUT_US \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 49))
#define VL53L1_TUNINGPARM_LITE_MM_CONFIG_TIMEOUT_US \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 50))
#define VL53L1_TUNINGPARM_TIMED_MM_CONFIG_TIMEOUT_US \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 51))
#define VL53L1_TUNINGPARM_LITE_RANGE_CONFIG_TIMEOUT_US \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 52))
#define VL53L1_TUNINGPARM_TIMED_RANGE_CONFIG_TIMEOUT_US \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 53))
#define VL53L1_TUNINGPARM_LOWPOWERAUTO_VHV_LOOP_BOUND \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 54))
#define VL53L1_TUNINGPARM_LOWPOWERAUTO_MM_CONFIG_TIMEOUT_US \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 55))
#define VL53L1_TUNINGPARM_LOWPOWERAUTO_RANGE_CONFIG_TIMEOUT_US \
	((VL53L1_TuningParms) (VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS + 56))


/** @} end of VL53L1_TuningParms_group */


#endif

/* _VL53L1_DEVICE_H_ */


