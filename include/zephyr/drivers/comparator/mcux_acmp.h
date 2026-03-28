/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COMP_MCUX_ACMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_COMP_MCUX_ACMP_H_

#include <zephyr/drivers/comparator.h>

#ifdef __cplusplus
extern "C" {
#endif

enum comp_mcux_acmp_offset_mode {
	COMP_MCUX_ACMP_OFFSET_MODE_LEVEL0 = 0,
	COMP_MCUX_ACMP_OFFSET_MODE_LEVEL1,
};

enum comp_mcux_acmp_hysteresis_mode {
	COMP_MCUX_ACMP_HYSTERESIS_MODE_LEVEL0 = 0,
	COMP_MCUX_ACMP_HYSTERESIS_MODE_LEVEL1,
	COMP_MCUX_ACMP_HYSTERESIS_MODE_LEVEL2,
	COMP_MCUX_ACMP_HYSTERESIS_MODE_LEVEL3,
};

struct comp_mcux_acmp_mode_config {
	enum comp_mcux_acmp_offset_mode offset_mode;
	enum comp_mcux_acmp_hysteresis_mode hysteresis_mode;
	bool enable_high_speed_mode;
	bool invert_output;
	bool use_unfiltered_output;
	bool enable_pin_output;
};

enum comp_mcux_acmp_mux_input {
	COMP_MCUX_ACMP_MUX_INPUT_IN0 = 0,
	COMP_MCUX_ACMP_MUX_INPUT_IN1,
	COMP_MCUX_ACMP_MUX_INPUT_IN2,
	COMP_MCUX_ACMP_MUX_INPUT_IN3,
	COMP_MCUX_ACMP_MUX_INPUT_IN4,
	COMP_MCUX_ACMP_MUX_INPUT_IN5,
	COMP_MCUX_ACMP_MUX_INPUT_IN6,
	COMP_MCUX_ACMP_MUX_INPUT_IN7,
};

enum comp_mcux_acmp_port_input {
	COMP_MCUX_ACMP_PORT_INPUT_DAC = 0,
	COMP_MCUX_ACMP_PORT_INPUT_MUX,
};

struct comp_mcux_acmp_input_config {
	enum comp_mcux_acmp_mux_input positive_mux_input;
	enum comp_mcux_acmp_mux_input negative_mux_input;
	enum comp_mcux_acmp_port_input positive_port_input;
	enum comp_mcux_acmp_port_input negative_port_input;
};

struct comp_mcux_acmp_filter_config {
	bool enable_sample;
	uint8_t filter_count;
	uint8_t filter_period;
};

enum comp_mcux_acmp_dac_vref_source {
	COMP_MCUX_ACMP_DAC_VREF_SOURCE_VIN1 = 0,
	COMP_MCUX_ACMP_DAC_VREF_SOURCE_VIN2,
};

struct comp_mcux_acmp_dac_config {
	enum comp_mcux_acmp_dac_vref_source vref_source;
	uint8_t value;
	bool enable_output;
	bool enable_high_speed_mode;
};

enum comp_mcux_acmp_dm_clock {
	COMP_MCUX_ACMP_DM_CLOCK_SLOW = 0,
	COMP_MCUX_ACMP_DM_CLOCK_FAST,
};

enum comp_mcux_acmp_dm_sample_time {
	COMP_MCUX_ACMP_DM_SAMPLE_TIME_T1 = 0,
	COMP_MCUX_ACMP_DM_SAMPLE_TIME_T2,
	COMP_MCUX_ACMP_DM_SAMPLE_TIME_T4,
	COMP_MCUX_ACMP_DM_SAMPLE_TIME_T8,
	COMP_MCUX_ACMP_DM_SAMPLE_TIME_T16,
	COMP_MCUX_ACMP_DM_SAMPLE_TIME_T32,
	COMP_MCUX_ACMP_DM_SAMPLE_TIME_T64,
	COMP_MCUX_ACMP_DM_SAMPLE_TIME_T256,
};

enum comp_mcux_acmp_dm_phase_time {
	COMP_MCUX_ACMP_DM_PHASE_TIME_ALT0 = 0,
	COMP_MCUX_ACMP_DM_PHASE_TIME_ALT1,
	COMP_MCUX_ACMP_DM_PHASE_TIME_ALT2,
	COMP_MCUX_ACMP_DM_PHASE_TIME_ALT3,
	COMP_MCUX_ACMP_DM_PHASE_TIME_ALT4,
	COMP_MCUX_ACMP_DM_PHASE_TIME_ALT5,
	COMP_MCUX_ACMP_DM_PHASE_TIME_ALT6,
	COMP_MCUX_ACMP_DM_PHASE_TIME_ALT7,
};

struct comp_mcux_acmp_dm_config {
	bool enable_positive_channel;
	bool enable_negative_channel;
	bool enable_resistor_divider;
	enum comp_mcux_acmp_dm_clock clock_source;
	enum comp_mcux_acmp_dm_sample_time sample_time;
	enum comp_mcux_acmp_dm_phase_time phase1_time;
	enum comp_mcux_acmp_dm_phase_time phase2_time;
};

int comp_mcux_acmp_set_mode_config(const struct device *dev,
				   const struct comp_mcux_acmp_mode_config *config);

int comp_mcux_acmp_set_input_config(const struct device *dev,
				    const struct comp_mcux_acmp_input_config *config);

int comp_mcux_acmp_set_filter_config(const struct device *dev,
				     const struct comp_mcux_acmp_filter_config *config);

int comp_mcux_acmp_set_dac_config(const struct device *dev,
				  const struct comp_mcux_acmp_dac_config *config);

int comp_mcux_acmp_set_dm_config(const struct device *dev,
				 const struct comp_mcux_acmp_dm_config *config);

int comp_mcux_acmp_set_window_mode(const struct device *dev, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_COMP_MCUX_ACMP_H_ */
