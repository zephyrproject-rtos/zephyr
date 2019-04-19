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
 * @file   vl53l1_nvm_map.h
 * @brief  NVM Map  definitions for EwokPlus25 NVM Interface Functions.
 *
 */

/*
 * Include platform specific and register map definitions
 */


#ifndef _VL53L1_NVM_MAP_H_
#define _VL53L1_NVM_MAP_H_


#ifdef __cplusplus
extern "C"
{
#endif


/** @defgroup VL53L1_nvm_DefineRegisters_group Define Registers *  @brief List of all the defined registers
 *  @{
 */

#define VL53L1_NVM__IDENTIFICATION__MODEL_ID                                             0x0008
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__identification_model_id
*/
#define VL53L1_NVM__IDENTIFICATION__MODULE_TYPE                                          0x000C
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__identification_module_type
*/
#define VL53L1_NVM__IDENTIFICATION__REVISION_ID                                          0x000D
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [3:0] = nvm__identification_revision_id
*/
#define VL53L1_NVM__IDENTIFICATION__MODULE_ID                                            0x000E
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [15:0] = nvm__identification_module_id
*/
#define VL53L1_NVM__I2C_VALID                                                            0x0010
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__i2c_valid
*/
#define VL53L1_NVM__I2C_SLAVE__DEVICE_ADDRESS                                            0x0011
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__i2c_device_address_ews
*/
#define VL53L1_NVM__EWS__OSC_MEASURED__FAST_OSC_FREQUENCY                                0x0014
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [15:0] = nvm__ews__fast_osc_frequency (fixed point 4.12)
*/
#define VL53L1_NVM__EWS__FAST_OSC_TRIM_MAX                                               0x0016
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  6
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [6:0] = nvm__ews__fast_osc_trim_max
*/
#define VL53L1_NVM__EWS__FAST_OSC_FREQ_SET                                               0x0017
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  2
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [2:0] = nvm__ews__fast_osc_freq_set
*/
#define VL53L1_NVM__EWS__SLOW_OSC_CALIBRATION                                            0x0018
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb =  9
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [9:0] = nvm__ews__slow_osc_calibration
*/
#define VL53L1_NVM__FMT__OSC_MEASURED__FAST_OSC_FREQUENCY                                0x001C
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [15:0] = nvm__fmt__fast_osc_frequency (fixed point 4.12)
*/
#define VL53L1_NVM__FMT__FAST_OSC_TRIM_MAX                                               0x001E
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  6
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [6:0] = nvm__fmt__fast_osc_trim_max
*/
#define VL53L1_NVM__FMT__FAST_OSC_FREQ_SET                                               0x001F
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  2
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [2:0] = nvm__fmt__fast_osc_freq_set
*/
#define VL53L1_NVM__FMT__SLOW_OSC_CALIBRATION                                            0x0020
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb =  9
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [9:0] = nvm__fmt__slow_osc_calibration
*/
#define VL53L1_NVM__VHV_CONFIG_UNLOCK                                                    0x0028
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__vhv_config_unlock
*/
#define VL53L1_NVM__REF_SELVDDPIX                                                        0x0029
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [3:0] = nvm__ref_selvddpix
*/
#define VL53L1_NVM__REF_SELVQUENCH                                                       0x002A
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  6
		- lsb =  3
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [6:3] = nvm__ref_selvquench
*/
#define VL53L1_NVM__REGAVDD1V2_SEL_REGDVDD1V2_SEL                                        0x002B
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [3:2] = nvm__regavdd1v2_sel
		- [1:0] = nvm__regdvdd1v2_sel
*/
#define VL53L1_NVM__VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND                                0x002C
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [1:0] = nvm__vhv_timeout__macrop
		- [7:2] = nvm__vhv_loop_bound
*/
#define VL53L1_NVM__VHV_CONFIG__COUNT_THRESH                                             0x002D
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__vhv_count_threshold
*/
#define VL53L1_NVM__VHV_CONFIG__OFFSET                                                   0x002E
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [5:0] = nvm__vhv_offset
*/
#define VL53L1_NVM__VHV_CONFIG__INIT                                                     0x002F
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		-   [7] = nvm__vhv_init_enable
		- [5:0] = nvm__vhv_init_value
*/
#define VL53L1_NVM__LASER_SAFETY__VCSEL_TRIM_LL                                          0x0030
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  2
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [2:0] = nvm__laser_safety_vcsel_trim_ll
*/
#define VL53L1_NVM__LASER_SAFETY__VCSEL_SELION_LL                                        0x0031
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [5:0] = nvm__laser_safety_vcsel_selion_ll
*/
#define VL53L1_NVM__LASER_SAFETY__VCSEL_SELION_MAX_LL                                    0x0032
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [5:0] = nvm__laser_safety_vcsel_selion_max_ll
*/
#define VL53L1_NVM__LASER_SAFETY__MULT_LL                                                0x0034
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [5:0] = nvm__laser_safety_mult_ll
*/
#define VL53L1_NVM__LASER_SAFETY__CLIP_LL                                                0x0035
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [5:0] = nvm__laser_safety_clip_ll
*/
#define VL53L1_NVM__LASER_SAFETY__VCSEL_TRIM_LD                                          0x0038
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  2
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [2:0] = nvm__laser_safety_vcsel_trim_ld
*/
#define VL53L1_NVM__LASER_SAFETY__VCSEL_SELION_LD                                        0x0039
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [5:0] = nvm__laser_safety_vcsel_selion_ld
*/
#define VL53L1_NVM__LASER_SAFETY__VCSEL_SELION_MAX_LD                                    0x003A
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [5:0] = nvm__laser_safety_vcsel_selion_max_ld
*/
#define VL53L1_NVM__LASER_SAFETY__MULT_LD                                                0x003C
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [5:0] = nvm__laser_safety_mult_ld
*/
#define VL53L1_NVM__LASER_SAFETY__CLIP_LD                                                0x003D
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [5:0] = nvm__laser_safety_clip_ld
*/
#define VL53L1_NVM__LASER_SAFETY_LOCK_BYTE                                               0x0040
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__laser_safety_lock_byte
*/
#define VL53L1_NVM__LASER_SAFETY_UNLOCK_BYTE                                             0x0044
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__laser_safety_unlock_byte
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_0_                                             0x0048
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_0_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_1_                                             0x0049
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_1_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_2_                                             0x004A
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_2_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_3_                                             0x004B
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_3_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_4_                                             0x004C
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_4_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_5_                                             0x004D
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_5_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_6_                                             0x004E
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_6_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_7_                                             0x004F
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_7_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_8_                                             0x0050
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_8_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_9_                                             0x0051
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_9_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_10_                                            0x0052
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_10_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_11_                                            0x0053
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_11_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_12_                                            0x0054
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_12_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_13_                                            0x0055
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_13_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_14_                                            0x0056
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_14_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_15_                                            0x0057
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_15_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_16_                                            0x0058
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_16_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_17_                                            0x0059
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_17_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_18_                                            0x005A
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_18_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_19_                                            0x005B
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_19_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_20_                                            0x005C
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_20_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_21_                                            0x005D
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_21_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_22_                                            0x005E
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_22_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_23_                                            0x005F
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_23_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_24_                                            0x0060
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_24_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_25_                                            0x0061
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_25_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_26_                                            0x0062
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_26_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_27_                                            0x0063
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_27_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_28_                                            0x0064
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_28_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_29_                                            0x0065
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_29_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_30_                                            0x0066
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_30_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_RTN_31_                                            0x0067
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_rtn_31_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC1_0_                                       0x0068
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc1_0_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC1_1_                                       0x0069
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc1_1_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC1_2_                                       0x006A
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc1_2_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC1_3_                                       0x006B
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc1_3_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC1_4_                                       0x006C
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc1_4_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC1_5_                                       0x006D
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc1_5_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC2_0_                                       0x0070
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc2_0_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC2_1_                                       0x0071
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc2_1_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC2_2_                                       0x0072
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc2_2_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC2_3_                                       0x0073
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc2_3_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC2_4_                                       0x0074
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc2_4_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC2_5_                                       0x0075
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc2_5_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC3_0_                                       0x0078
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc3_0_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC3_1_                                       0x0079
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc3_1_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC3_2_                                       0x007A
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc3_2_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC3_3_                                       0x007B
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc3_3_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC3_4_                                       0x007C
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc3_4_
*/
#define VL53L1_NVM__EWS__SPAD_ENABLES_REF__LOC3_5_                                       0x007D
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__spad_enables_ref__loc3_5_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_0_                                             0x0080
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_0_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_1_                                             0x0081
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_1_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_2_                                             0x0082
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_2_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_3_                                             0x0083
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_3_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_4_                                             0x0084
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_4_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_5_                                             0x0085
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_5_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_6_                                             0x0086
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_6_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_7_                                             0x0087
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_7_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_8_                                             0x0088
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_8_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_9_                                             0x0089
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_9_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_10_                                            0x008A
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_10_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_11_                                            0x008B
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_11_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_12_                                            0x008C
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_12_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_13_                                            0x008D
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_13_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_14_                                            0x008E
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_14_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_15_                                            0x008F
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_15_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_16_                                            0x0090
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_16_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_17_                                            0x0091
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_17_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_18_                                            0x0092
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_18_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_19_                                            0x0093
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_19_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_20_                                            0x0094
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_20_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_21_                                            0x0095
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_21_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_22_                                            0x0096
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_22_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_23_                                            0x0097
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_23_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_24_                                            0x0098
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_24_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_25_                                            0x0099
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_25_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_26_                                            0x009A
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_26_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_27_                                            0x009B
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_27_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_28_                                            0x009C
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_28_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_29_                                            0x009D
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_29_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_30_                                            0x009E
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_30_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_RTN_31_                                            0x009F
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_rtn_31_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC1_0_                                       0x00A0
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc1_0_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC1_1_                                       0x00A1
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc1_1_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC1_2_                                       0x00A2
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc1_2_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC1_3_                                       0x00A3
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc1_3_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC1_4_                                       0x00A4
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc1_4_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC1_5_                                       0x00A5
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc1_5_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC2_0_                                       0x00A8
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc2_0_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC2_1_                                       0x00A9
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc2_1_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC2_2_                                       0x00AA
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc2_2_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC2_3_                                       0x00AB
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc2_3_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC2_4_                                       0x00AC
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc2_4_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC2_5_                                       0x00AD
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc2_5_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC3_0_                                       0x00B0
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc3_0_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC3_1_                                       0x00B1
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc3_1_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC3_2_                                       0x00B2
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc3_2_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC3_3_                                       0x00B3
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc3_3_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC3_4_                                       0x00B4
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc3_4_
*/
#define VL53L1_NVM__FMT__SPAD_ENABLES_REF__LOC3_5_                                       0x00B5
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spad_enables_ref__loc3_5_
*/
#define VL53L1_NVM__FMT__ROI_CONFIG__MODE_ROI_CENTRE_SPAD                                0x00B8
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__roi_config__mode_roi_centre_spad
*/
#define VL53L1_NVM__FMT__ROI_CONFIG__MODE_ROI_XY_SIZE                                    0x00B9
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:4] = nvm__fmt__roi_config__mode_roi_x_size
		- [3:0] = nvm__fmt__roi_config__mode_roi_y_size
*/
#define VL53L1_NVM__FMT__REF_SPAD_APPLY__NUM_REQUESTED_REF_SPAD                          0x00BC
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__ref_spad_apply__num_requested_ref_spad
*/
#define VL53L1_NVM__FMT__REF_SPAD_MAN__REF_LOCATION                                      0x00BD
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [1:0] = nvm__fmt__ref_spad_man__ref_location
*/
#define VL53L1_NVM__FMT__MM_CONFIG__INNER_OFFSET_MM                                      0x00C0
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [15:0] = nvm__fmt__mm_config__inner_offset_mm
*/
#define VL53L1_NVM__FMT__MM_CONFIG__OUTER_OFFSET_MM                                      0x00C2
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [15:0] = nvm__fmt__mm_config__outer_offset_mm
*/
#define VL53L1_NVM__FMT__ALGO__PART_TO_PART_RANGE_OFFSET_MM                              0x00C4
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb = 11
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [11:0] = nvm__fmt__algo_part_to_part_range_offset_mm (fixed point 10.2)
*/
#define VL53L1_NVM__FMT__ALGO__CROSSTALK_COMPENSATION_PLANE_OFFSET_KCPS                  0x00C8
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [15:0] = nvm__fmt__algo__crosstalk_compensation_plane_offset_kcps (fixed point 7.9)
*/
#define VL53L1_NVM__FMT__ALGO__CROSSTALK_COMPENSATION_X_PLANE_GRADIENT_KCPS              0x00CA
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [15:0] = nvm__fmt__algo__crosstalk_compensation_x_plane_gradient_kcps (fixed point 5.11)
*/
#define VL53L1_NVM__FMT__ALGO__CROSSTALK_COMPENSATION_Y_PLANE_GRADIENT_KCPS              0x00CC
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [15:0] = nvm__fmt__algo__crosstalk_compensation_y_plane_gradient_kcps (fixed point 5.11)
*/
#define VL53L1_NVM__FMT__SPARE_HOST_CONFIG__NVM_CONFIG_SPARE_0                           0x00CE
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spare__host_config__nvm_config_spare_0
*/
#define VL53L1_NVM__FMT__SPARE_HOST_CONFIG__NVM_CONFIG_SPARE_1                           0x00CF
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__spare__host_config__nvm_config_spare_1
*/
#define VL53L1_NVM__CUSTOMER_NVM_SPACE_PROGRAMMED                                        0x00E0
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__customer_space_programmed
*/
#define VL53L1_NVM__CUST__I2C_SLAVE__DEVICE_ADDRESS                                      0x00E4
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__cust__i2c_device_address
*/
#define VL53L1_NVM__CUST__REF_SPAD_APPLY__NUM_REQUESTED_REF_SPAD                         0x00E8
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__cust__ref_spad_apply__num_requested_ref_spad
*/
#define VL53L1_NVM__CUST__REF_SPAD_MAN__REF_LOCATION                                     0x00E9
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [1:0] = nvm__cust__ref_spad_man__ref_location
*/
#define VL53L1_NVM__CUST__MM_CONFIG__INNER_OFFSET_MM                                     0x00EC
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [15:0] = nvm__cust__mm_config__inner_offset_mm
*/
#define VL53L1_NVM__CUST__MM_CONFIG__OUTER_OFFSET_MM                                     0x00EE
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [15:0] = nvm__cust__mm_config__outer_offset_mm
*/
#define VL53L1_NVM__CUST__ALGO__PART_TO_PART_RANGE_OFFSET_MM                             0x00F0
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb = 11
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [11:0] = nvm__cust__algo_part_to_part_range_offset_mm (fixed point 10.2)
*/
#define VL53L1_NVM__CUST__ALGO__CROSSTALK_COMPENSATION_PLANE_OFFSET_KCPS                 0x00F4
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [15:0] = nvm__cust__algo__crosstalk_compensation_plane_offset_kcps (fixed point 7.9)
*/
#define VL53L1_NVM__CUST__ALGO__CROSSTALK_COMPENSATION_X_PLANE_GRADIENT_KCPS             0x00F6
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [15:0] = nvm__cust__algo__crosstalk_compensation_x_plane_gradient_kcps (fixed point 5.11)
*/
#define VL53L1_NVM__CUST__ALGO__CROSSTALK_COMPENSATION_Y_PLANE_GRADIENT_KCPS             0x00F8
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [15:0] = nvm__cust__algo__crosstalk_compensation_y_plane_gradient_kcps (fixed point 5.11)
*/
#define VL53L1_NVM__CUST__SPARE_HOST_CONFIG__NVM_CONFIG_SPARE_0                          0x00FA
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__cust__spare__host_config__nvm_config_spare_0
*/
#define VL53L1_NVM__CUST__SPARE_HOST_CONFIG__NVM_CONFIG_SPARE_1                          0x00FB
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__cust__spare__host_config__nvm_config_spare_1
*/
#define VL53L1_NVM__FMT__FGC__BYTE_0                                                     0x01DC
/*!<
	type:     char \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  1
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:1] = nvm__fmt__fgc_0_
*/
#define VL53L1_NVM__FMT__FGC__BYTE_1                                                     0x01DD
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb =  8
		- lsb =  2
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [8:2] = nvm__fmt__fgc_1_
*/
#define VL53L1_NVM__FMT__FGC__BYTE_2                                                     0x01DE
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb =  9
		- lsb =  3
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [9:3] = nvm__fmt__fgc_2_
*/
#define VL53L1_NVM__FMT__FGC__BYTE_3                                                     0x01DF
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb = 10
		- lsb =  4
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [10:4] = nvm__fmt__fgc_3_
*/
#define VL53L1_NVM__FMT__FGC__BYTE_4                                                     0x01E0
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb = 11
		- lsb =  5
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [11:5] = nvm__fmt__fgc_4_
*/
#define VL53L1_NVM__FMT__FGC__BYTE_5                                                     0x01E1
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb = 12
		- lsb =  6
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [12:6] = nvm__fmt__fgc_5_
*/
#define VL53L1_NVM__FMT__FGC__BYTE_6                                                     0x01E2
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb = 13
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [13:7] = nvm__fmt__fgc_6_
		- [6:0] = nvm__fmt__fgc_7_
*/
#define VL53L1_NVM__FMT__FGC__BYTE_7                                                     0x01E3
/*!<
	type:     char \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  1
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:1] = nvm__fmt__fgc_8_
*/
#define VL53L1_NVM__FMT__FGC__BYTE_8                                                     0x01E4
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb =  8
		- lsb =  2
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [8:2] = nvm__fmt__fgc_9_
*/
#define VL53L1_NVM__FMT__FGC__BYTE_9                                                     0x01E5
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb =  9
		- lsb =  3
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [9:3] = nvm__fmt__fgc_10_
*/
#define VL53L1_NVM__FMT__FGC__BYTE_10                                                    0x01E6
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb = 10
		- lsb =  4
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [10:4] = nvm__fmt__fgc_11_
*/
#define VL53L1_NVM__FMT__FGC__BYTE_11                                                    0x01E7
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb = 11
		- lsb =  5
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [11:5] = nvm__fmt__fgc_12_
*/
#define VL53L1_NVM__FMT__FGC__BYTE_12                                                    0x01E8
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb = 12
		- lsb =  6
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [12:6] = nvm__fmt__fgc_13_
*/
#define VL53L1_NVM__FMT__FGC__BYTE_13                                                    0x01E9
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb = 13
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [13:7] = nvm__fmt__fgc_14_
		- [6:0] = nvm__fmt__fgc_15_
*/
#define VL53L1_NVM__FMT__FGC__BYTE_14                                                    0x01EA
/*!<
	type:     char \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  1
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:1] = nvm__fmt__fgc_16_
*/
#define VL53L1_NVM__FMT__FGC__BYTE_15                                                    0x01EB
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb =  8
		- lsb =  2
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [8:2] = nvm__fmt__fgc_17_
*/
#define VL53L1_NVM__FMT__TEST_PROGRAM_MAJOR_MINOR                                        0x01EC
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:5] = nvm__fmt__test_program_major
		- [4:0] = nvm__fmt__test_program_minor
*/
#define VL53L1_NVM__FMT__MAP_MAJOR_MINOR                                                 0x01ED
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:5] = nvm__fmt__map_major
		- [4:0] = nvm__fmt__map_minor
*/
#define VL53L1_NVM__FMT__YEAR_MONTH                                                      0x01EE
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:4] = nvm__fmt__year
		- [3:0] = nvm__fmt__month
*/
#define VL53L1_NVM__FMT__DAY_MODULE_DATE_PHASE                                           0x01EF
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:3] = nvm__fmt__day
		- [2:0] = nvm__fmt__module_date_phase
*/
#define VL53L1_NVM__FMT__TIME                                                            0x01F0
/*!<
	type:     uint16_t \n
	default:  0x0000 \n
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [15:0] = nvm__fmt__time
*/
#define VL53L1_NVM__FMT__TESTER_ID                                                       0x01F2
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__tester_id
*/
#define VL53L1_NVM__FMT__SITE_ID                                                         0x01F3
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__fmt__site_id
*/
#define VL53L1_NVM__EWS__TEST_PROGRAM_MAJOR_MINOR                                        0x01F4
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:5] = nvm__ews__test_program_major
		- [4:0] = nvm__ews__test_program_minor
*/
#define VL53L1_NVM__EWS__PROBE_CARD_MAJOR_MINOR                                          0x01F5
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:4] = nvm__ews__probe_card_major
		- [3:0] = nvm__ews__probe_card_minor
*/
#define VL53L1_NVM__EWS__TESTER_ID                                                       0x01F6
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__tester_id
*/
#define VL53L1_NVM__EWS__LOT__BYTE_0                                                     0x01F8
/*!<
	type:     char \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  2
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:2] = nvm__ews__lot_6_
*/
#define VL53L1_NVM__EWS__LOT__BYTE_1                                                     0x01F9
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb =  9
		- lsb =  4
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [9:4] = nvm__ews__lot_5_
*/
#define VL53L1_NVM__EWS__LOT__BYTE_2                                                     0x01FA
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb = 11
		- lsb =  0
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [11:6] = nvm__ews__lot_4_
		- [5:0] = nvm__ews__lot_3_
*/
#define VL53L1_NVM__EWS__LOT__BYTE_3                                                     0x01FB
/*!<
	type:     char \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  2
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:2] = nvm__ews__lot_2_
*/
#define VL53L1_NVM__EWS__LOT__BYTE_4                                                     0x01FC
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb =  9
		- lsb =  4
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [9:4] = nvm__ews__lot_1_
*/
#define VL53L1_NVM__EWS__LOT__BYTE_5                                                     0x01FD
/*!<
	type:     char \n
	default:  0x0000 \n
	info: \n
		- msb = 11
		- lsb =  6
		- i2c_size =  2

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [11:6] = nvm__ews__lot_0_
*/
#define VL53L1_NVM__EWS__WAFER                                                           0x01FD
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  4
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [4:0] = nvm__ews__wafer
*/
#define VL53L1_NVM__EWS__XCOORD                                                          0x01FE
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__xcoord
*/
#define VL53L1_NVM__EWS__YCOORD                                                          0x01FF
/*!<
	type:     uint8_t \n
	default:  0x00 \n
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	groups: \n
		['decoded_nvm_data']

	fields: \n
		- [7:0] = nvm__ews__ycoord
*/

#define VL53L1_NVM__FMT__OPTICAL_CENTRE_DATA_INDEX     0x00B8
#define VL53L1_NVM__FMT__OPTICAL_CENTRE_DATA_SIZE      4

#define VL53L1_NVM__FMT__CAL_PEAK_RATE_MAP_DATA_INDEX  0x015C
#define VL53L1_NVM__FMT__CAL_PEAK_RATE_MAP_DATA_SIZE   56

#define VL53L1_NVM__FMT__ADDITIONAL_OFFSET_CAL_DATA_INDEX  0x0194
#define VL53L1_NVM__FMT__ADDITIONAL_OFFSET_CAL_DATA_SIZE   8

#define VL53L1_NVM__FMT__RANGE_RESULTS__140MM_MM_PRE_RANGE 0x019C
#define VL53L1_NVM__FMT__RANGE_RESULTS__140MM_DARK         0x01AC
#define VL53L1_NVM__FMT__RANGE_RESULTS__400MM_DARK         0x01BC
#define VL53L1_NVM__FMT__RANGE_RESULTS__400MM_AMBIENT      0x01CC
#define VL53L1_NVM__FMT__RANGE_RESULTS__SIZE_BYTES         16

/** @} VL53L1_nvm_DefineRegisters_group */





#ifdef __cplusplus
}
#endif

#endif
