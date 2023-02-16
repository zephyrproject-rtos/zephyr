/*
 * Copyright 2023 Chirp Microsystems. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file ch101_gpr_sr.c
 *
 * @brief Chirp CH101 General Purpose Rangefinding / Short Range firmware interface
 *
 * This file contains function definitions to interface a specific sensor firmware
 * package to SonicLib, including the main initialization routine for the firmware.
 * That routine initializes various fields within the \a ch_dev_t device descriptor
 * and specifies the proper functions to implement SonicLib API calls.  Those may
 * either be common implementations or firmware-specific routines located in this file.
 */

#include "soniclib.h"
#include "ch101_gpr_sr.h"
#include "ch_common.h"

uint8_t ch101_gpr_sr_init(ch_dev_t *dev_ptr, ch_group_t *grp_ptr, uint8_t i2c_addr,
			  uint8_t io_index, uint8_t i2c_bus_index)
{

	dev_ptr->part_number = CH101_PART_NUMBER;
	dev_ptr->app_i2c_address = i2c_addr;
	dev_ptr->io_index = io_index;
	dev_ptr->i2c_bus_index = i2c_bus_index;

	dev_ptr->freqCounterCycles = CH101_COMMON_FREQCOUNTERCYCLES;
	dev_ptr->freqLockValue = CH101_COMMON_READY_FREQ_LOCKED;

	/* Init firmware-specific function pointers */
	dev_ptr->firmware = ch101_gpr_sr_fw;
	dev_ptr->fw_version_string = ch101_gpr_sr_version;
	dev_ptr->ram_init = get_ram_ch101_gpr_sr_init_ptr();
	dev_ptr->get_fw_ram_init_size = get_ch101_gpr_sr_fw_ram_init_size;
	dev_ptr->get_fw_ram_init_addr = get_ch101_gpr_sr_fw_ram_init_addr;

	dev_ptr->prepare_pulse_timer = ch_common_prepare_pulse_timer;
	dev_ptr->store_pt_result = ch_common_store_pt_result;
	dev_ptr->store_op_freq = ch_common_store_op_freq;
	dev_ptr->store_bandwidth = NULL;
	dev_ptr->store_scalefactor = ch_common_store_scale_factor;
	dev_ptr->get_locked_state = ch_common_get_locked_state;

	/* Init API function pointers */
	dev_ptr->api_funcs.fw_load = ch_common_fw_load;
	dev_ptr->api_funcs.set_mode = ch_common_set_mode;
	dev_ptr->api_funcs.set_sample_interval = ch_common_set_sample_interval;
	dev_ptr->api_funcs.set_num_samples = ch_common_set_num_samples;
	dev_ptr->api_funcs.set_max_range = ch_common_set_max_range;
	dev_ptr->api_funcs.set_static_range = ch_common_set_static_range;
	dev_ptr->api_funcs.set_rx_holdoff = ch_common_set_rx_holdoff;
	dev_ptr->api_funcs.get_rx_holdoff = ch_common_get_rx_holdoff;
	dev_ptr->api_funcs.get_range = ch_common_get_range;
	dev_ptr->api_funcs.get_amplitude = ch_common_get_amplitude;
	dev_ptr->api_funcs.get_iq_data = ch_common_get_iq_data;
	dev_ptr->api_funcs.get_amplitude_data = ch_common_get_amplitude_data;
	dev_ptr->api_funcs.samples_to_mm = ch_common_samples_to_mm;
	dev_ptr->api_funcs.mm_to_samples = ch_common_mm_to_samples;
	dev_ptr->api_funcs.set_thresholds = NULL;
	dev_ptr->api_funcs.get_thresholds = NULL;
	dev_ptr->api_funcs.set_sample_window = ch_common_set_sample_window;
	dev_ptr->api_funcs.get_amplitude_avg = ch_common_get_amplitude_avg;
	dev_ptr->api_funcs.set_cal_result = ch_common_set_cal_result;
	dev_ptr->api_funcs.get_cal_result = ch_common_get_cal_result;

	/* Init max sample count */
	dev_ptr->max_samples = CH101_GPR_SR_MAX_SAMPLES;

	/* This firmware uses oversampling */
	dev_ptr->oversample = 2; /* 4x oversampling (value is power of 2) */

	/* Init device and group descriptor linkage */
	dev_ptr->group = grp_ptr;            /* set parent group pointer */
	grp_ptr->device[io_index] = dev_ptr; /* add to parent group */

	return 0;
}
