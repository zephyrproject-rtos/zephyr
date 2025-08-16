/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Nordic Semiconductor ASA
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_NRF_SAADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_NRF_SAADC_H_

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
#define NRF_SAADC_GND  0
#define NRF_SAADC_AIN0 1
#define NRF_SAADC_AIN1 2
#define NRF_SAADC_AIN2 3
#define NRF_SAADC_AIN3 4
#define NRF_SAADC_AIN4 5
#define NRF_SAADC_AIN5 6
#define NRF_SAADC_AIN6 7
#define NRF_SAADC_AIN7 8

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_NRF_SAADC_H_ */
