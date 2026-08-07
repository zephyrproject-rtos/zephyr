/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Intel Corporation
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef __INTEL_DAI_DRIVER_DMIC_NHLT_H__
#define __INTEL_DAI_DRIVER_DMIC_NHLT_H__

/* For NHLT DMIC configuration parsing */
#define DMIC_HW_CONTROLLERS_MAX	4
#define DMIC_HW_FIFOS_MAX	2

struct nhlt_dmic_gateway_attributes {
	uint32_t dw;
};

/* Time-slot mappings */
struct nhlt_dmic_ts_group {
	uint32_t ts_group[4];
};

/* Global configuration settings */
struct nhlt_dmic_global_config {
	uint32_t clock_on_delay;
};

/* PDM channels to be programmed using data from channel_cfg array. */
struct nhlt_dmic_channel_ctrl_mask {
	/* i'th bit = 1 means that configuration for PDM channel # i is provided. */
	uint8_t channel_ctrl_mask;
	uint8_t clock_source;
	uint16_t rsvd;
};

/* Channel configuration, see PDM HW specification for details. */
struct nhlt_dmic_channel_config {
	uint32_t out_control;
};

struct nhlt_dmic_config_blob {
	struct nhlt_dmic_gateway_attributes gtw_attributes;
	struct nhlt_dmic_ts_group time_slot;
	struct nhlt_dmic_global_config global_config;
	struct nhlt_dmic_channel_ctrl_mask ctrl_mask;
	struct nhlt_dmic_channel_config channel_config[];
};

struct nhlt_pdm_ctrl_mask {
	uint32_t pdm_ctrl_mask;
};

/* FIR configuration, see PDM HW specification for details.
 *
 * If there is only one PDM controller configuration passed, the other (missing) one is configured
 * by the driver just by clearing CIC_CONTROL.SOFT_RESET bit.
 *
 * The driver needs to make sure that all mics are disabled before starting to program PDM
 * controllers.
 */
struct nhlt_pdm_ctrl_fir_cfg {
	uint32_t fir_control;
	uint32_t fir_config;
	int32_t dc_offset_left;
	int32_t dc_offset_right;
	int32_t out_gain_left;
	int32_t out_gain_right;
	uint32_t reserved[2];
};

/* PDM controller configuration, see PDM HW specification for details. */
struct nhlt_pdm_ctrl_cfg {
	uint32_t cic_control;
	uint32_t cic_config;

	uint32_t reserved0;
	uint32_t mic_control;

	/* PDM SoundWire Map
	 *
	 * This field is used on platforms with SoundWire, otherwise ignored.
	 */
	uint32_t pdm_sdw_map;

	/* Index of another nhlt_pdm_ctrl_cfg to be used as a source of FIR coefficients.
	 *
	 * The index is 1-based, value of 0 means that FIR coefficients	array fir_coeffs is provided
	 * by this item.
	 * This is a very common case that the same FIR coefficients are used to program more than
	 * one PDM controller. In this case, fir_coeffs array may be provided in a single copy
	 * following nhlt_pdm_ctrl_cfg #0 and be reused by nhlt_pdm_ctrl_cfg #1 by setting
	 * reuse_fir_from_pdm to 1 (1-based index).
	 */
	uint32_t reuse_fir_from_pdm;
	uint32_t reserved1[2];

	/* FIR configurations */
	struct nhlt_pdm_ctrl_fir_cfg fir_config[2];

	/* Array of FIR coefficients, channel A goes first, then channel B.
	 *
	 * Actual size of the array depends on the number of active taps of the	FIR filter for
	 * channel A plus the number of active taps of the FIR filter for channel B (see FIR_CONFIG)
	 * as well as on the form (packed/unpacked) of values.
	 */
	uint32_t fir_coeffs[];
};

/* Tag indicating that FIRs are in a packed 24-bit format.
 *
 * Size of a single coefficient is 20-bit. Coefficients may be sent in either unpacked form where
 * each value takes one DWORD (32-bits) or in packed form where the array begins with
 * (FIR_COEFFS_PACKED_TO_24_BITS) value to indicate packed form (unpacked coefficient has always
 * most significant byte set to 0) followed by array of 24-bit values (in little endian form).
 */
#define FIR_COEFFS_PACKED_TO_24_BITS 0xFFFFFFFF

#endif /* __INTEL_DAI_DRIVER_DMIC_NHLT_H__ */
