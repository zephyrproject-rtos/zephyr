/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Nordic Semiconductor ASA
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_NRF_SAADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_NRF_SAADC_H_

/* The values are nrfx_analog_input_t values, defined by nrfx. */
#include <nrfx_analog/nrfx_analog_inputs.h>

#define NRF_SAADC_AIN0 NRFX_ANALOG_EXTERNAL_AIN0_VAL
#define NRF_SAADC_AIN1 NRFX_ANALOG_EXTERNAL_AIN1_VAL
#define NRF_SAADC_AIN2 NRFX_ANALOG_EXTERNAL_AIN2_VAL
#define NRF_SAADC_AIN3 NRFX_ANALOG_EXTERNAL_AIN3_VAL
#define NRF_SAADC_AIN4 NRFX_ANALOG_EXTERNAL_AIN4_VAL
#define NRF_SAADC_AIN5 NRFX_ANALOG_EXTERNAL_AIN5_VAL
#define NRF_SAADC_AIN6 NRFX_ANALOG_EXTERNAL_AIN6_VAL
#define NRF_SAADC_AIN7 NRFX_ANALOG_EXTERNAL_AIN7_VAL
#define NRF_SAADC_AIN8 NRFX_ANALOG_EXTERNAL_AIN8_VAL
#define NRF_SAADC_AIN9 NRFX_ANALOG_EXTERNAL_AIN9_VAL
#define NRF_SAADC_AIN10 NRFX_ANALOG_EXTERNAL_AIN10_VAL
#define NRF_SAADC_AIN11 NRFX_ANALOG_EXTERNAL_AIN11_VAL
#define NRF_SAADC_AIN12 NRFX_ANALOG_EXTERNAL_AIN12_VAL
#define NRF_SAADC_AIN13 NRFX_ANALOG_EXTERNAL_AIN13_VAL

/**
 * @brief Short ADC negative input to ground
 *
 * @details The nRF SAADC hardware only supports differential readings.
 * The nRF SAADC SE (single ended) mode is differential with the negative
 * input shorted to GND (ground). To use the nRF SAADC SE mode, set the
 * negative input to NRF_SAADC_GND:
 *
 * @code{.dts}
 *   zephyr,input-positive = <NRF_SAADC_AIN3>;
 *   zephyr,input-negative = <NRF_SAADC_GND>;
 * @endcode
 *
 * The nRF SAADC driver also supports using the nRF SAADC SE mode in
 * emulated "single-ended" mode, as defined by zephyr. In this mode,
 * negative readings will be clamped to 0 by software to emulate the
 * behavior of an ADC in "single-ended" mode, as defined by zephyr. To
 * do this, only define the positive input:
 *
 * @code{.dts}
 *   zephyr,input-positive = <NRF_SAADC_AIN3>;
 * @endcode
 */
#define NRF_SAADC_GND  (NRF_SAADC_AIN_VDD_SHIM_OFFSET - 1)

/** @brief Symbol specifying offset to configure "TEST" SAADC inputs */
#define NRF_SAADC_AIN_TEST_SHIM_OFFSET NRFX_ANALOG_AIN_TEST_OFFSET
/** @brief Symbol specifying offset to configure AVSS as a channel input */
#define NRF_SAADC_AVSS		      NRFX_ANALOG_TEST_AVSS_VAL

#define NRF_SAADC_AIN_VDD_SHIM_OFFSET NRFX_ANALOG_AIN_INTERNAL_OFFSET
#define NRF_SAADC_VDD		      NRFX_ANALOG_INTERNAL_VDD_VAL
#define NRF_SAADC_VDDDIV2	      NRFX_ANALOG_INTERNAL_VDDDIV2_VAL
#define NRF_SAADC_AVDD		      NRFX_ANALOG_INTERNAL_AVDD_VAL
#define NRF_SAADC_DVDD		      NRFX_ANALOG_INTERNAL_DVDD_VAL
#define NRF_SAADC_VDDHDIV5	      NRFX_ANALOG_INTERNAL_VDDHDIV5_VAL
#define NRF_SAADC_VDDL		      NRFX_ANALOG_INTERNAL_VDDL_VAL
#define NRF_SAADC_DECB		      NRFX_ANALOG_INTERNAL_DECB_VAL
#define NRF_SAADC_VSS		      NRFX_ANALOG_INTERNAL_VSS_VAL
#define NRF_SAADC_VDDAO3V0	      NRFX_ANALOG_INTERNAL_VDDAO3V0_VAL
#define NRF_SAADC_VDDAO1V8	      NRFX_ANALOG_INTERNAL_VDDAO1V8_VAL
#define NRF_SAADC_VDDAO0V8	      NRFX_ANALOG_INTERNAL_VDDAO0V8_VAL
#define NRF_SAADC_VDDRF		      NRFX_ANALOG_INTERNAL_VDDRF_VAL
#define NRF_SAADC_VBAT		      NRFX_ANALOG_INTERNAL_VBAT_VAL
#define NRF_SAADC_INTERNAL0	      NRFX_ANALOG_INTERNAL0_VAL
#define NRF_SAADC_INTERNAL1	      NRFX_ANALOG_INTERNAL1_VAL
#define NRF_SAADC_INTERNAL2	      NRFX_ANALOG_INTERNAL2_VAL
#define NRF_SAADC_INTERNAL3	      NRFX_ANALOG_INTERNAL3_VAL
#define NRF_SAADC_AIN_DISABLED	      NRFX_ANALOG_INPUT_DISABLED_VAL

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_NRF_SAADC_H_ */
