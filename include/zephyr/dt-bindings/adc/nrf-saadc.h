/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Nordic Semiconductor ASA
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_NRF_SAADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_NRF_SAADC_H_

#define NRF_SAADC_AIN0 0
#define NRF_SAADC_AIN1 1
#define NRF_SAADC_AIN2 2
#define NRF_SAADC_AIN3 3
#define NRF_SAADC_AIN4 4
#define NRF_SAADC_AIN5 5
#define NRF_SAADC_AIN6 6
#define NRF_SAADC_AIN7 7
#define NRF_SAADC_AIN8 8
#define NRF_SAADC_AIN9 9
#define NRF_SAADC_AIN10 10
#define NRF_SAADC_AIN11 11
#define NRF_SAADC_AIN12 12
#define NRF_SAADC_AIN13 13

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

#define NRF_SAADC_AIN_VDD_SHIM_OFFSET 128
#define NRF_SAADC_VDD		      (NRF_SAADC_AIN_VDD_SHIM_OFFSET + 0)
#define NRF_SAADC_VDDDIV2	      (NRF_SAADC_AIN_VDD_SHIM_OFFSET + 1)
#define NRF_SAADC_AVDD		      (NRF_SAADC_AIN_VDD_SHIM_OFFSET + 2)
#define NRF_SAADC_DVDD		      (NRF_SAADC_AIN_VDD_SHIM_OFFSET + 3)
#define NRF_SAADC_VDDHDIV5	      (NRF_SAADC_AIN_VDD_SHIM_OFFSET + 4)
#define NRF_SAADC_VDDL		      (NRF_SAADC_AIN_VDD_SHIM_OFFSET + 5)
#define NRF_SAADC_DECB		      (NRF_SAADC_AIN_VDD_SHIM_OFFSET + 6)
#define NRF_SAADC_VSS		      (NRF_SAADC_AIN_VDD_SHIM_OFFSET + 7)
#define NRF_SAADC_VDDAO3V0	      (NRF_SAADC_AIN_VDD_SHIM_OFFSET + 8)
#define NRF_SAADC_VDDAO1V8	      (NRF_SAADC_AIN_VDD_SHIM_OFFSET + 9)
#define NRF_SAADC_VDDAO0V8	      (NRF_SAADC_AIN_VDD_SHIM_OFFSET + 10)
#define NRF_SAADC_VDDRF		      (NRF_SAADC_AIN_VDD_SHIM_OFFSET + 11)
#define NRF_SAADC_VBAT		      (NRF_SAADC_AIN_VDD_SHIM_OFFSET + 12)
#define NRF_SAADC_AIN_DISABLED	      255 /* UINT8_MAX */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_NRF_SAADC_H_ */
