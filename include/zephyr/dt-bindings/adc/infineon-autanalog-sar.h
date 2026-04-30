/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for Infineon AutAnalog SAR ADC.
 *
 * These constants match the PDL enum values and are intended for use in
 * devicetree properties that configure the SAR ADC channels.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_AUTANALOG_SAR_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_AUTANALOG_SAR_H_

/**
 * @name ADC Vref source (cy_en_autanalog_sar_vref_t)
 * @{
 */
#define IFX_AUTANALOG_SAR_VREF_VDDA      0 /**< Vdda supply */
#define IFX_AUTANALOG_SAR_VREF_EXT       1 /**< External reference */
#define IFX_AUTANALOG_SAR_VREF_VBGR      2 /**< Band-gap reference */
#define IFX_AUTANALOG_SAR_VREF_VDDA_BY_2 3 /**< Vdda / 2 */
#define IFX_AUTANALOG_SAR_VREF_PRB_OUT0  4 /**< PRB Vref0 */
#define IFX_AUTANALOG_SAR_VREF_PRB_OUT1  5 /**< PRB Vref1 */
/** @} */

/**
 * @name Accumulator / averaging mode (cy_en_autanalog_sar_acc_mode_t)
 * @{
 */
#define IFX_AUTANALOG_SAR_ACC_DISABLED  0 /**< Averaging disabled */
#define IFX_AUTANALOG_SAR_ACC_ACCUNDUMP 1 /**< Accumulate and dump averaging */
/** @} */

/**
 * @name MUX pin assignments (cy_en_autanalog_sar_pin_mux_t)
 * @{
 */
#define IFX_AUTANALOG_SAR_PIN_MUX_CTB0_PIN1    0  /**< CTB0 GPIO pin 1 */
#define IFX_AUTANALOG_SAR_PIN_MUX_CTB0_PIN4    1  /**< CTB0 GPIO pin 4 */
#define IFX_AUTANALOG_SAR_PIN_MUX_CTB0_PIN6    2  /**< CTB0 GPIO pin 6 */
#define IFX_AUTANALOG_SAR_PIN_MUX_CTB0_PIN7    3  /**< CTB0 GPIO pin 7 */
#define IFX_AUTANALOG_SAR_PIN_MUX_CTB1_PIN1    4  /**< CTB1 GPIO pin 1 */
#define IFX_AUTANALOG_SAR_PIN_MUX_CTB1_PIN4    5  /**< CTB1 GPIO pin 4 */
#define IFX_AUTANALOG_SAR_PIN_MUX_CTB1_PIN6    6  /**< CTB1 GPIO pin 6 */
#define IFX_AUTANALOG_SAR_PIN_MUX_CTB1_PIN7    7  /**< CTB1 GPIO pin 7 */
#define IFX_AUTANALOG_SAR_PIN_MUX_CTB0_OA0_OUT 8  /**< CTB0 OpAmp0 output */
#define IFX_AUTANALOG_SAR_PIN_MUX_CTB0_OA1_OUT 9  /**< CTB0 OpAmp1 output */
#define IFX_AUTANALOG_SAR_PIN_MUX_CTB1_OA0_OUT 10 /**< CTB1 OpAmp0 output */
#define IFX_AUTANALOG_SAR_PIN_MUX_CTB1_OA1_OUT 11 /**< CTB1 OpAmp1 output */
#define IFX_AUTANALOG_SAR_PIN_MUX_DAC0         12 /**< DAC0 output */
#define IFX_AUTANALOG_SAR_PIN_MUX_DAC1         13 /**< DAC1 output */
#define IFX_AUTANALOG_SAR_PIN_MUX_TEMP_SENSOR  14 /**< Temperature sensor */
#define IFX_AUTANALOG_SAR_PIN_MUX_GPIO0        15 /**< GPIO 0 */
#define IFX_AUTANALOG_SAR_PIN_MUX_GPIO1        16 /**< GPIO 1 */
#define IFX_AUTANALOG_SAR_PIN_MUX_GPIO2        17 /**< GPIO 2 */
#define IFX_AUTANALOG_SAR_PIN_MUX_GPIO3        18 /**< GPIO 3 */
#define IFX_AUTANALOG_SAR_PIN_MUX_GPIO4        19 /**< GPIO 4 */
#define IFX_AUTANALOG_SAR_PIN_MUX_GPIO5        20 /**< GPIO 5 */
#define IFX_AUTANALOG_SAR_PIN_MUX_GPIO6        21 /**< GPIO 6 */
#define IFX_AUTANALOG_SAR_PIN_MUX_GPIO7        22 /**< GPIO 7 */
#define IFX_AUTANALOG_SAR_PIN_MUX_VSSA         25 /**< ADC ground */
/** @} */

/**
 * @name MUX mode for HS sequencer (cy_stc_autanalog_sar_seq_tab_hs_t::muxMode)
 * @{
 */
#define IFX_AUTANALOG_SAR_MUX_DISABLED           0 /**< MUX channels disabled */
#define IFX_AUTANALOG_SAR_MUX0_SINGLE_ENDED      1 /**< MUX channel 0 single ended */
#define IFX_AUTANALOG_SAR_MUX1_SINGLE_ENDED      2 /**< MUX channel 1 single ended */
#define IFX_AUTANALOG_SAR_MUX0_MUX1_SINGLE_ENDED 3 /**< MUX channel 0, 1 single ended */
#define IFX_AUTANALOG_SAR_MUX0_PSEUDO_DIFF       4 /**< MUX channel pseudo differential */
/** @} */

/**
 * @name Buffer power modes (cy_en_autanalog_sar_buf_pwr_t)
 * @{
 */
#define IFX_AUTANALOG_SAR_BUF_PWR_OFF             0  /**< Buffer off */
#define IFX_AUTANALOG_SAR_BUF_PWR_ULTRA_LOW       1  /**< Ultra low, charge pump off */
#define IFX_AUTANALOG_SAR_BUF_PWR_ULTRA_LOW_RAIL  2  /**< Ultra low, charge pump on */
#define IFX_AUTANALOG_SAR_BUF_PWR_LOW_RAIL        4  /**< Low, charge pump on */
#define IFX_AUTANALOG_SAR_BUF_PWR_MEDIUM_RAIL     6  /**< Medium, charge pump on (recommended) */
#define IFX_AUTANALOG_SAR_BUF_PWR_HIGH_RAIL       8  /**< High, charge pump on */
#define IFX_AUTANALOG_SAR_BUF_PWR_ULTRA_HIGH_RAIL 10 /**< Ultra high, charge pump on */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_AUTANALOG_SAR_H_ */
