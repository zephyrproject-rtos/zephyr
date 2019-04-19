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
 * @file  vl53l1_api_debug.c
 * @brief EwokPlus25 low level Driver debug function definition
 */

#include "vl53l1_ll_def.h"
#include "vl53l1_ll_device.h"
#include "vl53l1_register_structs.h"
#include "vl53l1_core.h"
#include "vl53l1_api_debug.h"

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L1_TRACE_MODULE_CORE, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L1_TRACE_MODULE_CORE, status, ##__VA_ARGS__)
#define LOG_FUNCTION_END_FMT(status, fmt, ...) \
	_LOG_FUNCTION_END_FMT(VL53L1_TRACE_MODULE_CORE, status, \
	fmt, ##__VA_ARGS__)

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(trace_flags, \
	level, VL53L1_TRACE_FUNCTION_NONE, ##__VA_ARGS__)


/* Start Patch_AdditionalDebugData_11823 */

VL53L1_Error VL53L1_get_additional_data(
	VL53L1_DEV                       Dev,
	VL53L1_additional_data_t        *pdata)
{
	/*
	 * Gets the addition debug data
	 */

	VL53L1_Error  status = VL53L1_ERROR_NONE;

	VL53L1_LLDriverData_t *pdev = VL53L1DevStructGetLLDriverHandle(Dev);

	LOG_FUNCTION_START("");

	/* get LL Driver configuration parameters */

	pdata->preset_mode             = pdev->preset_mode;
	pdata->measurement_mode        = pdev->measurement_mode;

	pdata->phasecal_config_timeout_us  = pdev->phasecal_config_timeout_us;
	pdata->mm_config_timeout_us        = pdev->mm_config_timeout_us;
	pdata->range_config_timeout_us     = pdev->range_config_timeout_us;
	pdata->inter_measurement_period_ms = pdev->inter_measurement_period_ms;
	pdata->dss_config__target_total_rate_mcps =
			pdev->dss_config__target_total_rate_mcps;

	LOG_FUNCTION_END(status);

	return status;
}

/* End Patch_AdditionalDebugData_11823 */

#ifdef VL53L1_LOG_ENABLE

void  VL53L1_signed_fixed_point_sprintf(
	int32_t    signed_fp_value,
	uint8_t    frac_bits,
	uint16_t   buf_size,
	char      *pbuffer)
{
	/*
	 * Converts input signed fixed point number into a string
	 */

	uint32_t  fp_value      = 0;
	uint32_t  unity_fp_value = 0;
	uint32_t  sign_bit       = 0;
	uint32_t  int_part       = 0;
	uint32_t  frac_part      = 0;
	uint32_t  dec_points     = 0;
	uint32_t  dec_scaler     = 0;
	uint32_t  dec_part       = 0;

	uint64_t  tmp_long_int   = 0;

	char  fmt[VL53L1_MAX_STRING_LENGTH];

	SUPPRESS_UNUSED_WARNING(buf_size);

	/* split into integer and fractional values */

	sign_bit       =  signed_fp_value >> 31;

	if (sign_bit > 0) {
		fp_value = 0x80000000 -
			(0x7FFFFFFF & (uint32_t)signed_fp_value);
	} else
		fp_value = (uint32_t)signed_fp_value;

	int_part       =  fp_value >> frac_bits;
	unity_fp_value =  0x01 << frac_bits;
	frac_part      =  fp_value & (unity_fp_value-1);

	/* Calculate decimal scale factor and required decimal points
	 * min number of displayed places is 2
	 */
	dec_points =   2;
	dec_scaler = 100;

	while (dec_scaler < unity_fp_value) {
		dec_points++;
		dec_scaler *= 10;
	}

	/* Build format string */
	if (sign_bit > 0)
		sprintf(fmt, "-%%u.%%0%uu", dec_points);
	else
		sprintf(fmt,  "%%u.%%0%uu", dec_points);

	/* Convert fractional part into a decimal
	 * need 64-bit head room at this point
	 */
	tmp_long_int  = (uint64_t)frac_part * (uint64_t)dec_scaler;
	tmp_long_int += (uint64_t)unity_fp_value/2;

	tmp_long_int = do_division_u(tmp_long_int, (uint64_t)unity_fp_value);

	dec_part = (uint32_t)tmp_long_int;

	/* Generate string for fixed point number */
	sprintf(
		pbuffer,
		fmt,
		int_part,
		dec_part);
}


void VL53L1_print_static_nvm_managed(
	VL53L1_static_nvm_managed_t   *pdata,
	char                          *pprefix,
	uint32_t                       trace_flags)
{
	/**
	 * Prints out VL53L1_static_nvm_managed_t for debug
	*/

	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = 0x%02X\n",
		pprefix,
		"i2c_slave__device_address",
		pdata->i2c_slave__device_address);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__vhv_ref_sel_vddpix",
		pdata->ana_config__vhv_ref_sel_vddpix);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__vhv_ref_sel_vquench",
		pdata->ana_config__vhv_ref_sel_vquench);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__reg_avdd1v2_sel",
		pdata->ana_config__reg_avdd1v2_sel);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__fast_osc__trim",
		pdata->ana_config__fast_osc__trim);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->osc_measured__fast_osc__frequency,
		12,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"osc_measured__fast_osc__frequency",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"vhv_config__timeout_macrop_loop_bound",
		pdata->vhv_config__timeout_macrop_loop_bound);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"vhv_config__count_thresh",
		pdata->vhv_config__count_thresh);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"vhv_config__offset",
		pdata->vhv_config__offset);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"vhv_config__init",
		pdata->vhv_config__init);
}


void VL53L1_print_customer_nvm_managed(
	VL53L1_customer_nvm_managed_t *pdata,
	char                          *pprefix,
	uint32_t                       trace_flags)
{
	/*
	 * Prints out VL53L1_customer_nvm_managed_t for debug
	 */

	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_ref_0",
		pdata->global_config__spad_enables_ref_0);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_ref_1",
		pdata->global_config__spad_enables_ref_1);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_ref_2",
		pdata->global_config__spad_enables_ref_2);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_ref_3",
		pdata->global_config__spad_enables_ref_3);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_ref_4",
		pdata->global_config__spad_enables_ref_4);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_ref_5",
		pdata->global_config__spad_enables_ref_5);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__ref_en_start_select",
		pdata->global_config__ref_en_start_select);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ref_spad_man__num_requested_ref_spads",
		pdata->ref_spad_man__num_requested_ref_spads);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ref_spad_man__ref_location",
		pdata->ref_spad_man__ref_location);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->algo__crosstalk_compensation_plane_offset_kcps,
		9,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_compensation_plane_offset_kcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->algo__crosstalk_compensation_x_plane_gradient_kcps,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_compensation_x_plane_gradient_kcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->algo__crosstalk_compensation_y_plane_gradient_kcps,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_compensation_y_plane_gradient_kcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->ref_spad_char__total_rate_target_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"ref_spad_char__total_rate_target_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->algo__part_to_part_range_offset_mm,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__part_to_part_range_offset_mm",
		fp_text);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"mm_config__inner_offset_mm",
		pdata->mm_config__inner_offset_mm);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"mm_config__outer_offset_mm",
		pdata->mm_config__outer_offset_mm);
}


void VL53L1_print_nvm_copy_data(
	VL53L1_nvm_copy_data_t      *pdata,
	char                        *pprefix,
	uint32_t                     trace_flags)
{
	/**
	 * Prints out VL53L1_nvm_copy_data_t for debug
	 */

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"identification__model_id",
		pdata->identification__model_id);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"identification__module_type",
		pdata->identification__module_type);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"identification__revision_id",
		pdata->identification__revision_id);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"identification__module_id",
		pdata->identification__module_id);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__fast_osc__trim_max",
		pdata->ana_config__fast_osc__trim_max);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__fast_osc__freq_set",
		pdata->ana_config__fast_osc__freq_set);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__vcsel_trim",
		pdata->ana_config__vcsel_trim);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__vcsel_selion",
		pdata->ana_config__vcsel_selion);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"ana_config__vcsel_selion_max",
		pdata->ana_config__vcsel_selion_max);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"protected_laser_safety__lock_bit",
		pdata->protected_laser_safety__lock_bit);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"laser_safety__key",
		pdata->laser_safety__key);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"laser_safety__key_ro",
		pdata->laser_safety__key_ro);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"laser_safety__clip",
		pdata->laser_safety__clip);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"laser_safety__mult",
		pdata->laser_safety__mult);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_0",
		pdata->global_config__spad_enables_rtn_0);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_1",
		pdata->global_config__spad_enables_rtn_1);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_2",
		pdata->global_config__spad_enables_rtn_2);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_3",
		pdata->global_config__spad_enables_rtn_3);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_4",
		pdata->global_config__spad_enables_rtn_4);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_5",
		pdata->global_config__spad_enables_rtn_5);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_6",
		pdata->global_config__spad_enables_rtn_6);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_7",
		pdata->global_config__spad_enables_rtn_7);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_8",
		pdata->global_config__spad_enables_rtn_8);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_9",
		pdata->global_config__spad_enables_rtn_9);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_10",
		pdata->global_config__spad_enables_rtn_10);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_11",
		pdata->global_config__spad_enables_rtn_11);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_12",
		pdata->global_config__spad_enables_rtn_12);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_13",
		pdata->global_config__spad_enables_rtn_13);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_14",
		pdata->global_config__spad_enables_rtn_14);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_15",
		pdata->global_config__spad_enables_rtn_15);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_16",
		pdata->global_config__spad_enables_rtn_16);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_17",
		pdata->global_config__spad_enables_rtn_17);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_18",
		pdata->global_config__spad_enables_rtn_18);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_19",
		pdata->global_config__spad_enables_rtn_19);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_20",
		pdata->global_config__spad_enables_rtn_20);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_21",
		pdata->global_config__spad_enables_rtn_21);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_22",
		pdata->global_config__spad_enables_rtn_22);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_23",
		pdata->global_config__spad_enables_rtn_23);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_24",
		pdata->global_config__spad_enables_rtn_24);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_25",
		pdata->global_config__spad_enables_rtn_25);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_26",
		pdata->global_config__spad_enables_rtn_26);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_27",
		pdata->global_config__spad_enables_rtn_27);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_28",
		pdata->global_config__spad_enables_rtn_28);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_29",
		pdata->global_config__spad_enables_rtn_29);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_30",
		pdata->global_config__spad_enables_rtn_30);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_config__spad_enables_rtn_31",
		pdata->global_config__spad_enables_rtn_31);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"roi_config__mode_roi_centre_spad",
		pdata->roi_config__mode_roi_centre_spad);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = 0x%02X\n",
		pprefix,
		"roi_config__mode_roi_xy_size",
		pdata->roi_config__mode_roi_xy_size);
}


void VL53L1_print_range_data(
	VL53L1_range_data_t *pdata,
	char                *pprefix,
	uint32_t             trace_flags)
{
	/*
	 * Prints out the range data structure for debug
	 */

	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"range_id",
		pdata->range_id);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"time_stamp",
		pdata->time_stamp);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->width,
		 4,	VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"width",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"woi",
		pdata->woi);

	/* Fast Oscillator Frequency */

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->fast_osc_frequency,
		12,	VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"fast_osc_frequency",
		fp_text);

	/* Zero Distance Phase */

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->zero_distance_phase,
		11, VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"zero_distance_phase",
		fp_text);

	/* Actual effective SPAD count */

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->actual_effective_spads,
		8, VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"actual_effective_spad",
		fp_text);


	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"total_periods_elapsed",
		pdata->total_periods_elapsed);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"peak_duration_us",
		pdata->peak_duration_us);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"woi_duration_us",
		pdata->woi_duration_us);

	trace_print(
			VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
			"ambient_window_events",
			pdata->ambient_window_events);

	trace_print(
			VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
			"ranging_total_events",
			pdata->ranging_total_events);

	trace_print(
			VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
			"signal_total_events",
			pdata->signal_total_events);

	/* Rates */

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->peak_signal_count_rate_mcps,
		7, VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"peak_signal_count_rate_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->avg_signal_count_rate_mcps,
		7, VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"avg_signal_count_rate_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->ambient_count_rate_mcps,
		7, VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"ambient_count_rate_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->total_rate_per_spad_mcps,
		13, VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"total_rate_per_spad_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->peak_rate_per_spad_kcps,
		11, VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"peak_rate_per_spad_kcps",
		fp_text);

	/* Sigma */

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->sigma_mm,
		 2,	VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"sigma_mm",
		fp_text);

	/* Phase */

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->median_phase,
		11,	VL53L1_MAX_STRING_LENGTH, fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"median_phase",
		fp_text);

	/* Offset Corrected Range */

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"median_range_mm",
		pdata->median_range_mm);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"range_status",
		pdata->range_status);
}


void VL53L1_print_range_results(
	VL53L1_range_results_t *pdata,
	char                   *pprefix,
	uint32_t                trace_flags)
{
	/*
	 * Prints out the range results data structure for debug
	 */

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"cfg_device_state",
		pdata->cfg_device_state);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"rd_device_state",
		pdata->rd_device_state);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"stream_count",
		pdata->stream_count);

	trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s%s = %u\n",
			pprefix,
			"device_status",
			pdata->device_status);

}

void VL53L1_print_offset_range_results(
	VL53L1_offset_range_results_t *pdata,
	char                          *pprefix,
	uint32_t                       trace_flags)
{
	/*
	 * Prints out the offset range results data structure for debug
	 */

	char  pre_text[VL53L1_MAX_STRING_LENGTH];
	char *ppre_text = &(pre_text[0]);

	uint8_t  i = 0;

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"cal_distance_mm",
		pdata->cal_distance_mm);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"cal_status",
		pdata->cal_status);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"cal_report",
		pdata->cal_report);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"max_results",
		pdata->max_results);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"active_results",
		pdata->active_results);

	for (i = 0 ; i < pdata->active_results ; i++) {
		sprintf(ppre_text, "%sdata[%u].", pprefix, i);
		VL53L1_print_offset_range_data(
			&(pdata->data[i]),
			ppre_text, trace_flags);
	}
}

void VL53L1_print_offset_range_data(
	VL53L1_offset_range_data_t *pdata,
	char                       *pprefix,
	uint32_t                    trace_flags)
{
	/*
	 * Prints out the xtalk range (ROI) data structure for debug
	 */

	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"preset_mode",
		pdata->preset_mode);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"dss_config__roi_mode_control",
		pdata->dss_config__roi_mode_control);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->dss_config__manual_effective_spads_select,
		8,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"dss_config__manual_effective_spads_select",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"no_of_samples",
		pdata->no_of_samples);


	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->effective_spads,
		8,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"effective_spads",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->peak_rate_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"peak_rate_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->sigma_mm,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"sigma_mm",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"median_range_mm",
		pdata->median_range_mm);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %d\n",
		pprefix,
		"range_mm_offset",
		pdata->range_mm_offset);
}

void VL53L1_print_additional_offset_cal_data(
	VL53L1_additional_offset_cal_data_t *pdata,
	char                                *pprefix,
	uint32_t                             trace_flags)
{
	/*
	 * Prints out the xtalk range (ROI) data structure for debug
	 */

	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->result__mm_inner_actual_effective_spads,
		8,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"result__mm_inner_actual_effective_spads",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->result__mm_outer_actual_effective_spads,
		8,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"result__mm_outer_actual_effective_spads",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->result__mm_inner_peak_signal_count_rtn_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"result__mm_inner_peak_signal_count_rtn_mcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->result__mm_outer_peak_signal_count_rtn_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"result__mm_outer_peak_signal_count_rtn_mcps",
		fp_text);
}


void VL53L1_print_cal_peak_rate_map(
	VL53L1_cal_peak_rate_map_t *pdata,
	char                       *pprefix,
	uint32_t                    trace_flags)
{
	/*
	 * Prints out peak rate map structure for debug
	 */

	char  fp_text[VL53L1_MAX_STRING_LENGTH];
	char  pre_text[VL53L1_MAX_STRING_LENGTH];
	char *ppre_text = &(pre_text[0]);

	uint8_t   i = 0;
	uint8_t   x = 0;
	uint8_t   y = 0;

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->cal_distance_mm,
		2,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"cal_distance_mm",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"max_samples",
		pdata->max_samples);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"width",
		pdata->width);

	trace_print(
	VL53L1_TRACE_LEVEL_INFO,
	"%s%s = %u\n",
	pprefix,
	"height",
	pdata->height);

	i = 0;
	for (y = 0 ; y < pdata->height ; y++) {
		for (x = 0 ; x < pdata->width ; x++) {

			sprintf(ppre_text, "%speak_rate_mcps[%u]", pprefix, i);

			VL53L1_signed_fixed_point_sprintf(
				(int32_t)pdata->peak_rate_mcps[i],
				7,
				VL53L1_MAX_STRING_LENGTH,
				fp_text);

			trace_print(
				VL53L1_TRACE_LEVEL_INFO,
				"%s = %s\n",
				ppre_text,
				fp_text);

			i++;
		}
	}
}

void VL53L1_print_additional_data(
	VL53L1_additional_data_t *pdata,
	char                     *pprefix,
	uint32_t                 trace_flags)
{

	/*
	 * Prints out the Additional data structure for debug
	 */

	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"preset_mode",
		pdata->preset_mode);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"measurement_mode",
		pdata->measurement_mode);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"phasecal_config_timeout_us",
		pdata->phasecal_config_timeout_us);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"mm_config_timeout_us",
		pdata->mm_config_timeout_us);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"range_config_timeout_us",
		pdata->range_config_timeout_us);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"inter_measurement_period_ms",
		pdata->inter_measurement_period_ms);


	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->dss_config__target_total_rate_mcps,
		7,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"dss_config__target_total_rate_mcps",
		fp_text);

}


void VL53L1_print_gain_calibration_data(
	VL53L1_gain_calibration_data_t *pdata,
	char                           *pprefix,
	uint32_t                        trace_flags)
{
	/*
	 * Prints out the LL Driver state data for debug
	 */

	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->standard_ranging_gain_factor,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"standard_ranging_gain_factor",
		fp_text);

}


void VL53L1_print_xtalk_config(
	VL53L1_xtalk_config_t *pdata,
	char                  *pprefix,
	uint32_t               trace_flags)
{
	/*
	 * Prints out the xtalk config data structure for debug
	 */

	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->algo__crosstalk_compensation_plane_offset_kcps,
		9,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_compensation_plane_offset_kcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->algo__crosstalk_compensation_x_plane_gradient_kcps,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_compensation_x_plane_gradient_kcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->algo__crosstalk_compensation_y_plane_gradient_kcps,
		11,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"algo__crosstalk_compensation_y_plane_gradient_kcps",
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"global_crosstalk_compensation_enable",
		pdata->global_crosstalk_compensation_enable);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->lite_mode_crosstalk_margin_kcps,
		9,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"lite_mode_crosstalk_margin_kcps",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->crosstalk_range_ignore_threshold_mult,
		5,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"crosstalk_range_ignore_threshold_mult",
		fp_text);


	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->crosstalk_range_ignore_threshold_rate_mcps,
		13,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"crosstalk_range_ignore_threshold_rate_mcps",
		fp_text);

}



void VL53L1_print_optical_centre(
	VL53L1_optical_centre_t  *pdata,
	char                     *pprefix,
	uint32_t                  trace_flags)
{

	/* Prints out the optical centre data structure for debug
	 */

	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->x_centre,
		4,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"x_centre",
		fp_text);

	VL53L1_signed_fixed_point_sprintf(
		(int32_t)pdata->y_centre,
		4,
		VL53L1_MAX_STRING_LENGTH,
		fp_text);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %s\n",
		pprefix,
		"y_centre",
		fp_text);
}


void VL53L1_print_user_zone(
	VL53L1_user_zone_t   *pdata,
	char                 *pprefix,
	uint32_t              trace_flags)
{

	/* Prints out the zone (ROI) data structure for debug
	 */

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"x_centre",
		pdata->x_centre);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"y_centre",
		pdata->y_centre);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"width",
		pdata->width);

	trace_print(VL53L1_TRACE_LEVEL_INFO,
		"%s%s = %u\n",
		pprefix,
		"height",
		pdata->height);
}


void VL53L1_print_spad_rate_data(
	VL53L1_spad_rate_data_t  *pspad_rates,
	char                     *pprefix,
	uint32_t                  trace_flags)
{

    /**
     *  Print per SPAD rates generated by SSC
     */

	uint16_t spad_no = 0;
	uint8_t  row     = 0;
	uint8_t  col     = 0;

	char  fp_text[VL53L1_MAX_STRING_LENGTH];

    trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%8s,%4s,%4s, %s\n",
		pprefix,
		"spad_no",
		"row",
		"col",
		"peak_rate_mcps");

    for (spad_no = 0 ; spad_no < pspad_rates->no_of_values ; spad_no++) {

		/* generate row / col location from SPAD number  */
		VL53L1_decode_row_col(
			(uint8_t)spad_no,
			&row,
			&col);

		/* Convert fixed point rate value to string */

		VL53L1_signed_fixed_point_sprintf(
			(int32_t)pspad_rates->rate_data[spad_no],
			pspad_rates->fractional_bits,
			VL53L1_MAX_STRING_LENGTH,
			fp_text);

		/* Print data */

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s%8u,%4u,%4u, %s\n",
			pprefix,
			spad_no,
			row,
			col,
			fp_text);
    }
}


void VL53L1_print_spad_rate_map(
	VL53L1_spad_rate_data_t  *pspad_rates,
	char                     *pprefix,
	uint32_t                  trace_flags)
{

    /**
     *  Print per SPAD rates generated by SSC as a map
     */

	uint8_t  spad_no = 0;
	uint8_t  row     = 0;
	uint8_t  col     = 0;

	char  fp_text[VL53L1_MAX_STRING_LENGTH];

	/* Print column headers  */
	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"%s%4s",
		pprefix,
		" ");

    for (col = 0 ;  col < VL53L1_SPAD_ARRAY_WIDTH ; col++)
		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			",%8u",
			col);

	trace_print(
		VL53L1_TRACE_LEVEL_INFO,
		"\n");

    /* Print rate data  */

    for (row = 0 ;  row < VL53L1_SPAD_ARRAY_HEIGHT ; row++) {

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"%s%4u",
			pprefix,
			row);

		for (col = 0 ;  col < VL53L1_SPAD_ARRAY_HEIGHT ; col++) {

			/* generate SPAD number from (row, col) location */

			VL53L1_encode_row_col(
				row,
				col,
				&spad_no);

			/* Convert fixed point rate value to string */

			VL53L1_signed_fixed_point_sprintf(
				(int32_t)pspad_rates->rate_data[spad_no],
				pspad_rates->fractional_bits,
				VL53L1_MAX_STRING_LENGTH,
				fp_text);

			/* Print data */

			trace_print(
				VL53L1_TRACE_LEVEL_INFO,
				",%8s",
				fp_text);
		}

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"\n");
    }
}


#endif /* VL53L1_LOG_ENABLE */

