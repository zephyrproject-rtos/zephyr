/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
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

/**
 * @name FIR filter input channel (cy_en_autanalog_sar_fir_channel_t)
 * @{
 */
#define IFX_AUTANALOG_SAR_FIR_CH_DISABLED 0  /**< FIR filter input disabled */
#define IFX_AUTANALOG_SAR_FIR_CH_GPIO0    1  /**< GPIO channel 0 */
#define IFX_AUTANALOG_SAR_FIR_CH_GPIO1    2  /**< GPIO channel 1 */
#define IFX_AUTANALOG_SAR_FIR_CH_GPIO2    3  /**< GPIO channel 2 */
#define IFX_AUTANALOG_SAR_FIR_CH_GPIO3    4  /**< GPIO channel 3 */
#define IFX_AUTANALOG_SAR_FIR_CH_GPIO4    5  /**< GPIO channel 4 */
#define IFX_AUTANALOG_SAR_FIR_CH_GPIO5    6  /**< GPIO channel 5 */
#define IFX_AUTANALOG_SAR_FIR_CH_GPIO6    7  /**< GPIO channel 6 */
#define IFX_AUTANALOG_SAR_FIR_CH_GPIO7    8  /**< GPIO channel 7 */
#define IFX_AUTANALOG_SAR_FIR_CH_MUX0     9  /**< MUX channel 0 */
#define IFX_AUTANALOG_SAR_FIR_CH_MUX1     10 /**< MUX channel 1 */
#define IFX_AUTANALOG_SAR_FIR_CH_MUX2     11 /**< MUX channel 2 */
#define IFX_AUTANALOG_SAR_FIR_CH_MUX3     12 /**< MUX channel 3 */
#define IFX_AUTANALOG_SAR_FIR_CH_MUX4     13 /**< MUX channel 4 */
#define IFX_AUTANALOG_SAR_FIR_CH_MUX5     14 /**< MUX channel 5 */
#define IFX_AUTANALOG_SAR_FIR_CH_MUX6     15 /**< MUX channel 6 */
#define IFX_AUTANALOG_SAR_FIR_CH_MUX7     16 /**< MUX channel 7 */
#define IFX_AUTANALOG_SAR_FIR_CH_MUX8     17 /**< MUX channel 8 */
#define IFX_AUTANALOG_SAR_FIR_CH_MUX9     18 /**< MUX channel 9 */
#define IFX_AUTANALOG_SAR_FIR_CH_MUX10    19 /**< MUX channel 10 */
#define IFX_AUTANALOG_SAR_FIR_CH_MUX11    20 /**< MUX channel 11 */
#define IFX_AUTANALOG_SAR_FIR_CH_MUX12    21 /**< MUX channel 12 */
#define IFX_AUTANALOG_SAR_FIR_CH_MUX13    22 /**< MUX channel 13 */
#define IFX_AUTANALOG_SAR_FIR_CH_MUX14    23 /**< MUX channel 14 */
#define IFX_AUTANALOG_SAR_FIR_CH_MUX15    24 /**< MUX channel 15 */
/** @} */

/**
 * @name FIFO selection (cy_en_autanalog_fifo_sel_t)
 * @{
 */
#define IFX_AUTANALOG_SAR_FIFO_DISABLED 0 /**< FIFO disabled */
#define IFX_AUTANALOG_SAR_FIFO_0        1 /**< FIFO 0 */
#define IFX_AUTANALOG_SAR_FIFO_1        2 /**< FIFO 1 */
#define IFX_AUTANALOG_SAR_FIFO_2        3 /**< FIFO 2 */
#define IFX_AUTANALOG_SAR_FIFO_3        4 /**< FIFO 3 */
#define IFX_AUTANALOG_SAR_FIFO_4        5 /**< FIFO 4 */
#define IFX_AUTANALOG_SAR_FIFO_5        6 /**< FIFO 5 */
#define IFX_AUTANALOG_SAR_FIFO_6        7 /**< FIFO 6 */
#define IFX_AUTANALOG_SAR_FIFO_7        8 /**< FIFO 7 */
/** @} */

/**
 * @name Limit status selection (cy_en_autanalog_sar_limit_t)
 * @{
 */
#define IFX_AUTANALOG_SAR_LIMIT_DISABLED 0 /**< Limit status disabled */
#define IFX_AUTANALOG_SAR_LIMIT_STC0     1 /**< Limit status config 0 */
#define IFX_AUTANALOG_SAR_LIMIT_STC1     2 /**< Limit status config 1 */
#define IFX_AUTANALOG_SAR_LIMIT_STC2     3 /**< Limit status config 2 */
#define IFX_AUTANALOG_SAR_LIMIT_STC3     4 /**< Limit status config 3 */
/** @} */

/**
 * @name FIFO split configuration (cy_en_autanalog_fifo_split_t)
 * @{
 */
#define IFX_AUTANALOG_SAR_FIFO_SPLIT1 0 /**< Single 512-word buffer */
#define IFX_AUTANALOG_SAR_FIFO_SPLIT2 1 /**< Two 256-word buffers */
#define IFX_AUTANALOG_SAR_FIFO_SPLIT4 2 /**< Four 128-word buffers */
#define IFX_AUTANALOG_SAR_FIFO_SPLIT8 3 /**< Eight 64-word buffers */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_AUTANALOG_SAR_H_ */
