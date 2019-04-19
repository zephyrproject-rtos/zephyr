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
 * @file vl53l1_register_settings.h
 *
 * @brief Device register setting defines.
 */

#ifndef _VL53L1_REGISTER_SETTINGS_H_
#define _VL53L1_REGISTER_SETTINGS_H_


/** @defgroup VL53L1_RegisterSettings_group  Functionality
 *  @brief Defines the register settings for key device
 *         configuration registers
 *  @{
 */

/** @defgroup VL53L1_DeviceSchedulerMode_group - Pseudo, Streaming & Hist
 *  @brief Values below match the bit positions in the SYSTEM__MODE_START
 *         register do not change
 *  @{
 */

#define VL53L1_DEVICESCHEDULERMODE_PSEUDO_SOLO  0x00
#define VL53L1_DEVICESCHEDULERMODE_STREAMING    0x01
#define VL53L1_DEVICESCHEDULERMODE_HISTOGRAM    0x02

/** @} end of VL53L1_DeviceReadoutMode_group */

/** @defgroup VL53L1_DeviceReadoutMode_group - Single, Dual, Split & Manual
 *  @brief Values below match the bit positions in the SYSTEM__MODE_START
 *         register do not change
 *  @{
 */

#define VL53L1_DEVICEREADOUTMODE_SINGLE_SD        (0x00 << 2)
#define VL53L1_DEVICEREADOUTMODE_DUAL_SD          (0x01 << 2)
#define VL53L1_DEVICEREADOUTMODE_SPLIT_READOUT    (0x02 << 2)
#define VL53L1_DEVICEREADOUTMODE_SPLIT_MANUAL     (0x03 << 2)

/** @} end of VL53L1_DeviceReadoutMode_group */

/** @defgroup VL53L1_DeviceMeasurementMode_group - SingleShot, BackToBack & timed
 *  @brief Values below match the bit positions in the SYSTEM__MODE_START
 *         register do not change
 *  @{
 */

/*
#define VL53L1_DEVICEMEASUREMENTMODE_STOP               0x00
#define VL53L1_DEVICEMEASUREMENTMODE_SINGLESHOT         0x10
#define VL53L1_DEVICEMEASUREMENTMODE_BACKTOBACK         0x20
#define VL53L1_DEVICEMEASUREMENTMODE_TIMED              0x40
#define VL53L1_DEVICEMEASUREMENTMODE_ABORT              0x80
*/
#define VL53L1_DEVICEMEASUREMENTMODE_MODE_MASK          0xF0
#define VL53L1_DEVICEMEASUREMENTMODE_STOP_MASK          0x0F

#define VL53L1_GROUPEDPARAMETERHOLD_ID_MASK             0x02

/** @} end of VL53L1_DeviceMeasurementMode_group */

#define VL53L1_EWOK_I2C_DEV_ADDR_DEFAULT                0x29
	/*!< Device default 7-bit I2C address */
#define VL53L1_OSC_FREQUENCY                            0x00
#define VL53L1_OSC_TRIM_DEFAULT                         0x00
#define VL53L1_OSC_FREQ_SET_DEFAULT                     0x00

#define VL53L1_RANGE_HISTOGRAM_REF                      0x08
#define VL53L1_RANGE_HISTOGRAM_RET                      0x10
#define VL53L1_RANGE_HISTOGRAM_BOTH                     0x18
#define VL53L1_RANGE_HISTOGRAM_INIT                     0x20
#define VL53L1_RANGE_VHV_INIT                           0x40

/* Result Status */
#define VL53L1_RESULT_RANGE_STATUS                      0x1F

/* */
#define VL53L1_SYSTEM__SEED_CONFIG__MANUAL              0x00
#define VL53L1_SYSTEM__SEED_CONFIG__STANDARD            0x01
#define VL53L1_SYSTEM__SEED_CONFIG__EVEN_UPDATE_ONLY    0x02

/* Interrupt Config */
#define VL53L1_INTERRUPT_CONFIG_LEVEL_LOW               0x00
#define VL53L1_INTERRUPT_CONFIG_LEVEL_HIGH              0x01
#define VL53L1_INTERRUPT_CONFIG_OUT_OF_WINDOW           0x02
#define VL53L1_INTERRUPT_CONFIG_IN_WINDOW               0x03
#define VL53L1_INTERRUPT_CONFIG_NEW_SAMPLE_READY        0x20

/* Interrupt Clear */
#define VL53L1_CLEAR_RANGE_INT                          0x01
#define VL53L1_CLEAR_ERROR_INT                          0x02

/* Sequence Config */
#define VL53L1_SEQUENCE_VHV_EN						    0x01
#define VL53L1_SEQUENCE_PHASECAL_EN                     0x02
#define VL53L1_SEQUENCE_REFERENCE_PHASE_EN              0x04
#define VL53L1_SEQUENCE_DSS1_EN                         0x08
#define VL53L1_SEQUENCE_DSS2_EN                         0x10
#define VL53L1_SEQUENCE_MM1_EN                          0x20
#define VL53L1_SEQUENCE_MM2_EN                          0x40
#define VL53L1_SEQUENCE_RANGE_EN                        0x80

/* defines for DSS__ROI_CONTROL */
#define VL53L1_DSS_CONTROL__ROI_SUBTRACT                0x20
#define VL53L1_DSS_CONTROL__ROI_INTERSECT               0x10

#define VL53L1_DSS_CONTROL__MODE_DISABLED               0x00
#define VL53L1_DSS_CONTROL__MODE_TARGET_RATE            0x01
#define VL53L1_DSS_CONTROL__MODE_EFFSPADS               0x02
#define VL53L1_DSS_CONTROL__MODE_BLOCKSELECT            0x03

/* SPAD Readout defines
 *
 * 7:6 - SPAD_IN_SEL_REF
 * 5:4 - SPAD_IN_SEL_RTN
 *   2 - SPAD_PS_BYPASS
 *   0 - SPAD_EN_PULSE_EXTENDER
 */

#define VL53L1_RANGING_CORE__SPAD_READOUT__STANDARD              0x45
#define VL53L1_RANGING_CORE__SPAD_READOUT__RETURN_ARRAY_ONLY     0x05
#define VL53L1_RANGING_CORE__SPAD_READOUT__REFERENCE_ARRAY_ONLY  0x55
#define VL53L1_RANGING_CORE__SPAD_READOUT__RETURN_SPLIT_ARRAY    0x25
#define VL53L1_RANGING_CORE__SPAD_READOUT__CALIB_PULSES          0xF5


#define VL53L1_LASER_SAFETY__KEY_VALUE                  0x6C

/* Range Status defines
 *
 *   7 - GPH ID
 *   6 - Min threshold hit
 *   5 - Max threshold hit
 * 4:0 - Range Status
 */

#define VL53L1_RANGE_STATUS__RANGE_STATUS_MASK          0x1F
#define VL53L1_RANGE_STATUS__MAX_THRESHOLD_HIT_MASK     0x20
#define VL53L1_RANGE_STATUS__MIN_THRESHOLD_HIT_MASK     0x40
#define VL53L1_RANGE_STATUS__GPH_ID_RANGE_STATUS_MASK   0x80

/* Interrupt Status defines
 *
 *   5 - GPH ID
 * 4:3 - Interrupt Error Status
 * 2:0 - Interrupt Status
 */

#define VL53L1_INTERRUPT_STATUS__INT_STATUS_MASK            0x07
#define VL53L1_INTERRUPT_STATUS__INT_ERROR_STATUS_MASK      0x18
#define VL53L1_INTERRUPT_STATUS__GPH_ID_INT_STATUS_MASK     0x20

/** @} end of VL53L1_RegisterSettings_group */


#endif

/* _VL53L1_REGISTER_SETTINGS_H_ */


