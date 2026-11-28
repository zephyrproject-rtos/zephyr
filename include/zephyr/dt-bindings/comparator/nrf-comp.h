/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_NRF_COMP_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_NRF_COMP_H_

/* The values are nrfx_analog_input_t values, defined by nrfx. */
#include <nrfx_analog/nrfx_analog_inputs.h>

#define NRF_COMP_AIN0 NRFX_ANALOG_EXTERNAL_AIN0_VAL /** AIN0 external input */
#define NRF_COMP_AIN1 NRFX_ANALOG_EXTERNAL_AIN1_VAL /** AIN1 external input */
#define NRF_COMP_AIN2 NRFX_ANALOG_EXTERNAL_AIN2_VAL /** AIN2 external input */
#define NRF_COMP_AIN3 NRFX_ANALOG_EXTERNAL_AIN3_VAL /** AIN3 external input */
#define NRF_COMP_AIN4 NRFX_ANALOG_EXTERNAL_AIN4_VAL /** AIN4 external input */
#define NRF_COMP_AIN5 NRFX_ANALOG_EXTERNAL_AIN5_VAL /** AIN5 external input */
#define NRF_COMP_AIN6 NRFX_ANALOG_EXTERNAL_AIN6_VAL /** AIN6 external input */
#define NRF_COMP_AIN7 NRFX_ANALOG_EXTERNAL_AIN7_VAL /** AIN7 external input */

#define NRF_COMP_AIN_VDD_SHIM_OFFSET NRFX_ANALOG_AIN_INTERNAL_OFFSET
#define NRF_COMP_AIN_VDD_DIV2	     NRFX_ANALOG_INTERNAL_VDDDIV2_VAL
#define NRF_COMP_AIN_VDDH_DIV5	     NRFX_ANALOG_INTERNAL_VDDHDIV5_VAL

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_NRF_COMP_H_ */
