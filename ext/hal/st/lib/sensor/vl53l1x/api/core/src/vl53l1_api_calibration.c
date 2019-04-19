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
 * @file  vl53l1_api_core.c
 *
 * @brief EwokPlus25 low level API function definition
 */


#include "vl53l1_ll_def.h"
#include "vl53l1_ll_device.h"
#include "vl53l1_platform.h"
#include "vl53l1_register_map.h"
#include "vl53l1_register_funcs.h"
#include "vl53l1_register_settings.h"
#include "vl53l1_core.h"
#include "vl53l1_wait.h"
#include "vl53l1_api_preset_modes.h"
#include "vl53l1_silicon_core.h"
#include "vl53l1_api_core.h"
#include "vl53l1_api_calibration.h"

#ifdef VL53L1_LOG_ENABLE
  #include "vl53l1_api_debug.h"
#endif
#ifdef VL53L1_LOGGING
  #include "vl53l1_debug.h"
#endif

#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L1_TRACE_MODULE_CORE, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L1_TRACE_MODULE_CORE, status, ##__VA_ARGS__)
#define LOG_FUNCTION_END_FMT(status, fmt, ...) \
	_LOG_FUNCTION_END_FMT(VL53L1_TRACE_MODULE_CORE, status, \
		fmt, ##__VA_ARGS__)

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L1_TRACE_MODULE_CORE, \
	level, VL53L1_TRACE_FUNCTION_NONE, ##__VA_ARGS__)


#ifndef VL53L1_NOCALIB
VL53L1_Error VL53L1_run_ref_spad_char(
	VL53L1_DEV        Dev,
	VL53L1_Error     *pcal_status)
{
	/*
	 *  Runs Reference SPAD Characterisation
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	VL53L1_LLDriverData_t *pdev = VL53L1DevStructGetLLDriverHandle(Dev);

	uint8_t comms_buffer[6];

	VL53L1_refspadchar_config_t *prefspadchar  = &(pdev->refspadchar);

	LOG_FUNCTION_START("");

	/*
	 * Ensure power force is enabled
	 */

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status = VL53L1_enable_powerforce(Dev);

	/*
	 * Configure device
	 */

	if (status == VL53L1_ERROR_NONE)
		status =
			VL53L1_set_ref_spad_char_config(
				Dev,
				prefspadchar->vcsel_period,
				prefspadchar->timeout_us,
				prefspadchar->target_count_rate_mcps,
				prefspadchar->max_count_rate_limit_mcps,
				prefspadchar->min_count_rate_limit_mcps,
				pdev->stat_nvm.osc_measured__fast_osc__frequency);

	/*
	 * Run device test
	 */

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_run_device_test(
					Dev,
					prefspadchar->device_test_mode);

	/*
	 * Read results
	 */

	if (status == VL53L1_ERROR_NONE)
		status =
			VL53L1_ReadMulti(
				Dev,
				VL53L1_REF_SPAD_CHAR_RESULT__NUM_ACTUAL_REF_SPADS,
				comms_buffer,
				2);

	if (status == VL53L1_ERROR_NONE) {
		pdev->dbg_results.ref_spad_char_result__num_actual_ref_spads =
				comms_buffer[0];
		pdev->dbg_results.ref_spad_char_result__ref_location =
				comms_buffer[1];
	}

	/*
	 * copy results to customer nvm managed G02 registers
	 */

	if (status == VL53L1_ERROR_NONE)
		status =
			VL53L1_WriteMulti(
				Dev,
				VL53L1_REF_SPAD_MAN__NUM_REQUESTED_REF_SPADS,
				comms_buffer,
				2);

	if (status == VL53L1_ERROR_NONE) {
		pdev->customer.ref_spad_man__num_requested_ref_spads =
				comms_buffer[0];
		pdev->customer.ref_spad_man__ref_location =
				comms_buffer[1];
	}

	/* After Ref Spad Char the final set of good SPAD enables
	 * are stored in the NCY results registers below
	 *
	 *  - RESULT__SPARE_0_SD_1
	 *  - RESULT__SPARE_1_SD_1
	 *  - RESULT__SPARE_2_SD_1
	 */

	if (status == VL53L1_ERROR_NONE)
		status =
			VL53L1_ReadMulti(
				Dev,
				VL53L1_RESULT__SPARE_0_SD1,
				comms_buffer,
				6);

	/*
	 * copy reference SPAD enables to customer nvm managed
	 * G02 registers
	 */

	if (status == VL53L1_ERROR_NONE)
		status =
			VL53L1_WriteMulti(
				Dev,
				VL53L1_GLOBAL_CONFIG__SPAD_ENABLES_REF_0,
				comms_buffer,
				6);

	if (status == VL53L1_ERROR_NONE) {
		pdev->customer.global_config__spad_enables_ref_0 =
				comms_buffer[0];
		pdev->customer.global_config__spad_enables_ref_1 =
				comms_buffer[1];
		pdev->customer.global_config__spad_enables_ref_2 =
				comms_buffer[2];
		pdev->customer.global_config__spad_enables_ref_3 =
				comms_buffer[3];
		pdev->customer.global_config__spad_enables_ref_4 =
				comms_buffer[4];
		pdev->customer.global_config__spad_enables_ref_5 =
				comms_buffer[5];
	}

#ifdef VL53L1_LOG_ENABLE
	/* Print customer nvm managed data */
	if (status == VL53L1_ERROR_NONE)
		VL53L1_print_customer_nvm_managed(
			&(pdev->customer),
			"run_ref_spad_char():pdev->lldata.customer.",
			VL53L1_TRACE_MODULE_REF_SPAD_CHAR);
#endif

	if (status == VL53L1_ERROR_NONE) {

		switch (pdev->sys_results.result__range_status) {

		case VL53L1_DEVICEERROR_REFSPADCHARNOTENOUGHDPADS:
			status = VL53L1_WARNING_REF_SPAD_CHAR_NOT_ENOUGH_SPADS;
			break;

		case VL53L1_DEVICEERROR_REFSPADCHARMORETHANTARGET:
			status = VL53L1_WARNING_REF_SPAD_CHAR_RATE_TOO_HIGH;
			break;

		case VL53L1_DEVICEERROR_REFSPADCHARLESSTHANTARGET:
			status = VL53L1_WARNING_REF_SPAD_CHAR_RATE_TOO_LOW;
			break;
		}
	}

	/*
	 * Save unfiltered status
	 */

	*pcal_status = status;

	/* Status exception code */

	IGNORE_STATUS(
		IGNORE_REF_SPAD_CHAR_NOT_ENOUGH_SPADS,
		VL53L1_WARNING_REF_SPAD_CHAR_NOT_ENOUGH_SPADS,
		status);

	IGNORE_STATUS(
		IGNORE_REF_SPAD_CHAR_RATE_TOO_HIGH,
		VL53L1_WARNING_REF_SPAD_CHAR_RATE_TOO_HIGH,
		status);

	IGNORE_STATUS(
		IGNORE_REF_SPAD_CHAR_RATE_TOO_LOW,
		VL53L1_WARNING_REF_SPAD_CHAR_RATE_TOO_LOW,
		status);


	LOG_FUNCTION_END(status);

	return status;
}

VL53L1_Error VL53L1_run_offset_calibration(
	VL53L1_DEV	                  Dev,
	int16_t                       cal_distance_mm,
	VL53L1_Error                 *pcal_status)
{
	/*
	 * Runs offset calibration
	 *
	 * Recommended tuning parm settings:
	 *
	 *  - pre_num_of_samples    =    32
	 *  - mm1_num_of_samples    =   100
	 *  - mm2_num_of_samples    =    64
	 *  - target_distance_mm    =   140mm
	 *  - target reflectance    =     5%
	 *
	 * Standard Ranging (sigma delta mode):
	 *  - dss_config__target_total_rate_mcps = 20.0 -40.0 Mcps
	 *  - phasecal_config_timeout_us        =  1000
	 *  - range_config_timeout_us           = 13000
	 *  - mm_config_timeout_us              = 13000
	 *
	 *
	 * Note: function parms simplified as part of
	 * Patch_CalFunctionSimplification_11791
	 *
	 */

	VL53L1_Error status        = VL53L1_ERROR_NONE;
	VL53L1_LLDriverData_t *pdev =
		VL53L1DevStructGetLLDriverHandle(Dev);

	VL53L1_DevicePresetModes device_preset_modes[VL53L1_MAX_OFFSET_RANGE_RESULTS];

	VL53L1_range_results_t      range_results;
	VL53L1_range_results_t     *prange_results = &range_results;
	VL53L1_range_data_t        *prange_data = NULL;
	VL53L1_offset_range_data_t *poffset     = NULL;

	uint8_t  i                      = 0;
	uint8_t  m                      = 0;
	uint8_t  measurement_mode       =
		VL53L1_DEVICEMEASUREMENTMODE_BACKTOBACK;
	uint16_t manual_effective_spads =
		pdev->gen_cfg.dss_config__manual_effective_spads_select;

	uint8_t num_of_samples[VL53L1_MAX_OFFSET_RANGE_RESULTS];

	LOG_FUNCTION_START("");

	/* select requested offset calibration mode */

	switch (pdev->offset_calibration_mode) {

	default:
		device_preset_modes[0] =
			VL53L1_DEVICEPRESETMODE_STANDARD_RANGING;
		device_preset_modes[1] =
			VL53L1_DEVICEPRESETMODE_STANDARD_RANGING_MM1_CAL;
		device_preset_modes[2] =
			VL53L1_DEVICEPRESETMODE_STANDARD_RANGING_MM2_CAL;
	break;
	}

	/* initialise num_of_samples */
	/* Start Patch_CalFunctionSimplification_11791 */
	num_of_samples[0] = pdev->offsetcal_cfg.pre_num_of_samples;
	num_of_samples[1] = pdev->offsetcal_cfg.mm1_num_of_samples;
	num_of_samples[2] = pdev->offsetcal_cfg.mm2_num_of_samples;
	/* End Patch_CalFunctionSimplification_11791 */

	/* force all offsets to zero */

	switch (pdev->offset_calibration_mode) {

	case VL53L1_OFFSETCALIBRATIONMODE__MM1_MM2__STANDARD_PRE_RANGE_ONLY:
		/* only run pre range */
		pdev->offset_results.active_results  = 1;

	break;

	default:

		pdev->customer.mm_config__inner_offset_mm  = 0;
		pdev->customer.mm_config__outer_offset_mm  = 0;
		pdev->offset_results.active_results  =
			VL53L1_MAX_OFFSET_RANGE_RESULTS;

	break;
	}

	pdev->customer.algo__part_to_part_range_offset_mm = 0;

	/* initialise offset range results */

	pdev->offset_results.max_results           = VL53L1_MAX_OFFSET_RANGE_RESULTS;
	pdev->offset_results.cal_distance_mm       = cal_distance_mm;

	for (m = 0 ; m <  VL53L1_MAX_OFFSET_RANGE_RESULTS; m++) {

		poffset = &(pdev->offset_results.data[m]);
		poffset->preset_mode         = 0;
		poffset->no_of_samples       = 0;
		poffset->effective_spads     = 0;
		poffset->peak_rate_mcps      = 0;
		poffset->sigma_mm            = 0;
		poffset->median_range_mm     = 0;
	}

	for (m = 0 ; m < pdev->offset_results.active_results ; m++) {

		poffset = &(pdev->offset_results.data[m]);

		poffset->preset_mode         = device_preset_modes[m];

		/* Apply preset mode */

		if (status == VL53L1_ERROR_NONE)
			status =
				VL53L1_set_preset_mode(
					Dev,
					device_preset_modes[m],
					/* Start Patch_CalFunctionSimplification_11791 */
					pdev->offsetcal_cfg.dss_config__target_total_rate_mcps,
					pdev->offsetcal_cfg.phasecal_config_timeout_us,
					pdev->offsetcal_cfg.mm_config_timeout_us,
					pdev->offsetcal_cfg.range_config_timeout_us,
					/* End Patch_CalFunctionSimplification_11791 */
					100);

		pdev->gen_cfg.dss_config__manual_effective_spads_select =
				manual_effective_spads;

		/* Initialise device and start range */

		if (status == VL53L1_ERROR_NONE)
			status =
				VL53L1_init_and_start_range(
					Dev,
					measurement_mode,
					VL53L1_DEVICECONFIGLEVEL_CUSTOMER_ONWARDS);

		for (i = 0 ; i <= (num_of_samples[m]+2) ; i++) {

			/* Wait for range completion */

			if (status == VL53L1_ERROR_NONE)
				status =
					VL53L1_wait_for_range_completion(Dev);

			/*
			 *  Get Device Results
			 *  - Checks the stream count is the expected one
			 *  - Read device system results
			 */

			if (status == VL53L1_ERROR_NONE)
				status =
					VL53L1_get_device_results(
						Dev,
						VL53L1_DEVICERESULTSLEVEL_FULL,
						prange_results);

			/*
			 * Ignore 1st two ranges to give the sigma delta initial
			 * phase time to settle
			 *
			 * accummulate range results if range is successful
			 */

			prange_data  = &(prange_results->data[0]);

			if (prange_results->stream_count   > 1) {

				if (prange_data->range_status ==
						VL53L1_DEVICEERROR_RANGECOMPLETE) {

					poffset->no_of_samples++;
					poffset->effective_spads +=
						(uint32_t)prange_data->actual_effective_spads;
					poffset->peak_rate_mcps  +=
						(uint32_t)prange_data->peak_signal_count_rate_mcps;
					poffset->sigma_mm        +=
						(uint32_t)prange_data->sigma_mm;
					poffset->median_range_mm +=
						(int32_t)prange_data->median_range_mm;

					poffset->dss_config__roi_mode_control =
						pdev->gen_cfg.dss_config__roi_mode_control;
					poffset->dss_config__manual_effective_spads_select =
						pdev->gen_cfg.dss_config__manual_effective_spads_select;
				}
			}

			/*
			 * Conditional wait for firmware ready. Only waits for timed
			 * and single shot modes. Mode check is performed inside the
			 * wait function
			 */

			if (status == VL53L1_ERROR_NONE)
				status =
					VL53L1_wait_for_firmware_ready(Dev);

			/*
			 * Send ranging handshake
			 *
			 *  - Update Zone management
			 *  - Update GPH registers
			 *  - Clear current interrupt
			 *  - Initialise SYSTEM__MODE_START for next range (if there is one!)
			 */

			if (status == VL53L1_ERROR_NONE)
				status =
					VL53L1_clear_interrupt_and_enable_next_range(
						Dev,
						measurement_mode);
		}

		/* Stop range */

		if (status == VL53L1_ERROR_NONE)
			status = VL53L1_stop_range(Dev);

		/* Wait for Stop (abort) range to complete */

		if (status == VL53L1_ERROR_NONE)
			status = VL53L1_WaitUs(Dev, 1000);

		/* generate average values */
		if (poffset->no_of_samples > 0) {

			poffset->effective_spads += (poffset->no_of_samples/2);
			poffset->effective_spads /= poffset->no_of_samples;

			poffset->peak_rate_mcps  += (poffset->no_of_samples/2);
			poffset->peak_rate_mcps  /= poffset->no_of_samples;

			poffset->sigma_mm        += (poffset->no_of_samples/2);
			poffset->sigma_mm        /= poffset->no_of_samples;

			poffset->median_range_mm += (poffset->no_of_samples/2);
			poffset->median_range_mm /= poffset->no_of_samples;

			poffset->range_mm_offset  =  (int32_t)cal_distance_mm;
			poffset->range_mm_offset -= poffset->median_range_mm;

			/* remember the number of SPADs for standard ranging */
			if (poffset->preset_mode ==
				VL53L1_DEVICEPRESETMODE_STANDARD_RANGING)
				manual_effective_spads =
					(uint16_t)poffset->effective_spads;
		}
	}

	/* Calculate offsets */

	switch (pdev->offset_calibration_mode) {

	case VL53L1_OFFSETCALIBRATIONMODE__MM1_MM2__STANDARD_PRE_RANGE_ONLY:

		/* copy offsets to customer data structure */
		pdev->customer.mm_config__inner_offset_mm +=
			(int16_t)pdev->offset_results.data[0].range_mm_offset;
		pdev->customer.mm_config__outer_offset_mm +=
			(int16_t)pdev->offset_results.data[0].range_mm_offset;
	break;

	default:
		/* copy offsets to customer data structure */
		pdev->customer.mm_config__inner_offset_mm =
			(int16_t)pdev->offset_results.data[1].range_mm_offset;
		pdev->customer.mm_config__outer_offset_mm =
			(int16_t)pdev->offset_results.data[2].range_mm_offset;
		pdev->customer.algo__part_to_part_range_offset_mm = 0;

		/* copy average rate and effective SPAD count to
		   additional offset calibration data structure */

		pdev->add_off_cal_data.result__mm_inner_actual_effective_spads =
			(uint16_t)pdev->offset_results.data[1].effective_spads;
		pdev->add_off_cal_data.result__mm_outer_actual_effective_spads =
			(uint16_t)pdev->offset_results.data[2].effective_spads;

		pdev->add_off_cal_data.result__mm_inner_peak_signal_count_rtn_mcps =
			(uint16_t)pdev->offset_results.data[1].peak_rate_mcps;
		pdev->add_off_cal_data.result__mm_outer_peak_signal_count_rtn_mcps =
			(uint16_t)pdev->offset_results.data[2].peak_rate_mcps;

		break;
	}


	/* apply to device */

	if (status == VL53L1_ERROR_NONE)
		status =
			VL53L1_set_customer_nvm_managed(
				Dev,
				&(pdev->customer));

	/*
	 *  Check the peak rates, sigma, min spads for each stage
	 */

	for (m = 0 ; m < pdev->offset_results.active_results ; m++) {

		poffset = &(pdev->offset_results.data[m]);

		if (status == VL53L1_ERROR_NONE) {

			pdev->offset_results.cal_report = m;

			if (poffset->no_of_samples < num_of_samples[m])
				status = VL53L1_WARNING_OFFSET_CAL_MISSING_SAMPLES;

			/* only check sigma for the pre-range as
			 * the it is not calculated by the device
			 * for the MM1 and MM2 stages
			 */
			if (m == 0 && poffset->sigma_mm >
				((uint32_t)VL53L1_OFFSET_CAL_MAX_SIGMA_MM<<5))
				status = VL53L1_WARNING_OFFSET_CAL_SIGMA_TOO_HIGH;

			if (poffset->peak_rate_mcps >
				VL53L1_OFFSET_CAL_MAX_PRE_PEAK_RATE_MCPS)
				status = VL53L1_WARNING_OFFSET_CAL_RATE_TOO_HIGH;

			if (poffset->dss_config__manual_effective_spads_select <
				VL53L1_OFFSET_CAL_MIN_EFFECTIVE_SPADS)
				status = VL53L1_WARNING_OFFSET_CAL_SPAD_COUNT_TOO_LOW;

			if (poffset->dss_config__manual_effective_spads_select == 0)
				status = VL53L1_ERROR_OFFSET_CAL_NO_SPADS_ENABLED_FAIL;

			if (poffset->no_of_samples == 0)
				status = VL53L1_ERROR_OFFSET_CAL_NO_SAMPLE_FAIL;
		}
	}

	/*
	 * Save unfiltered status
	 */

	pdev->offset_results.cal_status = status;
	*pcal_status = pdev->offset_results.cal_status;

	/* Status exception codes */

	IGNORE_STATUS(
		IGNORE_OFFSET_CAL_MISSING_SAMPLES,
		VL53L1_WARNING_OFFSET_CAL_MISSING_SAMPLES,
		status);

	IGNORE_STATUS(
		IGNORE_OFFSET_CAL_SIGMA_TOO_HIGH,
		VL53L1_WARNING_OFFSET_CAL_SIGMA_TOO_HIGH,
		status);

	IGNORE_STATUS(
		IGNORE_OFFSET_CAL_RATE_TOO_HIGH,
		VL53L1_WARNING_OFFSET_CAL_RATE_TOO_HIGH,
		status);

	IGNORE_STATUS(
		IGNORE_OFFSET_CAL_SPAD_COUNT_TOO_LOW,
		VL53L1_WARNING_OFFSET_CAL_SPAD_COUNT_TOO_LOW,
		status);

#ifdef VL53L1_LOG_ENABLE

	/* Prints out the offset calibration data for debug */

	VL53L1_print_customer_nvm_managed(
		&(pdev->customer),
		"run_offset_calibration():pdev->lldata.customer.",
		VL53L1_TRACE_MODULE_OFFSET_DATA);

	VL53L1_print_additional_offset_cal_data(
		&(pdev->add_off_cal_data),
		"run_offset_calibration():pdev->lldata.add_off_cal_data.",
		VL53L1_TRACE_MODULE_OFFSET_DATA);

	VL53L1_print_offset_range_results(
		&(pdev->offset_results),
		"run_offset_calibration():pdev->lldata.offset_results.",
		VL53L1_TRACE_MODULE_OFFSET_DATA);
#endif

	LOG_FUNCTION_END(status);

	return status;
}
#endif

#ifndef VL53L1_NOCALIB
VL53L1_Error VL53L1_run_spad_rate_map(
	VL53L1_DEV                 Dev,
	VL53L1_DeviceTestMode      device_test_mode,
	VL53L1_DeviceSscArray      array_select,
	uint32_t                   ssc_config_timeout_us,
    VL53L1_spad_rate_data_t   *pspad_rate_data)
{

    /**
     *  Runs SPAD Rate Map
     */

	VL53L1_Error status = VL53L1_ERROR_NONE;

	VL53L1_LLDriverData_t *pdev =
		VL53L1DevStructGetLLDriverHandle(Dev);

	LOG_FUNCTION_START("");

	/*
	 * Ensure power force is enabled
	 */
	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_enable_powerforce(Dev);

	/*
	 * Configure the test
	 */

	if (status == VL53L1_ERROR_NONE) {
		pdev->ssc_cfg.array_select = array_select;
		pdev->ssc_cfg.timeout_us   = ssc_config_timeout_us;
		status =
			VL53L1_set_ssc_config(
				Dev,
				&(pdev->ssc_cfg),
				pdev->stat_nvm.osc_measured__fast_osc__frequency);
	}

	/*
	 * Run device test
	 */

	if (status == VL53L1_ERROR_NONE)
		status =
			VL53L1_run_device_test(
				Dev,
				device_test_mode);

	/*
	 * Read Rate Data from Patch Ram
	 */

    if (status == VL53L1_ERROR_NONE)
		status =
			VL53L1_get_spad_rate_data(
				Dev,
				pspad_rate_data);

	if (device_test_mode == VL53L1_DEVICETESTMODE_LCR_VCSEL_ON)
		pspad_rate_data->fractional_bits =  7;
	else
		pspad_rate_data->fractional_bits = 15;

	/* Ensure power force is disabled */

	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_disable_powerforce(Dev);

#ifdef VL53L1_LOG_ENABLE
    /* Print return rate data and map */

    if (status == VL53L1_ERROR_NONE) {
		VL53L1_print_spad_rate_data(
			pspad_rate_data,
			"run_spad_rate_map():",
			VL53L1_TRACE_MODULE_SPAD_RATE_MAP);
		VL53L1_print_spad_rate_map(
			pspad_rate_data,
			"run_spad_rate_map():",
			VL53L1_TRACE_MODULE_SPAD_RATE_MAP);
    }
#endif

	LOG_FUNCTION_END(status);

	return status;
}
#endif


#ifndef VL53L1_NOCALIB
VL53L1_Error VL53L1_run_device_test(
	VL53L1_DEV             Dev,
	VL53L1_DeviceTestMode  device_test_mode)
{
	/*
	 *  Runs the selected Device Test Mode
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	VL53L1_LLDriverData_t *pdev = VL53L1DevStructGetLLDriverHandle(Dev);

	uint8_t      comms_buffer[2];
	uint8_t      gpio_hv_mux__ctrl = 0;

	LOG_FUNCTION_START("");

	/*
	 * Get current interrupt config
	 */

	if (status == VL53L1_ERROR_NONE) /*lint !e774 always true*/
		status =
			VL53L1_RdByte(
				Dev,
				VL53L1_GPIO_HV_MUX__CTRL,
				&gpio_hv_mux__ctrl);

	if (status == VL53L1_ERROR_NONE)
		pdev->stat_cfg.gpio_hv_mux__ctrl = gpio_hv_mux__ctrl;

	/*
	 * Trigger the test
	 */
	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_start_test(
					Dev,
					device_test_mode);

	/*
	 * Wait for test completion
	 */
	if (status == VL53L1_ERROR_NONE)
		status = VL53L1_wait_for_test_completion(Dev);

	/*
	 * Read range and report status
	 */
	if (status == VL53L1_ERROR_NONE)
		status =
			VL53L1_ReadMulti(
				Dev,
				VL53L1_RESULT__RANGE_STATUS,
				comms_buffer,
				2);

	if (status == VL53L1_ERROR_NONE) {
		pdev->sys_results.result__range_status  = comms_buffer[0];
		pdev->sys_results.result__report_status = comms_buffer[1];
	}

	/* mask range status bits */

	pdev->sys_results.result__range_status &=
		VL53L1_RANGE_STATUS__RANGE_STATUS_MASK;

	if (status == VL53L1_ERROR_NONE) {
		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"    Device Test Complete:\n\t%-32s = %3u\n\t%-32s = %3u\n",
			"result__range_status",
			pdev->sys_results.result__range_status,
			"result__report_status",
			pdev->sys_results.result__report_status);

		/*
		 * Clear interrupt
		 */
		if (status == VL53L1_ERROR_NONE)
			status = VL53L1_clear_interrupt(Dev);
	}

	/*
	 * Clear test mode register
	 *  - required so that next test command will trigger
	 *    internal MCU interrupt
	 */

	if (status == VL53L1_ERROR_NONE)
		status =
			VL53L1_start_test(
				Dev,
				0x00);

	LOG_FUNCTION_END(status);

	return status;
}
#endif
