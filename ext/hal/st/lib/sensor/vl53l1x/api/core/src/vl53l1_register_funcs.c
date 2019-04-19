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
 * @file   vl53l1_register_funcs.c
 * @brief  VL53L1 Register Function definitions
 */

#include "vl53l1_ll_def.h"
#include "vl53l1_platform.h"
#include "vl53l1_platform_log.h"
#include "vl53l1_core.h"
#include "vl53l1_register_map.h"
#include "vl53l1_register_structs.h"
#include "vl53l1_register_funcs.h"

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L1_TRACE_MODULE_REGISTERS, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L1_TRACE_MODULE_REGISTERS, status, ##__VA_ARGS__)
#define LOG_FUNCTION_END_FMT(status, fmt, ...) \
	_LOG_FUNCTION_END_FMT(VL53L1_TRACE_MODULE_REGISTERS, status, fmt, ##__VA_ARGS__)


VL53L1_Error VL53L1_i2c_encode_static_nvm_managed(
	VL53L1_static_nvm_managed_t *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_static_nvm_managed_t into a I2C write buffer
	 * Buffer must be at least 11 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdata->i2c_slave__device_address & 0x7F;
	*(pbuffer +   1) =
		pdata->ana_config__vhv_ref_sel_vddpix & 0xF;
	*(pbuffer +   2) =
		pdata->ana_config__vhv_ref_sel_vquench & 0x7F;
	*(pbuffer +   3) =
		pdata->ana_config__reg_avdd1v2_sel & 0x3;
	*(pbuffer +   4) =
		pdata->ana_config__fast_osc__trim & 0x7F;
	VL53L1_i2c_encode_uint16_t(
		pdata->osc_measured__fast_osc__frequency,
		2,
		pbuffer +   5);
	*(pbuffer +   7) =
		pdata->vhv_config__timeout_macrop_loop_bound;
	*(pbuffer +   8) =
		pdata->vhv_config__count_thresh;
	*(pbuffer +   9) =
		pdata->vhv_config__offset & 0x3F;
	*(pbuffer +  10) =
		pdata->vhv_config__init;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_static_nvm_managed(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_static_nvm_managed_t  *pdata)
{
	/**
	 * Decodes data structure VL53L1_static_nvm_managed_t from the input I2C read buffer
	 * Buffer must be at least 11 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->i2c_slave__device_address =
		(*(pbuffer +   0)) & 0x7F;
	pdata->ana_config__vhv_ref_sel_vddpix =
		(*(pbuffer +   1)) & 0xF;
	pdata->ana_config__vhv_ref_sel_vquench =
		(*(pbuffer +   2)) & 0x7F;
	pdata->ana_config__reg_avdd1v2_sel =
		(*(pbuffer +   3)) & 0x3;
	pdata->ana_config__fast_osc__trim =
		(*(pbuffer +   4)) & 0x7F;
	pdata->osc_measured__fast_osc__frequency =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   5));
	pdata->vhv_config__timeout_macrop_loop_bound =
		(*(pbuffer +   7));
	pdata->vhv_config__count_thresh =
		(*(pbuffer +   8));
	pdata->vhv_config__offset =
		(*(pbuffer +   9)) & 0x3F;
	pdata->vhv_config__init =
		(*(pbuffer +  10));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_static_nvm_managed(
	VL53L1_DEV                 Dev,
	VL53L1_static_nvm_managed_t  *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_static_nvm_managed_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_static_nvm_managed(
			pdata,
			VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_I2C_SLAVE__DEVICE_ADDRESS,
			comms_buffer,
			VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_static_nvm_managed(
	VL53L1_DEV                 Dev,
	VL53L1_static_nvm_managed_t  *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_static_nvm_managed_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_I2C_SLAVE__DEVICE_ADDRESS,
			comms_buffer,
			VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_static_nvm_managed(
			VL53L1_STATIC_NVM_MANAGED_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_customer_nvm_managed(
	VL53L1_customer_nvm_managed_t *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_customer_nvm_managed_t into a I2C write buffer
	 * Buffer must be at least 23 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdata->global_config__spad_enables_ref_0;
	*(pbuffer +   1) =
		pdata->global_config__spad_enables_ref_1;
	*(pbuffer +   2) =
		pdata->global_config__spad_enables_ref_2;
	*(pbuffer +   3) =
		pdata->global_config__spad_enables_ref_3;
	*(pbuffer +   4) =
		pdata->global_config__spad_enables_ref_4;
	*(pbuffer +   5) =
		pdata->global_config__spad_enables_ref_5 & 0xF;
	*(pbuffer +   6) =
		pdata->global_config__ref_en_start_select;
	*(pbuffer +   7) =
		pdata->ref_spad_man__num_requested_ref_spads & 0x3F;
	*(pbuffer +   8) =
		pdata->ref_spad_man__ref_location & 0x3;
	VL53L1_i2c_encode_uint16_t(
		pdata->algo__crosstalk_compensation_plane_offset_kcps,
		2,
		pbuffer +   9);
	VL53L1_i2c_encode_int16_t(
		pdata->algo__crosstalk_compensation_x_plane_gradient_kcps,
		2,
		pbuffer +  11);
	VL53L1_i2c_encode_int16_t(
		pdata->algo__crosstalk_compensation_y_plane_gradient_kcps,
		2,
		pbuffer +  13);
	VL53L1_i2c_encode_uint16_t(
		pdata->ref_spad_char__total_rate_target_mcps,
		2,
		pbuffer +  15);
	VL53L1_i2c_encode_int16_t(
		pdata->algo__part_to_part_range_offset_mm & 0x1FFF,
		2,
		pbuffer +  17);
	VL53L1_i2c_encode_int16_t(
		pdata->mm_config__inner_offset_mm,
		2,
		pbuffer +  19);
	VL53L1_i2c_encode_int16_t(
		pdata->mm_config__outer_offset_mm,
		2,
		pbuffer +  21);
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_customer_nvm_managed(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_customer_nvm_managed_t  *pdata)
{
	/**
	 * Decodes data structure VL53L1_customer_nvm_managed_t from the input I2C read buffer
	 * Buffer must be at least 23 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->global_config__spad_enables_ref_0 =
		(*(pbuffer +   0));
	pdata->global_config__spad_enables_ref_1 =
		(*(pbuffer +   1));
	pdata->global_config__spad_enables_ref_2 =
		(*(pbuffer +   2));
	pdata->global_config__spad_enables_ref_3 =
		(*(pbuffer +   3));
	pdata->global_config__spad_enables_ref_4 =
		(*(pbuffer +   4));
	pdata->global_config__spad_enables_ref_5 =
		(*(pbuffer +   5)) & 0xF;
	pdata->global_config__ref_en_start_select =
		(*(pbuffer +   6));
	pdata->ref_spad_man__num_requested_ref_spads =
		(*(pbuffer +   7)) & 0x3F;
	pdata->ref_spad_man__ref_location =
		(*(pbuffer +   8)) & 0x3;
	pdata->algo__crosstalk_compensation_plane_offset_kcps =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   9));
	pdata->algo__crosstalk_compensation_x_plane_gradient_kcps =
		(VL53L1_i2c_decode_int16_t(2, pbuffer +  11));
	pdata->algo__crosstalk_compensation_y_plane_gradient_kcps =
		(VL53L1_i2c_decode_int16_t(2, pbuffer +  13));
	pdata->ref_spad_char__total_rate_target_mcps =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  15));
	pdata->algo__part_to_part_range_offset_mm =
		(VL53L1_i2c_decode_int16_t(2, pbuffer +  17)) & 0x1FFF;
	pdata->mm_config__inner_offset_mm =
		(VL53L1_i2c_decode_int16_t(2, pbuffer +  19));
	pdata->mm_config__outer_offset_mm =
		(VL53L1_i2c_decode_int16_t(2, pbuffer +  21));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_customer_nvm_managed(
	VL53L1_DEV                 Dev,
	VL53L1_customer_nvm_managed_t  *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_customer_nvm_managed_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_customer_nvm_managed(
			pdata,
			VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_GLOBAL_CONFIG__SPAD_ENABLES_REF_0,
			comms_buffer,
			VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_customer_nvm_managed(
	VL53L1_DEV                 Dev,
	VL53L1_customer_nvm_managed_t  *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_customer_nvm_managed_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_GLOBAL_CONFIG__SPAD_ENABLES_REF_0,
			comms_buffer,
			VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_customer_nvm_managed(
			VL53L1_CUSTOMER_NVM_MANAGED_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_static_config(
	VL53L1_static_config_t   *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_static_config_t into a I2C write buffer
	 * Buffer must be at least 32 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	VL53L1_i2c_encode_uint16_t(
		pdata->dss_config__target_total_rate_mcps,
		2,
		pbuffer +   0);
	*(pbuffer +   2) =
		pdata->debug__ctrl & 0x1;
	*(pbuffer +   3) =
		pdata->test_mode__ctrl & 0xF;
	*(pbuffer +   4) =
		pdata->clk_gating__ctrl & 0xF;
	*(pbuffer +   5) =
		pdata->nvm_bist__ctrl & 0x1F;
	*(pbuffer +   6) =
		pdata->nvm_bist__num_nvm_words & 0x7F;
	*(pbuffer +   7) =
		pdata->nvm_bist__start_address & 0x7F;
	*(pbuffer +   8) =
		pdata->host_if__status & 0x1;
	*(pbuffer +   9) =
		pdata->pad_i2c_hv__config;
	*(pbuffer +  10) =
		pdata->pad_i2c_hv__extsup_config & 0x1;
	*(pbuffer +  11) =
		pdata->gpio_hv_pad__ctrl & 0x3;
	*(pbuffer +  12) =
		pdata->gpio_hv_mux__ctrl & 0x1F;
	*(pbuffer +  13) =
		pdata->gpio__tio_hv_status & 0x3;
	*(pbuffer +  14) =
		pdata->gpio__fio_hv_status & 0x3;
	*(pbuffer +  15) =
		pdata->ana_config__spad_sel_pswidth & 0x7;
	*(pbuffer +  16) =
		pdata->ana_config__vcsel_pulse_width_offset & 0x1F;
	*(pbuffer +  17) =
		pdata->ana_config__fast_osc__config_ctrl & 0x1;
	*(pbuffer +  18) =
		pdata->sigma_estimator__effective_pulse_width_ns;
	*(pbuffer +  19) =
		pdata->sigma_estimator__effective_ambient_width_ns;
	*(pbuffer +  20) =
		pdata->sigma_estimator__sigma_ref_mm;
	*(pbuffer +  21) =
		pdata->algo__crosstalk_compensation_valid_height_mm;
	*(pbuffer +  22) =
		pdata->spare_host_config__static_config_spare_0;
	*(pbuffer +  23) =
		pdata->spare_host_config__static_config_spare_1;
	VL53L1_i2c_encode_uint16_t(
		pdata->algo__range_ignore_threshold_mcps,
		2,
		pbuffer +  24);
	*(pbuffer +  26) =
		pdata->algo__range_ignore_valid_height_mm;
	*(pbuffer +  27) =
		pdata->algo__range_min_clip;
	*(pbuffer +  28) =
		pdata->algo__consistency_check__tolerance & 0xF;
	*(pbuffer +  29) =
		pdata->spare_host_config__static_config_spare_2;
	*(pbuffer +  30) =
		pdata->sd_config__reset_stages_msb & 0xF;
	*(pbuffer +  31) =
		pdata->sd_config__reset_stages_lsb;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_static_config(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_static_config_t    *pdata)
{
	/**
	 * Decodes data structure VL53L1_static_config_t from the input I2C read buffer
	 * Buffer must be at least 32 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->dss_config__target_total_rate_mcps =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   0));
	pdata->debug__ctrl =
		(*(pbuffer +   2)) & 0x1;
	pdata->test_mode__ctrl =
		(*(pbuffer +   3)) & 0xF;
	pdata->clk_gating__ctrl =
		(*(pbuffer +   4)) & 0xF;
	pdata->nvm_bist__ctrl =
		(*(pbuffer +   5)) & 0x1F;
	pdata->nvm_bist__num_nvm_words =
		(*(pbuffer +   6)) & 0x7F;
	pdata->nvm_bist__start_address =
		(*(pbuffer +   7)) & 0x7F;
	pdata->host_if__status =
		(*(pbuffer +   8)) & 0x1;
	pdata->pad_i2c_hv__config =
		(*(pbuffer +   9));
	pdata->pad_i2c_hv__extsup_config =
		(*(pbuffer +  10)) & 0x1;
	pdata->gpio_hv_pad__ctrl =
		(*(pbuffer +  11)) & 0x3;
	pdata->gpio_hv_mux__ctrl =
		(*(pbuffer +  12)) & 0x1F;
	pdata->gpio__tio_hv_status =
		(*(pbuffer +  13)) & 0x3;
	pdata->gpio__fio_hv_status =
		(*(pbuffer +  14)) & 0x3;
	pdata->ana_config__spad_sel_pswidth =
		(*(pbuffer +  15)) & 0x7;
	pdata->ana_config__vcsel_pulse_width_offset =
		(*(pbuffer +  16)) & 0x1F;
	pdata->ana_config__fast_osc__config_ctrl =
		(*(pbuffer +  17)) & 0x1;
	pdata->sigma_estimator__effective_pulse_width_ns =
		(*(pbuffer +  18));
	pdata->sigma_estimator__effective_ambient_width_ns =
		(*(pbuffer +  19));
	pdata->sigma_estimator__sigma_ref_mm =
		(*(pbuffer +  20));
	pdata->algo__crosstalk_compensation_valid_height_mm =
		(*(pbuffer +  21));
	pdata->spare_host_config__static_config_spare_0 =
		(*(pbuffer +  22));
	pdata->spare_host_config__static_config_spare_1 =
		(*(pbuffer +  23));
	pdata->algo__range_ignore_threshold_mcps =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  24));
	pdata->algo__range_ignore_valid_height_mm =
		(*(pbuffer +  26));
	pdata->algo__range_min_clip =
		(*(pbuffer +  27));
	pdata->algo__consistency_check__tolerance =
		(*(pbuffer +  28)) & 0xF;
	pdata->spare_host_config__static_config_spare_2 =
		(*(pbuffer +  29));
	pdata->sd_config__reset_stages_msb =
		(*(pbuffer +  30)) & 0xF;
	pdata->sd_config__reset_stages_lsb =
		(*(pbuffer +  31));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_static_config(
	VL53L1_DEV                 Dev,
	VL53L1_static_config_t    *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_static_config_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_static_config(
			pdata,
			VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_DSS_CONFIG__TARGET_TOTAL_RATE_MCPS,
			comms_buffer,
			VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES);

	LOG_FUNCTION_END(status);

	return status;
}


#ifdef VL53L1_DEBUG
VL53L1_Error VL53L1_get_static_config(
	VL53L1_DEV                 Dev,
	VL53L1_static_config_t    *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_static_config_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_DSS_CONFIG__TARGET_TOTAL_RATE_MCPS,
			comms_buffer,
			VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_static_config(
			VL53L1_STATIC_CONFIG_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


VL53L1_Error VL53L1_i2c_encode_general_config(
	VL53L1_general_config_t  *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_general_config_t into a I2C write buffer
	 * Buffer must be at least 22 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdata->gph_config__stream_count_update_value;
	*(pbuffer +   1) =
		pdata->global_config__stream_divider;
	*(pbuffer +   2) =
		pdata->system__interrupt_config_gpio;
	*(pbuffer +   3) =
		pdata->cal_config__vcsel_start & 0x7F;
	VL53L1_i2c_encode_uint16_t(
		pdata->cal_config__repeat_rate & 0xFFF,
		2,
		pbuffer +   4);
	*(pbuffer +   6) =
		pdata->global_config__vcsel_width & 0x7F;
	*(pbuffer +   7) =
		pdata->phasecal_config__timeout_macrop;
	*(pbuffer +   8) =
		pdata->phasecal_config__target;
	*(pbuffer +   9) =
		pdata->phasecal_config__override & 0x1;
	*(pbuffer +  11) =
		pdata->dss_config__roi_mode_control & 0x7;
	VL53L1_i2c_encode_uint16_t(
		pdata->system__thresh_rate_high,
		2,
		pbuffer +  12);
	VL53L1_i2c_encode_uint16_t(
		pdata->system__thresh_rate_low,
		2,
		pbuffer +  14);
	VL53L1_i2c_encode_uint16_t(
		pdata->dss_config__manual_effective_spads_select,
		2,
		pbuffer +  16);
	*(pbuffer +  18) =
		pdata->dss_config__manual_block_select;
	*(pbuffer +  19) =
		pdata->dss_config__aperture_attenuation;
	*(pbuffer +  20) =
		pdata->dss_config__max_spads_limit;
	*(pbuffer +  21) =
		pdata->dss_config__min_spads_limit;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_general_config(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_general_config_t   *pdata)
{
	/**
	 * Decodes data structure VL53L1_general_config_t from the input I2C read buffer
	 * Buffer must be at least 22 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->gph_config__stream_count_update_value =
		(*(pbuffer +   0));
	pdata->global_config__stream_divider =
		(*(pbuffer +   1));
	pdata->system__interrupt_config_gpio =
		(*(pbuffer +   2));
	pdata->cal_config__vcsel_start =
		(*(pbuffer +   3)) & 0x7F;
	pdata->cal_config__repeat_rate =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   4)) & 0xFFF;
	pdata->global_config__vcsel_width =
		(*(pbuffer +   6)) & 0x7F;
	pdata->phasecal_config__timeout_macrop =
		(*(pbuffer +   7));
	pdata->phasecal_config__target =
		(*(pbuffer +   8));
	pdata->phasecal_config__override =
		(*(pbuffer +   9)) & 0x1;
	pdata->dss_config__roi_mode_control =
		(*(pbuffer +  11)) & 0x7;
	pdata->system__thresh_rate_high =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  12));
	pdata->system__thresh_rate_low =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  14));
	pdata->dss_config__manual_effective_spads_select =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  16));
	pdata->dss_config__manual_block_select =
		(*(pbuffer +  18));
	pdata->dss_config__aperture_attenuation =
		(*(pbuffer +  19));
	pdata->dss_config__max_spads_limit =
		(*(pbuffer +  20));
	pdata->dss_config__min_spads_limit =
		(*(pbuffer +  21));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_general_config(
	VL53L1_DEV                 Dev,
	VL53L1_general_config_t   *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_general_config_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_general_config(
			pdata,
			VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_GPH_CONFIG__STREAM_COUNT_UPDATE_VALUE,
			comms_buffer,
			VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES);

	LOG_FUNCTION_END(status);

	return status;
}


#ifdef VL53L1_DEBUG
VL53L1_Error VL53L1_get_general_config(
	VL53L1_DEV                 Dev,
	VL53L1_general_config_t   *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_general_config_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_GPH_CONFIG__STREAM_COUNT_UPDATE_VALUE,
			comms_buffer,
			VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_general_config(
			VL53L1_GENERAL_CONFIG_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


VL53L1_Error VL53L1_i2c_encode_timing_config(
	VL53L1_timing_config_t   *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_timing_config_t into a I2C write buffer
	 * Buffer must be at least 23 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdata->mm_config__timeout_macrop_a_hi & 0xF;
	*(pbuffer +   1) =
		pdata->mm_config__timeout_macrop_a_lo;
	*(pbuffer +   2) =
		pdata->mm_config__timeout_macrop_b_hi & 0xF;
	*(pbuffer +   3) =
		pdata->mm_config__timeout_macrop_b_lo;
	*(pbuffer +   4) =
		pdata->range_config__timeout_macrop_a_hi & 0xF;
	*(pbuffer +   5) =
		pdata->range_config__timeout_macrop_a_lo;
	*(pbuffer +   6) =
		pdata->range_config__vcsel_period_a & 0x3F;
	*(pbuffer +   7) =
		pdata->range_config__timeout_macrop_b_hi & 0xF;
	*(pbuffer +   8) =
		pdata->range_config__timeout_macrop_b_lo;
	*(pbuffer +   9) =
		pdata->range_config__vcsel_period_b & 0x3F;
	VL53L1_i2c_encode_uint16_t(
		pdata->range_config__sigma_thresh,
		2,
		pbuffer +  10);
	VL53L1_i2c_encode_uint16_t(
		pdata->range_config__min_count_rate_rtn_limit_mcps,
		2,
		pbuffer +  12);
	*(pbuffer +  14) =
		pdata->range_config__valid_phase_low;
	*(pbuffer +  15) =
		pdata->range_config__valid_phase_high;
	VL53L1_i2c_encode_uint32_t(
		pdata->system__intermeasurement_period,
		4,
		pbuffer +  18);
	*(pbuffer +  22) =
		pdata->system__fractional_enable & 0x1;
	LOG_FUNCTION_END(status);


	return status;
}


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_i2c_decode_timing_config(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_timing_config_t    *pdata)
{
	/**
	 * Decodes data structure VL53L1_timing_config_t from the input I2C read buffer
	 * Buffer must be at least 23 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->mm_config__timeout_macrop_a_hi =
		(*(pbuffer +   0)) & 0xF;
	pdata->mm_config__timeout_macrop_a_lo =
		(*(pbuffer +   1));
	pdata->mm_config__timeout_macrop_b_hi =
		(*(pbuffer +   2)) & 0xF;
	pdata->mm_config__timeout_macrop_b_lo =
		(*(pbuffer +   3));
	pdata->range_config__timeout_macrop_a_hi =
		(*(pbuffer +   4)) & 0xF;
	pdata->range_config__timeout_macrop_a_lo =
		(*(pbuffer +   5));
	pdata->range_config__vcsel_period_a =
		(*(pbuffer +   6)) & 0x3F;
	pdata->range_config__timeout_macrop_b_hi =
		(*(pbuffer +   7)) & 0xF;
	pdata->range_config__timeout_macrop_b_lo =
		(*(pbuffer +   8));
	pdata->range_config__vcsel_period_b =
		(*(pbuffer +   9)) & 0x3F;
	pdata->range_config__sigma_thresh =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  10));
	pdata->range_config__min_count_rate_rtn_limit_mcps =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  12));
	pdata->range_config__valid_phase_low =
		(*(pbuffer +  14));
	pdata->range_config__valid_phase_high =
		(*(pbuffer +  15));
	pdata->system__intermeasurement_period =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  18));
	pdata->system__fractional_enable =
		(*(pbuffer +  22)) & 0x1;

	LOG_FUNCTION_END(status);

	return status;
}
#endif


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_set_timing_config(
	VL53L1_DEV                 Dev,
	VL53L1_timing_config_t    *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_timing_config_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_timing_config(
			pdata,
			VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_MM_CONFIG__TIMEOUT_MACROP_A_HI,
			comms_buffer,
			VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


#ifdef VL53L1_DEBUG
VL53L1_Error VL53L1_get_timing_config(
	VL53L1_DEV                 Dev,
	VL53L1_timing_config_t    *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_timing_config_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_MM_CONFIG__TIMEOUT_MACROP_A_HI,
			comms_buffer,
			VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_timing_config(
			VL53L1_TIMING_CONFIG_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


VL53L1_Error VL53L1_i2c_encode_dynamic_config(
	VL53L1_dynamic_config_t  *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_dynamic_config_t into a I2C write buffer
	 * Buffer must be at least 18 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdata->system__grouped_parameter_hold_0 & 0x3;
	VL53L1_i2c_encode_uint16_t(
		pdata->system__thresh_high,
		2,
		pbuffer +   1);
	VL53L1_i2c_encode_uint16_t(
		pdata->system__thresh_low,
		2,
		pbuffer +   3);
	*(pbuffer +   5) =
		pdata->system__enable_xtalk_per_quadrant & 0x1;
	*(pbuffer +   6) =
		pdata->system__seed_config & 0x7;
	*(pbuffer +   7) =
		pdata->sd_config__woi_sd0;
	*(pbuffer +   8) =
		pdata->sd_config__woi_sd1;
	*(pbuffer +   9) =
		pdata->sd_config__initial_phase_sd0 & 0x7F;
	*(pbuffer +  10) =
		pdata->sd_config__initial_phase_sd1 & 0x7F;
	*(pbuffer +  11) =
		pdata->system__grouped_parameter_hold_1 & 0x3;
	*(pbuffer +  12) =
		pdata->sd_config__first_order_select & 0x3;
	*(pbuffer +  13) =
		pdata->sd_config__quantifier & 0xF;
	*(pbuffer +  14) =
		pdata->roi_config__user_roi_centre_spad;
	*(pbuffer +  15) =
		pdata->roi_config__user_roi_requested_global_xy_size;
	*(pbuffer +  16) =
		pdata->system__sequence_config;
	*(pbuffer +  17) =
		pdata->system__grouped_parameter_hold & 0x3;
	LOG_FUNCTION_END(status);


	return status;
}

#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_i2c_decode_dynamic_config(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_dynamic_config_t   *pdata)
{
	/**
	 * Decodes data structure VL53L1_dynamic_config_t from the input I2C read buffer
	 * Buffer must be at least 18 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->system__grouped_parameter_hold_0 =
		(*(pbuffer +   0)) & 0x3;
	pdata->system__thresh_high =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   1));
	pdata->system__thresh_low =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   3));
	pdata->system__enable_xtalk_per_quadrant =
		(*(pbuffer +   5)) & 0x1;
	pdata->system__seed_config =
		(*(pbuffer +   6)) & 0x7;
	pdata->sd_config__woi_sd0 =
		(*(pbuffer +   7));
	pdata->sd_config__woi_sd1 =
		(*(pbuffer +   8));
	pdata->sd_config__initial_phase_sd0 =
		(*(pbuffer +   9)) & 0x7F;
	pdata->sd_config__initial_phase_sd1 =
		(*(pbuffer +  10)) & 0x7F;
	pdata->system__grouped_parameter_hold_1 =
		(*(pbuffer +  11)) & 0x3;
	pdata->sd_config__first_order_select =
		(*(pbuffer +  12)) & 0x3;
	pdata->sd_config__quantifier =
		(*(pbuffer +  13)) & 0xF;
	pdata->roi_config__user_roi_centre_spad =
		(*(pbuffer +  14));
	pdata->roi_config__user_roi_requested_global_xy_size =
		(*(pbuffer +  15));
	pdata->system__sequence_config =
		(*(pbuffer +  16));
	pdata->system__grouped_parameter_hold =
		(*(pbuffer +  17)) & 0x3;

	LOG_FUNCTION_END(status);

	return status;
}
#endif


VL53L1_Error VL53L1_set_dynamic_config(
	VL53L1_DEV                 Dev,
	VL53L1_dynamic_config_t   *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_dynamic_config_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_dynamic_config(
			pdata,
			VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_SYSTEM__GROUPED_PARAMETER_HOLD_0,
			comms_buffer,
			VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES);

	LOG_FUNCTION_END(status);

	return status;
}


#ifdef VL53L1_DEBUG
VL53L1_Error VL53L1_get_dynamic_config(
	VL53L1_DEV                 Dev,
	VL53L1_dynamic_config_t   *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_dynamic_config_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_SYSTEM__GROUPED_PARAMETER_HOLD_0,
			comms_buffer,
			VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_dynamic_config(
			VL53L1_DYNAMIC_CONFIG_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


VL53L1_Error VL53L1_i2c_encode_system_control(
	VL53L1_system_control_t  *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_system_control_t into a I2C write buffer
	 * Buffer must be at least 5 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdata->power_management__go1_power_force & 0x1;
	*(pbuffer +   1) =
		pdata->system__stream_count_ctrl & 0x1;
	*(pbuffer +   2) =
		pdata->firmware__enable & 0x1;
	*(pbuffer +   3) =
		pdata->system__interrupt_clear & 0x3;
	*(pbuffer +   4) =
		pdata->system__mode_start;
	LOG_FUNCTION_END(status);


	return status;
}


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_i2c_decode_system_control(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_system_control_t   *pdata)
{
	/**
	 * Decodes data structure VL53L1_system_control_t from the input I2C read buffer
	 * Buffer must be at least 5 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->power_management__go1_power_force =
		(*(pbuffer +   0)) & 0x1;
	pdata->system__stream_count_ctrl =
		(*(pbuffer +   1)) & 0x1;
	pdata->firmware__enable =
		(*(pbuffer +   2)) & 0x1;
	pdata->system__interrupt_clear =
		(*(pbuffer +   3)) & 0x3;
	pdata->system__mode_start =
		(*(pbuffer +   4));

	LOG_FUNCTION_END(status);

	return status;
}
#endif


VL53L1_Error VL53L1_set_system_control(
	VL53L1_DEV                 Dev,
	VL53L1_system_control_t   *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_system_control_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_system_control(
			pdata,
			VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_POWER_MANAGEMENT__GO1_POWER_FORCE,
			comms_buffer,
			VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES);

	LOG_FUNCTION_END(status);

	return status;
}


#ifdef VL53L1_DEBUG
VL53L1_Error VL53L1_get_system_control(
	VL53L1_DEV                 Dev,
	VL53L1_system_control_t   *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_system_control_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_POWER_MANAGEMENT__GO1_POWER_FORCE,
			comms_buffer,
			VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_system_control(
			VL53L1_SYSTEM_CONTROL_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_i2c_encode_system_results(
	VL53L1_system_results_t  *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_system_results_t into a I2C write buffer
	 * Buffer must be at least 44 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdata->result__interrupt_status & 0x3F;
	*(pbuffer +   1) =
		pdata->result__range_status;
	*(pbuffer +   2) =
		pdata->result__report_status & 0xF;
	*(pbuffer +   3) =
		pdata->result__stream_count;
	VL53L1_i2c_encode_uint16_t(
		pdata->result__dss_actual_effective_spads_sd0,
		2,
		pbuffer +   4);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__peak_signal_count_rate_mcps_sd0,
		2,
		pbuffer +   6);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__ambient_count_rate_mcps_sd0,
		2,
		pbuffer +   8);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__sigma_sd0,
		2,
		pbuffer +  10);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__phase_sd0,
		2,
		pbuffer +  12);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__final_crosstalk_corrected_range_mm_sd0,
		2,
		pbuffer +  14);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0,
		2,
		pbuffer +  16);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__mm_inner_actual_effective_spads_sd0,
		2,
		pbuffer +  18);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__mm_outer_actual_effective_spads_sd0,
		2,
		pbuffer +  20);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__avg_signal_count_rate_mcps_sd0,
		2,
		pbuffer +  22);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__dss_actual_effective_spads_sd1,
		2,
		pbuffer +  24);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__peak_signal_count_rate_mcps_sd1,
		2,
		pbuffer +  26);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__ambient_count_rate_mcps_sd1,
		2,
		pbuffer +  28);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__sigma_sd1,
		2,
		pbuffer +  30);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__phase_sd1,
		2,
		pbuffer +  32);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__final_crosstalk_corrected_range_mm_sd1,
		2,
		pbuffer +  34);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__spare_0_sd1,
		2,
		pbuffer +  36);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__spare_1_sd1,
		2,
		pbuffer +  38);
	VL53L1_i2c_encode_uint16_t(
		pdata->result__spare_2_sd1,
		2,
		pbuffer +  40);
	*(pbuffer +  42) =
		pdata->result__spare_3_sd1;
	*(pbuffer +  43) =
		pdata->result__thresh_info;
	LOG_FUNCTION_END(status);


	return status;
}
#endif


VL53L1_Error VL53L1_i2c_decode_system_results(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_system_results_t   *pdata)
{
	/**
	 * Decodes data structure VL53L1_system_results_t from the input I2C read buffer
	 * Buffer must be at least 44 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->result__interrupt_status =
		(*(pbuffer +   0)) & 0x3F;
	pdata->result__range_status =
		(*(pbuffer +   1));
	pdata->result__report_status =
		(*(pbuffer +   2)) & 0xF;
	pdata->result__stream_count =
		(*(pbuffer +   3));
	pdata->result__dss_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   4));
	pdata->result__peak_signal_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   6));
	pdata->result__ambient_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   8));
	pdata->result__sigma_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  10));
	pdata->result__phase_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  12));
	pdata->result__final_crosstalk_corrected_range_mm_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  14));
	pdata->result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  16));
	pdata->result__mm_inner_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  18));
	pdata->result__mm_outer_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  20));
	pdata->result__avg_signal_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  22));
	pdata->result__dss_actual_effective_spads_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  24));
	pdata->result__peak_signal_count_rate_mcps_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  26));
	pdata->result__ambient_count_rate_mcps_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  28));
	pdata->result__sigma_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  30));
	pdata->result__phase_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  32));
	pdata->result__final_crosstalk_corrected_range_mm_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  34));
	pdata->result__spare_0_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  36));
	pdata->result__spare_1_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  38));
	pdata->result__spare_2_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  40));
	pdata->result__spare_3_sd1 =
		(*(pbuffer +  42));
	pdata->result__thresh_info =
		(*(pbuffer +  43));

	LOG_FUNCTION_END(status);

	return status;
}


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_set_system_results(
	VL53L1_DEV                 Dev,
	VL53L1_system_results_t   *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_system_results_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_system_results(
			pdata,
			VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_RESULT__INTERRUPT_STATUS,
			comms_buffer,
			VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


VL53L1_Error VL53L1_get_system_results(
	VL53L1_DEV                 Dev,
	VL53L1_system_results_t   *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_system_results_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_RESULT__INTERRUPT_STATUS,
			comms_buffer,
			VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_system_results(
			VL53L1_SYSTEM_RESULTS_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_i2c_encode_core_results(
	VL53L1_core_results_t    *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_core_results_t into a I2C write buffer
	 * Buffer must be at least 33 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_CORE_RESULTS_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	VL53L1_i2c_encode_uint32_t(
		pdata->result_core__ambient_window_events_sd0,
		4,
		pbuffer +   0);
	VL53L1_i2c_encode_uint32_t(
		pdata->result_core__ranging_total_events_sd0,
		4,
		pbuffer +   4);
	VL53L1_i2c_encode_int32_t(
		pdata->result_core__signal_total_events_sd0,
		4,
		pbuffer +   8);
	VL53L1_i2c_encode_uint32_t(
		pdata->result_core__total_periods_elapsed_sd0,
		4,
		pbuffer +  12);
	VL53L1_i2c_encode_uint32_t(
		pdata->result_core__ambient_window_events_sd1,
		4,
		pbuffer +  16);
	VL53L1_i2c_encode_uint32_t(
		pdata->result_core__ranging_total_events_sd1,
		4,
		pbuffer +  20);
	VL53L1_i2c_encode_int32_t(
		pdata->result_core__signal_total_events_sd1,
		4,
		pbuffer +  24);
	VL53L1_i2c_encode_uint32_t(
		pdata->result_core__total_periods_elapsed_sd1,
		4,
		pbuffer +  28);
	*(pbuffer +  32) =
		pdata->result_core__spare_0;
	LOG_FUNCTION_END(status);


	return status;
}
#endif


VL53L1_Error VL53L1_i2c_decode_core_results(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_core_results_t     *pdata)
{
	/**
	 * Decodes data structure VL53L1_core_results_t from the input I2C read buffer
	 * Buffer must be at least 33 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_CORE_RESULTS_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->result_core__ambient_window_events_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +   0));
	pdata->result_core__ranging_total_events_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +   4));
	pdata->result_core__signal_total_events_sd0 =
		(VL53L1_i2c_decode_int32_t(4, pbuffer +   8));
	pdata->result_core__total_periods_elapsed_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  12));
	pdata->result_core__ambient_window_events_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  16));
	pdata->result_core__ranging_total_events_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  20));
	pdata->result_core__signal_total_events_sd1 =
		(VL53L1_i2c_decode_int32_t(4, pbuffer +  24));
	pdata->result_core__total_periods_elapsed_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  28));
	pdata->result_core__spare_0 =
		(*(pbuffer +  32));

	LOG_FUNCTION_END(status);

	return status;
}


#ifdef VL53L1_DEBUG
VL53L1_Error VL53L1_set_core_results(
	VL53L1_DEV                 Dev,
	VL53L1_core_results_t     *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_core_results_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_CORE_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_core_results(
			pdata,
			VL53L1_CORE_RESULTS_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_RESULT_CORE__AMBIENT_WINDOW_EVENTS_SD0,
			comms_buffer,
			VL53L1_CORE_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_get_core_results(
	VL53L1_DEV                 Dev,
	VL53L1_core_results_t     *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_core_results_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_CORE_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_RESULT_CORE__AMBIENT_WINDOW_EVENTS_SD0,
			comms_buffer,
			VL53L1_CORE_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_core_results(
			VL53L1_CORE_RESULTS_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


#ifdef VL53L1_DEBUG
VL53L1_Error VL53L1_i2c_encode_debug_results(
	VL53L1_debug_results_t   *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_debug_results_t into a I2C write buffer
	 * Buffer must be at least 56 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	VL53L1_i2c_encode_uint16_t(
		pdata->phasecal_result__reference_phase,
		2,
		pbuffer +   0);
	*(pbuffer +   2) =
		pdata->phasecal_result__vcsel_start & 0x7F;
	*(pbuffer +   3) =
		pdata->ref_spad_char_result__num_actual_ref_spads & 0x3F;
	*(pbuffer +   4) =
		pdata->ref_spad_char_result__ref_location & 0x3;
	*(pbuffer +   5) =
		pdata->vhv_result__coldboot_status & 0x1;
	*(pbuffer +   6) =
		pdata->vhv_result__search_result & 0x3F;
	*(pbuffer +   7) =
		pdata->vhv_result__latest_setting & 0x3F;
	VL53L1_i2c_encode_uint16_t(
		pdata->result__osc_calibrate_val & 0x3FF,
		2,
		pbuffer +   8);
	*(pbuffer +  10) =
		pdata->ana_config__powerdown_go1 & 0x3;
	*(pbuffer +  11) =
		pdata->ana_config__ref_bg_ctrl & 0x3;
	*(pbuffer +  12) =
		pdata->ana_config__regdvdd1v2_ctrl & 0xF;
	*(pbuffer +  13) =
		pdata->ana_config__osc_slow_ctrl & 0x7;
	*(pbuffer +  14) =
		pdata->test_mode__status & 0x1;
	*(pbuffer +  15) =
		pdata->firmware__system_status & 0x3;
	*(pbuffer +  16) =
		pdata->firmware__mode_status;
	*(pbuffer +  17) =
		pdata->firmware__secondary_mode_status;
	VL53L1_i2c_encode_uint16_t(
		pdata->firmware__cal_repeat_rate_counter & 0xFFF,
		2,
		pbuffer +  18);
	VL53L1_i2c_encode_uint16_t(
		pdata->gph__system__thresh_high,
		2,
		pbuffer +  22);
	VL53L1_i2c_encode_uint16_t(
		pdata->gph__system__thresh_low,
		2,
		pbuffer +  24);
	*(pbuffer +  26) =
		pdata->gph__system__enable_xtalk_per_quadrant & 0x1;
	*(pbuffer +  27) =
		pdata->gph__spare_0 & 0x7;
	*(pbuffer +  28) =
		pdata->gph__sd_config__woi_sd0;
	*(pbuffer +  29) =
		pdata->gph__sd_config__woi_sd1;
	*(pbuffer +  30) =
		pdata->gph__sd_config__initial_phase_sd0 & 0x7F;
	*(pbuffer +  31) =
		pdata->gph__sd_config__initial_phase_sd1 & 0x7F;
	*(pbuffer +  32) =
		pdata->gph__sd_config__first_order_select & 0x3;
	*(pbuffer +  33) =
		pdata->gph__sd_config__quantifier & 0xF;
	*(pbuffer +  34) =
		pdata->gph__roi_config__user_roi_centre_spad;
	*(pbuffer +  35) =
		pdata->gph__roi_config__user_roi_requested_global_xy_size;
	*(pbuffer +  36) =
		pdata->gph__system__sequence_config;
	*(pbuffer +  37) =
		pdata->gph__gph_id & 0x1;
	*(pbuffer +  38) =
		pdata->system__interrupt_set & 0x3;
	*(pbuffer +  39) =
		pdata->interrupt_manager__enables & 0x1F;
	*(pbuffer +  40) =
		pdata->interrupt_manager__clear & 0x1F;
	*(pbuffer +  41) =
		pdata->interrupt_manager__status & 0x1F;
	*(pbuffer +  42) =
		pdata->mcu_to_host_bank__wr_access_en & 0x1;
	*(pbuffer +  43) =
		pdata->power_management__go1_reset_status & 0x1;
	*(pbuffer +  44) =
		pdata->pad_startup_mode__value_ro & 0x3;
	*(pbuffer +  45) =
		pdata->pad_startup_mode__value_ctrl & 0x3F;
	VL53L1_i2c_encode_uint32_t(
		pdata->pll_period_us & 0x3FFFF,
		4,
		pbuffer +  46);
	VL53L1_i2c_encode_uint32_t(
		pdata->interrupt_scheduler__data_out,
		4,
		pbuffer +  50);
	*(pbuffer +  54) =
		pdata->nvm_bist__complete & 0x1;
	*(pbuffer +  55) =
		pdata->nvm_bist__status & 0x1;
	LOG_FUNCTION_END(status);


	return status;
}
#endif


VL53L1_Error VL53L1_i2c_decode_debug_results(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_debug_results_t    *pdata)
{
	/**
	 * Decodes data structure VL53L1_debug_results_t from the input I2C read buffer
	 * Buffer must be at least 56 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->phasecal_result__reference_phase =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   0));
	pdata->phasecal_result__vcsel_start =
		(*(pbuffer +   2)) & 0x7F;
	pdata->ref_spad_char_result__num_actual_ref_spads =
		(*(pbuffer +   3)) & 0x3F;
	pdata->ref_spad_char_result__ref_location =
		(*(pbuffer +   4)) & 0x3;
	pdata->vhv_result__coldboot_status =
		(*(pbuffer +   5)) & 0x1;
	pdata->vhv_result__search_result =
		(*(pbuffer +   6)) & 0x3F;
	pdata->vhv_result__latest_setting =
		(*(pbuffer +   7)) & 0x3F;
	pdata->result__osc_calibrate_val =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   8)) & 0x3FF;
	pdata->ana_config__powerdown_go1 =
		(*(pbuffer +  10)) & 0x3;
	pdata->ana_config__ref_bg_ctrl =
		(*(pbuffer +  11)) & 0x3;
	pdata->ana_config__regdvdd1v2_ctrl =
		(*(pbuffer +  12)) & 0xF;
	pdata->ana_config__osc_slow_ctrl =
		(*(pbuffer +  13)) & 0x7;
	pdata->test_mode__status =
		(*(pbuffer +  14)) & 0x1;
	pdata->firmware__system_status =
		(*(pbuffer +  15)) & 0x3;
	pdata->firmware__mode_status =
		(*(pbuffer +  16));
	pdata->firmware__secondary_mode_status =
		(*(pbuffer +  17));
	pdata->firmware__cal_repeat_rate_counter =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  18)) & 0xFFF;
	pdata->gph__system__thresh_high =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  22));
	pdata->gph__system__thresh_low =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  24));
	pdata->gph__system__enable_xtalk_per_quadrant =
		(*(pbuffer +  26)) & 0x1;
	pdata->gph__spare_0 =
		(*(pbuffer +  27)) & 0x7;
	pdata->gph__sd_config__woi_sd0 =
		(*(pbuffer +  28));
	pdata->gph__sd_config__woi_sd1 =
		(*(pbuffer +  29));
	pdata->gph__sd_config__initial_phase_sd0 =
		(*(pbuffer +  30)) & 0x7F;
	pdata->gph__sd_config__initial_phase_sd1 =
		(*(pbuffer +  31)) & 0x7F;
	pdata->gph__sd_config__first_order_select =
		(*(pbuffer +  32)) & 0x3;
	pdata->gph__sd_config__quantifier =
		(*(pbuffer +  33)) & 0xF;
	pdata->gph__roi_config__user_roi_centre_spad =
		(*(pbuffer +  34));
	pdata->gph__roi_config__user_roi_requested_global_xy_size =
		(*(pbuffer +  35));
	pdata->gph__system__sequence_config =
		(*(pbuffer +  36));
	pdata->gph__gph_id =
		(*(pbuffer +  37)) & 0x1;
	pdata->system__interrupt_set =
		(*(pbuffer +  38)) & 0x3;
	pdata->interrupt_manager__enables =
		(*(pbuffer +  39)) & 0x1F;
	pdata->interrupt_manager__clear =
		(*(pbuffer +  40)) & 0x1F;
	pdata->interrupt_manager__status =
		(*(pbuffer +  41)) & 0x1F;
	pdata->mcu_to_host_bank__wr_access_en =
		(*(pbuffer +  42)) & 0x1;
	pdata->power_management__go1_reset_status =
		(*(pbuffer +  43)) & 0x1;
	pdata->pad_startup_mode__value_ro =
		(*(pbuffer +  44)) & 0x3;
	pdata->pad_startup_mode__value_ctrl =
		(*(pbuffer +  45)) & 0x3F;
	pdata->pll_period_us =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  46)) & 0x3FFFF;
	pdata->interrupt_scheduler__data_out =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  50));
	pdata->nvm_bist__complete =
		(*(pbuffer +  54)) & 0x1;
	pdata->nvm_bist__status =
		(*(pbuffer +  55)) & 0x1;

	LOG_FUNCTION_END(status);

	return status;
}


#ifdef VL53L1_DEBUG
VL53L1_Error VL53L1_set_debug_results(
	VL53L1_DEV                 Dev,
	VL53L1_debug_results_t    *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_debug_results_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_debug_results(
			pdata,
			VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_PHASECAL_RESULT__REFERENCE_PHASE,
			comms_buffer,
			VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_debug_results(
	VL53L1_DEV                 Dev,
	VL53L1_debug_results_t    *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_debug_results_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_PHASECAL_RESULT__REFERENCE_PHASE,
			comms_buffer,
			VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_debug_results(
			VL53L1_DEBUG_RESULTS_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_i2c_encode_nvm_copy_data(
	VL53L1_nvm_copy_data_t   *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_nvm_copy_data_t into a I2C write buffer
	 * Buffer must be at least 49 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdata->identification__model_id;
	*(pbuffer +   1) =
		pdata->identification__module_type;
	*(pbuffer +   2) =
		pdata->identification__revision_id;
	VL53L1_i2c_encode_uint16_t(
		pdata->identification__module_id,
		2,
		pbuffer +   3);
	*(pbuffer +   5) =
		pdata->ana_config__fast_osc__trim_max & 0x7F;
	*(pbuffer +   6) =
		pdata->ana_config__fast_osc__freq_set & 0x7;
	*(pbuffer +   7) =
		pdata->ana_config__vcsel_trim & 0x7;
	*(pbuffer +   8) =
		pdata->ana_config__vcsel_selion & 0x3F;
	*(pbuffer +   9) =
		pdata->ana_config__vcsel_selion_max & 0x3F;
	*(pbuffer +  10) =
		pdata->protected_laser_safety__lock_bit & 0x1;
	*(pbuffer +  11) =
		pdata->laser_safety__key & 0x7F;
	*(pbuffer +  12) =
		pdata->laser_safety__key_ro & 0x1;
	*(pbuffer +  13) =
		pdata->laser_safety__clip & 0x3F;
	*(pbuffer +  14) =
		pdata->laser_safety__mult & 0x3F;
	*(pbuffer +  15) =
		pdata->global_config__spad_enables_rtn_0;
	*(pbuffer +  16) =
		pdata->global_config__spad_enables_rtn_1;
	*(pbuffer +  17) =
		pdata->global_config__spad_enables_rtn_2;
	*(pbuffer +  18) =
		pdata->global_config__spad_enables_rtn_3;
	*(pbuffer +  19) =
		pdata->global_config__spad_enables_rtn_4;
	*(pbuffer +  20) =
		pdata->global_config__spad_enables_rtn_5;
	*(pbuffer +  21) =
		pdata->global_config__spad_enables_rtn_6;
	*(pbuffer +  22) =
		pdata->global_config__spad_enables_rtn_7;
	*(pbuffer +  23) =
		pdata->global_config__spad_enables_rtn_8;
	*(pbuffer +  24) =
		pdata->global_config__spad_enables_rtn_9;
	*(pbuffer +  25) =
		pdata->global_config__spad_enables_rtn_10;
	*(pbuffer +  26) =
		pdata->global_config__spad_enables_rtn_11;
	*(pbuffer +  27) =
		pdata->global_config__spad_enables_rtn_12;
	*(pbuffer +  28) =
		pdata->global_config__spad_enables_rtn_13;
	*(pbuffer +  29) =
		pdata->global_config__spad_enables_rtn_14;
	*(pbuffer +  30) =
		pdata->global_config__spad_enables_rtn_15;
	*(pbuffer +  31) =
		pdata->global_config__spad_enables_rtn_16;
	*(pbuffer +  32) =
		pdata->global_config__spad_enables_rtn_17;
	*(pbuffer +  33) =
		pdata->global_config__spad_enables_rtn_18;
	*(pbuffer +  34) =
		pdata->global_config__spad_enables_rtn_19;
	*(pbuffer +  35) =
		pdata->global_config__spad_enables_rtn_20;
	*(pbuffer +  36) =
		pdata->global_config__spad_enables_rtn_21;
	*(pbuffer +  37) =
		pdata->global_config__spad_enables_rtn_22;
	*(pbuffer +  38) =
		pdata->global_config__spad_enables_rtn_23;
	*(pbuffer +  39) =
		pdata->global_config__spad_enables_rtn_24;
	*(pbuffer +  40) =
		pdata->global_config__spad_enables_rtn_25;
	*(pbuffer +  41) =
		pdata->global_config__spad_enables_rtn_26;
	*(pbuffer +  42) =
		pdata->global_config__spad_enables_rtn_27;
	*(pbuffer +  43) =
		pdata->global_config__spad_enables_rtn_28;
	*(pbuffer +  44) =
		pdata->global_config__spad_enables_rtn_29;
	*(pbuffer +  45) =
		pdata->global_config__spad_enables_rtn_30;
	*(pbuffer +  46) =
		pdata->global_config__spad_enables_rtn_31;
	*(pbuffer +  47) =
		pdata->roi_config__mode_roi_centre_spad;
	*(pbuffer +  48) =
		pdata->roi_config__mode_roi_xy_size;
	LOG_FUNCTION_END(status);


	return status;
}
#endif


VL53L1_Error VL53L1_i2c_decode_nvm_copy_data(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_nvm_copy_data_t    *pdata)
{
	/**
	 * Decodes data structure VL53L1_nvm_copy_data_t from the input I2C read buffer
	 * Buffer must be at least 49 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->identification__model_id =
		(*(pbuffer +   0));
	pdata->identification__module_type =
		(*(pbuffer +   1));
	pdata->identification__revision_id =
		(*(pbuffer +   2));
	pdata->identification__module_id =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   3));
	pdata->ana_config__fast_osc__trim_max =
		(*(pbuffer +   5)) & 0x7F;
	pdata->ana_config__fast_osc__freq_set =
		(*(pbuffer +   6)) & 0x7;
	pdata->ana_config__vcsel_trim =
		(*(pbuffer +   7)) & 0x7;
	pdata->ana_config__vcsel_selion =
		(*(pbuffer +   8)) & 0x3F;
	pdata->ana_config__vcsel_selion_max =
		(*(pbuffer +   9)) & 0x3F;
	pdata->protected_laser_safety__lock_bit =
		(*(pbuffer +  10)) & 0x1;
	pdata->laser_safety__key =
		(*(pbuffer +  11)) & 0x7F;
	pdata->laser_safety__key_ro =
		(*(pbuffer +  12)) & 0x1;
	pdata->laser_safety__clip =
		(*(pbuffer +  13)) & 0x3F;
	pdata->laser_safety__mult =
		(*(pbuffer +  14)) & 0x3F;
	pdata->global_config__spad_enables_rtn_0 =
		(*(pbuffer +  15));
	pdata->global_config__spad_enables_rtn_1 =
		(*(pbuffer +  16));
	pdata->global_config__spad_enables_rtn_2 =
		(*(pbuffer +  17));
	pdata->global_config__spad_enables_rtn_3 =
		(*(pbuffer +  18));
	pdata->global_config__spad_enables_rtn_4 =
		(*(pbuffer +  19));
	pdata->global_config__spad_enables_rtn_5 =
		(*(pbuffer +  20));
	pdata->global_config__spad_enables_rtn_6 =
		(*(pbuffer +  21));
	pdata->global_config__spad_enables_rtn_7 =
		(*(pbuffer +  22));
	pdata->global_config__spad_enables_rtn_8 =
		(*(pbuffer +  23));
	pdata->global_config__spad_enables_rtn_9 =
		(*(pbuffer +  24));
	pdata->global_config__spad_enables_rtn_10 =
		(*(pbuffer +  25));
	pdata->global_config__spad_enables_rtn_11 =
		(*(pbuffer +  26));
	pdata->global_config__spad_enables_rtn_12 =
		(*(pbuffer +  27));
	pdata->global_config__spad_enables_rtn_13 =
		(*(pbuffer +  28));
	pdata->global_config__spad_enables_rtn_14 =
		(*(pbuffer +  29));
	pdata->global_config__spad_enables_rtn_15 =
		(*(pbuffer +  30));
	pdata->global_config__spad_enables_rtn_16 =
		(*(pbuffer +  31));
	pdata->global_config__spad_enables_rtn_17 =
		(*(pbuffer +  32));
	pdata->global_config__spad_enables_rtn_18 =
		(*(pbuffer +  33));
	pdata->global_config__spad_enables_rtn_19 =
		(*(pbuffer +  34));
	pdata->global_config__spad_enables_rtn_20 =
		(*(pbuffer +  35));
	pdata->global_config__spad_enables_rtn_21 =
		(*(pbuffer +  36));
	pdata->global_config__spad_enables_rtn_22 =
		(*(pbuffer +  37));
	pdata->global_config__spad_enables_rtn_23 =
		(*(pbuffer +  38));
	pdata->global_config__spad_enables_rtn_24 =
		(*(pbuffer +  39));
	pdata->global_config__spad_enables_rtn_25 =
		(*(pbuffer +  40));
	pdata->global_config__spad_enables_rtn_26 =
		(*(pbuffer +  41));
	pdata->global_config__spad_enables_rtn_27 =
		(*(pbuffer +  42));
	pdata->global_config__spad_enables_rtn_28 =
		(*(pbuffer +  43));
	pdata->global_config__spad_enables_rtn_29 =
		(*(pbuffer +  44));
	pdata->global_config__spad_enables_rtn_30 =
		(*(pbuffer +  45));
	pdata->global_config__spad_enables_rtn_31 =
		(*(pbuffer +  46));
	pdata->roi_config__mode_roi_centre_spad =
		(*(pbuffer +  47));
	pdata->roi_config__mode_roi_xy_size =
		(*(pbuffer +  48));

	LOG_FUNCTION_END(status);

	return status;
}

#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_set_nvm_copy_data(
	VL53L1_DEV                 Dev,
	VL53L1_nvm_copy_data_t    *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_nvm_copy_data_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_nvm_copy_data(
			pdata,
			VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_IDENTIFICATION__MODEL_ID,
			comms_buffer,
			VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


VL53L1_Error VL53L1_get_nvm_copy_data(
	VL53L1_DEV                 Dev,
	VL53L1_nvm_copy_data_t    *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_nvm_copy_data_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_IDENTIFICATION__MODEL_ID,
			comms_buffer,
			VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_nvm_copy_data(
			VL53L1_NVM_COPY_DATA_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}


#ifdef VL53L1_DEBUG
VL53L1_Error VL53L1_i2c_encode_prev_shadow_system_results(
	VL53L1_prev_shadow_system_results_t *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_prev_shadow_system_results_t into a I2C write buffer
	 * Buffer must be at least 44 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdata->prev_shadow_result__interrupt_status & 0x3F;
	*(pbuffer +   1) =
		pdata->prev_shadow_result__range_status;
	*(pbuffer +   2) =
		pdata->prev_shadow_result__report_status & 0xF;
	*(pbuffer +   3) =
		pdata->prev_shadow_result__stream_count;
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__dss_actual_effective_spads_sd0,
		2,
		pbuffer +   4);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__peak_signal_count_rate_mcps_sd0,
		2,
		pbuffer +   6);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__ambient_count_rate_mcps_sd0,
		2,
		pbuffer +   8);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__sigma_sd0,
		2,
		pbuffer +  10);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__phase_sd0,
		2,
		pbuffer +  12);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__final_crosstalk_corrected_range_mm_sd0,
		2,
		pbuffer +  14);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0,
		2,
		pbuffer +  16);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__mm_inner_actual_effective_spads_sd0,
		2,
		pbuffer +  18);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__mm_outer_actual_effective_spads_sd0,
		2,
		pbuffer +  20);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__avg_signal_count_rate_mcps_sd0,
		2,
		pbuffer +  22);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__dss_actual_effective_spads_sd1,
		2,
		pbuffer +  24);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__peak_signal_count_rate_mcps_sd1,
		2,
		pbuffer +  26);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__ambient_count_rate_mcps_sd1,
		2,
		pbuffer +  28);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__sigma_sd1,
		2,
		pbuffer +  30);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__phase_sd1,
		2,
		pbuffer +  32);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__final_crosstalk_corrected_range_mm_sd1,
		2,
		pbuffer +  34);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__spare_0_sd1,
		2,
		pbuffer +  36);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__spare_1_sd1,
		2,
		pbuffer +  38);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__spare_2_sd1,
		2,
		pbuffer +  40);
	VL53L1_i2c_encode_uint16_t(
		pdata->prev_shadow_result__spare_3_sd1,
		2,
		pbuffer +  42);
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_prev_shadow_system_results(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_prev_shadow_system_results_t  *pdata)
{
	/**
	 * Decodes data structure VL53L1_prev_shadow_system_results_t from the input I2C read buffer
	 * Buffer must be at least 44 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->prev_shadow_result__interrupt_status =
		(*(pbuffer +   0)) & 0x3F;
	pdata->prev_shadow_result__range_status =
		(*(pbuffer +   1));
	pdata->prev_shadow_result__report_status =
		(*(pbuffer +   2)) & 0xF;
	pdata->prev_shadow_result__stream_count =
		(*(pbuffer +   3));
	pdata->prev_shadow_result__dss_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   4));
	pdata->prev_shadow_result__peak_signal_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   6));
	pdata->prev_shadow_result__ambient_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   8));
	pdata->prev_shadow_result__sigma_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  10));
	pdata->prev_shadow_result__phase_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  12));
	pdata->prev_shadow_result__final_crosstalk_corrected_range_mm_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  14));
	pdata->prev_shadow_result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  16));
	pdata->prev_shadow_result__mm_inner_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  18));
	pdata->prev_shadow_result__mm_outer_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  20));
	pdata->prev_shadow_result__avg_signal_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  22));
	pdata->prev_shadow_result__dss_actual_effective_spads_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  24));
	pdata->prev_shadow_result__peak_signal_count_rate_mcps_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  26));
	pdata->prev_shadow_result__ambient_count_rate_mcps_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  28));
	pdata->prev_shadow_result__sigma_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  30));
	pdata->prev_shadow_result__phase_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  32));
	pdata->prev_shadow_result__final_crosstalk_corrected_range_mm_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  34));
	pdata->prev_shadow_result__spare_0_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  36));
	pdata->prev_shadow_result__spare_1_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  38));
	pdata->prev_shadow_result__spare_2_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  40));
	pdata->prev_shadow_result__spare_3_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  42));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_prev_shadow_system_results(
	VL53L1_DEV                 Dev,
	VL53L1_prev_shadow_system_results_t  *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_prev_shadow_system_results_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_prev_shadow_system_results(
			pdata,
			VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_PREV_SHADOW_RESULT__INTERRUPT_STATUS,
			comms_buffer,
			VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_prev_shadow_system_results(
	VL53L1_DEV                 Dev,
	VL53L1_prev_shadow_system_results_t  *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_prev_shadow_system_results_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_PREV_SHADOW_RESULT__INTERRUPT_STATUS,
			comms_buffer,
			VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_prev_shadow_system_results(
			VL53L1_PREV_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_prev_shadow_core_results(
	VL53L1_prev_shadow_core_results_t *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_prev_shadow_core_results_t into a I2C write buffer
	 * Buffer must be at least 33 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	VL53L1_i2c_encode_uint32_t(
		pdata->prev_shadow_result_core__ambient_window_events_sd0,
		4,
		pbuffer +   0);
	VL53L1_i2c_encode_uint32_t(
		pdata->prev_shadow_result_core__ranging_total_events_sd0,
		4,
		pbuffer +   4);
	VL53L1_i2c_encode_int32_t(
		pdata->prev_shadow_result_core__signal_total_events_sd0,
		4,
		pbuffer +   8);
	VL53L1_i2c_encode_uint32_t(
		pdata->prev_shadow_result_core__total_periods_elapsed_sd0,
		4,
		pbuffer +  12);
	VL53L1_i2c_encode_uint32_t(
		pdata->prev_shadow_result_core__ambient_window_events_sd1,
		4,
		pbuffer +  16);
	VL53L1_i2c_encode_uint32_t(
		pdata->prev_shadow_result_core__ranging_total_events_sd1,
		4,
		pbuffer +  20);
	VL53L1_i2c_encode_int32_t(
		pdata->prev_shadow_result_core__signal_total_events_sd1,
		4,
		pbuffer +  24);
	VL53L1_i2c_encode_uint32_t(
		pdata->prev_shadow_result_core__total_periods_elapsed_sd1,
		4,
		pbuffer +  28);
	*(pbuffer +  32) =
		pdata->prev_shadow_result_core__spare_0;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_prev_shadow_core_results(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_prev_shadow_core_results_t  *pdata)
{
	/**
	 * Decodes data structure VL53L1_prev_shadow_core_results_t from the input I2C read buffer
	 * Buffer must be at least 33 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->prev_shadow_result_core__ambient_window_events_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +   0));
	pdata->prev_shadow_result_core__ranging_total_events_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +   4));
	pdata->prev_shadow_result_core__signal_total_events_sd0 =
		(VL53L1_i2c_decode_int32_t(4, pbuffer +   8));
	pdata->prev_shadow_result_core__total_periods_elapsed_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  12));
	pdata->prev_shadow_result_core__ambient_window_events_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  16));
	pdata->prev_shadow_result_core__ranging_total_events_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  20));
	pdata->prev_shadow_result_core__signal_total_events_sd1 =
		(VL53L1_i2c_decode_int32_t(4, pbuffer +  24));
	pdata->prev_shadow_result_core__total_periods_elapsed_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  28));
	pdata->prev_shadow_result_core__spare_0 =
		(*(pbuffer +  32));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_prev_shadow_core_results(
	VL53L1_DEV                 Dev,
	VL53L1_prev_shadow_core_results_t  *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_prev_shadow_core_results_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_prev_shadow_core_results(
			pdata,
			VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_PREV_SHADOW_RESULT_CORE__AMBIENT_WINDOW_EVENTS_SD0,
			comms_buffer,
			VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_prev_shadow_core_results(
	VL53L1_DEV                 Dev,
	VL53L1_prev_shadow_core_results_t  *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_prev_shadow_core_results_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_PREV_SHADOW_RESULT_CORE__AMBIENT_WINDOW_EVENTS_SD0,
			comms_buffer,
			VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_prev_shadow_core_results(
			VL53L1_PREV_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_patch_debug(
	VL53L1_patch_debug_t     *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_patch_debug_t into a I2C write buffer
	 * Buffer must be at least 2 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdata->result__debug_status;
	*(pbuffer +   1) =
		pdata->result__debug_stage;
	LOG_FUNCTION_END(status);


	return status;
}

VL53L1_Error VL53L1_i2c_decode_patch_debug(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_patch_debug_t      *pdata)
{
	/**
	 * Decodes data structure VL53L1_patch_debug_t from the input I2C read buffer
	 * Buffer must be at least 2 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->result__debug_status =
		(*(pbuffer +   0));
	pdata->result__debug_stage =
		(*(pbuffer +   1));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_patch_debug(
	VL53L1_DEV                 Dev,
	VL53L1_patch_debug_t      *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_patch_debug_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_patch_debug(
			pdata,
			VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_RESULT__DEBUG_STATUS,
			comms_buffer,
			VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_patch_debug(
	VL53L1_DEV                 Dev,
	VL53L1_patch_debug_t      *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_patch_debug_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_RESULT__DEBUG_STATUS,
			comms_buffer,
			VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_patch_debug(
			VL53L1_PATCH_DEBUG_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}
#endif

#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_i2c_encode_gph_general_config(
	VL53L1_gph_general_config_t *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_gph_general_config_t into a I2C write buffer
	 * Buffer must be at least 5 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	VL53L1_i2c_encode_uint16_t(
		pdata->gph__system__thresh_rate_high,
		2,
		pbuffer +   0);
	VL53L1_i2c_encode_uint16_t(
		pdata->gph__system__thresh_rate_low,
		2,
		pbuffer +   2);
	*(pbuffer +   4) =
		pdata->gph__system__interrupt_config_gpio;
	LOG_FUNCTION_END(status);


	return status;
}
#endif


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_i2c_decode_gph_general_config(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_gph_general_config_t  *pdata)
{
	/**
	 * Decodes data structure VL53L1_gph_general_config_t from the input I2C read buffer
	 * Buffer must be at least 5 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->gph__system__thresh_rate_high =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   0));
	pdata->gph__system__thresh_rate_low =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   2));
	pdata->gph__system__interrupt_config_gpio =
		(*(pbuffer +   4));

	LOG_FUNCTION_END(status);

	return status;
}
#endif


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_set_gph_general_config(
	VL53L1_DEV                 Dev,
	VL53L1_gph_general_config_t  *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_gph_general_config_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_gph_general_config(
			pdata,
			VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_GPH__SYSTEM__THRESH_RATE_HIGH,
			comms_buffer,
			VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_get_gph_general_config(
	VL53L1_DEV                 Dev,
	VL53L1_gph_general_config_t  *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_gph_general_config_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_GPH__SYSTEM__THRESH_RATE_HIGH,
			comms_buffer,
			VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_gph_general_config(
			VL53L1_GPH_GENERAL_CONFIG_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_i2c_encode_gph_static_config(
	VL53L1_gph_static_config_t *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_gph_static_config_t into a I2C write buffer
	 * Buffer must be at least 6 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdata->gph__dss_config__roi_mode_control & 0x7;
	VL53L1_i2c_encode_uint16_t(
		pdata->gph__dss_config__manual_effective_spads_select,
		2,
		pbuffer +   1);
	*(pbuffer +   3) =
		pdata->gph__dss_config__manual_block_select;
	*(pbuffer +   4) =
		pdata->gph__dss_config__max_spads_limit;
	*(pbuffer +   5) =
		pdata->gph__dss_config__min_spads_limit;
	LOG_FUNCTION_END(status);


	return status;
}
#endif

#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_i2c_decode_gph_static_config(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_gph_static_config_t  *pdata)
{
	/**
	 * Decodes data structure VL53L1_gph_static_config_t from the input I2C read buffer
	 * Buffer must be at least 6 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->gph__dss_config__roi_mode_control =
		(*(pbuffer +   0)) & 0x7;
	pdata->gph__dss_config__manual_effective_spads_select =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   1));
	pdata->gph__dss_config__manual_block_select =
		(*(pbuffer +   3));
	pdata->gph__dss_config__max_spads_limit =
		(*(pbuffer +   4));
	pdata->gph__dss_config__min_spads_limit =
		(*(pbuffer +   5));

	LOG_FUNCTION_END(status);

	return status;
}
#endif


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_set_gph_static_config(
	VL53L1_DEV                 Dev,
	VL53L1_gph_static_config_t  *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_gph_static_config_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_gph_static_config(
			pdata,
			VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_GPH__DSS_CONFIG__ROI_MODE_CONTROL,
			comms_buffer,
			VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_get_gph_static_config(
	VL53L1_DEV                 Dev,
	VL53L1_gph_static_config_t  *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_gph_static_config_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_GPH__DSS_CONFIG__ROI_MODE_CONTROL,
			comms_buffer,
			VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_gph_static_config(
			VL53L1_GPH_STATIC_CONFIG_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_i2c_encode_gph_timing_config(
	VL53L1_gph_timing_config_t *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_gph_timing_config_t into a I2C write buffer
	 * Buffer must be at least 16 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdata->gph__mm_config__timeout_macrop_a_hi & 0xF;
	*(pbuffer +   1) =
		pdata->gph__mm_config__timeout_macrop_a_lo;
	*(pbuffer +   2) =
		pdata->gph__mm_config__timeout_macrop_b_hi & 0xF;
	*(pbuffer +   3) =
		pdata->gph__mm_config__timeout_macrop_b_lo;
	*(pbuffer +   4) =
		pdata->gph__range_config__timeout_macrop_a_hi & 0xF;
	*(pbuffer +   5) =
		pdata->gph__range_config__timeout_macrop_a_lo;
	*(pbuffer +   6) =
		pdata->gph__range_config__vcsel_period_a & 0x3F;
	*(pbuffer +   7) =
		pdata->gph__range_config__vcsel_period_b & 0x3F;
	*(pbuffer +   8) =
		pdata->gph__range_config__timeout_macrop_b_hi & 0xF;
	*(pbuffer +   9) =
		pdata->gph__range_config__timeout_macrop_b_lo;
	VL53L1_i2c_encode_uint16_t(
		pdata->gph__range_config__sigma_thresh,
		2,
		pbuffer +  10);
	VL53L1_i2c_encode_uint16_t(
		pdata->gph__range_config__min_count_rate_rtn_limit_mcps,
		2,
		pbuffer +  12);
	*(pbuffer +  14) =
		pdata->gph__range_config__valid_phase_low;
	*(pbuffer +  15) =
		pdata->gph__range_config__valid_phase_high;
	LOG_FUNCTION_END(status);


	return status;
}
#endif


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_i2c_decode_gph_timing_config(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_gph_timing_config_t  *pdata)
{
	/**
	 * Decodes data structure VL53L1_gph_timing_config_t from the input I2C read buffer
	 * Buffer must be at least 16 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->gph__mm_config__timeout_macrop_a_hi =
		(*(pbuffer +   0)) & 0xF;
	pdata->gph__mm_config__timeout_macrop_a_lo =
		(*(pbuffer +   1));
	pdata->gph__mm_config__timeout_macrop_b_hi =
		(*(pbuffer +   2)) & 0xF;
	pdata->gph__mm_config__timeout_macrop_b_lo =
		(*(pbuffer +   3));
	pdata->gph__range_config__timeout_macrop_a_hi =
		(*(pbuffer +   4)) & 0xF;
	pdata->gph__range_config__timeout_macrop_a_lo =
		(*(pbuffer +   5));
	pdata->gph__range_config__vcsel_period_a =
		(*(pbuffer +   6)) & 0x3F;
	pdata->gph__range_config__vcsel_period_b =
		(*(pbuffer +   7)) & 0x3F;
	pdata->gph__range_config__timeout_macrop_b_hi =
		(*(pbuffer +   8)) & 0xF;
	pdata->gph__range_config__timeout_macrop_b_lo =
		(*(pbuffer +   9));
	pdata->gph__range_config__sigma_thresh =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  10));
	pdata->gph__range_config__min_count_rate_rtn_limit_mcps =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  12));
	pdata->gph__range_config__valid_phase_low =
		(*(pbuffer +  14));
	pdata->gph__range_config__valid_phase_high =
		(*(pbuffer +  15));

	LOG_FUNCTION_END(status);

	return status;
}
#endif


#ifdef VL53L1_DEBUG
VL53L1_Error VL53L1_set_gph_timing_config(
	VL53L1_DEV                 Dev,
	VL53L1_gph_timing_config_t  *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_gph_timing_config_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_gph_timing_config(
			pdata,
			VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_GPH__MM_CONFIG__TIMEOUT_MACROP_A_HI,
			comms_buffer,
			VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


#ifdef PAL_EXTENDED
VL53L1_Error VL53L1_get_gph_timing_config(
	VL53L1_DEV                 Dev,
	VL53L1_gph_timing_config_t  *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_gph_timing_config_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_GPH__MM_CONFIG__TIMEOUT_MACROP_A_HI,
			comms_buffer,
			VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_gph_timing_config(
			VL53L1_GPH_TIMING_CONFIG_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}
#endif


#ifdef VL53L1_DEBUG
VL53L1_Error VL53L1_i2c_encode_fw_internal(
	VL53L1_fw_internal_t     *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_fw_internal_t into a I2C write buffer
	 * Buffer must be at least 2 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_FW_INTERNAL_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdata->firmware__internal_stream_count_div;
	*(pbuffer +   1) =
		pdata->firmware__internal_stream_counter_val;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_fw_internal(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_fw_internal_t      *pdata)
{
	/**
	 * Decodes data structure VL53L1_fw_internal_t from the input I2C read buffer
	 * Buffer must be at least 2 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_FW_INTERNAL_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->firmware__internal_stream_count_div =
		(*(pbuffer +   0));
	pdata->firmware__internal_stream_counter_val =
		(*(pbuffer +   1));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_fw_internal(
	VL53L1_DEV                 Dev,
	VL53L1_fw_internal_t      *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_fw_internal_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_FW_INTERNAL_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_fw_internal(
			pdata,
			VL53L1_FW_INTERNAL_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_FIRMWARE__INTERNAL_STREAM_COUNT_DIV,
			comms_buffer,
			VL53L1_FW_INTERNAL_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}

VL53L1_Error VL53L1_get_fw_internal(
	VL53L1_DEV                 Dev,
	VL53L1_fw_internal_t      *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_fw_internal_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_FW_INTERNAL_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_FIRMWARE__INTERNAL_STREAM_COUNT_DIV,
			comms_buffer,
			VL53L1_FW_INTERNAL_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_fw_internal(
			VL53L1_FW_INTERNAL_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_patch_results(
	VL53L1_patch_results_t   *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_patch_results_t into a I2C write buffer
	 * Buffer must be at least 90 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdata->dss_calc__roi_ctrl & 0x3;
	*(pbuffer +   1) =
		pdata->dss_calc__spare_1;
	*(pbuffer +   2) =
		pdata->dss_calc__spare_2;
	*(pbuffer +   3) =
		pdata->dss_calc__spare_3;
	*(pbuffer +   4) =
		pdata->dss_calc__spare_4;
	*(pbuffer +   5) =
		pdata->dss_calc__spare_5;
	*(pbuffer +   6) =
		pdata->dss_calc__spare_6;
	*(pbuffer +   7) =
		pdata->dss_calc__spare_7;
	*(pbuffer +   8) =
		pdata->dss_calc__user_roi_spad_en_0;
	*(pbuffer +   9) =
		pdata->dss_calc__user_roi_spad_en_1;
	*(pbuffer +  10) =
		pdata->dss_calc__user_roi_spad_en_2;
	*(pbuffer +  11) =
		pdata->dss_calc__user_roi_spad_en_3;
	*(pbuffer +  12) =
		pdata->dss_calc__user_roi_spad_en_4;
	*(pbuffer +  13) =
		pdata->dss_calc__user_roi_spad_en_5;
	*(pbuffer +  14) =
		pdata->dss_calc__user_roi_spad_en_6;
	*(pbuffer +  15) =
		pdata->dss_calc__user_roi_spad_en_7;
	*(pbuffer +  16) =
		pdata->dss_calc__user_roi_spad_en_8;
	*(pbuffer +  17) =
		pdata->dss_calc__user_roi_spad_en_9;
	*(pbuffer +  18) =
		pdata->dss_calc__user_roi_spad_en_10;
	*(pbuffer +  19) =
		pdata->dss_calc__user_roi_spad_en_11;
	*(pbuffer +  20) =
		pdata->dss_calc__user_roi_spad_en_12;
	*(pbuffer +  21) =
		pdata->dss_calc__user_roi_spad_en_13;
	*(pbuffer +  22) =
		pdata->dss_calc__user_roi_spad_en_14;
	*(pbuffer +  23) =
		pdata->dss_calc__user_roi_spad_en_15;
	*(pbuffer +  24) =
		pdata->dss_calc__user_roi_spad_en_16;
	*(pbuffer +  25) =
		pdata->dss_calc__user_roi_spad_en_17;
	*(pbuffer +  26) =
		pdata->dss_calc__user_roi_spad_en_18;
	*(pbuffer +  27) =
		pdata->dss_calc__user_roi_spad_en_19;
	*(pbuffer +  28) =
		pdata->dss_calc__user_roi_spad_en_20;
	*(pbuffer +  29) =
		pdata->dss_calc__user_roi_spad_en_21;
	*(pbuffer +  30) =
		pdata->dss_calc__user_roi_spad_en_22;
	*(pbuffer +  31) =
		pdata->dss_calc__user_roi_spad_en_23;
	*(pbuffer +  32) =
		pdata->dss_calc__user_roi_spad_en_24;
	*(pbuffer +  33) =
		pdata->dss_calc__user_roi_spad_en_25;
	*(pbuffer +  34) =
		pdata->dss_calc__user_roi_spad_en_26;
	*(pbuffer +  35) =
		pdata->dss_calc__user_roi_spad_en_27;
	*(pbuffer +  36) =
		pdata->dss_calc__user_roi_spad_en_28;
	*(pbuffer +  37) =
		pdata->dss_calc__user_roi_spad_en_29;
	*(pbuffer +  38) =
		pdata->dss_calc__user_roi_spad_en_30;
	*(pbuffer +  39) =
		pdata->dss_calc__user_roi_spad_en_31;
	*(pbuffer +  40) =
		pdata->dss_calc__user_roi_0;
	*(pbuffer +  41) =
		pdata->dss_calc__user_roi_1;
	*(pbuffer +  42) =
		pdata->dss_calc__mode_roi_0;
	*(pbuffer +  43) =
		pdata->dss_calc__mode_roi_1;
	*(pbuffer +  44) =
		pdata->sigma_estimator_calc__spare_0;
	VL53L1_i2c_encode_uint16_t(
		pdata->vhv_result__peak_signal_rate_mcps,
		2,
		pbuffer +  46);
	VL53L1_i2c_encode_uint32_t(
		pdata->vhv_result__signal_total_events_ref,
		4,
		pbuffer +  48);
	VL53L1_i2c_encode_uint16_t(
		pdata->phasecal_result__phase_output_ref,
		2,
		pbuffer +  52);
	VL53L1_i2c_encode_uint16_t(
		pdata->dss_result__total_rate_per_spad,
		2,
		pbuffer +  54);
	*(pbuffer +  56) =
		pdata->dss_result__enabled_blocks;
	VL53L1_i2c_encode_uint16_t(
		pdata->dss_result__num_requested_spads,
		2,
		pbuffer +  58);
	VL53L1_i2c_encode_uint16_t(
		pdata->mm_result__inner_intersection_rate,
		2,
		pbuffer +  62);
	VL53L1_i2c_encode_uint16_t(
		pdata->mm_result__outer_complement_rate,
		2,
		pbuffer +  64);
	VL53L1_i2c_encode_uint16_t(
		pdata->mm_result__total_offset,
		2,
		pbuffer +  66);
	VL53L1_i2c_encode_uint32_t(
		pdata->xtalk_calc__xtalk_for_enabled_spads & 0xFFFFFF,
		4,
		pbuffer +  68);
	VL53L1_i2c_encode_uint32_t(
		pdata->xtalk_result__avg_xtalk_user_roi_kcps & 0xFFFFFF,
		4,
		pbuffer +  72);
	VL53L1_i2c_encode_uint32_t(
		pdata->xtalk_result__avg_xtalk_mm_inner_roi_kcps & 0xFFFFFF,
		4,
		pbuffer +  76);
	VL53L1_i2c_encode_uint32_t(
		pdata->xtalk_result__avg_xtalk_mm_outer_roi_kcps & 0xFFFFFF,
		4,
		pbuffer +  80);
	VL53L1_i2c_encode_uint32_t(
		pdata->range_result__accum_phase,
		4,
		pbuffer +  84);
	VL53L1_i2c_encode_uint16_t(
		pdata->range_result__offset_corrected_range,
		2,
		pbuffer +  88);
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_patch_results(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_patch_results_t    *pdata)
{
	/**
	 * Decodes data structure VL53L1_patch_results_t from the input I2C read buffer
	 * Buffer must be at least 90 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->dss_calc__roi_ctrl =
		(*(pbuffer +   0)) & 0x3;
	pdata->dss_calc__spare_1 =
		(*(pbuffer +   1));
	pdata->dss_calc__spare_2 =
		(*(pbuffer +   2));
	pdata->dss_calc__spare_3 =
		(*(pbuffer +   3));
	pdata->dss_calc__spare_4 =
		(*(pbuffer +   4));
	pdata->dss_calc__spare_5 =
		(*(pbuffer +   5));
	pdata->dss_calc__spare_6 =
		(*(pbuffer +   6));
	pdata->dss_calc__spare_7 =
		(*(pbuffer +   7));
	pdata->dss_calc__user_roi_spad_en_0 =
		(*(pbuffer +   8));
	pdata->dss_calc__user_roi_spad_en_1 =
		(*(pbuffer +   9));
	pdata->dss_calc__user_roi_spad_en_2 =
		(*(pbuffer +  10));
	pdata->dss_calc__user_roi_spad_en_3 =
		(*(pbuffer +  11));
	pdata->dss_calc__user_roi_spad_en_4 =
		(*(pbuffer +  12));
	pdata->dss_calc__user_roi_spad_en_5 =
		(*(pbuffer +  13));
	pdata->dss_calc__user_roi_spad_en_6 =
		(*(pbuffer +  14));
	pdata->dss_calc__user_roi_spad_en_7 =
		(*(pbuffer +  15));
	pdata->dss_calc__user_roi_spad_en_8 =
		(*(pbuffer +  16));
	pdata->dss_calc__user_roi_spad_en_9 =
		(*(pbuffer +  17));
	pdata->dss_calc__user_roi_spad_en_10 =
		(*(pbuffer +  18));
	pdata->dss_calc__user_roi_spad_en_11 =
		(*(pbuffer +  19));
	pdata->dss_calc__user_roi_spad_en_12 =
		(*(pbuffer +  20));
	pdata->dss_calc__user_roi_spad_en_13 =
		(*(pbuffer +  21));
	pdata->dss_calc__user_roi_spad_en_14 =
		(*(pbuffer +  22));
	pdata->dss_calc__user_roi_spad_en_15 =
		(*(pbuffer +  23));
	pdata->dss_calc__user_roi_spad_en_16 =
		(*(pbuffer +  24));
	pdata->dss_calc__user_roi_spad_en_17 =
		(*(pbuffer +  25));
	pdata->dss_calc__user_roi_spad_en_18 =
		(*(pbuffer +  26));
	pdata->dss_calc__user_roi_spad_en_19 =
		(*(pbuffer +  27));
	pdata->dss_calc__user_roi_spad_en_20 =
		(*(pbuffer +  28));
	pdata->dss_calc__user_roi_spad_en_21 =
		(*(pbuffer +  29));
	pdata->dss_calc__user_roi_spad_en_22 =
		(*(pbuffer +  30));
	pdata->dss_calc__user_roi_spad_en_23 =
		(*(pbuffer +  31));
	pdata->dss_calc__user_roi_spad_en_24 =
		(*(pbuffer +  32));
	pdata->dss_calc__user_roi_spad_en_25 =
		(*(pbuffer +  33));
	pdata->dss_calc__user_roi_spad_en_26 =
		(*(pbuffer +  34));
	pdata->dss_calc__user_roi_spad_en_27 =
		(*(pbuffer +  35));
	pdata->dss_calc__user_roi_spad_en_28 =
		(*(pbuffer +  36));
	pdata->dss_calc__user_roi_spad_en_29 =
		(*(pbuffer +  37));
	pdata->dss_calc__user_roi_spad_en_30 =
		(*(pbuffer +  38));
	pdata->dss_calc__user_roi_spad_en_31 =
		(*(pbuffer +  39));
	pdata->dss_calc__user_roi_0 =
		(*(pbuffer +  40));
	pdata->dss_calc__user_roi_1 =
		(*(pbuffer +  41));
	pdata->dss_calc__mode_roi_0 =
		(*(pbuffer +  42));
	pdata->dss_calc__mode_roi_1 =
		(*(pbuffer +  43));
	pdata->sigma_estimator_calc__spare_0 =
		(*(pbuffer +  44));
	pdata->vhv_result__peak_signal_rate_mcps =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  46));
	pdata->vhv_result__signal_total_events_ref =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  48));
	pdata->phasecal_result__phase_output_ref =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  52));
	pdata->dss_result__total_rate_per_spad =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  54));
	pdata->dss_result__enabled_blocks =
		(*(pbuffer +  56));
	pdata->dss_result__num_requested_spads =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  58));
	pdata->mm_result__inner_intersection_rate =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  62));
	pdata->mm_result__outer_complement_rate =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  64));
	pdata->mm_result__total_offset =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  66));
	pdata->xtalk_calc__xtalk_for_enabled_spads =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  68)) & 0xFFFFFF;
	pdata->xtalk_result__avg_xtalk_user_roi_kcps =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  72)) & 0xFFFFFF;
	pdata->xtalk_result__avg_xtalk_mm_inner_roi_kcps =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  76)) & 0xFFFFFF;
	pdata->xtalk_result__avg_xtalk_mm_outer_roi_kcps =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  80)) & 0xFFFFFF;
	pdata->range_result__accum_phase =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  84));
	pdata->range_result__offset_corrected_range =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  88));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_patch_results(
	VL53L1_DEV                 Dev,
	VL53L1_patch_results_t    *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_patch_results_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_patch_results(
			pdata,
			VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_DSS_CALC__ROI_CTRL,
			comms_buffer,
			VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}

VL53L1_Error VL53L1_get_patch_results(
	VL53L1_DEV                 Dev,
	VL53L1_patch_results_t    *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_patch_results_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_DSS_CALC__ROI_CTRL,
			comms_buffer,
			VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_patch_results(
			VL53L1_PATCH_RESULTS_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_shadow_system_results(
	VL53L1_shadow_system_results_t *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_shadow_system_results_t into a I2C write buffer
	 * Buffer must be at least 82 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	*(pbuffer +   0) =
		pdata->shadow_phasecal_result__vcsel_start;
	*(pbuffer +   2) =
		pdata->shadow_result__interrupt_status & 0x3F;
	*(pbuffer +   3) =
		pdata->shadow_result__range_status;
	*(pbuffer +   4) =
		pdata->shadow_result__report_status & 0xF;
	*(pbuffer +   5) =
		pdata->shadow_result__stream_count;
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__dss_actual_effective_spads_sd0,
		2,
		pbuffer +   6);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__peak_signal_count_rate_mcps_sd0,
		2,
		pbuffer +   8);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__ambient_count_rate_mcps_sd0,
		2,
		pbuffer +  10);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__sigma_sd0,
		2,
		pbuffer +  12);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__phase_sd0,
		2,
		pbuffer +  14);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__final_crosstalk_corrected_range_mm_sd0,
		2,
		pbuffer +  16);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0,
		2,
		pbuffer +  18);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__mm_inner_actual_effective_spads_sd0,
		2,
		pbuffer +  20);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__mm_outer_actual_effective_spads_sd0,
		2,
		pbuffer +  22);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__avg_signal_count_rate_mcps_sd0,
		2,
		pbuffer +  24);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__dss_actual_effective_spads_sd1,
		2,
		pbuffer +  26);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__peak_signal_count_rate_mcps_sd1,
		2,
		pbuffer +  28);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__ambient_count_rate_mcps_sd1,
		2,
		pbuffer +  30);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__sigma_sd1,
		2,
		pbuffer +  32);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__phase_sd1,
		2,
		pbuffer +  34);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__final_crosstalk_corrected_range_mm_sd1,
		2,
		pbuffer +  36);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__spare_0_sd1,
		2,
		pbuffer +  38);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__spare_1_sd1,
		2,
		pbuffer +  40);
	VL53L1_i2c_encode_uint16_t(
		pdata->shadow_result__spare_2_sd1,
		2,
		pbuffer +  42);
	*(pbuffer +  44) =
		pdata->shadow_result__spare_3_sd1;
	*(pbuffer +  45) =
		pdata->shadow_result__thresh_info;
	*(pbuffer +  80) =
		pdata->shadow_phasecal_result__reference_phase_hi;
	*(pbuffer +  81) =
		pdata->shadow_phasecal_result__reference_phase_lo;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_shadow_system_results(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_shadow_system_results_t  *pdata)
{
	/**
	 * Decodes data structure VL53L1_shadow_system_results_t from the input I2C read buffer
	 * Buffer must be at least 82 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->shadow_phasecal_result__vcsel_start =
		(*(pbuffer +   0));
	pdata->shadow_result__interrupt_status =
		(*(pbuffer +   2)) & 0x3F;
	pdata->shadow_result__range_status =
		(*(pbuffer +   3));
	pdata->shadow_result__report_status =
		(*(pbuffer +   4)) & 0xF;
	pdata->shadow_result__stream_count =
		(*(pbuffer +   5));
	pdata->shadow_result__dss_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   6));
	pdata->shadow_result__peak_signal_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +   8));
	pdata->shadow_result__ambient_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  10));
	pdata->shadow_result__sigma_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  12));
	pdata->shadow_result__phase_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  14));
	pdata->shadow_result__final_crosstalk_corrected_range_mm_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  16));
	pdata->shadow_result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  18));
	pdata->shadow_result__mm_inner_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  20));
	pdata->shadow_result__mm_outer_actual_effective_spads_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  22));
	pdata->shadow_result__avg_signal_count_rate_mcps_sd0 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  24));
	pdata->shadow_result__dss_actual_effective_spads_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  26));
	pdata->shadow_result__peak_signal_count_rate_mcps_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  28));
	pdata->shadow_result__ambient_count_rate_mcps_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  30));
	pdata->shadow_result__sigma_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  32));
	pdata->shadow_result__phase_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  34));
	pdata->shadow_result__final_crosstalk_corrected_range_mm_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  36));
	pdata->shadow_result__spare_0_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  38));
	pdata->shadow_result__spare_1_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  40));
	pdata->shadow_result__spare_2_sd1 =
		(VL53L1_i2c_decode_uint16_t(2, pbuffer +  42));
	pdata->shadow_result__spare_3_sd1 =
		(*(pbuffer +  44));
	pdata->shadow_result__thresh_info =
		(*(pbuffer +  45));
	pdata->shadow_phasecal_result__reference_phase_hi =
		(*(pbuffer +  80));
	pdata->shadow_phasecal_result__reference_phase_lo =
		(*(pbuffer +  81));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_shadow_system_results(
	VL53L1_DEV                 Dev,
	VL53L1_shadow_system_results_t  *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_shadow_system_results_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_shadow_system_results(
			pdata,
			VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_SHADOW_PHASECAL_RESULT__VCSEL_START,
			comms_buffer,
			VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}

VL53L1_Error VL53L1_get_shadow_system_results(
	VL53L1_DEV                 Dev,
	VL53L1_shadow_system_results_t  *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_shadow_system_results_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_SHADOW_PHASECAL_RESULT__VCSEL_START,
			comms_buffer,
			VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_shadow_system_results(
			VL53L1_SHADOW_SYSTEM_RESULTS_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_i2c_encode_shadow_core_results(
	VL53L1_shadow_core_results_t *pdata,
	uint16_t                  buf_size,
	uint8_t                  *pbuffer)
{
	/**
	 * Encodes data structure VL53L1_shadow_core_results_t into a I2C write buffer
	 * Buffer must be at least 33 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	VL53L1_i2c_encode_uint32_t(
		pdata->shadow_result_core__ambient_window_events_sd0,
		4,
		pbuffer +   0);
	VL53L1_i2c_encode_uint32_t(
		pdata->shadow_result_core__ranging_total_events_sd0,
		4,
		pbuffer +   4);
	VL53L1_i2c_encode_int32_t(
		pdata->shadow_result_core__signal_total_events_sd0,
		4,
		pbuffer +   8);
	VL53L1_i2c_encode_uint32_t(
		pdata->shadow_result_core__total_periods_elapsed_sd0,
		4,
		pbuffer +  12);
	VL53L1_i2c_encode_uint32_t(
		pdata->shadow_result_core__ambient_window_events_sd1,
		4,
		pbuffer +  16);
	VL53L1_i2c_encode_uint32_t(
		pdata->shadow_result_core__ranging_total_events_sd1,
		4,
		pbuffer +  20);
	VL53L1_i2c_encode_int32_t(
		pdata->shadow_result_core__signal_total_events_sd1,
		4,
		pbuffer +  24);
	VL53L1_i2c_encode_uint32_t(
		pdata->shadow_result_core__total_periods_elapsed_sd1,
		4,
		pbuffer +  28);
	*(pbuffer +  32) =
		pdata->shadow_result_core__spare_0;
	LOG_FUNCTION_END(status);


	return status;
}


VL53L1_Error VL53L1_i2c_decode_shadow_core_results(
	uint16_t                   buf_size,
	uint8_t                   *pbuffer,
	VL53L1_shadow_core_results_t  *pdata)
{
	/**
	 * Decodes data structure VL53L1_shadow_core_results_t from the input I2C read buffer
	 * Buffer must be at least 33 bytes
	*/

	VL53L1_Error status = VL53L1_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES > buf_size)
		return VL53L1_ERROR_COMMS_BUFFER_TOO_SMALL;

	pdata->shadow_result_core__ambient_window_events_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +   0));
	pdata->shadow_result_core__ranging_total_events_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +   4));
	pdata->shadow_result_core__signal_total_events_sd0 =
		(VL53L1_i2c_decode_int32_t(4, pbuffer +   8));
	pdata->shadow_result_core__total_periods_elapsed_sd0 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  12));
	pdata->shadow_result_core__ambient_window_events_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  16));
	pdata->shadow_result_core__ranging_total_events_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  20));
	pdata->shadow_result_core__signal_total_events_sd1 =
		(VL53L1_i2c_decode_int32_t(4, pbuffer +  24));
	pdata->shadow_result_core__total_periods_elapsed_sd1 =
		(VL53L1_i2c_decode_uint32_t(4, pbuffer +  28));
	pdata->shadow_result_core__spare_0 =
		(*(pbuffer +  32));

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_set_shadow_core_results(
	VL53L1_DEV                 Dev,
	VL53L1_shadow_core_results_t  *pdata)
{
	/**
	 * Serialises and sends the contents of VL53L1_shadow_core_results_t
	 * data structure to the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_i2c_encode_shadow_core_results(
			pdata,
			VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES,
			comms_buffer);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_WriteMulti(
			Dev,
			VL53L1_SHADOW_RESULT_CORE__AMBIENT_WINDOW_EVENTS_SD0,
			comms_buffer,
			VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_get_shadow_core_results(
	VL53L1_DEV                 Dev,
	VL53L1_shadow_core_results_t  *pdata)
{
	/**
	 * Reads and de-serialises the contents of VL53L1_shadow_core_results_t
	 * data structure from the device
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t comms_buffer[VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES];

	LOG_FUNCTION_START("");

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_disable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_ReadMulti(
			Dev,
			VL53L1_SHADOW_RESULT_CORE__AMBIENT_WINDOW_EVENTS_SD0,
			comms_buffer,
			VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_firmware(Dev);

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_i2c_decode_shadow_core_results(
			VL53L1_SHADOW_CORE_RESULTS_I2C_SIZE_BYTES,
			comms_buffer,
			pdata);

	LOG_FUNCTION_END(status);

	return status;
}
#endif
