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
 * @file   vl53l1_register_structs.h
 * @brief  VL53L1 Register Structure definitions
 */

#ifndef _VL53L1_REGISTER_STRUCTS_H_
#define _VL53L1_REGISTER_STRUCTS_H_

#include "vl53l1_types.h"
#include "vl53l1_register_map.h"

#define VL53L1_STATIC_NVM_MANAGED_I2C_INDEX               VL53L1_I2C_SLAVE__DEVICE_ADDRESS
#define VL53L1_CUSTOMER_NVM_MANAGED_I2C_INDEX             VL53L1_GLOBAL_CONFIG__SPAD_ENABLES_REF_0
#define VL53L1_STATIC_CONFIG_I2C_INDEX                    VL53L1_DSS_CONFIG__TARGET_TOTAL_RATE_MCPS
#define VL53L1_GENERAL_CONFIG_I2C_INDEX                   VL53L1_GPH_CONFIG__STREAM_COUNT_UPDATE_VALUE
#define VL53L1_TIMING_CONFIG_I2C_INDEX                    VL53L1_MM_CONFIG__TIMEOUT_MACROP_A_HI
#define VL53L1_DYNAMIC_CONFIG_I2C_INDEX                   VL53L1_SYSTEM__GROUPED_PARAMETER_HOLD_0
#define VL53L1_SYSTEM_CONTROL_I2C_INDEX                   VL53L1_POWER_MANAGEMENT__GO1_POWER_FORCE
#define VL53L1_SYSTEM_RESULTS_I2C_INDEX                   VL53L1_RESULT__INTERRUPT_STATUS
#define VL53L1_CORE_RESULTS_I2C_INDEX                     VL53L1_RESULT_CORE__AMBIENT_WINDOW_EVENTS_SD0
#define VL53L1_DEBUG_RESULTS_I2C_INDEX                    VL53L1_PHASECAL_RESULT__REFERENCE_PHASE
#define VL53L1_NVM_COPY_DATA_I2C_INDEX                    VL53L1_IDENTIFICATION__MODEL_ID
#define VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_INDEX       VL53L1_PREV_SHADOW_RESULT__INTERRUPT_STATUS
#define VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_INDEX         VL53L1_PREV_SHADOW_RESULT_CORE__AMBIENT_WINDOW_EVENTS_SD0
#define VL53L1_PATCH_DEBUG_I2C_INDEX                      VL53L1_RESULT__DEBUG_STATUS
#define VL53L1_GPH_GENERAL_CONFIG_I2C_INDEX               VL53L1_GPH__SYSTEM__THRESH_RATE_HIGH
#define VL53L1_GPH_STATIC_CONFIG_I2C_INDEX                VL53L1_GPH__DSS_CONFIG__ROI_MODE_CONTROL
#define VL53L1_GPH_TIMING_CONFIG_I2C_INDEX                VL53L1_GPH__MM_CONFIG__TIMEOUT_MACROP_A_HI
#define VL53L1_FW_INTERNAL_I2C_INDEX                      VL53L1_FIRMWARE__INTERNAL_STREAM_COUNT_DIV
#define VL53L1_PATCH_RESULTS_I2C_INDEX                    VL53L1_DSS_CALC__ROI_CTRL
#define VL53L1_SHADOW_SYSTEM_RESULTS_I2C_INDEX            VL53L1_SHADOW_PHASECAL_RESULT__VCSEL_START
#define VL53L1_SHADOW_CORE_RESULTS_I2C_INDEX              VL53L1_SHADOW_RESULT_CORE__AMBIENT_WINDOW_EVENTS_SD0

#define VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES           11
#define VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES         23
#define VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES                32
#define VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES               22
#define VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES                23
#define VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES               18
#define VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES                5
#define VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES               44
#define VL53L1_CORE_RESULTS_I2C_SIZE_BYTES                 33
#define VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES                56
#define VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES                49
#define VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES   44
#define VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES     33
#define VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES                   2
#define VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES            5
#define VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES             6
#define VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES            16
#define VL53L1_FW_INTERNAL_I2C_SIZE_BYTES                   2
#define VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES                90
#define VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES        82
#define VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES          33


/**
 * @struct VL53L1_static_nvm_managed_t
 *
 * - registers    =     10
 * - first_index  =      1 (0x0001)
 * - last _index  =     11 (0x000B)
 * - i2c_size     =     11
 */

typedef struct {
	uint8_t   i2c_slave__device_address;
/*!<
	info: \n
		- msb =  6
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [6:0] = i2c_slave_device_address
*/
	uint8_t   ana_config__vhv_ref_sel_vddpix;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = ref_sel_vddpix
*/
	uint8_t   ana_config__vhv_ref_sel_vquench;
/*!<
	info: \n
		- msb =  6
		- lsb =  3
		- i2c_size =  1

	fields: \n
		- [6:3] = ref_sel_vquench
*/
	uint8_t   ana_config__reg_avdd1v2_sel;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [1:0] = reg_avdd1v2_sel
*/
	uint8_t   ana_config__fast_osc__trim;
/*!<
	info: \n
		- msb =  6
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [6:0] = fast_osc_trim
*/
	uint16_t  osc_measured__fast_osc__frequency;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = osc_frequency (fixed point 4.12)
*/
	uint8_t   vhv_config__timeout_macrop_loop_bound;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [1:0] = vhv_timeout__macrop
		- [7:2] = vhv_loop_bound
*/
	uint8_t   vhv_config__count_thresh;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = vhv_count_thresh
*/
	uint8_t   vhv_config__offset;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [5:0] = vhv_step_val
*/
	uint8_t   vhv_config__init;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [7] = vhv0_init_enable
		- [5:0] = vhv0_init_value
*/
} VL53L1_static_nvm_managed_t;


/**
 * @struct VL53L1_customer_nvm_managed_t
 *
 * - registers    =     16
 * - first_index  =     13 (0x000D)
 * - last _index  =     34 (0x0022)
 * - i2c_size     =     23
 */

typedef struct {
	uint8_t   global_config__spad_enables_ref_0;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_ref_0
*/
	uint8_t   global_config__spad_enables_ref_1;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_ref_1
*/
	uint8_t   global_config__spad_enables_ref_2;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_ref_2
*/
	uint8_t   global_config__spad_enables_ref_3;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_ref_3
*/
	uint8_t   global_config__spad_enables_ref_4;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_ref_4
*/
	uint8_t   global_config__spad_enables_ref_5;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = spad_enables_ref_5
*/
	uint8_t   global_config__ref_en_start_select;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = ref_en_start_select
*/
	uint8_t   ref_spad_man__num_requested_ref_spads;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [5:0] = ref_spad_man__num_requested_ref_spad
*/
	uint8_t   ref_spad_man__ref_location;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [1:0] = ref_spad_man__ref_location
*/
	uint16_t  algo__crosstalk_compensation_plane_offset_kcps;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = crosstalk_compensation_plane_offset_kcps (fixed point 7.9)
*/
	int16_t   algo__crosstalk_compensation_x_plane_gradient_kcps;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = crosstalk_compensation_x_plane_gradient_kcps (fixed point 5.11)
*/
	int16_t   algo__crosstalk_compensation_y_plane_gradient_kcps;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = crosstalk_compensation_y_plane_gradient_kcps (fixed point 5.11)
*/
	uint16_t  ref_spad_char__total_rate_target_mcps;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = ref_spad_char__total_rate_target_mcps (fixed point 9.7)
*/
	int16_t   algo__part_to_part_range_offset_mm;
/*!<
	info: \n
		- msb = 12
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [12:0] = part_to_part_offset_mm (fixed point 11.2)
*/
	int16_t   mm_config__inner_offset_mm;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = mm_config__inner_offset_mm
*/
	int16_t   mm_config__outer_offset_mm;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = mm_config__outer_offset_mm
*/
} VL53L1_customer_nvm_managed_t;


/**
 * @struct VL53L1_static_config_t
 *
 * - registers    =     30
 * - first_index  =     36 (0x0024)
 * - last _index  =     67 (0x0043)
 * - i2c_size     =     32
 */

typedef struct {
	uint16_t  dss_config__target_total_rate_mcps;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = dss_config__target_total_rate_mcps (fixed point 9.7)
*/
	uint8_t   debug__ctrl;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = enable_result_logging
*/
	uint8_t   test_mode__ctrl;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = test_mode__cmd
*/
	uint8_t   clk_gating__ctrl;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = clk_gate_en__mcu_bank
		-   [1] = clk_gate_en__mcu_patch_ctrl
		-   [2] = clk_gate_en__mcu_timers
		-   [3] = clk_gate_en__mcu_mult_div
*/
	uint8_t   nvm_bist__ctrl;
/*!<
	info: \n
		- msb =  4
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [2:0] = nvm_bist__cmd
		-   [4] = nvm_bist__ctrl
*/
	uint8_t   nvm_bist__num_nvm_words;
/*!<
	info: \n
		- msb =  6
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [6:0] = nvm_bist__num_nvm_words
*/
	uint8_t   nvm_bist__start_address;
/*!<
	info: \n
		- msb =  6
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [6:0] = nvm_bist__start_address
*/
	uint8_t   host_if__status;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = host_interface
*/
	uint8_t   pad_i2c_hv__config;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = pad_scl_sda__vmodeint_hv
		-   [1] = i2c_pad__test_hv
		-   [2] = pad_scl__fpen_hv
		- [4:3] = pad_scl__progdel_hv
		-   [5] = pad_sda__fpen_hv
		- [7:6] = pad_sda__progdel_hv
*/
	uint8_t   pad_i2c_hv__extsup_config;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = pad_scl_sda__extsup_hv
*/
	uint8_t   gpio_hv_pad__ctrl;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = gpio__extsup_hv
		-   [1] = gpio__vmodeint_hv
*/
	uint8_t   gpio_hv_mux__ctrl;
/*!<
	info: \n
		- msb =  4
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = gpio__mux_select_hv
		-   [4] = gpio__mux_active_high_hv
*/
	uint8_t   gpio__tio_hv_status;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = gpio__tio_hv
		-   [1] = fresh_out_of_reset
*/
	uint8_t   gpio__fio_hv_status;
/*!<
	info: \n
		- msb =  1
		- lsb =  1
		- i2c_size =  1

	fields: \n
		-   [1] = gpio__fio_hv
*/
	uint8_t   ana_config__spad_sel_pswidth;
/*!<
	info: \n
		- msb =  2
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [2:0] = spad_sel_pswidth
*/
	uint8_t   ana_config__vcsel_pulse_width_offset;
/*!<
	info: \n
		- msb =  4
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [4:0] = vcsel_pulse_width_offset (fixed point 1.4)
*/
	uint8_t   ana_config__fast_osc__config_ctrl;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = osc_config__latch_bypass
*/
	uint8_t   sigma_estimator__effective_pulse_width_ns;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = sigma_estimator__eff_pulse_width
*/
	uint8_t   sigma_estimator__effective_ambient_width_ns;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = sigma_estimator__eff_ambient_width
*/
	uint8_t   sigma_estimator__sigma_ref_mm;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = sigma_estimator__sigma_ref
*/
	uint8_t   algo__crosstalk_compensation_valid_height_mm;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = crosstalk_compensation_valid_height_mm
*/
	uint8_t   spare_host_config__static_config_spare_0;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = static_config_spare_0
*/
	uint8_t   spare_host_config__static_config_spare_1;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = static_config_spare_1
*/
	uint16_t  algo__range_ignore_threshold_mcps;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = range_ignore_thresh_mcps (fixed point 3.13)
*/
	uint8_t   algo__range_ignore_valid_height_mm;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = range_ignore_height_mm
*/
	uint8_t   algo__range_min_clip;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = algo__range_min_clip_enable
		- [7:1] = algo__range_min_clip_value_mm
*/
	uint8_t   algo__consistency_check__tolerance;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = consistency_check_tolerance (fixed point 1.3)
*/
	uint8_t   spare_host_config__static_config_spare_2;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = static_config_spare_2
*/
	uint8_t   sd_config__reset_stages_msb;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = loop_init__clear_stage
*/
	uint8_t   sd_config__reset_stages_lsb;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:4] = accum_reset__clear_stage
		- [3:0] = count_reset__clear_stage
*/
} VL53L1_static_config_t;


/**
 * @struct VL53L1_general_config_t
 *
 * - registers    =     17
 * - first_index  =     68 (0x0044)
 * - last _index  =     89 (0x0059)
 * - i2c_size     =     22
 */

typedef struct {
	uint8_t   gph_config__stream_count_update_value;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = stream_count_update_value
*/
	uint8_t   global_config__stream_divider;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = stream_count_internal_div
*/
	uint8_t   system__interrupt_config_gpio;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [1:0] = int_mode_distance
		- [3:2] = int_mode_rate
		-   [4] = int_spare
		-   [5] = int_new_measure_ready
		-   [6] = int_no_target_en
		-   [7] = int_combined_mode
*/
	uint8_t   cal_config__vcsel_start;
/*!<
	info: \n
		- msb =  6
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [6:0] = cal_config__vcsel_start
*/
	uint16_t  cal_config__repeat_rate;
/*!<
	info: \n
		- msb = 11
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [11:0] = cal_config__repeat_rate
*/
	uint8_t   global_config__vcsel_width;
/*!<
	info: \n
		- msb =  6
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [6:0] = global_config__vcsel_width
*/
	uint8_t   phasecal_config__timeout_macrop;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = phasecal_config__timeout_macrop
*/
	uint8_t   phasecal_config__target;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = algo_phasecal_lim
*/
	uint8_t   phasecal_config__override;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = phasecal_config__override
*/
	uint8_t   dss_config__roi_mode_control;
/*!<
	info: \n
		- msb =  2
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [1:0] = dss_config__input_mode
		-   [2] = calculate_roi_enable
*/
	uint16_t  system__thresh_rate_high;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = thresh_rate_high (fixed point 9.7)
*/
	uint16_t  system__thresh_rate_low;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = thresh_rate_low (fixed point 9.7)
*/
	uint16_t  dss_config__manual_effective_spads_select;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = dss_config__manual_effective_spads_select
*/
	uint8_t   dss_config__manual_block_select;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_config__manual_block_select
*/
	uint8_t   dss_config__aperture_attenuation;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_config__aperture_attenuation
*/
	uint8_t   dss_config__max_spads_limit;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_config__max_spads_limit
*/
	uint8_t   dss_config__min_spads_limit;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_config__min_spads_limit
*/
} VL53L1_general_config_t;


/**
 * @struct VL53L1_timing_config_t
 *
 * - registers    =     16
 * - first_index  =     90 (0x005A)
 * - last _index  =    112 (0x0070)
 * - i2c_size     =     23
 */

typedef struct {
	uint8_t   mm_config__timeout_macrop_a_hi;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = mm_config__config_timeout_macrop_a_hi
*/
	uint8_t   mm_config__timeout_macrop_a_lo;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = mm_config__config_timeout_macrop_a_lo
*/
	uint8_t   mm_config__timeout_macrop_b_hi;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = mm_config__config_timeout_macrop_b_hi
*/
	uint8_t   mm_config__timeout_macrop_b_lo;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = mm_config__config_timeout_macrop_b_lo
*/
	uint8_t   range_config__timeout_macrop_a_hi;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = range_timeout_overall_periods_macrop_a_hi
*/
	uint8_t   range_config__timeout_macrop_a_lo;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = range_timeout_overall_periods_macrop_a_lo
*/
	uint8_t   range_config__vcsel_period_a;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [5:0] = range_config__vcsel_period_a
*/
	uint8_t   range_config__timeout_macrop_b_hi;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = range_timeout_overall_periods_macrop_b_hi
*/
	uint8_t   range_config__timeout_macrop_b_lo;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = range_timeout_overall_periods_macrop_b_lo
*/
	uint8_t   range_config__vcsel_period_b;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [5:0] = range_config__vcsel_period_b
*/
	uint16_t  range_config__sigma_thresh;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = range_config__sigma_thresh (fixed point 14.2)
*/
	uint16_t  range_config__min_count_rate_rtn_limit_mcps;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = range_config__min_count_rate_rtn_limit_mcps (fixed point 9.7)
*/
	uint8_t   range_config__valid_phase_low;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = range_config__valid_phase_low (fixed point 5.3)
*/
	uint8_t   range_config__valid_phase_high;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = range_config__valid_phase_high (fixed point 5.3)
*/
	uint32_t  system__intermeasurement_period;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = intermeasurement_period
*/
	uint8_t   system__fractional_enable;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = range_fractional_enable
*/
} VL53L1_timing_config_t;


/**
 * @struct VL53L1_dynamic_config_t
 *
 * - registers    =     16
 * - first_index  =    113 (0x0071)
 * - last _index  =    130 (0x0082)
 * - i2c_size     =     18
 */

typedef struct {
	uint8_t   system__grouped_parameter_hold_0;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = grouped_parameter_hold
		-   [1] = grouped_parameter_hold_id
*/
	uint16_t  system__thresh_high;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = thresh_high
*/
	uint16_t  system__thresh_low;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = thresh_low
*/
	uint8_t   system__enable_xtalk_per_quadrant;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = system__enable_xtalk_per_quadrant
*/
	uint8_t   system__seed_config;
/*!<
	info: \n
		- msb =  2
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [1:0] = system__seed_config
		-   [2] = system__fw_pause_ctrl
*/
	uint8_t   sd_config__woi_sd0;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = sd_config__woi_sd0
*/
	uint8_t   sd_config__woi_sd1;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = sd_config__woi_sd1
*/
	uint8_t   sd_config__initial_phase_sd0;
/*!<
	info: \n
		- msb =  6
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [6:0] = sd_config__initial_phase_sd0
*/
	uint8_t   sd_config__initial_phase_sd1;
/*!<
	info: \n
		- msb =  6
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [6:0] = sd_config__initial_phase_sd1
*/
	uint8_t   system__grouped_parameter_hold_1;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = grouped_parameter_hold
		-   [1] = grouped_parameter_hold_id
*/
	uint8_t   sd_config__first_order_select;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = sd_config__first_order_select_rtn
		-   [1] = sd_config__first_order_select_ref
*/
	uint8_t   sd_config__quantifier;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = sd_config__quantifier
*/
	uint8_t   roi_config__user_roi_centre_spad;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = user_roi_center_spad
*/
	uint8_t   roi_config__user_roi_requested_global_xy_size;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = roi_config__user_roi_requested_global_xy_size
*/
	uint8_t   system__sequence_config;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = sequence_vhv_en
		-   [1] = sequence_phasecal_en
		-   [2] = sequence_reference_phase_en
		-   [3] = sequence_dss1_en
		-   [4] = sequence_dss2_en
		-   [5] = sequence_mm1_en
		-   [6] = sequence_mm2_en
		-   [7] = sequence_range_en
*/
	uint8_t   system__grouped_parameter_hold;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = grouped_parameter_hold
		-   [1] = grouped_parameter_hold_id
*/
} VL53L1_dynamic_config_t;


/**
 * @struct VL53L1_system_control_t
 *
 * - registers    =      5
 * - first_index  =    131 (0x0083)
 * - last _index  =    135 (0x0087)
 * - i2c_size     =      5
 */

typedef struct {
	uint8_t   power_management__go1_power_force;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = go1_dig_powerforce
*/
	uint8_t   system__stream_count_ctrl;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = retain_stream_count
*/
	uint8_t   firmware__enable;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = firmware_enable
*/
	uint8_t   system__interrupt_clear;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = sys_interrupt_clear_range
		-   [1] = sys_interrupt_clear_error
*/
	uint8_t   system__mode_start;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [1:0] = scheduler_mode
		- [3:2] = readout_mode
		-   [4] = mode_range__single_shot
		-   [5] = mode_range__back_to_back
		-   [6] = mode_range__timed
		-   [7] = mode_range__abort
*/
} VL53L1_system_control_t;


/**
 * @struct VL53L1_system_results_t
 *
 * - registers    =     25
 * - first_index  =    136 (0x0088)
 * - last _index  =    179 (0x00B3)
 * - i2c_size     =     44
 */

typedef struct {
	uint8_t   result__interrupt_status;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [2:0] = int_status
		- [4:3] = int_error_status
		-   [5] = gph_id_gpio_status
*/
	uint8_t   result__range_status;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [4:0] = range_status
		-   [5] = max_threshold_hit
		-   [6] = min_threshold_hit
		-   [7] = gph_id_range_status
*/
	uint8_t   result__report_status;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = report_status
*/
	uint8_t   result__stream_count;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = result__stream_count
*/
	uint16_t  result__dss_actual_effective_spads_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__dss_actual_effective_spads_sd0 (fixed point 8.8)
*/
	uint16_t  result__peak_signal_count_rate_mcps_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__peak_signal_count_rate_mcps_sd0 (fixed point 9.7)
*/
	uint16_t  result__ambient_count_rate_mcps_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__ambient_count_rate_mcps_sd0 (fixed point 9.7)
*/
	uint16_t  result__sigma_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__sigma_sd0 (fixed point 14.2)
*/
	uint16_t  result__phase_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__phase_sd0 (fixed point 5.11)
*/
	uint16_t  result__final_crosstalk_corrected_range_mm_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__final_crosstalk_corrected_range_mm_sd0
*/
	uint16_t  result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0 (fixed point 9.7)
*/
	uint16_t  result__mm_inner_actual_effective_spads_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__mm_inner_actual_effective_spads_sd0 (fixed point 8.8)
*/
	uint16_t  result__mm_outer_actual_effective_spads_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__mm_outer_actual_effective_spads_sd0 (fixed point 8.8)
*/
	uint16_t  result__avg_signal_count_rate_mcps_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__avg_signal_count_rate_mcps_sd0 (fixed point 9.7)
*/
	uint16_t  result__dss_actual_effective_spads_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__dss_actual_effective_spads_sd1 (fixed point 8.8)
*/
	uint16_t  result__peak_signal_count_rate_mcps_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__peak_signal_count_rate_mcps_sd1 (fixed point 9.7)
*/
	uint16_t  result__ambient_count_rate_mcps_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__ambient_count_rate_mcps_sd1 (fixed point 9.7)
*/
	uint16_t  result__sigma_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__sigma_sd1 (fixed point 14.2)
*/
	uint16_t  result__phase_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__phase_sd1 (fixed point 5.11)
*/
	uint16_t  result__final_crosstalk_corrected_range_mm_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__final_crosstalk_corrected_range_mm_sd1
*/
	uint16_t  result__spare_0_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__spare_0_sd1
*/
	uint16_t  result__spare_1_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__spare_1_sd1
*/
	uint16_t  result__spare_2_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result__spare_2_sd1
*/
	uint8_t   result__spare_3_sd1;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = result__spare_3_sd1
*/
	uint8_t   result__thresh_info;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = result__distance_int_info
		- [7:4] = result__rate_int_info
*/
} VL53L1_system_results_t;


/**
 * @struct VL53L1_core_results_t
 *
 * - registers    =      9
 * - first_index  =    180 (0x00B4)
 * - last _index  =    212 (0x00D4)
 * - i2c_size     =     33
 */

typedef struct {
	uint32_t  result_core__ambient_window_events_sd0;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = result_core__ambient_window_events_sd0
*/
	uint32_t  result_core__ranging_total_events_sd0;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = result_core__ranging_total_events_sd0
*/
	int32_t   result_core__signal_total_events_sd0;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = result_core__signal_total_events_sd0
*/
	uint32_t  result_core__total_periods_elapsed_sd0;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = result_core__total_periods_elapsed_sd0
*/
	uint32_t  result_core__ambient_window_events_sd1;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = result_core__ambient_window_events_sd1
*/
	uint32_t  result_core__ranging_total_events_sd1;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = result_core__ranging_total_events_sd1
*/
	int32_t   result_core__signal_total_events_sd1;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = result_core__signal_total_events_sd1
*/
	uint32_t  result_core__total_periods_elapsed_sd1;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = result_core__total_periods_elapsed_sd1
*/
	uint8_t   result_core__spare_0;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = result_core__spare_0
*/
} VL53L1_core_results_t;


/**
 * @struct VL53L1_debug_results_t
 *
 * - registers    =     43
 * - first_index  =    214 (0x00D6)
 * - last _index  =    269 (0x010D)
 * - i2c_size     =     56
 */

typedef struct {
	uint16_t  phasecal_result__reference_phase;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = result_phasecal__reference_phase (fixed point 5.11)
*/
	uint8_t   phasecal_result__vcsel_start;
/*!<
	info: \n
		- msb =  6
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [6:0] = result_phasecal__vcsel_start
*/
	uint8_t   ref_spad_char_result__num_actual_ref_spads;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [5:0] = ref_spad_char_result__num_actual_ref_spads
*/
	uint8_t   ref_spad_char_result__ref_location;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [1:0] = ref_spad_char_result__ref_location
*/
	uint8_t   vhv_result__coldboot_status;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = vhv_result__coldboot_status
*/
	uint8_t   vhv_result__search_result;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [5:0] = cp_sel_result
*/
	uint8_t   vhv_result__latest_setting;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [5:0] = cp_sel_latest_setting
*/
	uint16_t  result__osc_calibrate_val;
/*!<
	info: \n
		- msb =  9
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [9:0] = osc_calibrate_val
*/
	uint8_t   ana_config__powerdown_go1;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = go2_ref_bg_disable_avdd
		-   [1] = go2_regdvdd1v2_enable_avdd
*/
	uint8_t   ana_config__ref_bg_ctrl;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = go2_ref_overdrvbg_avdd
		-   [1] = go2_ref_forcebgison_avdd
*/
	uint8_t   ana_config__regdvdd1v2_ctrl;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = go2_regdvdd1v2_sel_pulldown_avdd
		-   [1] = go2_regdvdd1v2_sel_boost_avdd
		- [3:2] = go2_regdvdd1v2_selv_avdd
*/
	uint8_t   ana_config__osc_slow_ctrl;
/*!<
	info: \n
		- msb =  2
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = osc_slow_en
		-   [1] = osc_slow_op_en
		-   [2] = osc_slow_freq_sel
*/
	uint8_t   test_mode__status;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = test_mode_status
*/
	uint8_t   firmware__system_status;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = firmware_bootup
		-   [1] = firmware_first_range
*/
	uint8_t   firmware__mode_status;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = firmware_mode_status
*/
	uint8_t   firmware__secondary_mode_status;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = fw_secondary_mode_status
*/
	uint16_t  firmware__cal_repeat_rate_counter;
/*!<
	info: \n
		- msb = 11
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [11:0] = firmware_cal_repeat_rate
*/
	uint16_t  gph__system__thresh_high;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_thresh_high
*/
	uint16_t  gph__system__thresh_low;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_thresh_low
*/
	uint8_t   gph__system__enable_xtalk_per_quadrant;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = shadow__enable_xtalk_per_quadrant
*/
	uint8_t   gph__spare_0;
/*!<
	info: \n
		- msb =  2
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = fw_safe_to_disable
		-   [1] = shadow__spare_0
		-   [2] = shadow__spare_1
*/
	uint8_t   gph__sd_config__woi_sd0;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = shadow_sd_config__woi_sd0
*/
	uint8_t   gph__sd_config__woi_sd1;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = shadow_sd_config__woi_sd1
*/
	uint8_t   gph__sd_config__initial_phase_sd0;
/*!<
	info: \n
		- msb =  6
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [6:0] = shadow_sd_config__initial_phase_sd0
*/
	uint8_t   gph__sd_config__initial_phase_sd1;
/*!<
	info: \n
		- msb =  6
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [6:0] = shadow_sd_config__initial_phase_sd1
*/
	uint8_t   gph__sd_config__first_order_select;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = shadow_sd_config__first_order_select_rtn
		-   [1] = shadow_sd_config__first_order_select_ref
*/
	uint8_t   gph__sd_config__quantifier;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = shadow_sd_config__quantifier
*/
	uint8_t   gph__roi_config__user_roi_centre_spad;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = shadow_user_roi_center_spad_q0
*/
	uint8_t   gph__roi_config__user_roi_requested_global_xy_size;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = shadow_user_roi_requested_global_xy_size
*/
	uint8_t   gph__system__sequence_config;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = shadow_sequence_vhv_en
		-   [1] = shadow_sequence_phasecal_en
		-   [2] = shadow_sequence_reference_phase_en
		-   [3] = shadow_sequence_dss1_en
		-   [4] = shadow_sequence_dss2_en
		-   [5] = shadow_sequence_mm1_en
		-   [6] = shadow_sequence_mm2_en
		-   [7] = shadow_sequence_range_en
*/
	uint8_t   gph__gph_id;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = shadow_gph_id
*/
	uint8_t   system__interrupt_set;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = sys_interrupt_set_range
		-   [1] = sys_interrupt_set_error
*/
	uint8_t   interrupt_manager__enables;
/*!<
	info: \n
		- msb =  4
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = interrupt_enable__single_shot
		-   [1] = interrupt_enable__back_to_back
		-   [2] = interrupt_enable__timed
		-   [3] = interrupt_enable__abort
		-   [4] = interrupt_enable__test
*/
	uint8_t   interrupt_manager__clear;
/*!<
	info: \n
		- msb =  4
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = interrupt_clear__single_shot
		-   [1] = interrupt_clear__back_to_back
		-   [2] = interrupt_clear__timed
		-   [3] = interrupt_clear__abort
		-   [4] = interrupt_clear__test
*/
	uint8_t   interrupt_manager__status;
/*!<
	info: \n
		- msb =  4
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = interrupt_status__single_shot
		-   [1] = interrupt_status__back_to_back
		-   [2] = interrupt_status__timed
		-   [3] = interrupt_status__abort
		-   [4] = interrupt_status__test
*/
	uint8_t   mcu_to_host_bank__wr_access_en;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = mcu_to_host_bank_wr_en
*/
	uint8_t   power_management__go1_reset_status;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = go1_status
*/
	uint8_t   pad_startup_mode__value_ro;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = pad_atest1_val_ro
		-   [1] = pad_atest2_val_ro
*/
	uint8_t   pad_startup_mode__value_ctrl;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = pad_atest1_val
		-   [1] = pad_atest2_val
		-   [4] = pad_atest1_dig_enable
		-   [5] = pad_atest2_dig_enable
*/
	uint32_t  pll_period_us;
/*!<
	info: \n
		- msb = 17
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [17:0] = pll_period_us (fixed point 0.24)
*/
	uint32_t  interrupt_scheduler__data_out;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = interrupt_scheduler_data_out
*/
	uint8_t   nvm_bist__complete;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = nvm_bist__complete
*/
	uint8_t   nvm_bist__status;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = nvm_bist__status
*/
} VL53L1_debug_results_t;


/**
 * @struct VL53L1_nvm_copy_data_t
 *
 * - registers    =     48
 * - first_index  =    271 (0x010F)
 * - last _index  =    319 (0x013F)
 * - i2c_size     =     49
 */

typedef struct {
	uint8_t   identification__model_id;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = model_id
*/
	uint8_t   identification__module_type;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = module_type
*/
	uint8_t   identification__revision_id;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = nvm_revision_id
		- [7:4] = mask_revision_id
*/
	uint16_t  identification__module_id;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = module_id
*/
	uint8_t   ana_config__fast_osc__trim_max;
/*!<
	info: \n
		- msb =  6
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [6:0] = osc_trim_max
*/
	uint8_t   ana_config__fast_osc__freq_set;
/*!<
	info: \n
		- msb =  2
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [2:0] = osc_freq_set
*/
	uint8_t   ana_config__vcsel_trim;
/*!<
	info: \n
		- msb =  2
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [2:0] = vcsel_trim
*/
	uint8_t   ana_config__vcsel_selion;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [5:0] = vcsel_selion
*/
	uint8_t   ana_config__vcsel_selion_max;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [5:0] = vcsel_selion_max
*/
	uint8_t   protected_laser_safety__lock_bit;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = laser_safety__lock_bit
*/
	uint8_t   laser_safety__key;
/*!<
	info: \n
		- msb =  6
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [6:0] = laser_safety__key
*/
	uint8_t   laser_safety__key_ro;
/*!<
	info: \n
		- msb =  0
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = laser_safety__key_ro
*/
	uint8_t   laser_safety__clip;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [5:0] = vcsel_pulse_width_clip
*/
	uint8_t   laser_safety__mult;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [5:0] = vcsel_pulse_width_mult
*/
	uint8_t   global_config__spad_enables_rtn_0;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_0
*/
	uint8_t   global_config__spad_enables_rtn_1;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_1
*/
	uint8_t   global_config__spad_enables_rtn_2;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_2
*/
	uint8_t   global_config__spad_enables_rtn_3;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_3
*/
	uint8_t   global_config__spad_enables_rtn_4;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_4
*/
	uint8_t   global_config__spad_enables_rtn_5;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_5
*/
	uint8_t   global_config__spad_enables_rtn_6;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_6
*/
	uint8_t   global_config__spad_enables_rtn_7;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_7
*/
	uint8_t   global_config__spad_enables_rtn_8;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_8
*/
	uint8_t   global_config__spad_enables_rtn_9;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_9
*/
	uint8_t   global_config__spad_enables_rtn_10;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_10
*/
	uint8_t   global_config__spad_enables_rtn_11;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_11
*/
	uint8_t   global_config__spad_enables_rtn_12;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_12
*/
	uint8_t   global_config__spad_enables_rtn_13;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_13
*/
	uint8_t   global_config__spad_enables_rtn_14;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_14
*/
	uint8_t   global_config__spad_enables_rtn_15;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_15
*/
	uint8_t   global_config__spad_enables_rtn_16;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_16
*/
	uint8_t   global_config__spad_enables_rtn_17;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_17
*/
	uint8_t   global_config__spad_enables_rtn_18;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_18
*/
	uint8_t   global_config__spad_enables_rtn_19;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_19
*/
	uint8_t   global_config__spad_enables_rtn_20;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_20
*/
	uint8_t   global_config__spad_enables_rtn_21;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_21
*/
	uint8_t   global_config__spad_enables_rtn_22;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_22
*/
	uint8_t   global_config__spad_enables_rtn_23;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_23
*/
	uint8_t   global_config__spad_enables_rtn_24;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_24
*/
	uint8_t   global_config__spad_enables_rtn_25;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_25
*/
	uint8_t   global_config__spad_enables_rtn_26;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_26
*/
	uint8_t   global_config__spad_enables_rtn_27;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_27
*/
	uint8_t   global_config__spad_enables_rtn_28;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_28
*/
	uint8_t   global_config__spad_enables_rtn_29;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_29
*/
	uint8_t   global_config__spad_enables_rtn_30;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_30
*/
	uint8_t   global_config__spad_enables_rtn_31;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = spad_enables_rtn_31
*/
	uint8_t   roi_config__mode_roi_centre_spad;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = mode_roi_center_spad
*/
	uint8_t   roi_config__mode_roi_xy_size;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = mode_roi_xy_size
*/
} VL53L1_nvm_copy_data_t;


/**
 * @struct VL53L1_prev_shadow_system_results_t
 *
 * - registers    =     24
 * - first_index  =   3792 (0x0ED0)
 * - last _index  =   3834 (0x0EFA)
 * - i2c_size     =     44
 */

typedef struct {
	uint8_t   prev_shadow_result__interrupt_status;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [2:0] = prev_shadow_int_status
		- [4:3] = prev_shadow_int_error_status
		-   [5] = prev_shadow_gph_id_gpio_status
*/
	uint8_t   prev_shadow_result__range_status;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [4:0] = prev_shadow_range_status
		-   [5] = prev_shadow_max_threshold_hit
		-   [6] = prev_shadow_min_threshold_hit
		-   [7] = prev_shadow_gph_id_range_status
*/
	uint8_t   prev_shadow_result__report_status;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = prev_shadow_report_status
*/
	uint8_t   prev_shadow_result__stream_count;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = prev_shadow_result__stream_count
*/
	uint16_t  prev_shadow_result__dss_actual_effective_spads_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__dss_actual_effective_spads_sd0 (fixed point 8.8)
*/
	uint16_t  prev_shadow_result__peak_signal_count_rate_mcps_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__peak_signal_count_rate_mcps_sd0 (fixed point 9.7)
*/
	uint16_t  prev_shadow_result__ambient_count_rate_mcps_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__ambient_count_rate_mcps_sd0 (fixed point 9.7)
*/
	uint16_t  prev_shadow_result__sigma_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__sigma_sd0 (fixed point 14.2)
*/
	uint16_t  prev_shadow_result__phase_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__phase_sd0 (fixed point 5.11)
*/
	uint16_t  prev_shadow_result__final_crosstalk_corrected_range_mm_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__final_crosstalk_corrected_range_mm_sd0
*/
	uint16_t  prev_shadow_result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0 (fixed point 9.7)
*/
	uint16_t  prev_shadow_result__mm_inner_actual_effective_spads_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__mm_inner_actual_effective_spads_sd0 (fixed point 8.8)
*/
	uint16_t  prev_shadow_result__mm_outer_actual_effective_spads_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__mm_outer_actual_effective_spads_sd0 (fixed point 8.8)
*/
	uint16_t  prev_shadow_result__avg_signal_count_rate_mcps_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__avg_signal_count_rate_mcps_sd0 (fixed point 9.7)
*/
	uint16_t  prev_shadow_result__dss_actual_effective_spads_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__dss_actual_effective_spads_sd1 (fixed point 8.8)
*/
	uint16_t  prev_shadow_result__peak_signal_count_rate_mcps_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__peak_signal_count_rate_mcps_sd1 (fixed point 9.7)
*/
	uint16_t  prev_shadow_result__ambient_count_rate_mcps_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__ambient_count_rate_mcps_sd1 (fixed point 9.7)
*/
	uint16_t  prev_shadow_result__sigma_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__sigma_sd1 (fixed point 14.2)
*/
	uint16_t  prev_shadow_result__phase_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__phase_sd1 (fixed point 5.11)
*/
	uint16_t  prev_shadow_result__final_crosstalk_corrected_range_mm_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__final_crosstalk_corrected_range_mm_sd1
*/
	uint16_t  prev_shadow_result__spare_0_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__spare_0_sd1
*/
	uint16_t  prev_shadow_result__spare_1_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__spare_1_sd1
*/
	uint16_t  prev_shadow_result__spare_2_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__spare_2_sd1
*/
	uint16_t  prev_shadow_result__spare_3_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = prev_shadow_result__spare_3_sd1
*/
} VL53L1_prev_shadow_system_results_t;


/**
 * @struct VL53L1_prev_shadow_core_results_t
 *
 * - registers    =      9
 * - first_index  =   3836 (0x0EFC)
 * - last _index  =   3868 (0x0F1C)
 * - i2c_size     =     33
 */

typedef struct {
	uint32_t  prev_shadow_result_core__ambient_window_events_sd0;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = prev_shadow_result_core__ambient_window_events_sd0
*/
	uint32_t  prev_shadow_result_core__ranging_total_events_sd0;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = prev_shadow_result_core__ranging_total_events_sd0
*/
	int32_t   prev_shadow_result_core__signal_total_events_sd0;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = prev_shadow_result_core__signal_total_events_sd0
*/
	uint32_t  prev_shadow_result_core__total_periods_elapsed_sd0;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = prev_shadow_result_core__total_periods_elapsed_sd0
*/
	uint32_t  prev_shadow_result_core__ambient_window_events_sd1;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = prev_shadow_result_core__ambient_window_events_sd1
*/
	uint32_t  prev_shadow_result_core__ranging_total_events_sd1;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = prev_shadow_result_core__ranging_total_events_sd1
*/
	int32_t   prev_shadow_result_core__signal_total_events_sd1;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = prev_shadow_result_core__signal_total_events_sd1
*/
	uint32_t  prev_shadow_result_core__total_periods_elapsed_sd1;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = prev_shadow_result_core__total_periods_elapsed_sd1
*/
	uint8_t   prev_shadow_result_core__spare_0;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = prev_shadow_result_core__spare_0
*/
} VL53L1_prev_shadow_core_results_t;


/**
 * @struct VL53L1_patch_debug_t
 *
 * - registers    =      2
 * - first_index  =   3872 (0x0F20)
 * - last _index  =   3873 (0x0F21)
 * - i2c_size     =      2
 */

typedef struct {
	uint8_t   result__debug_status;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = result_debug_status
*/
	uint8_t   result__debug_stage;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = result_debug_stage
*/
} VL53L1_patch_debug_t;


/**
 * @struct VL53L1_gph_general_config_t
 *
 * - registers    =      3
 * - first_index  =   3876 (0x0F24)
 * - last _index  =   3880 (0x0F28)
 * - i2c_size     =      5
 */

typedef struct {
	uint16_t  gph__system__thresh_rate_high;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = gph__system_thresh_rate_high (fixed point 9.7)
*/
	uint16_t  gph__system__thresh_rate_low;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = gph__system_thresh_rate_low (fixed point 9.7)
*/
	uint8_t   gph__system__interrupt_config_gpio;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [1:0] = gph__int_mode_distance
		- [3:2] = gph__int_mode_rate
		-   [4] = gph__int_spare
		-   [5] = gph__int_new_measure_ready
		-   [6] = gph__int_no_target_en
		-   [7] = gph__int_combined_mode
*/
} VL53L1_gph_general_config_t;


/**
 * @struct VL53L1_gph_static_config_t
 *
 * - registers    =      5
 * - first_index  =   3887 (0x0F2F)
 * - last _index  =   3892 (0x0F34)
 * - i2c_size     =      6
 */

typedef struct {
	uint8_t   gph__dss_config__roi_mode_control;
/*!<
	info: \n
		- msb =  2
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [1:0] = gph__dss_config__input_mode
		-   [2] = gph__calculate_roi_enable
*/
	uint16_t  gph__dss_config__manual_effective_spads_select;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = gph__dss_config__manual_effective_spads_select
*/
	uint8_t   gph__dss_config__manual_block_select;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = gph__dss_config__manual_block_select
*/
	uint8_t   gph__dss_config__max_spads_limit;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = gph__dss_config__max_spads_limit
*/
	uint8_t   gph__dss_config__min_spads_limit;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = gph__dss_config__min_spads_limit
*/
} VL53L1_gph_static_config_t;


/**
 * @struct VL53L1_gph_timing_config_t
 *
 * - registers    =     14
 * - first_index  =   3894 (0x0F36)
 * - last _index  =   3909 (0x0F45)
 * - i2c_size     =     16
 */

typedef struct {
	uint8_t   gph__mm_config__timeout_macrop_a_hi;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = gph_mm_config__config_timeout_macrop_a_hi
*/
	uint8_t   gph__mm_config__timeout_macrop_a_lo;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = gph_mm_config__config_timeout_macrop_a_lo
*/
	uint8_t   gph__mm_config__timeout_macrop_b_hi;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = gph_mm_config__config_timeout_macrop_b_hi
*/
	uint8_t   gph__mm_config__timeout_macrop_b_lo;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = gph_mm_config__config_timeout_macrop_b_lo
*/
	uint8_t   gph__range_config__timeout_macrop_a_hi;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = gph_range_timeout_overall_periods_macrop_a_hi
*/
	uint8_t   gph__range_config__timeout_macrop_a_lo;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = gph_range_timeout_overall_periods_macrop_a_lo
*/
	uint8_t   gph__range_config__vcsel_period_a;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [5:0] = gph_range_config__vcsel_period_a
*/
	uint8_t   gph__range_config__vcsel_period_b;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [5:0] = gph_range_config__vcsel_period_b
*/
	uint8_t   gph__range_config__timeout_macrop_b_hi;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = gph_range_timeout_overall_periods_macrop_b_hi
*/
	uint8_t   gph__range_config__timeout_macrop_b_lo;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = gph_range_timeout_overall_periods_macrop_b_lo
*/
	uint16_t  gph__range_config__sigma_thresh;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = gph_range_config__sigma_thresh (fixed point 14.2)
*/
	uint16_t  gph__range_config__min_count_rate_rtn_limit_mcps;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = gph_range_config__min_count_rate_rtn_limit_mcps (fixed point 9.7)
*/
	uint8_t   gph__range_config__valid_phase_low;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = gph_range_config__valid_phase_low (fixed point 5.3)
*/
	uint8_t   gph__range_config__valid_phase_high;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = gph_range_config__valid_phase_high (fixed point 5.3)
*/
} VL53L1_gph_timing_config_t;


/**
 * @struct VL53L1_fw_internal_t
 *
 * - registers    =      2
 * - first_index  =   3910 (0x0F46)
 * - last _index  =   3911 (0x0F47)
 * - i2c_size     =      2
 */

typedef struct {
	uint8_t   firmware__internal_stream_count_div;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = fw__internal_stream_count_div
*/
	uint8_t   firmware__internal_stream_counter_val;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = fw__internal_stream_counter_val
*/
} VL53L1_fw_internal_t;


/**
 * @struct VL53L1_patch_results_t
 *
 * - registers    =     60
 * - first_index  =   3924 (0x0F54)
 * - last _index  =   4012 (0x0FAC)
 * - i2c_size     =     90
 */

typedef struct {
	uint8_t   dss_calc__roi_ctrl;
/*!<
	info: \n
		- msb =  1
		- lsb =  0
		- i2c_size =  1

	fields: \n
		-   [0] = dss_calc__roi_intersect_enable
		-   [1] = dss_calc__roi_subtract_enable
*/
	uint8_t   dss_calc__spare_1;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__spare_1
*/
	uint8_t   dss_calc__spare_2;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__spare_2
*/
	uint8_t   dss_calc__spare_3;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__spare_3
*/
	uint8_t   dss_calc__spare_4;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__spare_4
*/
	uint8_t   dss_calc__spare_5;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__spare_5
*/
	uint8_t   dss_calc__spare_6;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__spare_6
*/
	uint8_t   dss_calc__spare_7;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__spare_7
*/
	uint8_t   dss_calc__user_roi_spad_en_0;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_0
*/
	uint8_t   dss_calc__user_roi_spad_en_1;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_1
*/
	uint8_t   dss_calc__user_roi_spad_en_2;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_2
*/
	uint8_t   dss_calc__user_roi_spad_en_3;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_3
*/
	uint8_t   dss_calc__user_roi_spad_en_4;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_4
*/
	uint8_t   dss_calc__user_roi_spad_en_5;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_5
*/
	uint8_t   dss_calc__user_roi_spad_en_6;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_6
*/
	uint8_t   dss_calc__user_roi_spad_en_7;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_7
*/
	uint8_t   dss_calc__user_roi_spad_en_8;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_8
*/
	uint8_t   dss_calc__user_roi_spad_en_9;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_9
*/
	uint8_t   dss_calc__user_roi_spad_en_10;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_10
*/
	uint8_t   dss_calc__user_roi_spad_en_11;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_11
*/
	uint8_t   dss_calc__user_roi_spad_en_12;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_12
*/
	uint8_t   dss_calc__user_roi_spad_en_13;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_13
*/
	uint8_t   dss_calc__user_roi_spad_en_14;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_14
*/
	uint8_t   dss_calc__user_roi_spad_en_15;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_15
*/
	uint8_t   dss_calc__user_roi_spad_en_16;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_16
*/
	uint8_t   dss_calc__user_roi_spad_en_17;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_17
*/
	uint8_t   dss_calc__user_roi_spad_en_18;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_18
*/
	uint8_t   dss_calc__user_roi_spad_en_19;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_19
*/
	uint8_t   dss_calc__user_roi_spad_en_20;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_20
*/
	uint8_t   dss_calc__user_roi_spad_en_21;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_21
*/
	uint8_t   dss_calc__user_roi_spad_en_22;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_22
*/
	uint8_t   dss_calc__user_roi_spad_en_23;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_23
*/
	uint8_t   dss_calc__user_roi_spad_en_24;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_24
*/
	uint8_t   dss_calc__user_roi_spad_en_25;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_25
*/
	uint8_t   dss_calc__user_roi_spad_en_26;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_26
*/
	uint8_t   dss_calc__user_roi_spad_en_27;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_27
*/
	uint8_t   dss_calc__user_roi_spad_en_28;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_28
*/
	uint8_t   dss_calc__user_roi_spad_en_29;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_29
*/
	uint8_t   dss_calc__user_roi_spad_en_30;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_30
*/
	uint8_t   dss_calc__user_roi_spad_en_31;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_spad_en_31
*/
	uint8_t   dss_calc__user_roi_0;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_0
*/
	uint8_t   dss_calc__user_roi_1;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__user_roi_1
*/
	uint8_t   dss_calc__mode_roi_0;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__mode_roi_0
*/
	uint8_t   dss_calc__mode_roi_1;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_calc__mode_roi_1
*/
	uint8_t   sigma_estimator_calc__spare_0;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = sigma_estimator_calc__spare_0
*/
	uint16_t  vhv_result__peak_signal_rate_mcps;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = vhv_result__peak_signal_rate_mcps
*/
	uint32_t  vhv_result__signal_total_events_ref;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = vhv_result__signal_total_events_ref
*/
	uint16_t  phasecal_result__phase_output_ref;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = phasecal_result__normalised_phase_ref
*/
	uint16_t  dss_result__total_rate_per_spad;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = dss_result__total_rate_per_spad
*/
	uint8_t   dss_result__enabled_blocks;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = dss_result__enabled_blocks
*/
	uint16_t  dss_result__num_requested_spads;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = dss_result__num_requested_spads (fixed point 8.8)
*/
	uint16_t  mm_result__inner_intersection_rate;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = mm_result__inner_intersection_rate
*/
	uint16_t  mm_result__outer_complement_rate;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = mm_result__outer_complement_rate
*/
	uint16_t  mm_result__total_offset;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = mm_result__total_offset
*/
	uint32_t  xtalk_calc__xtalk_for_enabled_spads;
/*!<
	info: \n
		- msb = 23
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [23:0] = xtalk_calc__xtalk_for_enabled_spads (fixed point 11.13)
*/
	uint32_t  xtalk_result__avg_xtalk_user_roi_kcps;
/*!<
	info: \n
		- msb = 23
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [23:0] = xtalk_result__avg_xtalk_user_roi_kcps (fixed point 11.13)
*/
	uint32_t  xtalk_result__avg_xtalk_mm_inner_roi_kcps;
/*!<
	info: \n
		- msb = 23
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [23:0] = xtalk_result__avg_xtalk_mm_inner_roi_kcps (fixed point 11.13)
*/
	uint32_t  xtalk_result__avg_xtalk_mm_outer_roi_kcps;
/*!<
	info: \n
		- msb = 23
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [23:0] = xtalk_result__avg_xtalk_mm_outer_roi_kcps (fixed point 11.13)
*/
	uint32_t  range_result__accum_phase;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = range_result__accum_phase
*/
	uint16_t  range_result__offset_corrected_range;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = range_result__offset_corrected_range
*/
} VL53L1_patch_results_t;


/**
 * @struct VL53L1_shadow_system_results_t
 *
 * - registers    =     28
 * - first_index  =   4014 (0x0FAE)
 * - last _index  =   4095 (0x0FFF)
 * - i2c_size     =     82
 */

typedef struct {
	uint8_t   shadow_phasecal_result__vcsel_start;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = shadow_phasecal_result__vcsel_start
*/
	uint8_t   shadow_result__interrupt_status;
/*!<
	info: \n
		- msb =  5
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [2:0] = shadow_int_status
		- [4:3] = shadow_int_error_status
		-   [5] = shadow_gph_id_gpio_status
*/
	uint8_t   shadow_result__range_status;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [4:0] = shadow_range_status
		-   [5] = shadow_max_threshold_hit
		-   [6] = shadow_min_threshold_hit
		-   [7] = shadow_gph_id_range_status
*/
	uint8_t   shadow_result__report_status;
/*!<
	info: \n
		- msb =  3
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = shadow_report_status
*/
	uint8_t   shadow_result__stream_count;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = shadow_result__stream_count
*/
	uint16_t  shadow_result__dss_actual_effective_spads_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__dss_actual_effective_spads_sd0 (fixed point 8.8)
*/
	uint16_t  shadow_result__peak_signal_count_rate_mcps_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__peak_signal_count_rate_mcps_sd0 (fixed point 9.7)
*/
	uint16_t  shadow_result__ambient_count_rate_mcps_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__ambient_count_rate_mcps_sd0 (fixed point 9.7)
*/
	uint16_t  shadow_result__sigma_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__sigma_sd0 (fixed point 14.2)
*/
	uint16_t  shadow_result__phase_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__phase_sd0 (fixed point 5.11)
*/
	uint16_t  shadow_result__final_crosstalk_corrected_range_mm_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__final_crosstalk_corrected_range_mm_sd0
*/
	uint16_t  shadow_result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0 (fixed point 9.7)
*/
	uint16_t  shadow_result__mm_inner_actual_effective_spads_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__mm_inner_actual_effective_spads_sd0 (fixed point 8.8)
*/
	uint16_t  shadow_result__mm_outer_actual_effective_spads_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__mm_outer_actual_effective_spads_sd0 (fixed point 8.8)
*/
	uint16_t  shadow_result__avg_signal_count_rate_mcps_sd0;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__avg_signal_count_rate_mcps_sd0 (fixed point 9.7)
*/
	uint16_t  shadow_result__dss_actual_effective_spads_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__dss_actual_effective_spads_sd1 (fixed point 8.8)
*/
	uint16_t  shadow_result__peak_signal_count_rate_mcps_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__peak_signal_count_rate_mcps_sd1 (fixed point 9.7)
*/
	uint16_t  shadow_result__ambient_count_rate_mcps_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__ambient_count_rate_mcps_sd1 (fixed point 9.7)
*/
	uint16_t  shadow_result__sigma_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__sigma_sd1 (fixed point 14.2)
*/
	uint16_t  shadow_result__phase_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__phase_sd1 (fixed point 5.11)
*/
	uint16_t  shadow_result__final_crosstalk_corrected_range_mm_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__final_crosstalk_corrected_range_mm_sd1
*/
	uint16_t  shadow_result__spare_0_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__spare_0_sd1
*/
	uint16_t  shadow_result__spare_1_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__spare_1_sd1
*/
	uint16_t  shadow_result__spare_2_sd1;
/*!<
	info: \n
		- msb = 15
		- lsb =  0
		- i2c_size =  2

	fields: \n
		- [15:0] = shadow_result__spare_2_sd1
*/
	uint8_t   shadow_result__spare_3_sd1;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = shadow_result__spare_3_sd1
*/
	uint8_t   shadow_result__thresh_info;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [3:0] = shadow_result__distance_int_info
		- [7:4] = shadow_result__rate_int_info
*/
	uint8_t   shadow_phasecal_result__reference_phase_hi;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = shadow_phasecal_result__reference_phase_hi
*/
	uint8_t   shadow_phasecal_result__reference_phase_lo;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = shadow_phasecal_result__reference_phase_lo
*/
} VL53L1_shadow_system_results_t;


/**
 * @struct VL53L1_shadow_core_results_t
 *
 * - registers    =      9
 * - first_index  =   4060 (0x0FDC)
 * - last _index  =   4092 (0x0FFC)
 * - i2c_size     =     33
 */

typedef struct {
	uint32_t  shadow_result_core__ambient_window_events_sd0;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = shadow_result_core__ambient_window_events_sd0
*/
	uint32_t  shadow_result_core__ranging_total_events_sd0;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = shadow_result_core__ranging_total_events_sd0
*/
	int32_t   shadow_result_core__signal_total_events_sd0;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = shadow_result_core__signal_total_events_sd0
*/
	uint32_t  shadow_result_core__total_periods_elapsed_sd0;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = shadow_result_core__total_periods_elapsed_sd0
*/
	uint32_t  shadow_result_core__ambient_window_events_sd1;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = shadow_result_core__ambient_window_events_sd1
*/
	uint32_t  shadow_result_core__ranging_total_events_sd1;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = shadow_result_core__ranging_total_events_sd1
*/
	int32_t   shadow_result_core__signal_total_events_sd1;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = shadow_result_core__signal_total_events_sd1
*/
	uint32_t  shadow_result_core__total_periods_elapsed_sd1;
/*!<
	info: \n
		- msb = 31
		- lsb =  0
		- i2c_size =  4

	fields: \n
		- [31:0] = shadow_result_core__total_periods_elapsed_sd1
*/
	uint8_t   shadow_result_core__spare_0;
/*!<
	info: \n
		- msb =  7
		- lsb =  0
		- i2c_size =  1

	fields: \n
		- [7:0] = shadow_result_core__spare_0
*/
} VL53L1_shadow_core_results_t;


#endif

