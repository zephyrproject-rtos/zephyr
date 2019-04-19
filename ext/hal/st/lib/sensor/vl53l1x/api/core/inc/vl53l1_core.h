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
 * @file  vl53l1_core.h
 *
 * @brief EwokPlus25 core function definitions
 */

#ifndef _VL53L1_CORE_H_
#define _VL53L1_CORE_H_

#include "vl53l1_platform.h"
#include "vl53l1_core_support.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Initialise version info in pdev
 *
 * @param[out]   Dev   : Device handle
 */

void VL53L1_init_version(
	VL53L1_DEV         Dev);


/**
 * @brief Initialise LL Driver State
 *
 * @param[out]   Dev       : Device handle
 * @param[in]    ll_state  : Device state
 */

void VL53L1_init_ll_driver_state(
	VL53L1_DEV         Dev,
	VL53L1_DeviceState ll_state);


/**
 * @brief Update LL Driver Read State
 *
 * @param[out]   Dev       : Device handle
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_update_ll_driver_rd_state(
	VL53L1_DEV         Dev);


/**
 * @brief  Checks if the LL Driver Read state and expected stream count
 *         matches the state and stream count received from the device
 *
 * @param[out]   Dev       : Device handle
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_check_ll_driver_rd_state(
	VL53L1_DEV         Dev);


/**
 * @brief Update LL Driver Configuration State
 *
 * @param[out]   Dev       : Device handle
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_update_ll_driver_cfg_state(
	VL53L1_DEV         Dev);


/**
 * @brief Convenience function to copy return SPAD enables to buffer
 *
 * @param[in ]   pdata   : pointer to VL53L1_nvm_copy_data_t
 * @param[out]   pbuffer : pointer to buffer
 */

void VL53L1_copy_rtn_good_spads_to_buffer(
	VL53L1_nvm_copy_data_t  *pdata,
	uint8_t                 *pbuffer);


/**
 * @brief Initialise system results structure to all ones
 *
 * This mimics what the device firmware does the the results registers
 * at the start of each range
 *
 * @param[out]   pdata   : pointer to VL53L1_system_results_t
 */

void VL53L1_init_system_results(
	VL53L1_system_results_t      *pdata);


/**
 * @brief Initialise zone dynamic config DSS control values
 *
 * @param[in]    Dev           : Device handler
 */

void V53L1_init_zone_dss_configs(
	VL53L1_DEV              Dev);


/**
 * @brief  Encodes a uint16_t register value into an I2C write buffer
 *
 * The register is encoded MS byte first is the buffer i.e as per the device register map
 *
 * @param[in]  ip_value : input uint16_t value
 * @param[in]  count    : register size of in bytes (1, 2, 3 or 4)
 * @param[out] pbuffer  : uint8_t pointer to the I2C write buffer
 */

void VL53L1_i2c_encode_uint16_t(
	uint16_t    ip_value,
	uint16_t    count,
	uint8_t    *pbuffer);


/**
 * @brief  Decodes a uint16_t register value from an I2C read buffer
 *
 * The I2C read buffer is assumed to be MS byte first i.e. matching the device register map
 *
 * @param[in]  count   : register size of in bytes (1, 2, 3 or 4)
 * @param[in]  pbuffer : uint8_t pointer to the I2C read buffer
 *
 * @return     value   : decoded uint16_t value
 */

uint16_t VL53L1_i2c_decode_uint16_t(
	uint16_t    count,
	uint8_t    *pbuffer);


/**
 * @brief  Encodes a int16_t register value into an I2C write buffer
 *
 * The register is encoded MS byte first is the buffer i.e as per the device register map
 *
 * @param[in]  ip_value : input int16_t value
 * @param[in]  count    : register size of in bytes (1, 2, 3 or 4)
 * @param[out] pbuffer  : uint8_t pointer to the I2C write buffer
 */

void VL53L1_i2c_encode_int16_t(
	int16_t     ip_value,
	uint16_t    count,
	uint8_t    *pbuffer);


/**
 * @brief  Decodes a int16_t register value from an I2C read buffer
 *
 * The I2C read buffer is assumed to be MS byte first i.e. matching the device register map
 *
 * @param[in]  count   : register size of in bytes (1, 2, 3 or 4)
 * @param[in]  pbuffer : uint8_t pointer to the I2C read buffer
 *
 * @return     value   : decoded int16_t value
 */

int16_t VL53L1_i2c_decode_int16_t(
	uint16_t    count,
	uint8_t    *pbuffer);


/**
 * @brief  Encodes a uint32_t register value into an I2C write buffer
 *
 * The register is encoded MS byte first is the buffer i.e as per the device register map
 *
 * @param[in]  ip_value : input uint32_t value
 * @param[in]  count    : register size of in bytes (1, 2, 3 or 4)
 * @param[out] pbuffer  : uint8_t pointer to the I2C write buffer
 */

void VL53L1_i2c_encode_uint32_t(
	uint32_t    ip_value,
	uint16_t    count,
	uint8_t    *pbuffer);


/**
 * @brief  Decodes a uint32_t register value from an I2C read buffer
 *
 * The I2C read buffer is assumed to be MS byte first i.e. matching the device register map
 *
 * @param[in]  count   : register size of in bytes (1, 2, 3 or 4)
 * @param[in]  pbuffer : uint8_t pointer to the I2C read buffer
 *
 * @return     value   : decoded uint32_t value
 */

uint32_t VL53L1_i2c_decode_uint32_t(
	uint16_t    count,
	uint8_t    *pbuffer);


/**
 * @brief  Decodes an integer register value from an I2C read buffer
 *
 * The I2C read buffer is assumed to be MS byte first i.e. matching the device register map
 *
 * @param[in]  count       : integer size of in bytes (1, 2, 3 or 4)
 * @param[in]  pbuffer     : uint8_t pointer to the I2C read buffer
 * @param[in]  bit_mask    : bit mask to apply
 * @param[in]  down_shift  : down shift to apply
 * @param[in]  offset      : offset to apply after the down shift
 *
 * @return     value    : decoded integer value
 */

uint32_t VL53L1_i2c_decode_with_mask(
	uint16_t    count,
	uint8_t    *pbuffer,
	uint32_t    bit_mask,
	uint32_t    down_shift,
	uint32_t    offset);


/**
 * @brief  Encodes a int32_t register value into an I2C write buffer
 *
 * The register is encoded MS byte first is the buffer i.e as per the device register map
 *
 * @param[in]  ip_value : input int32_t value
 * @param[in]  count    : register size of in bytes (1, 2, 3 or 4)
 * @param[out] pbuffer  : uint8_t pointer to the I2C write buffer
 */

void VL53L1_i2c_encode_int32_t(
	int32_t     ip_value,
	uint16_t    count,
	uint8_t    *pbuffer);


/**
 * @brief  Decodes a int32_t register value from an I2C read buffer
 *
 * The I2C read buffer is assumed to be MS byte first i.e. matching the device register map
 *
 * @param[in]  count   : register size of in bytes (1, 2, 3 or 4)
 * @param[in]  pbuffer : uint8_t pointer to the I2C read buffer
 *
 * @return     value   : decoded int32_t value
 */

int32_t VL53L1_i2c_decode_int32_t(
	uint16_t    count,
	uint8_t    *pbuffer);


/**
 * @brief Triggers the start of the provided test_mode.
 *
 * @param[in]  Dev                 : Device handle
 * @param[in]  test_mode__ctrl      : VL53L1_TEST_MODE__CTRL register value
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

#ifndef VL53L1_NOCALIB
VL53L1_Error VL53L1_start_test(
	VL53L1_DEV     Dev,
	uint8_t        test_mode__ctrl);
#endif


/**
 * @brief Set firmware enable register
 *
 * Wrapper for setting power force register state
 * Also updates pdev->sys_ctrl.firmware__enable
 *
 * @param[in]   Dev   : Device handle
 * @param[in]   value  : register value
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_firmware_enable_register(
	VL53L1_DEV         Dev,
	uint8_t            value);


/**
 * @brief Enables MCU firmware
 *
 * @param[in]   Dev   : Device handle
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_enable_firmware(
	VL53L1_DEV         Dev);


/**
 * @brief Disables MCU firmware
 *
 * This required to write to MCU G02, G01 registers and access the patch RAM
 *
 * @param[in]   Dev   : Device handle
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_disable_firmware(
	VL53L1_DEV         Dev);


/**
 * @brief Set power force register
 *
 * Wrapper for setting power force register state
 * Also updates pdev->sys_ctrl.power_management__go1_power_force
 *
 * @param[in]   Dev   : Device handle
 * @param[in]   value  : register value
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_set_powerforce_register(
	VL53L1_DEV         Dev,
	uint8_t            value);


/**
 * @brief Enables power force
 *
 * This prevents the G01 and patch RAM from powering down between
 * ranges to enable reading of G01 and patch RAM data
 *
 * Calls VL53L1_set_powerforce_register()
 *
 * @param[in]   Dev   : Device handle
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */


VL53L1_Error VL53L1_enable_powerforce(
	VL53L1_DEV         Dev);

/**
 * @brief Disables power force
 *
 * @param[in]   Dev   : Device handle
 *
 * Disable power force
 *
 * Calls VL53L1_set_powerforce_register()
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_disable_powerforce(
	VL53L1_DEV         Dev);


/**
 * @brief Clears Ranging Interrupt
 *
 * @param[in]   Dev   : Device handle
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */


VL53L1_Error VL53L1_clear_interrupt(
	VL53L1_DEV         Dev);


/**
 * @brief Forces shadow stream count to zero
 *
 * @param[in]   Dev   : Device handle
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */


#ifdef VL53L1_DEBUG
VL53L1_Error VL53L1_force_shadow_stream_count_to_zero(
	VL53L1_DEV         Dev);
#endif

/**
 * @brief Calculates the macro period in [us] for the input
 *        fast_osc_frequency and VCSEL period register value
 *
 * Processing steps:
 *
 *  1. PLL  period from fast_osc_frequency
 *  2. VCSEL period
 *  3. Macro period
 *
 * @param[in] fast_osc_frequency : fast oscillator frequency in 4.12 MHz format
 * @param[in] vcsel_period       : VCSEL period register value
 *
 * @return    macro_period_us : macro period in [us] 12.12 format
 */

uint32_t VL53L1_calc_macro_period_us(
	uint16_t fast_osc_frequency,
	uint8_t  vcsel_period);


/**
 * @brief Calculates the Xtalk Range Ignore Threshold
 * rate per spad in 3.13Mcps
 *
 * This is done by calculating the worst case xtalk seen on the array
 * and applying a user specified fractional multiplier.
 *
 * @param[in] central_rate  : xtalk central rate in 9.9u Kcps format
 * @param[in] x_gradient    : xtalk xgradient rate in 4.11s Kcps format
 * @param[in] y_gradient    : xtalk ygradient rate in 4.11s Kcps format
 * @param[in] rate_mult     : rate multiplier in  2.5 fractional format
 *
 * @return    range_ignore_threshold_kcps : rate per spad in mcps 3.13
 */

uint16_t VL53L1_calc_range_ignore_threshold(
	uint32_t central_rate,
	int16_t  x_gradient,
	int16_t  y_gradient,
	uint8_t  rate_mult);


/**
 * @brief Calculates the timeout value in macro period based on the input
 *        timeout period in milliseconds and the macro period in us
 *
 * @param[in]   timeout_us        : timeout period in microseconds
 * @param[in]   macro_period_us   : macro  period in microseconds 12.12
 *
 * @return      timeout_mclks     : timeout in macro periods
 */

uint32_t VL53L1_calc_timeout_mclks(
	uint32_t  timeout_us,
	uint32_t  macro_period_us);

/**
 * @brief Calculates the encoded timeout register value based on the input
 *        timeout period in milliseconds and the macro period in us
 *
 * @param[in]   timeout_us        : timeout period in microseconds
 * @param[in]   macro_period_us   : macro  period in microseconds 12.12
 *
 * @return      timeout_encoded   : encoded timeout register value
 */

uint16_t VL53L1_calc_encoded_timeout(
	uint32_t  timeout_us,
	uint32_t  macro_period_us);


/**
 * @brief Calculates the timeout in us based on the input
 *        timeout im macro periods value and the macro period in us
 *
 * @param[in]   timeout_mclks    : timeout im macro periods
 * @param[in]   macro_period_us  : macro  period in microseconds 12.12
 *
 * @return      timeout_us    : encoded timeout register value
 */

uint32_t VL53L1_calc_timeout_us(
	uint32_t  timeout_mclks,
	uint32_t  macro_period_us);

/**
 * @brief Calculates the decoded timeout in us based on the input
 *        encoded timeout register value and the macro period in us
 *
 * @param[in]   timeout_encoded   : encoded timeout register value
 * @param[in]   macro_period_us   : macro  period in microseconds 12.12
 *
 * @return      timeout_us    : encoded timeout register value
 */

uint32_t VL53L1_calc_decoded_timeout_us(
	uint16_t  timeout_encoded,
	uint32_t  macro_period_us);


/**
 * @brief  Encode timeout in (LSByte * 2^MSByte) + 1 register format.
 *
 * @param[in]  timeout_mclks  : uint32_t timeout value (macro periods)
 *
 * @return encoded_timeout : 16-bit encoded value
 */

uint16_t VL53L1_encode_timeout(
	uint32_t timeout_mclks);


/**
 * @brief  Decode 16-bit timeout register value.
 *
 * @param[in]   encoded_timeout : 16-bit timeout register value
 *
 * @return timeout_macro_clks : uint32_t decoded value
 *
 */

uint32_t VL53L1_decode_timeout(
	uint16_t encoded_timeout);


/**
 * @brief   Converts the input MM and range timeouts in [us]
 *          into the appropriate register values
 *
 * Must also be run after the VCSEL period settings are changed
 *
 * @param[in]   phasecal_config_timeout_us : requested Phase Cal Timeout e.g. 1000us
 * @param[in]   mm_config_timeout_us       : requested MM Timeout e.g. 2000us
 * @param[in]   range_config_timeout_us    : requested Ranging Timeout e.g 10000us
 * @param[in]   fast_osc_frequency         : fast oscillator frequency in 4.12 MHz format
 * @param[out]  pgeneral                   : general data struct
 * @param[out]  ptiming                    : timing data struct with input vcsel period data
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error  VL53L1_calc_timeout_register_values(
	uint32_t                 phasecal_config_timeout_us,
	uint32_t                 mm_config_timeout_us,
	uint32_t                 range_config_timeout_us,
	uint16_t                 fast_osc_frequency,
	VL53L1_general_config_t *pgeneral,
	VL53L1_timing_config_t  *ptiming);


/**
 * @brief  Encodes the real period in PLL clocks into the register value
 *
 * @param[in]  vcsel_period_pclks : 8-bit value
 *
 * @return vcsel_period_reg   : 8-bit encoded value
 *
 */

uint8_t VL53L1_encode_vcsel_period(
	uint8_t vcsel_period_pclks);


/**
 * @brief Decodes an unsigned integer from a uint8_t buffer  16-bit, 24-bit or 32-bit integers.
 *
 * Assumes MS Byte first
 *
 * @param[in]  pbuffer     : pointer to start of integer uint8_t data
 * @param[in]  no_of_bytes : size of integer in bytes
 *
 * @return decoded_value   : decoded integer value
 *
 */

uint32_t VL53L1_decode_unsigned_integer(
	uint8_t  *pbuffer,
	uint8_t   no_of_bytes);


/**
 * @brief Encodes an unsigned integer into a uint8_t buffer MS byte first
 *
 * @param[in]   ip_value   : input unsigned integer
 * @param[in]   no_of_bytes : size of integer storage in bytes
 * @param[out]  pbuffer     : pointer to output buffer
 *
 */

void   VL53L1_encode_unsigned_integer(
	uint32_t  ip_value,
	uint8_t   no_of_bytes,
	uint8_t  *pbuffer);

/**
 * @brief Get the SPAD  number, byte index (0-31) and bit index (0-7) for
 *
 * Takes the map index (0 - 255) and calculated the SPAD number, byte index
 * within the SPAD enable byte array and but position within the SPAD enable
 * byte
 *
 *
 * @param[in]  spad_number  : spad number
 * @param[out] pbyte_index  : pointer to output 0-31 byte index for SPAD enables
 * @param[out] pbit_index   : pointer to output 0-7 bit index
 * @param[out] pbit_mask    : pointer to output bit mask for the byte
 *
 */

void VL53L1_spad_number_to_byte_bit_index(
	uint8_t  spad_number,
	uint8_t *pbyte_index,
	uint8_t *pbit_index,
	uint8_t *pbit_mask);


/**
 * @brief Encodes a (col,row) coord value into ByteIndex.BitIndex format
 *
 *
 * @param[in]   row              : Row
 * @param[in]   col              : Column
 * @param[out]  pspad_number     : Encoded Coord in Byte.Bit format
 *
 */

void VL53L1_encode_row_col(
	uint8_t  row,
	uint8_t  col,
	uint8_t *pspad_number);


/**
 * @brief Decodes encoded zone size format into width and height values
 *
 * @param[in]   encoded_xy_size : Encoded zone size
 * @param[out]  pwidth          : Decoded zone width
 * @param[out]  pheight         : Decoded zone height
 *
 */

void VL53L1_decode_zone_size(
	uint8_t   encoded_xy_size,
	uint8_t  *pwidth,
	uint8_t  *pheight);


/**
 * @brief Encodes a zone width & height into encoded size format
 *
 * @param[in]   width            : Zone Width
 * @param[in]   height           : Zone height
 * @param[out]  pencoded_xy_size : Encoded zone size
 *
 */

void VL53L1_encode_zone_size(
	uint8_t  width,
	uint8_t  height,
	uint8_t *pencoded_xy_size);


/**
 * @brief Decodes encoded zone info into min/max limits
 *
 * Note both the lower left and upper right coordinated lie within
 * the zone (inclusive)
 *
 * @param[in]   encoded_xy_centre : Encoded zone centre (spad number)
 * @param[in]   encoded_xy_size   : Encoded zone size
 * @param[out]  px_ll             : Decoded zone lower left x coord
 * @param[out]  py_ll             : Decoded zone lower left y coord
 * @param[out]  px_ur             : Decoded zone upper right x coord
 * @param[out]  py_ur             : Decoded zone upper right y coord
 */

void VL53L1_decode_zone_limits(
	uint8_t   encoded_xy_centre,
	uint8_t   encoded_xy_size,
	int16_t  *px_ll,
	int16_t  *py_ll,
	int16_t  *px_ur,
	int16_t  *py_ur);


/**
 * @brief Returns > 0 if input (row,col) location is an apertured SPAD
 *
 * @param[in]   row              : Row
 * @param[in]   col              : Column
 *
 * @return  is_aperture          : if > 0 the location is an apertured SPAD
 */

uint8_t VL53L1_is_aperture_location(
	uint8_t   row,
	uint8_t   col);


/**
 * @brief Calculates the effective SPAD counts for the MM inner and outer
 *        regions based on the input MM ROI, Zone info and return good
 *        SPAD map
 *
 * @param[in]   encoded_mm_roi_centre     : Encoded MM ROI centre - spad number
 * @param[in]   encoded_mm_roi_size       : Encoded MM ROI size
 * @param[in]   encoded_zone_centre       : Encoded Zone centre - spad number
 * @param[in]   encoded_zone_size         : Encoded Zone size
 * @param[in]   pgood_spads               : Return array good SPAD map (32 bytes)
 * @param[in]   aperture_attenuation      : Aperture attenuation value, Format 8.8
 * @param[out]  pmm_inner_effective_spads : MM Inner effective SPADs, Format 8.8
 * @param[out]  pmm_outer_effective_spads : MM Outer effective SPADs, Format 8.8
 *
 */

void VL53L1_calc_mm_effective_spads(
	uint8_t     encoded_mm_roi_centre,
	uint8_t     encoded_mm_roi_size,
	uint8_t     encoded_zone_centre,
	uint8_t     encoded_zone_size,
	uint8_t    *pgood_spads,
	uint16_t    aperture_attenuation,
	uint16_t   *pmm_inner_effective_spads,
	uint16_t   *pmm_outer_effective_spads);

/**
 * @brief Function to save dynamic config data per zone at init and start range
 *
 * @param[in]   Dev          : Pointer to data structure
 *
 *
 * @return  VL53L1_ERROR_NONE     Success
 *
 */

VL53L1_Error VL53L1_save_cfg_data(
	VL53L1_DEV  Dev);


/**
 * @brief Encodes VL53L1_GPIO_interrupt_config_t structure to FW register format
 *
 * @param[in]	pintconf	: pointer to gpio interrupt config structure
 * @return	The encoded system__interrupt_config_gpio byte
 */

uint8_t	VL53L1_encode_GPIO_interrupt_config(
	VL53L1_GPIO_interrupt_config_t	*pintconf);

/**
 * @brief Decodes FW register to VL53L1_GPIO_interrupt_config_t structure
 *
 * @param[in]	system__interrupt_config : interrupt config register byte
 * @return	The decoded structure
 */

VL53L1_GPIO_interrupt_config_t VL53L1_decode_GPIO_interrupt_config(
	uint8_t		system__interrupt_config);

/**
 * @brief SET GPIO distance threshold
 *
 * @param[in]    Dev    	: Device Handle
 * @param[in]    threshold_high	: High distance threshold in mm
 * @param[in]    threshold_low  : Low distance threshold in mm
 */

VL53L1_Error VL53L1_set_GPIO_distance_threshold(
	VL53L1_DEV                      Dev,
	uint16_t			threshold_high,
	uint16_t			threshold_low);

/**
 * @brief SET GPIO rate threshold
 *
 * @param[in]    Dev    	: Device Handle
 * @param[in]    threshold_high	: High rate threshold in 9.7 Mcps
 * @param[in]    threshold_low  : Low rate threshold in 9.7 Mcps
 */

VL53L1_Error VL53L1_set_GPIO_rate_threshold(
	VL53L1_DEV                      Dev,
	uint16_t			threshold_high,
	uint16_t			threshold_low);

/**
 * @brief SET GPIO thresholds from structure. Sets both rate and distance
 * thresholds
 *
 * @param[in]    Dev    	: Device Handle
 * @param[in]    pintconf	: Pointer to structure
 */

VL53L1_Error VL53L1_set_GPIO_thresholds_from_struct(
	VL53L1_DEV                      Dev,
	VL53L1_GPIO_interrupt_config_t *pintconf);


/**
 * @brief Set Ref SPAD Characterisation Config
 *
 * Initialises the timeout and VCSEL period for the Reference
 * SPAD Characterisation test mode.
 *
 * Register Mapping:
 *
 * - timeout                        -> VL53L1_PHASECAL_CONFIG__TIMEOUT_MACROP
 * - vcsel_period                   -> VL53L1_RANGE_CONFIG__VCSEL_PERIOD_A \n
 *                                  -> VL53L1_SD_CONFIG__WOI_SD0 \n
 *                                  -> VL53L1_SD_CONFIG__WOI_SD1
 * - total_rate_target_mcps         -> VL53L1_REF_SPAD_CHAR__TOTAL_RATE_TARGET_MCPS
 * - max_count_rate_rtn_limit_mcps  -> VL53L1_RANGE_CONFIG__SIGMA_THRESH
 * - min_count_rate_rtn_limit_mcps  -> VL53L1_SRANGE_CONFIG__SIGMA_THRESH
 *
 * @param[in]   Dev                           : Device handle
 * @param[in]   vcsel_period_a                : VCSEL period A register value
 * @param[in]   phasecal_timeout_us           : requested PhaseCal Timeout in [us]
 *                                               e.g 1000us
 * @param[in]   total_rate_target_mcps        : Target reference rate [Mcps] 9.7 format
 * @param[in]   max_count_rate_rtn_limit_mcps : Max rate final check limit [Mcps] 9.7 format
 * @param[in]   min_count_rate_rtn_limit_mcps : Min rate final check limit [Mcps] 9.7 format
 * @param[in]   fast_osc_frequency            : Fast Osc Frequency (4.12) format
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */


#ifndef VL53L1_NOCALIB
VL53L1_Error VL53L1_set_ref_spad_char_config(
	VL53L1_DEV    Dev,
	uint8_t       vcsel_period_a,
	uint32_t      phasecal_timeout_us,
	uint16_t      total_rate_target_mcps,
	uint16_t      max_count_rate_rtn_limit_mcps,
	uint16_t      min_count_rate_rtn_limit_mcps,
	uint16_t      fast_osc_frequency);
#endif


/**
 * @brief Applies SSC (SPAD Self Check) configuration to device.
 *
 * Prior to calling this function it is assumed both the Fast Osc
 * and VCSEL have already been trimmed and the register values set.
 *
 * Internally the required timeout in macro periods is calculated
 * from the input VCSEL period, fast_osc_frequency and requested
 * timeout in microseconds.
 *
 * Register Mapping:
 *
 * - rate_select    -> VL53L1_NVM_BIST__CTRL
 * - timeout        -> VL53L1_RANGE_CONFIG__TIMEOUT_MACROP_ B_HI & _LO
 * - vcsel_period   -> VL53L1_RANGE_CONFIG__VCSEL_PERIOD_B \n
 *                  -> VL53L1_SD_CONFIG__WOI_SD0 \n
 *                  -> VL53L1_SD_CONFIG__WOI_SD1
 * - vcsel_start    -> VL53L1_CAL_CONFIG__VCSEL_START
 * - vcsel_width    -> VL53L1_GLOBAL_CONFIG__VCSEL_WIDTH
 * - ssc_limit_mcps -> VL53L1_RANGE_CONFIG__SIGMA_THRESH
 *
 * ssc_rate_limit_mcps format:
 *
 *  - 1.15 for DCR/LCR test modes with VCSEL off
 *  -  9.7 LCR test mode with VCSEL on
 *
 * The configuration is set to the device via a 5-byte multi byte write.
 *
 * @param[in]   Dev                : Device handle
 * @param[in]   pssc_cfg           : pointer to VL53L1_ssc_config_t
 * @param[in]   fast_osc_frequency : Fast Osc Frequency (4.12) format
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

#ifndef VL53L1_NOCALIB
VL53L1_Error VL53L1_set_ssc_config(
	VL53L1_DEV           Dev,
	VL53L1_ssc_config_t *pssc_cfg,
	uint16_t             fast_osc_frequency);
#endif


/**
 * @brief Gets the 256 return array SSC rates from the Patch RAM
 *
 * Each SSC rate is 1.15 or 9.7 dependent on test run.
 * Total of 512 bytes to read!
 *
 * ssc_rate_mcps buffer format:
 *
 *  - 1.15 for DCR/LCR test modes with VCSEL off (ambient)
 *  -  9.7 LCR test mode with VCSEL on (which rate?)
 *
 * Assumes that power force has already been enabled
 *
 * Function disables firmware at start to allow patch RAM to be read
 * and then enables the firmware before returning
 *
 * The order of the rates is in SPAD number order (increasing)
 *
 * @param[in]  Dev               : Device handle
 * @param[out] pspad_rates       : pointer to VL53L1_spad_rate_data_t
 *
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

#ifndef VL53L1_NOCALIB
VL53L1_Error VL53L1_get_spad_rate_data(
	VL53L1_DEV                Dev,
	VL53L1_spad_rate_data_t  *pspad_rates);
#endif

/**
 * @brief Function to add signed margin to the
 * xtalk plane offset value, dealing with signed and unsigned
 * value mixing
 *
 * Clips output to 0 if negative
 * Clips to max possible value of 9.9 Kcps if exceeds this limitation
 *
 * @param[in]  plane_offset_kcps  : xtalk plane offset (9.9 format) Kcps
 * @param[in] margin_offset_kcps  : margin offset signed (6.9 format) Kcps
 *
 * @return plane_offset_with_margin in Kcps (7.9 format)
 */

uint32_t VL53L1_calc_crosstalk_plane_offset_with_margin(
		uint32_t     plane_offset_kcps,
		int16_t      margin_offset_kcps);

/**
 * @brief Initialize the Low Power Auto data structure
 *
 * Patch_LowPowerAutoMode
 *
 * @param[in] Dev    	       : Device Handle
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_low_power_auto_data_init(
	VL53L1_DEV                     Dev
	);

/**
 * @brief Reset internal state but leave low_power_auto mode intact
 *
 * Patch_LowPowerAutoMode
 *
 * @param[in] Dev    	       : Device Handle
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_low_power_auto_data_stop_range(
	VL53L1_DEV                     Dev
	);

/**
 * @brief Initialize the config strcutures when low power auto preset modes are
 * selected
 *
 * Patch_LowPowerAutoMode
 *
 * @param[in] pgeneral : pointer to the general config structure
 * @param[in] pdynamic : pointer to the dynamic config structure
 * @param[in] plpadata : pointer to the low power auto config structure
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_config_low_power_auto_mode(
	VL53L1_general_config_t   *pgeneral,
	VL53L1_dynamic_config_t   *pdynamic,
	VL53L1_low_power_auto_data_t *plpadata
	);

/**
 * @brief Setup ranges after the first one in low power auto mode by turning
 * off FW calibration steps and programming static values
 *
 * Patch_LowPowerAutoMode
 *
 * @param[out]   Dev   : Device handle
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_low_power_auto_setup_manual_calibration(
	VL53L1_DEV        Dev);

/**
 * @brief Do a DSS calculation and update manual config
 *
 * Patch_LowPowerAutoMode
 *
 * @param[out]   Dev   : Device handle
 *
 * @return   VL53L1_ERROR_NONE    Success
 * @return  "Other error code"    See ::VL53L1_Error
 */

VL53L1_Error VL53L1_low_power_auto_update_DSS(
	VL53L1_DEV        Dev);

#ifdef __cplusplus
}
#endif

#endif /* _VL53L1_API_CORE_H_ */
