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
 * @file  vl53l1_api_core.h
 *
 * @brief EwokPlus25 low level API function definitions
 */

#ifndef _VL53L1_API_CORE_H_
#define _VL53L1_API_CORE_H_

#include "vl53l1_platform.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Get LL Driver version information
 *
 * @param[in]   Dev       : Device handle
 * @param[out]  pversion  : pointer to VL53L1_ll_version_t
 *
 * @return  VL53L1_ERROR_NONE     Success
 */

#ifdef VL53L1_DEBUG
VL53L1_Error VL53L1_get_version(
	VL53L1_DEV            Dev,
	VL53L1_ll_version_t  *pversion);

/**
 * @brief Gets Device Firmware version
 *
 * @param[in]   Dev           : Device handle
 * @param[out]  pfw_version   : pointer to uint16_t FW version
 *
 * @return  VL53L1_ERROR_NONE     Success
 */

VL53L1_Error VL53L1_get_device_firmware_version(
	VL53L1_DEV         Dev,
	uint16_t          *pfw_version);
#endif


/**
 * @brief Initialises pdev structure and optionally read the
 *        part to part information form he device G02 registers
 *
 * Important: VL53L1_platform_initialise() *must* called before calling
 * this function
 *
 * @param[in]   Dev           : Device handle
 * @param[out]  read_p2p_data : if > 0 then reads and caches P2P data
 *
 * @return  VL53L1_ERROR_NONE     Success
 */

VL53L1_Error VL53L1_data_init(
	VL53L1_DEV         Dev,
	uint8_t            read_p2p_data);


/**
 * @brief  For C-API one time initialization only reads device
 *         G02 registers containing data copied from NVM
 *
 *  Contains the key NVM data e.g identification info
 *  fast oscillator freq, max trim and laser safety info
 *
 *  Function all only be called after the device has finished booting
 *
 * @param[in]   Dev           : Device handle
 *
 * @return  VL53L1_ERROR_NONE     Success
 */

VL53L1_Error VL53L1_read_p2p_data(
	VL53L1_DEV      Dev);


/**
 * @brief Performs device software reset and then waits for  the firmware
 *        to finish booting
 *
 * @param[in]   Dev           : Device handle
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_software_reset(
	VL53L1_DEV      Dev);


/**
 * @brief  Sets the customer part to part data
 *
 * Important: does **not** apply the settings to the device.
 * Updates the following structures in the device structure
 *
 * Just an internal memcpy to
 *
 *
 * @param[in]   Dev             : Device handle
 * @param[in]   pcal_data       : pointer to VL53L1_calibration_data_t
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_part_to_part_data(
	VL53L1_DEV                            Dev,
	VL53L1_calibration_data_t            *pcal_data);


/**
 * @brief  Gets the customer part to part data
 *
 * Just an internal memory copy
 *
 * @param[in]   Dev             : Device handle
 * @param[out]  pcal_data       : pointer to VL53L1_calibration_data_t
 *
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_part_to_part_data(
	VL53L1_DEV                            Dev,
	VL53L1_calibration_data_t            *pcal_data);


/**
 * @brief  Gets the tuning parm part to part data
 *
 * Just an internal copy
 *
 * @param[in]   Dev             : Device handle
 * @param[out]  ptun_data       : pointer to VL53L1_tuning_parameters_t
 *
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

#ifdef VL53L1_DEBUG
VL53L1_Error VL53L1_get_tuning_debug_data(
	VL53L1_DEV                            Dev,
	VL53L1_tuning_parameters_t            *ptun_data);
#endif


/**
 * @brief  Sets the inter measurement period in the VL53L1_timing_config_t structure
 *
 * Uses the pdev->dbg_results.result__osc_calibrate_val value convert from [ms]
 *
 * @param[in]   Dev                         : Device handle
 * @param[in]   inter_measurement_period_ms : requested inter measurement period in [ms]
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_inter_measurement_period_ms(
	VL53L1_DEV          Dev,
	uint32_t            inter_measurement_period_ms);


/**
 * @brief  Gets inter measurement period from the VL53L1_timing_config_t structure
 *
 * Uses the pdev->dbg_results.result__osc_calibrate_val value convert into [ms]
 *
 * @param[in]   Dev                           : Device handle
 * @param[out]  pinter_measurement_period_ms  : current inter measurement period in [ms]
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_inter_measurement_period_ms(
	VL53L1_DEV          Dev,
	uint32_t           *pinter_measurement_period_ms);


/**
 * @brief  Sets the phasecal, mode mitigation and ranging timeouts
 *         in the VL53L1_timing_config_t structure
 *
 * Uses the pdev->stat_nvm.osc_measured__fast_osc__frequency value convert from [us]
 *
 * @param[in]   Dev                         : Device handle
 * @param[in]   phasecal_config_timeout_us  : requested Phase Cal Timeout e.g. 1000us
 * @param[in]   mm_config_timeout_us        : requested MM Timeout e.g. 2000us
 * @param[in]   range_config_timeout_us     : requested Ranging Timeout e.g 13000us
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_timeouts_us(
	VL53L1_DEV          Dev,
	uint32_t            phasecal_config_timeout_us,
	uint32_t            mm_config_timeout_us,
	uint32_t            range_config_timeout_us);


/**
 * @brief  Gets the phasecal, mode mitigation and ranging timeouts
 *         for the VL53L1_timing_config_t structure
 *
 * Uses the pdev->stat_nvm.osc_measured__fast_osc__frequency convert into [us]
 *
 * @param[in]   Dev                          : Device handle
 * @param[out]  pphasecal_config_timeout_us  : current Phase Cal Timeout in [us]
 * @param[out]  pmm_config_timeout_us        : current MM Timeout in [us]
 * @param[out]  prange_config_timeout_us     : current Ranging Timeout in [us]
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_timeouts_us(
	VL53L1_DEV          Dev,
	uint32_t           *pphasecal_config_timeout_us,
	uint32_t           *pmm_config_timeout_us,
	uint32_t           *prange_config_timeout_us);


/**
 * @brief  Sets the 12-bit calibration repeat period value
 *
 * Sets the repeat period for VHV and phasecal in ranges.
 * Setting to zero to disables the repeat, but the VHV
 * and PhaseCal is still run on the very first range in
 * this case.
 *
 * Only even values should be set
 *
 * The max value is 4094 i.e. every
 *
 * @param[in]   Dev                        : Device handle
 * @param[in]   cal_config__repeat_period  : current calibration config repeat period
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_calibration_repeat_period(
	VL53L1_DEV          Dev,
	uint16_t            cal_config__repeat_period);


/**
 * @brief  Gets the current 12-bit calibration repeat period value
 *
 * @param[in]   Dev                         : Device handle
 * @param[out]  pcal_config__repeat_period  : current calibration config repeat period
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_calibration_repeat_period(
	VL53L1_DEV          Dev,
	uint16_t           *pcal_config__repeat_period);


/**
 * @brief  Set system sequence config bit value
 *
 * @param[in]   Dev              : Device handle
 * @param[in]   bit_id           : VL53L1_DeviceSequenceConfig bit id
 * @param[in]   value            : Input Bit value = 0 or 1
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_sequence_config_bit(
	VL53L1_DEV                   Dev,
	VL53L1_DeviceSequenceConfig  bit_id,
	uint8_t                      value);


/**
 * @brief  Get system sequence config bit value
 *
 * @param[in]   Dev              : Device handle
 * @param[in]   bit_id           : VL53L1_DeviceSequenceConfig bit id
 * @param[out]  pvalue           : Output Bit value = 0 or 1
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_sequence_config_bit(
	VL53L1_DEV                   Dev,
	VL53L1_DeviceSequenceConfig  bit_id,
	uint8_t                     *pvalue);


/**
 * @brief  Set the interrupt polarity bit in
 *
 * @param[in]   Dev                 : Device handle
 * @param[in]   interrupt_polarity  : Interrupt Polarity
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_interrupt_polarity(
	VL53L1_DEV                       Dev,
	VL53L1_DeviceInterruptPolarity  interrupt_polarity);


/**
 * @brief  Get the interrupt polarity bit state
 *
 * @param[in]   Dev                  : Device handle
 * @param[out]  pinterrupt_polarity  : Interrupt Polarity Bit value
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_interrupt_polarity(
	VL53L1_DEV                      Dev,
	VL53L1_DeviceInterruptPolarity  *pinterrupt_polarity);

/**
 * @brief  Set the Ref spad char cfg struct internal to pdev
 *
 * @param[in]  Dev     : Device handle
 * @param[in]  pdata   : Input pointer to VL53L1_refspadchar_config_t
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

#ifndef VL53L1_NOCALIB
VL53L1_Error VL53L1_get_refspadchar_config_struct(
	VL53L1_DEV                     Dev,
	VL53L1_refspadchar_config_t   *pdata);
#endif

/**
 * @brief  Extract the Ref spad char cfg struct from pdev
 *
 * @param[in]  Dev     : Device handle
 * @param[out] pdata   : Output pointer to VL53L1_refspadchar_config_t
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

#ifndef VL53L1_NOCALIB
VL53L1_Error VL53L1_set_refspadchar_config_struct(
	VL53L1_DEV                     Dev,
	VL53L1_refspadchar_config_t   *pdata);
#endif

/**
 * @brief  Set the Range Ignore Threshold Rate value
 *
 * @param[in]  Dev                  : Device handle
 * @param[in]  range_ignore_thresh_mult     : Input RIT Mult value, in \
 *                                            3.5 fractional value
 * @param[in]  range_ignore_threshold_mcps  : Range Ignore Threshold value
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_range_ignore_threshold(
	VL53L1_DEV              Dev,
	uint8_t                 range_ignore_thresh_mult,
	uint16_t                range_ignore_threshold_mcps);

/**
 * @brief  Get the Range Ignore Threshold Rate value
 *
 *
 *
 * @param[in]   Dev                  : Device handle
 * @param[out]  prange_ignore_thresh_mult              : \
 *              Range ignore threshold internal multiplier value in \
 *              3.5 fractional format.
 * @param[out]  prange_ignore_threshold_mcps_internal  : \
 *              Range Ignore Threshold Rate value generated from \
 *              current xtalk parms
 * @param[out]  prange_ignore_threshold_mcps_current   : \
 *              Range Ignore Threshold Rate value generated from \
 *              device static config struct
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_range_ignore_threshold(
	VL53L1_DEV              Dev,
	uint8_t                *prange_ignore_thresh_mult,
	uint16_t               *prange_ignore_threshold_mcps_internal,
	uint16_t               *prange_ignore_threshold_mcps_current);


/**
 * @brief  Sets the current user Zone (ROI) configuration structure data
 *
 * @param[in]   Dev            : Device handle
 * @param[in]   puser_zone     : pointer to user Zone (ROI) data structure
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_user_zone(
	VL53L1_DEV          Dev,
	VL53L1_user_zone_t *puser_zone);


/**
 * @brief  Gets the current user zone (ROI) configuration structure data
 *
 * @param[in]   Dev            : Device handle
 * @param[out]  puser_zone     : pointer to user zone (ROI) data structure
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_user_zone(
	VL53L1_DEV          Dev,
	VL53L1_user_zone_t *puser_zone);


/**
 * @brief  Gets the current mode mitigation zone (ROI) configuration structure data
 *
 * @param[in]   Dev         : Device handle
 * @param[out]  pmm_roi     : pointer to user zone (ROI) data structure
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_mode_mitigation_roi(
	VL53L1_DEV          Dev,
	VL53L1_user_zone_t *pmm_roi);


/**
 * @brief  Initialises the configuration data structures for
 *         the selected preset mode
 *
 * Important: does **not** apply the settings to the device.
 * Updates the following structures in the device structure
 *
 * - static config  : pdev->stat_cfg
 * - dynamic config : pdev->dyn_cfg
 * - system_control  :pdev->sys_ctrl
 *
 * Uses osc_measured__fast_osc__frequency in  pdev->stat_nvm to
 * convert the input timeouts in [us] to internal macro period
 * timeout values.
 *
 * Please call VL53L1_init_and_start_range() to update device
 * and start a range
 *
 * @param[in]   Dev                                : Device handle
 * @param[in]   device_preset_mode                 : selected device preset mode
 * @param[in]   dss_config__target_total_rate_mcps : DSS target rate in 9.7 format e.g 0x0A00 -> 20.0 Mcps
 * @param[in]   phasecal_config_timeout_us         : requested Phase Cal Timeout e.g. 1000us
 * @param[in]   mm_config_timeout_us               : requested MM Timeout e.g. 2000us
 * @param[in]   range_config_timeout_us            : requested Ranging Timeout e.g 10000us
 * @param[in]   inter_measurement_period_ms        : requested timed mode repeat rate e.g 100ms
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_preset_mode(
	VL53L1_DEV                   Dev,
	VL53L1_DevicePresetModes     device_preset_mode,
	uint16_t                     dss_config__target_total_rate_mcps,
	uint32_t                     phasecal_config_timeout_us,
	uint32_t                     mm_config_timeout_us,
	uint32_t                     range_config_timeout_us,
	uint32_t                     inter_measurement_period_ms);


/**
 * @brief  Gets the requested preset mode configuration tuning parameters
 *
 * Function Added as part of Patch_TuningParmPresetModeAddition_11839
 *
 * @param[in]   Dev                : Device Handle
 * @param[in]  device_preset_mode  : selected device preset mode
 * @param[out] pdss_config__target_total_rate_mcps : dss rate output value
 * @param[out] pphasecal_config_timeout_us : phasecal timeout output value
 * @param[out] pmm_config_timeout_us    : mm timeout output value
 * @param[out] prange_config_timeout_us : range timeout output value
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_preset_mode_timing_cfg(
	VL53L1_DEV                   Dev,
	VL53L1_DevicePresetModes     device_preset_mode,
	uint16_t                    *pdss_config__target_total_rate_mcps,
	uint32_t                    *pphasecal_config_timeout_us,
	uint32_t                    *pmm_config_timeout_us,
	uint32_t                    *prange_config_timeout_us);

/**
 * @brief  Simple function to enable xtalk compensation
 *
 * Applies xtalk compensation to hist post processing,
 * and applies settings to device,
 *
 * @param[in]   Dev             : Device Handle
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_enable_xtalk_compensation(
	VL53L1_DEV                 Dev);

/**
 * @brief  Simple function to disable xtalk compensation
 *
 * Disables xtalk compensation from hist post processing,
 * and clears settings to device
 *
 * @param[in]   Dev             : Device Handle
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_disable_xtalk_compensation(
	VL53L1_DEV                 Dev);


/**
 * @brief  Simple function to retrieve xtalk compensation
 * status
 *
 * @param[in]   Dev                             : Device Handle
 * @param[out]  pcrosstalk_compensation_enable  : pointer to \
 * uint8 type, returns crosstalk compensation status \
 * where 0 means disabled and 1 means enabled.
 *
 */

void VL53L1_get_xtalk_compensation_enable(
	VL53L1_DEV    Dev,
	uint8_t       *pcrosstalk_compensation_enable);

/**
 * @brief  Builds and sends the I2C buffer to initialize the device
 *         and start a range measurement
 *
 * The device_config_level input controls the level of initialization
 * applied to the device
 *
 *  - System control only
 *  - Dynamic Onwards
 *      - dynamic_config and system_control
 *  - Static Onwards
 *      - static_config, dynamic_config & system_control
 *      - software standby mode only
 *  - Customer Onwards
 *      - customer_nvm_managed, static_config, dynamic_config & system_control
 *      - software standby mode only
 *  - Full
 *      - static_nvm_managed, customer_nvm_managed, static_config, dynamic_config & system_control
 *      - software standby mode only
 *
 * The system_control register group is always written as the last
 * register of this group SYSTEM__MODE_START decides what happens next ...
 *
 * @param[in]   Dev                   : Device handle
 * @param[in]   measurement_mode      : Options: \n
 *                                      VL53L1_DEVICEMEASUREMENTMODE_STOP \n
 *                                      VL53L1_DEVICEMEASUREMENTMODE_SINGLESHOT \n
 *                                      VL53L1_DEVICEMEASUREMENTMODE_BACKTOBACK \n
 *                                      VL53L1_DEVICEMEASUREMENTMODE_TIMED
 * @param[in]   device_config_level   : Options: \n
 *                                      VL53L1_DEVICECONFIGLEVEL_SYSTEM_CONTROL \n
 *                                      VL53L1_DEVICECONFIGLEVEL_DYNAMIC_ONWARDS \n
 *                                      VL53L1_DEVICECONFIGLEVEL_TIMING_ONWARDS \n
 *                                      VL53L1_DEVICECONFIGLEVEL_STATIC_ONWARDS \n
 *                                      VL53L1_DEVICECONFIGLEVEL_CUSTOMER_ONWARDS \n
 *                                      VL53L1_DEVICECONFIGLEVEL_FULL
 *
 * The system_control register group is always written as the last
 * register of this group SYSTEM__MODE_START decides what happens next ..
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_init_and_start_range(
	VL53L1_DEV                      Dev,
	uint8_t                         measurement_mode,
	VL53L1_DeviceConfigLevel        device_config_level);


/**
 * @brief  Sends an abort command to stop the in progress range.
 *         Also clears all of the measurement mode bits.
 *
 * @param[in]   Dev                  : Device handle
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_stop_range(
	VL53L1_DEV  Dev);


/**
 * @brief  Get range measurement result data
 *
 * Reads the required number of registers as single multi byte read
 * transaction and decodes the data into the data structures
 *
 * range_result_level controls which result data is read back from the
 * device
 *
 *  - Full
 *     - both system and core results are read back
 *  - System Results
 *     - only system results are read back
 *     - only the system_results structure is updated in this case
 *
 * @param[in]   Dev                   : Device Handle
 * @param[in]   device_result_level   : Options: \n
 *                                      VL53L1_DEVICERESULTSLEVEL_FULL \n
 *                                      VL53L1_DEVICERESULTSLEVEL_UPTO_CORE \n
 *                                      VL53L1_DEVICERESULTSLEVEL_SYSTEM_RESULTS
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_measurement_results(
	VL53L1_DEV                  Dev,
	VL53L1_DeviceResultsLevel   device_result_level);


/**
 * @brief  Get device system results, updates GPH registers and
 *         clears interrupt and configures SYSTEM__MODE_START
 *         based on the input measurement mode
 *
 * Internally uses the following functions:
 *
 *  - VL53L1_get_measurement_results()
 *  - VLS3L1_init_and_start_range()
 *
 * The system_control register group is always written as the last
 * register of this group SYSTEM__MODE_START decides what happens next ...
 *
 * @param[in]   Dev                  : Device Handle
 * @param[in]   device_result_level  : Options: \n
 *                                      VL53L1_DEVICERESULTSLEVEL_FULL \n
 *                                      VL53L1_DEVICERESULTSLEVEL_UPTO_CORE \n
 *                                      VL53L1_DEVICERESULTSLEVEL_SYSTEM_RESULTS
 * @param[out]  prange_results      : pointer to VL53L1_range_results_t

 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_device_results(
	VL53L1_DEV                 Dev,
	VL53L1_DeviceResultsLevel  device_result_level,
	VL53L1_range_results_t    *prange_results);


/**
 * @brief Sends the ranging handshake to clear the interrupt
 *        allow the device to move onto the next range
 *
 * updates GPH registers and clears interrupt and configures
 * SYSTEM__MODE_START register based on the input measurement mode
 *
 * Internally uses the following functions:
 *
 *  - VLS3L1_init_and_start_range()
 *
 * The system_control register group is always written as the last
 * register of this group SYSTEM__MODE_START decides what happens next ...
 *
 * @param[in]   Dev                   : Device Handle
 * @param[in]   measurement_mode      : Options: \n
 *                                      VL53L1_DEVICEMEASUREMENTMODE_STOP \n
 *                                      VL53L1_DEVICEMEASUREMENTMODE_SINGLESHOT \n
 *                                      VL53L1_DEVICEMEASUREMENTMODE_BACKTOBACK \n
 *                                      VL53L1_DEVICEMEASUREMENTMODE_TIMED
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_clear_interrupt_and_enable_next_range(
	VL53L1_DEV       Dev,
	uint8_t          measurement_mode);


/**
 * @brief Copies system and core results to range results data structure
 *
 * @param[in]   gain_factor   : gain correction factor 1.11 format
 * @param[in]   psys          : pointer to VL53L1_system_results_t
 * @param[in]   pcore         : pointer to VL53L1_core_results_t
 * @param[out]  presults      : pointer to VL53L1_range_results_t
 */

void VL53L1_copy_sys_and_core_results_to_range_results(
	int32_t                           gain_factor,
	VL53L1_system_results_t          *psys,
	VL53L1_core_results_t            *pcore,
	VL53L1_range_results_t           *presults);

/**
 * @brief Configure the GPIO interrupt config, from the given input
 *
 * @param[in]    Dev                    : Device Handle
 * @param[in]    intr_mode_distance     : distance interrupt mode
 * @param[in]    intr_mode_rate         : rate interrupt mode
 * @param[in]    intr_new_measure_ready : trigger when new interrupt ready
 * @param[in]    intr_no_target         : trigger when no target found
 * @param[in]    intr_combined_mode     : switch on combined mode
 * @param[in]    thresh_distance_high	: High distance threshold in mm
 * @param[in]    thresh_distance_low  	: Low distance threshold in mm
 * @param[in]    thresh_rate_high	: High rate threshold in 9.7 Mcps
 * @param[in]    thresh_rate_low 	: Low rate threshold in 9.7 Mcps
 */

VL53L1_Error VL53L1_set_GPIO_interrupt_config(
	VL53L1_DEV                      Dev,
	VL53L1_GPIO_Interrupt_Mode	intr_mode_distance,
	VL53L1_GPIO_Interrupt_Mode	intr_mode_rate,
	uint8_t				intr_new_measure_ready,
	uint8_t				intr_no_target,
	uint8_t				intr_combined_mode,
	uint16_t			thresh_distance_high,
	uint16_t			thresh_distance_low,
	uint16_t			thresh_rate_high,
	uint16_t			thresh_rate_low
	);

/**
 * @brief Configure the GPIO interrupt config, from the given structure
 *
 * @param[in]    Dev    	: Device Handle
 * @param[in]    intconf	: input structure (note, not a pointer)
 */

VL53L1_Error VL53L1_set_GPIO_interrupt_config_struct(
	VL53L1_DEV                      Dev,
	VL53L1_GPIO_interrupt_config_t	intconf);

/**
 * @brief Retrieves the GPIO interrupt config structure currently programmed
 * 		into the API
 *
 * @param[in]    Dev    	: Device Handle
 * @param[out]   pintconf	: output pointer to structure (note, pointer)
 */

VL53L1_Error VL53L1_get_GPIO_interrupt_config(
	VL53L1_DEV                      Dev,
	VL53L1_GPIO_interrupt_config_t	*pintconf);

/**
 * @brief Set function for offset calibration mode
 *
 * @param[in]    Dev    	      : Device Handle
 * @param[in]    offset_cal_mode  : input calibration mode
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_offset_calibration_mode(
	VL53L1_DEV                      Dev,
	VL53L1_OffsetCalibrationMode   offset_cal_mode);


/**
 * @brief Get function for offset calibration mode
 *
 * @param[in]    Dev    	        : Device Handle
 * @param[out]   poffset_cal_mode	: output pointer for calibration mode
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_offset_calibration_mode(
	VL53L1_DEV                      Dev,
	VL53L1_OffsetCalibrationMode  *poffset_cal_mode);


/**
 * @brief Set function for offset correction mode
 *
 * @param[in]    Dev    	      : Device Handle
 * @param[in]    offset_cor_mode  : input correction mode
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_offset_correction_mode(
	VL53L1_DEV                     Dev,
	VL53L1_OffsetCalibrationMode   offset_cor_mode);


/**
 * @brief Get function for offset correction mode
 *
 * @param[in]    Dev    	        : Device Handle
 * @param[out]   poffset_cor_mode	: output pointer for correction mode
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_offset_correction_mode(
	VL53L1_DEV                    Dev,
	VL53L1_OffsetCorrectionMode  *poffset_cor_mode);

/**
 * @brief Get function for Xtalk Margin setting
 * Histogram Mode version
 *
 * @param[in]    Dev    	    : Device Handle
 * @param[out]   pxtalk_margin	: output pointer for int16_t xtalk_margin factor \
 *                            in fixed point 7.9 Kcps.
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_lite_xtalk_margin_kcps(
	VL53L1_DEV                          Dev,
	int16_t                           *pxtalk_margin);

/**
 * @brief Set function for Xtalk Margin setting
 *  Histogram Mode version
 *
 * @param[in]  Dev    	     : Device Handle
 * @param[in]  xtalk_margin : Input int16_t xtalk_margin factor \
 *                            in fixed point 7.9 Kcps.
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_lite_xtalk_margin_kcps(
	VL53L1_DEV                          Dev,
	int16_t                             xtalk_margin);

/**
 * @brief Get function for Lite Mode Minimum Count Rate
 * parameter, used to filter and validate ranges based on
 * signal rate
 *
 * Note: the min count rate value is overwritten when setting preset
 * modes, and should only be altered after preset mode has been
 * selected when running Lite Mode.
 *
 * @param[in]  Dev    	     : Device Handle
 * @param[out] plite_mincountrate : Output pointer for uint16_t min count rate\
 *                            in fixed point 9.7 Mcps
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_lite_min_count_rate(
	VL53L1_DEV                          Dev,
	uint16_t                           *plite_mincountrate);


/**
 * @brief Set function for Lite Mode Minimum Count Rate
 * parameter, used to filter and validate ranges based on
 * signal rate
 *
 * Note: the min count rate value is overwritten when setting preset
 * modes, and should only be altered after preset mode has been
 * selected when running Lite Mode.
 *
 * @param[in]  Dev    	     : Device Handle
 * @param[in]  lite_mincountrate : Input uint16_t min count rate
 *                                  in fixed point 9.7 Mcps
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_lite_min_count_rate(
	VL53L1_DEV                          Dev,
	uint16_t                            lite_mincountrate);


/**
 * @brief Get function for Lite Mode Max Sigma Threshold
 * parameter, used to filter and validate ranges based on
 * estimated sigma level
 *
 * Note: the sigma thresh value is overwritten when setting preset
 * modes, and should only be altered after preset mode has been
 * selected when running Lite Mode.
 *
 * @param[in]  Dev    	   : Device Handle
 * @param[out] plite_sigma : Output pointer for uint16_t sigma thresh\
 *                            in fixed point 14.2 mm
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */


VL53L1_Error VL53L1_get_lite_sigma_threshold(
	VL53L1_DEV                          Dev,
	uint16_t                           *plite_sigma);


/**
 * @brief Set function for Lite Mode Max Sigma Threshold
 * parameter, used to filter and validate ranges based on
 * estimated sigma level
 *
 * Note: the sigma thresh value is overwritten when setting preset
 * modes, and should only be altered after preset mode has been
 * selected when running Lite Mode.
 *
 * @param[in]  Dev    	   : Device Handle
 * @param[in]  lite_sigma  : Input  uint16_t sigma thresh\
 *                            in fixed point 14.2 mm
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_lite_sigma_threshold(
	VL53L1_DEV                          Dev,
	uint16_t                            lite_sigma);


/**
 * @brief Function to restore the plane_offset, x gradient and y gradient
 * values to original NVM values.
 *
 * This function does not recalculate all xtalk parms derived from the
 * rates in question. In order to ensure correct ranging, a call to
 * VL53L1_enable_xtalk_compensation should always be called prior
 * to starting the ranging process.
 *
 * @param[in]  Dev    	   : Device Handle
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_restore_xtalk_nvm_default(
	VL53L1_DEV                     Dev);

/**
 * @brief Set function for VHV Config values
 * sets two parms into individual internal byte
 *
 * init_en - enables use of the init value instead of latest setting
 * init_value - custom vhv setting or populated from nvm
 *
 * @param[in] Dev    	     : Device Handle
 * @param[in] vhv_init_en    : input init_en setting
 * @param[in] vhv_init_value : input vhv custom value
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_vhv_config(
	VL53L1_DEV                   Dev,
	uint8_t                      vhv_init_en,
	uint8_t                      vhv_init_value);

/**
 * @brief Get function for VHV Config values
 * extracts two parms from individual internal byte
 *
 * init_en - enables use of the init value instead of latest setting
 * init_value - custom vhv setting or populated from nvm
 *
 * @param[in] Dev    	       : Device Handle
 * @param[out] pvhv_init_en    : Output pointer to uint8_t value
 * @param[out] pvhv_init_value : Output pointer to uint8_t value
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_vhv_config(
	VL53L1_DEV                   Dev,
	uint8_t                     *pvhv_init_en,
	uint8_t                     *pvhv_init_value);

/**
 * @brief Set function for VHV loopbound config
 *
 * Loopbound sets the number of +/- vhv settings to try
 * around the vhv init value during vhv search
 * i.e. if init_value = 10 and loopbound = 2
 * vhv search will run from 10-2 to 10+2
 * 8 to 12.
 *
 * @param[in] Dev    	       : Device Handle
 * @param[in] vhv_loopbound    : input loopbound value
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_vhv_loopbound(
	VL53L1_DEV                   Dev,
	uint8_t                      vhv_loopbound);

/**
 * @brief Get function for VHV loopbound config
 *
 * Loopbound sets the number of +/- vhv settings to try
 * around the vhv init value during vhv search
 * i.e. if init_value = 10 and loopbound = 2
 * vhv search will run from 10-2 to 10+2
 * 8 to 12.
 *
 * @param[in]  Dev    	       : Device Handle
 * @param[out] pvhv_loopbound  : pointer to byte to extract loopbound
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_vhv_loopbound(
	VL53L1_DEV                   Dev,
	uint8_t                     *pvhv_loopbound);

/**
 * @brief Generic Tuning Parameter extraction function
 *
 * User passes a key input to retrieve a specific tuning parameter
 * value, this will be cast to the int32_t type and output via
 * the ptuning_parm_value pointer
 *
 * If the key does not match any tuning parameter, status is returned
 * with VL53L1_ERROR_INVALID_PARAMS
 *
 * Patch_AddedTuningParms_11761
 *
 * @param[in] Dev    	       : Device Handle
 * @param[in] tuning_parm_key  : Key value to access specific tuning parm
 * @param[out] ptuning_parm_value : Pointer to output int32_t type, retrieves \
 *                                  requested tuning parm
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_get_tuning_parm(
	VL53L1_DEV                     Dev,
	VL53L1_TuningParms             tuning_parm_key,
	int32_t                       *ptuning_parm_value);

/**
 * @brief Generic Tuning Parameter set function
 *
 * User passes a key input to set a specific tuning parameter
 * value, this will be cast to the internal data type from the
 * input int32_t data type.
 *
 * If the key does not match any tuning parameter, status is returned
 * with VL53L1_ERROR_INVALID_PARAMS
 *
 * Patch_AddedTuningParms_11761
 *
 * @param[in] Dev    	       : Device Handle
 * @param[in] tuning_parm_key  : Key value to access specific tuning parm
 * @param[in] tuning_parm_value : Input tuning parm value of int32_t type, \
 *                                sets requested tuning parm.
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_tuning_parm(
	VL53L1_DEV                     Dev,
	VL53L1_TuningParms             tuning_parm_key,
	int32_t                        tuning_parm_value);


#ifdef __cplusplus
}
#endif

#endif /* _VL53L1_API_CORE_H_ */
