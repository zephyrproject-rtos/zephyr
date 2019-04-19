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
 * @file vl53l1_ll_def.h
 *
 * @brief Type definitions for VL53L1 LL Driver.
 *
 */


#ifndef _VL53L1_LL_DEF_H_
#define _VL53L1_LL_DEF_H_

#include "vl53l1_ll_device.h"
#include "vl53l1_error_codes.h"
#include "vl53l1_register_structs.h"
#include "vl53l1_platform_user_config.h"
#include "vl53l1_platform_user_defines.h"
#include "vl53l1_error_exceptions.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup VL53L1_globalLLDriverDefine_group VL53L1 Defines
 *  @brief    VL53L1 LL Driver Defines
 *  @{
 */

/** VL53L1 Low Level Driver IMPLEMENTATION major version */
#define VL53L1_LL_API_IMPLEMENTATION_VER_MAJOR       1
/** VL53L1 Low Level DriverI IMPLEMENTATION minor version */
#define VL53L1_LL_API_IMPLEMENTATION_VER_MINOR       2
/** VL53L1 Low Level DriverI IMPLEMENTATION sub version */
#define VL53L1_LL_API_IMPLEMENTATION_VER_SUB         10
/** VL53L1 Low Level Driver IMPLEMENTATION sub version */
#define VL53L1_LL_API_IMPLEMENTATION_VER_REVISION    1840

#define VL53L1_LL_API_IMPLEMENTATION_VER_STRING "1.2.11.1840"

/** VL53L1_FIRMWARE min and max compatible revisions */
#define VL53L1_FIRMWARE_VER_MINIMUM         398
#define VL53L1_FIRMWARE_VER_MAXIMUM         400


/****************************************
 * PRIVATE define do not edit
 ****************************************/

#define VL53L1_LL_CALIBRATION_DATA_STRUCT_VERSION       0xECAB0102
    /** VL53L1 Calibration Data struct version */

/* Start Patch_ZoneCalDataStructVersion_11854 */

#define VL53L1_LL_ZONE_CALIBRATION_DATA_STRUCT_VERSION  0xECAE0101
    /** VL53L1 Zone Calibration Data struct version */

/* End Patch_ZoneCalDataStructVersion_11854 */

#define VL53L1_MAX_OFFSET_RANGE_RESULTS       3
	/*!< Sets the maximum number of offset range results
		 required for the offset calibration.
		 Order is RANGE, MM1, MM2  */

#define VL53L1_NVM_MAX_FMT_RANGE_DATA         4
	/*!< The number of FMT range data points stored in NVM */

#define VL53L1_NVM_PEAK_RATE_MAP_SAMPLES  25
    /*!< The number of samples in the NVM peak rate signal map */
#define VL53L1_NVM_PEAK_RATE_MAP_WIDTH     5
    /*!< Array width of NVM peak rate signal map */
#define VL53L1_NVM_PEAK_RATE_MAP_HEIGHT     5
    /*!< Array height the NVM peak rate signal map */

/** @defgroup VL53L1_defineExtraError_group Error and Warning code returned by API
 *  The following DEFINE are used to identify the PAL ERROR
 *  @{
 */

#define VL53L1_ERROR_DEVICE_FIRMWARE_TOO_OLD           ((VL53L1_Error) - 80)
	/*!< Device Firmware too old .. */
#define VL53L1_ERROR_DEVICE_FIRMWARE_TOO_NEW           ((VL53L1_Error) - 85)
	/*!< Device Firmware too new .. */
#define VL53L1_ERROR_UNIT_TEST_FAIL                    ((VL53L1_Error) - 90)
	/*!< Unit Test Fail */
#define VL53L1_ERROR_FILE_READ_FAIL                    ((VL53L1_Error) - 95)
	/*!< File Read  Fail */
#define VL53L1_ERROR_FILE_WRITE_FAIL                   ((VL53L1_Error) - 96)
	/*!< File Write Fail */
	/*!< Tells requested functionality has not been implemented yet or
	 * not compatible with the device */
/** @} VL53L1_defineExtraError_group */


/** @brief Defines the parameters of the LL driver Get Version Functions
 */
typedef struct {
	uint32_t     ll_revision; /*!< revision number */
	uint8_t      ll_major;    /*!< major number */
	uint8_t      ll_minor;    /*!< minor number */
	uint8_t      ll_build;    /*!< build number */
} VL53L1_ll_version_t;


/** @brief Reference SPAD Characterization (RefSpadChar) Config
 */

typedef struct {

	uint8_t    device_test_mode;     /*!< Device test mode */
	uint8_t    vcsel_period;         /*!< VCSEL period (register) value */
	uint32_t   timeout_us;           /*!< timeout in [us] */
	uint16_t   target_count_rate_mcps;
		/*!< Target reference total count rate in [Mcps] - 9.7 format */
	uint16_t   min_count_rate_limit_mcps;
		/*!< Min valid reference rate [Mcps] - 9.7 format */
	uint16_t   max_count_rate_limit_mcps;
		/*!< Max valid reference rate [Mcps] - 9.7 format */

} VL53L1_refspadchar_config_t;


/** @brief SPAD Self Check (SSC) Config data structure
 */


typedef struct {

	VL53L1_DeviceSscArray  array_select;
		/*!< SPAD Array select
		 * 0 - store RTN array count rates \n
		 * 1 - store REF array count rates */
	uint8_t    vcsel_period;
		/*!< VCSEL period (register) value */
	uint8_t    vcsel_start;
		/*!< VCSEL start register value */
	uint8_t    vcsel_width;
		/*!< VCSEL ssc_timeout_us width register value e.g. 2 */
	uint32_t   timeout_us;
		/*!< requested Ranging Timeout in [us] e.g 100000us */
	uint16_t   rate_limit_mcps;
		/*!< Rate limit for checks either 1.15 or
		 *    9.7 dependent on test_mode
		 */

} VL53L1_ssc_config_t;


/** @brief Xtalk Extraction and Paramter Config
 */

typedef struct {


	uint32_t  algo__crosstalk_compensation_plane_offset_kcps;
		/*!< Private crosstalk_compensation_plane_offset_kcps (fixed point 9.9) */
	int16_t   algo__crosstalk_compensation_x_plane_gradient_kcps;
		/*!< Private crosstalk_compensation_x_plane_gradient_kcps (fixed point 5.11) */
	int16_t   algo__crosstalk_compensation_y_plane_gradient_kcps;
		/*!< Private crosstalk_compensation_y_plane_gradient_kcps (fixed point 5.11) */
	uint32_t  nvm_default__crosstalk_compensation_plane_offset_kcps;
		/*!< NVm stored crosstalk_compensation_plane_offset_kcps (fixed point 9.9) */
	int16_t   nvm_default__crosstalk_compensation_x_plane_gradient_kcps;
		/*!< NVM stored crosstalk_compensation_x_plane_gradient_kcps (fixed point 5.11) */
	int16_t   nvm_default__crosstalk_compensation_y_plane_gradient_kcps;
		/*!< NVM stored crosstalk_compensation_y_plane_gradient_kcps (fixed point 5.11) */
	uint8_t   global_crosstalk_compensation_enable;
		/*!< Enable switch for crosstalk compensation in all modes */
	int16_t   lite_mode_crosstalk_margin_kcps;
		/*!< Additional xtalk factor rate, added to plane_offset value in both
		 * SD mode, applied as a seperate addition at point of
		 * application to the device, plane_offset
		 * value remains unaltered. (fixed point 7.9)
		 */
	uint8_t   crosstalk_range_ignore_threshold_mult;
	    /*!< User set multiplier for range ignore threshold setting (fixed point 3.5) */
	uint16_t  crosstalk_range_ignore_threshold_rate_mcps;
	    /*!< Generated range ignore threshold rate in Mcps per spad (fixed
	     * point 3.13)
	     */

} VL53L1_xtalk_config_t;


/** @brief TuningParameter Storage
 *
 * - Storage structure for any LLD tuning parms
 * which are dynamically altered by low level functions
 * mostly when programming directly to the device
 *
 *- Added as part of Patch_AddingTuningParmStorage_11821
 */

typedef struct {


	uint16_t  tp_tuning_parm_version;
		/*!< Programmed Global tuning version num for debug
		 */
	uint16_t  tp_tuning_parm_key_table_version;
			/*!< Key Table tuning structure \
			 * version
			 */
	uint16_t  tp_tuning_parm_lld_version;
		/*!< Programmed LLD version to ensure matching tuning structure \
		 * key table
		 */
	uint8_t   tp_init_phase_rtn_lite_long;
		/*!< initial phase value for rtn array \
		 * in Lite Long Ranging Mode
		 */
	uint8_t   tp_init_phase_rtn_lite_med;
		/*!< initial phase value for rtn array \
		 * in Lite Medium Ranging Mode
		 */
	uint8_t   tp_init_phase_rtn_lite_short;
		/*!< initial phase value for rtn array \
		 * in Lite Short Ranging Mode
		 */
	uint8_t   tp_init_phase_ref_lite_long;
		/*!< initial phase value for ref array \
		 * in Lite Long Ranging Mode
		 */
	uint8_t   tp_init_phase_ref_lite_med;
		/*!< initial phase value for ref array \
		 * in Lite Medium Ranging Mode
		 */
	uint8_t   tp_init_phase_ref_lite_short;
		/*!< initial phase value for ref array \
		 * in Lite short Ranging Mode
		 */

	uint8_t   tp_consistency_lite_phase_tolerance;
		/*!< Phase tolerance consistency value to be used \
		 * in Lite modes
	     */
	uint8_t   tp_phasecal_target;
		/*!< Phasecal target value
		 */
	uint16_t  tp_cal_repeat_rate;
		/*!< Auto VHV/Calibration repeat rate for \
		 * use in Lite mode
		 */
	uint8_t   tp_lite_min_clip;
		/*!< Min Clip value in mm applied to device in Lite \
		 * modes
		 */

	uint16_t  tp_lite_long_sigma_thresh_mm;
		/*!< Sigma threshold limit for Lite Long mode  \
		 * in 14.2 format mm
		 */
	uint16_t  tp_lite_med_sigma_thresh_mm;
		/*!< Sigma threshold limit for Lite Medium mode  \
		 * in 14.2 format mm
		 */
	uint16_t  tp_lite_short_sigma_thresh_mm;
		/*!< Sigma threshold limit for Lite Short mode  \
		 * in 14.2 format mm
		 */

	uint16_t  tp_lite_long_min_count_rate_rtn_mcps;
		/*!< Min count rate level used in lite long mode \
		 * in 9.7 Mcps format
		 */
	uint16_t  tp_lite_med_min_count_rate_rtn_mcps;
		/*!< Min count rate level used in lite medium mode \
		 * in 9.7 Mcps format
		 */
	uint16_t  tp_lite_short_min_count_rate_rtn_mcps;
		/*!< Min count rate level used in lite short mode \
		 * in 9.7 Mcps format
		 */

	uint8_t   tp_lite_sigma_est_pulse_width_ns;
		/*!< Sigma thresholding tunign parm for Lite mode
		 */
	uint8_t   tp_lite_sigma_est_amb_width_ns;
		/*!< Sigma thresholding tunign parm for Lite mode
	     */
	uint8_t   tp_lite_sigma_ref_mm;
		/*!< Sigma thresholding tunign parm for Lite mode
		 */
	uint8_t   tp_lite_seed_cfg;
		/*!< Lite Mode Seed mode switch
		 */
	uint8_t   tp_timed_seed_cfg;
		/*!< Timed Mode Seed mode switch
		 */

	uint8_t   tp_lite_quantifier;
		/*!< Low level quantifier setting for lite modes
		 */
	uint8_t   tp_lite_first_order_select;
		/*!< Low level First order select setting for lite modes
		 */

	uint16_t  tp_dss_target_lite_mcps;
		/*!< DSS Target rate in 9.7 format Mcps for lite modes
		 */
	uint16_t  tp_dss_target_timed_mcps;
		/*!< DSS Target rate in 9.7 format Mcps for Timed modes
		 */

	uint32_t  tp_phasecal_timeout_lite_us;
		/*!< Phasecal timeout in us for lite modes
		 */

	uint32_t  tp_phasecal_timeout_timed_us;
		/*!< Phasecal timeout in us for Timed modes
		 */

	uint32_t  tp_mm_timeout_lite_us;
		/*!< MM stage timeout in us for Lite modes
		 */
	uint32_t  tp_mm_timeout_timed_us;
		/*!< MM stage timeout in us for Timed modes
		 */
	uint32_t  tp_mm_timeout_lpa_us;
		/*!< MM stage timeout in us for Low Power Auto modes
		 */

	uint32_t  tp_range_timeout_lite_us;
		/*!< Ranging stage timeout in us for Lite modes
		 */
	uint32_t  tp_range_timeout_timed_us;
		/*!< Ranging stage timeout in us for Timed modes
		 */
	uint32_t  tp_range_timeout_lpa_us;
		/*!< Ranging stage timeout in us for Low Power Auto modes
		 */

} VL53L1_tuning_parm_storage_t;



/** @brief Optical Centre data
 *
 */

typedef struct {

	uint8_t   x_centre;  /*!< Optical x centre : 4.4 format */
	uint8_t   y_centre;  /*!< Optical y centre : 4.4 format */

} VL53L1_optical_centre_t;


/** @brief Defines User Zone(ROI) parameters
 *
 */

typedef struct {

	uint8_t   x_centre;   /*!< Zone x centre :  0-15 range */
	uint8_t   y_centre;   /*!< Zone y centre :  0-15 range */
	uint8_t   width;      /*!< Width  of Zone  0 = 1, 7 = 8, 15 = 16 */
	uint8_t   height;     /*!< Height of Zone  0 = 1, 7 = 8, 15 = 16 */

} VL53L1_user_zone_t;


/**
 * @struct VL53L1_GPIO_interrupt_config_t
 *
 * @brief Structure to configure conditions when GPIO interrupt is trigerred
 *
 */

typedef struct {

	/*! Distance interrupt mode */
	VL53L1_GPIO_Interrupt_Mode	intr_mode_distance;

	/*! Rate interrupt mode */
	VL53L1_GPIO_Interrupt_Mode	intr_mode_rate;

	/*! trigger interrupt if a new measurement is ready
	 * 	__WARNING!__ will override other settings
	 */
	uint8_t				intr_new_measure_ready;

	/*! Trigger interrupt if no target found */
	uint8_t				intr_no_target;

	/*! If set to 0, interrupts will only be triggered if BOTH rate AND
	 * distance thresholds are triggered (combined mode). If set to 1,
	 * interrupts will be triggered if EITHER rate OR distance thresholds
	 * are triggered (independent mode). */
	uint8_t				intr_combined_mode;

	/* -- thresholds -- */
	/* The struct holds a copy of the thresholds but they are written when
	 * this structure is set using VL53L1_set_GPIO_interrupt_config/_struct
	 * */

	/*! Distance threshold high limit (mm) */
	uint16_t			threshold_distance_high;

	/*! Distance threshold low limit (mm) */
	uint16_t			threshold_distance_low;

	/*! Rate threshold high limit (9.7 Mcps) */
	uint16_t			threshold_rate_high;

	/*! Rate threshold low limit (9.7 Mcps) */
	uint16_t			threshold_rate_low;

} VL53L1_GPIO_interrupt_config_t;

/* Start Patch_LowPowerAutoMode */
/**
 * @struct VL53L1_low_power_auto_data_t
 *
 * @brief Structure to hold state, tuning and output variables for the low
 * power auto mode (Presence)
 *
 */

typedef struct {

	/*! Tuning variable for the VHV loop bound setting in low power auto
	 * mode. This is zero based, so the number of loops in VHV is this + 1.
	 * Please note, the first range will run with default VHV settings.
	 * Only lower 6 bits are allowed */
	uint8_t		vhv_loop_bound;

	/*! Indicates if we are or are not in low power auto mode */
	uint8_t		is_low_power_auto_mode;

	/*! Used to check if we're running the first range or not. Not to be
	 * used as a stream count */
	uint8_t		low_power_auto_range_count;

	/*! saved interrupt config byte to restore */
	uint8_t		saved_interrupt_config;

	/*! saved vhv config init byte to restore */
	uint8_t		saved_vhv_init;

	/*! saved vhv config timeout byte to restore */
	uint8_t		saved_vhv_timeout;

	/*! phase cal resutl from the first range */
	uint8_t		first_run_phasecal_result;

	/*! DSS. Total rate per spad given from the current range */
	uint32_t	dss__total_rate_per_spad_mcps;

	/*! DSS. Calculated required SPADs value */
	uint16_t	dss__required_spads;

} VL53L1_low_power_auto_data_t;

/* End Patch_LowPowerAutoMode */

/**
 * @struct  VL53L1_range_data_t
 * @brief   Internal data structure for storing post processed ranges
 *
 */

typedef struct {

	/* Info size */

	uint8_t  range_id;
		/*!< Range Result id e.g 0, 1, 2 */
	uint32_t time_stamp;
		/*!< 32-bit time stamp */

	uint16_t   width;
		/*!< VCSEL pulse width in [PLL clocks] 6.4 format */
	uint8_t    woi;
		/*!< WOI width in [PLL clocks] */

	uint16_t   fast_osc_frequency;
		/*!< Oscillator frequency in 4.12 format */
	uint16_t   zero_distance_phase;
		/*!< Zero Distance phase in  5.11 format */
	uint16_t   actual_effective_spads;
		/*!< effective SPAD count in 8.8 format */

	uint32_t   total_periods_elapsed;
		/*!< Elapsed time in macro periods for readout channel */

	uint32_t   peak_duration_us;
		/*!< Peak VCSEL width time in us */

	uint32_t   woi_duration_us;
		/*!< WOI duration time in us */


	/* Event counts */

	uint32_t   ambient_window_events;
		/*!< Return event count for the ambient window   */
	uint32_t   ranging_total_events;
		/*!< Return ranging event count for the ranging window.
			This includes both VCSEL and ambient contributions */
	int32_t    signal_total_events;
		/*!< Return event count for the ranging window with ambient
			 subtracted, Note it is 32-bit signed register */

	/* Rates */

	uint16_t    peak_signal_count_rate_mcps;
		/*! Peak signal (VCSEL) Rate in 9.7 format */
	uint16_t    avg_signal_count_rate_mcps;
		/*! Average signal (VCSEL) Rate in 9.7 format */
	uint16_t    ambient_count_rate_mcps;
		/*! Ambient Rate in 9.7 format */
	uint16_t    total_rate_per_spad_mcps;
		/*! Total Rate Per SPAD in 3.13 format */
	uint32_t    peak_rate_per_spad_kcps;
		/*! Peak Rate Per SPAD in 13.11 format */

	/*  Sigma */

	uint16_t   sigma_mm;
		/*!< Range sigma Estimate [mm]  9.7 format */

	/* Phase */

	uint16_t   median_phase;
		/*!< Median Phase in 5.11 format */

	/* Range */

	int16_t    median_range_mm;
		/*!< Median Range in [mm] by default there are no fractional bits
			 Optionally 1 or 2 fractional can be enabled via the
			 VL53L1_SYSTEM__FRACTIONAL_ENABLE register */

	/* Range status */

	uint8_t    range_status;

} VL53L1_range_data_t;


/**
 * @struct  VL53L1_range_results_t
 * @brief   Structure for storing the set of range results
 *
 */

typedef struct {

	VL53L1_DeviceState     cfg_device_state;
		/*!< Configuration Device State */
	VL53L1_DeviceState     rd_device_state;
		/*!< Read Device State */
	uint8_t                stream_count;
		/*!< 8-bit stream count */

	uint8_t                device_status;
		/*!<  Global device status for result set */

	VL53L1_range_data_t    data[2];
		/*!< Range data each target distance */

} VL53L1_range_results_t;

/**
 * @struct  VL53L1_offset_range_data_t
 * @brief   Structure for storing the set of range results
 *          required for the mm1 and mm2 offset calibration
 *          functions
 *
 */

typedef struct {

	uint8_t    preset_mode;
		/*!<  Preset Mode use for range  */
	uint8_t    dss_config__roi_mode_control;
		/*!< Dynamic SPAD selection mode */
	uint16_t   dss_config__manual_effective_spads_select;
		/*!<  Requested number of manual effective SPAD's */
	uint8_t    no_of_samples;
		/*!<  Number of ranges  */
	uint32_t   effective_spads;
		/*!< Average effective SPAD's 8.8 format */
	uint32_t   peak_rate_mcps;
		/*!< Average peak rate Mcps 9.7 format  */
	uint32_t   sigma_mm;
		/*!< Average sigma in [mm] 14.2 format */
	int32_t    median_range_mm;
		/*!< Avg of median range over all ranges \
			 note value is signed */
	int32_t    range_mm_offset;
		/*!< The calculated range offset value */

} VL53L1_offset_range_data_t;


/**
 * @struct  VL53L1_offset_range_results_t
 * @brief   Structure for storing the set of range results
 *          required for the offset calibration functions
 *
 */

typedef struct {

	int16_t      cal_distance_mm;
		/*!< the calibration distance in [mm]*/
	VL53L1_Error cal_status;
		/*!< Calibration status, check for warning codes */
	uint8_t      cal_report;
		/*!< Stage for above cal status - 0 Pre, 1 = MM1, 2 = MM2  */
	uint8_t      max_results;
		/*!< Array size for histogram range data i.e. max number
			of results */
	uint8_t      active_results;
		/*!< Number of active measurements */
	VL53L1_offset_range_data_t data[VL53L1_MAX_OFFSET_RANGE_RESULTS];
		/*!< Range results for each offset measurement */

} VL53L1_offset_range_results_t;

/**
 *  @struct  VL53L1_additional_offset_cal_data_t
 *  @brief   Additional Offset Calibration Data
 *
 *  Additional offset calibration data. Contains the rate
 *  and effective SPAD counts for the MM inner and outer
 *  calibration steps.
 */

typedef struct {

	uint16_t  result__mm_inner_actual_effective_spads;
	/*!< MM Inner actual effective SPADs, 8.8 format */
	uint16_t  result__mm_outer_actual_effective_spads;
	/*!< MM Outer actual effective SPADs, 8.8 format */
	uint16_t  result__mm_inner_peak_signal_count_rtn_mcps;
	/*!< Mean value of MM Inner return peak rate in [Mcps], 9.7 format */
	uint16_t  result__mm_outer_peak_signal_count_rtn_mcps;
	/*!< Mean value of MM Outer return peak rate in [Mcps], 9.7 format */

} VL53L1_additional_offset_cal_data_t;


/**
 * @struct  VL53L1_cal_peak_rate_map_t
 * @brief   Structure for storing the calibration peak rate map
 *          Used by DMAX to understand the spatial roll off
 *          in the signal rate map towards the corner of the
 *          SPAD array.
 */

typedef struct {

	int16_t     cal_distance_mm;
	    /*!< calibration distance in [mm], 14.2 format */
	uint16_t    max_samples;
		/*!< Array size for rate map i.e. max number samples */
	uint16_t    width;
		/*!< Array width */
	uint16_t    height;
		/*!< Array height */
	uint16_t    peak_rate_mcps[VL53L1_NVM_PEAK_RATE_MAP_SAMPLES];
		/*!< Array of rate map samples */

} VL53L1_cal_peak_rate_map_t;


/**
 * @struct VL53L1_gain_calibration_data_t
 *
 * @brief Gain calibration data
 *
 */

typedef struct {

	uint16_t   standard_ranging_gain_factor;
		/*!< Standard ranging gain correction factor 1.11 format */

} VL53L1_gain_calibration_data_t;


/**
 * @struct VL53L1_ll_driver_state_t
 *
 * @brief  Contains the driver state information
 *
 */

typedef struct {

	VL53L1_DeviceState   cfg_device_state;
		/*!< Configuration Device State */
	uint8_t   cfg_stream_count;
		/*!< configuration stream count, becomes expected
			 stream count for zone */
	uint8_t   cfg_gph_id;
		/*!< Config Grouped Parameter Hold ID */
	uint8_t   cfg_timing_status;
		/*!< Timing A or B flag 0 = A, 1 = B */

	VL53L1_DeviceState   rd_device_state;
		/*!< Read Device State */
	uint8_t   rd_stream_count;
		/*!< rd stream count, used to check actual stream count */
	uint8_t   rd_gph_id;
		/*!< Read Grouped Parameter Hold ID */
	uint8_t   rd_timing_status;
		/*!< Timing A or B flag 0 = A, 1 = B */

} VL53L1_ll_driver_state_t;

/** @brief Run Offset Cal Function (offsetcal) Config
 */

typedef struct {

	uint16_t  dss_config__target_total_rate_mcps;
		/*!< DSS Target rate in MCPS (9.7 format) used \
		 * during run_offset_calibration() */
	uint32_t  phasecal_config_timeout_us;
		/*!< Phasecal timeout in us \
		 * used during run_offset_calibration() */
	uint32_t  range_config_timeout_us;
		/*!< Range timeout in us used during \
		 * run_offset_calibration() */
	uint32_t  mm_config_timeout_us;
		/*!< MM timeout in us used during \
		 * run_offset_calibration() \
		 * Added as part of Patch_AddedOffsetCalMMTuningParm_11791 */
	uint8_t   pre_num_of_samples;
		/*!< Number of Ranging samples used during \
		 * run_offset_calibration() */
	uint8_t   mm1_num_of_samples;
		/*!< Number of MM1 samples used during \
		 * run_offset_calibration() */
	uint8_t   mm2_num_of_samples;
	    /*!< Number of MM2 samples used during \
	     * run_offset_calibration() */

} VL53L1_offsetcal_config_t;



/**
 * @struct VL53L1_LLDriverData_t
 *
 * @brief VL53L1 LL Driver ST private data structure \n
 *
 */

typedef struct {

	uint8_t   wait_method;
		/*!< Wait type : blocking or non blocking */
	VL53L1_DevicePresetModes        preset_mode;
		/*!< Current preset mode */
	VL53L1_DeviceMeasurementModes   measurement_mode;
		/*!< Current measurement mode */
	VL53L1_OffsetCalibrationMode    offset_calibration_mode;
		/*!< Current offset calibration mode */
	VL53L1_OffsetCorrectionMode     offset_correction_mode;
		/*!< Current offset_ correction mode */
	uint32_t  phasecal_config_timeout_us;
		/*!< requested Phase Cal Timeout e.g. 1000us */
	uint32_t  mm_config_timeout_us;
		/*!< requested MM Timeout e.g. 2000us */
	uint32_t  range_config_timeout_us;
		/*!< requested Ranging Timeout e.g 13000us */
	uint32_t  inter_measurement_period_ms;
		/*!< requested Timing mode repeat period e.g 100ms */
	uint16_t  dss_config__target_total_rate_mcps;
		/*!< requested DSS Target Total Rate in 9.7 format e.g. 40.0Mcps
		 *   - Patch_ChangingPresetModeInputParms_11780 */
	uint32_t  fw_ready_poll_duration_ms;
		/*!< FW ready poll duration in ms*/
	uint8_t   fw_ready;
		/*!< Result of FW ready check */
	uint8_t   debug_mode;
		/*!< Internal Only - read extra debug data */

	/*!< version info structure */
	VL53L1_ll_version_t                 version;

	/*!< version info structure */
	VL53L1_ll_driver_state_t            ll_state;

	/*!< decoded GPIO interrupt config */
	VL53L1_GPIO_interrupt_config_t	    gpio_interrupt_config;

	/*!< public register data structures */
	VL53L1_customer_nvm_managed_t       customer;
	VL53L1_cal_peak_rate_map_t          cal_peak_rate_map;
	VL53L1_additional_offset_cal_data_t add_off_cal_data;
	VL53L1_gain_calibration_data_t      gain_cal;
	VL53L1_user_zone_t                  mm_roi;
	VL53L1_optical_centre_t             optical_centre;

	/*!< tuning parameter storage */
	VL53L1_tuning_parm_storage_t        tuning_parms;

	/*!< private return good SPAD map */
	uint8_t rtn_good_spads[VL53L1_RTN_SPAD_BUFFER_SIZE];

	/*!< private internal configuration structures */
	VL53L1_refspadchar_config_t         refspadchar;
	VL53L1_ssc_config_t                 ssc_cfg;
	VL53L1_xtalk_config_t               xtalk_cfg;
	VL53L1_offsetcal_config_t           offsetcal_cfg;

	/*!< private internal register data structures */
	VL53L1_static_nvm_managed_t         stat_nvm;
	VL53L1_static_config_t              stat_cfg;
	VL53L1_general_config_t             gen_cfg;
	VL53L1_timing_config_t              tim_cfg;
	VL53L1_dynamic_config_t             dyn_cfg;
	VL53L1_system_control_t             sys_ctrl;
	VL53L1_system_results_t             sys_results;
	VL53L1_nvm_copy_data_t              nvm_copy_data;

	/*!< Private Offset structure */
	VL53L1_offset_range_results_t       offset_results;

	/*!<  private debug register data structures */
	VL53L1_core_results_t               core_results;
	VL53L1_debug_results_t              dbg_results;

	/* Start Patch_LowPowerAutoMode */
	/*!< Low Powr Auto Mode Data */
	VL53L1_low_power_auto_data_t		low_power_auto_data;
	/* End Patch_LowPowerAutoMode */

#ifdef PAL_EXTENDED
	/* Patch Debug Data */
	VL53L1_patch_results_t                patch_results;
	VL53L1_shadow_core_results_t          shadow_core_results;
	VL53L1_shadow_system_results_t        shadow_sys_results;
	VL53L1_prev_shadow_core_results_t     prev_shadow_core_results;
	VL53L1_prev_shadow_system_results_t   prev_shadow_sys_results;
#endif

} VL53L1_LLDriverData_t;


/**
 * @struct VL53L1_LLDriverResults_t
 *
 * @brief VL53L1 LL Driver ST private results structure
 *
 */

typedef struct {

	/* Private last range results */
	VL53L1_range_results_t             range_results;

} VL53L1_LLDriverResults_t;

/**
 * @struct VL53L1_calibration_data_t
 *
 * @brief Per Part calibration data
 *
 */

typedef struct {

	uint32_t                             struct_version;
	VL53L1_customer_nvm_managed_t        customer;
	VL53L1_additional_offset_cal_data_t  add_off_cal_data;
	VL53L1_optical_centre_t              optical_centre;
	VL53L1_gain_calibration_data_t       gain_cal;
	VL53L1_cal_peak_rate_map_t           cal_peak_rate_map;

} VL53L1_calibration_data_t;


/**
 * @struct VL53L1_tuning_parameters_t
 *
 * @brief Tuning Parameters Debug data
 *
 */

typedef struct {
	uint16_t        vl53l1_tuningparm_version;
	uint16_t        vl53l1_tuningparm_key_table_version;
	uint16_t        vl53l1_tuningparm_lld_version;
	uint8_t        vl53l1_tuningparm_consistency_lite_phase_tolerance;
	uint8_t        vl53l1_tuningparm_phasecal_target;
	uint16_t        vl53l1_tuningparm_lite_cal_repeat_rate;
	uint16_t        vl53l1_tuningparm_lite_ranging_gain_factor;
	uint8_t        vl53l1_tuningparm_lite_min_clip_mm;
	uint16_t        vl53l1_tuningparm_lite_long_sigma_thresh_mm;
	uint16_t        vl53l1_tuningparm_lite_med_sigma_thresh_mm;
	uint16_t        vl53l1_tuningparm_lite_short_sigma_thresh_mm;
	uint16_t        vl53l1_tuningparm_lite_long_min_count_rate_rtn_mcps;
	uint16_t        vl53l1_tuningparm_lite_med_min_count_rate_rtn_mcps;
	uint16_t        vl53l1_tuningparm_lite_short_min_count_rate_rtn_mcps;
	uint8_t        vl53l1_tuningparm_lite_sigma_est_pulse_width;
	uint8_t        vl53l1_tuningparm_lite_sigma_est_amb_width_ns;
	uint8_t        vl53l1_tuningparm_lite_sigma_ref_mm;
	uint8_t        vl53l1_tuningparm_lite_rit_mult;
	uint8_t        vl53l1_tuningparm_lite_seed_config;
	uint8_t        vl53l1_tuningparm_lite_quantifier;
	uint8_t        vl53l1_tuningparm_lite_first_order_select;
	int16_t        vl53l1_tuningparm_lite_xtalk_margin_kcps;
	uint8_t        vl53l1_tuningparm_initial_phase_rtn_lite_long_range;
	uint8_t        vl53l1_tuningparm_initial_phase_rtn_lite_med_range;
	uint8_t        vl53l1_tuningparm_initial_phase_rtn_lite_short_range;
	uint8_t        vl53l1_tuningparm_initial_phase_ref_lite_long_range;
	uint8_t        vl53l1_tuningparm_initial_phase_ref_lite_med_range;
	uint8_t        vl53l1_tuningparm_initial_phase_ref_lite_short_range;
	uint8_t        vl53l1_tuningparm_timed_seed_config;
	uint8_t        vl53l1_tuningparm_vhv_loopbound;
	uint8_t        vl53l1_tuningparm_refspadchar_device_test_mode;
	uint8_t        vl53l1_tuningparm_refspadchar_vcsel_period;
	uint32_t        vl53l1_tuningparm_refspadchar_phasecal_timeout_us;
	uint16_t        vl53l1_tuningparm_refspadchar_target_count_rate_mcps;
	uint16_t        vl53l1_tuningparm_refspadchar_min_countrate_limit_mcps;
	uint16_t        vl53l1_tuningparm_refspadchar_max_countrate_limit_mcps;
	uint16_t        vl53l1_tuningparm_offset_cal_dss_rate_mcps;
	uint32_t        vl53l1_tuningparm_offset_cal_phasecal_timeout_us;
	uint32_t        vl53l1_tuningparm_offset_cal_mm_timeout_us;
	uint32_t        vl53l1_tuningparm_offset_cal_range_timeout_us;
	uint8_t        vl53l1_tuningparm_offset_cal_pre_samples;
	uint8_t        vl53l1_tuningparm_offset_cal_mm1_samples;
	uint8_t        vl53l1_tuningparm_offset_cal_mm2_samples;
	uint8_t        vl53l1_tuningparm_spadmap_vcsel_period;
	uint8_t        vl53l1_tuningparm_spadmap_vcsel_start;
	uint16_t        vl53l1_tuningparm_spadmap_rate_limit_mcps;
	uint16_t        vl53l1_tuningparm_lite_dss_config_target_total_rate_mcps;
	uint16_t        vl53l1_tuningparm_timed_dss_config_target_total_rate_mcps;
	uint32_t        vl53l1_tuningparm_lite_phasecal_config_timeout_us;
	uint32_t        vl53l1_tuningparm_timed_phasecal_config_timeout_us;
	uint32_t        vl53l1_tuningparm_lite_mm_config_timeout_us;
	uint32_t        vl53l1_tuningparm_timed_mm_config_timeout_us;
	uint32_t        vl53l1_tuningparm_lite_range_config_timeout_us;
	uint32_t        vl53l1_tuningparm_timed_range_config_timeout_us;
	uint8_t        vl53l1_tuningparm_lowpowerauto_vhv_loop_bound;
	uint32_t        vl53l1_tuningparm_lowpowerauto_mm_config_timeout_us;
	uint32_t        vl53l1_tuningparm_lowpowerauto_range_config_timeout_us;
} VL53L1_tuning_parameters_t;


/**
 * @struct  VL53L1_spad_rate_data_t
 * @brief   SPAD Rate Data output by SSC
 *
 * Container for the SPAD Rate data output by SPAD select check (SSC)
 * The data is stored in the buffer in SPAD number order and not
 * raster order
 *
 * Rate data is it either 1.15 or 9.7 fixed point format
 */

typedef struct {

	uint8_t    spad_type;
		/*!< Type of rate data stored */
	uint16_t   buffer_size;
		/*!< SPAD buffer size : should be at least 256 for EwokPlus25 */
	uint16_t   rate_data[VL53L1_NO_OF_SPAD_ENABLES];
		/*!< word buffer containing the SPAD rates  */
	uint16_t    no_of_values;
		/*!< Number of bytes used in the buffer */
	uint8_t    fractional_bits;
		/*!< Number of fractional bits either 7 or 15 */
	uint8_t    error_status;
		/*!< Set if supplied buffer is too small */

} VL53L1_spad_rate_data_t;


/* Start Patch_AdditionalDebugData_11823 */

/**
 * @struct  VL53L1_additional_data_t
 * @brief   Additional debug data
 *
 * Contains the LL Driver configuration information
 */

typedef struct {

	VL53L1_DevicePresetModes        preset_mode;
		/*!< Current preset mode */
	VL53L1_DeviceMeasurementModes   measurement_mode;
		/*!< Current measurement mode */

	uint32_t  phasecal_config_timeout_us;
		/*!< requested Phase Cal Timeout e.g. 1000us */
	uint32_t  mm_config_timeout_us;
		/*!< requested MM Timeout e.g. 2000us */
	uint32_t  range_config_timeout_us;
		/*!< requested Ranging Timeout e.g 13000us */
	uint32_t  inter_measurement_period_ms;
		/*!< requested Timing mode repeat period e.g 100ms */
	uint16_t  dss_config__target_total_rate_mcps;
		/*!< requested DSS Target Total Rate in 9.7 format e.g. 40.0Mcps*/

} VL53L1_additional_data_t;

/* End Patch_AdditionalDebugData_11823 */


/** @} VL53L1_globalLLDriverDefine_group */


#define SUPPRESS_UNUSED_WARNING(x) \
	((void) (x))


#define IGNORE_STATUS(__FUNCTION_ID__, __ERROR_STATUS_CHECK__, __STATUS__) \
	do { \
		DISABLE_WARNINGS(); \
		if (__FUNCTION_ID__) { \
			if (__STATUS__ == __ERROR_STATUS_CHECK__) { \
				__STATUS__ = VL53L1_ERROR_NONE; \
				WARN_OVERRIDE_STATUS(__FUNCTION_ID__); \
			} \
		} \
		ENABLE_WARNINGS(); \
	} \
	while (0)

#define VL53L1_COPYSTRING(str, ...) \
	(strncpy(str, ##__VA_ARGS__, VL53L1_MAX_STRING_LENGTH-1))

#ifdef __cplusplus
}
#endif

#endif /* _VL53L1_LL_DEF_H_ */


