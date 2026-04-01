/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for Infineon AutAnalog CTDAC.
 *
 * These constants match the PDL enum values and are intended for use in
 * devicetree properties that configure the AutAnalog continuous-time DAC.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DAC_INFINEON_AUTANALOG_CTDAC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DAC_INFINEON_AUTANALOG_CTDAC_H_

/**
 * @name Reference buffer power modes (cy_en_autanalog_dac_ref_buf_pwr_t)
 * @{
 */
#define IFX_AUTANALOG_CTDAC_REF_BUF_PWR_OFF             0  /**< Reference buffer powered off */
#define IFX_AUTANALOG_CTDAC_REF_BUF_PWR_ULTRA_LOW       1  /**< Ultra low power, charge pump off */
#define IFX_AUTANALOG_CTDAC_REF_BUF_PWR_ULTRA_LOW_RAIL  2  /**< Ultra low power, charge pump on */
#define IFX_AUTANALOG_CTDAC_REF_BUF_PWR_LOW_RAIL        4  /**< Low power, charge pump on */
#define IFX_AUTANALOG_CTDAC_REF_BUF_PWR_MEDIUM_RAIL     6  /**< Medium power, charge pump on */
#define IFX_AUTANALOG_CTDAC_REF_BUF_PWR_HIGH            7  /**< High power mode */
#define IFX_AUTANALOG_CTDAC_REF_BUF_PWR_HIGH_RAIL       8  /**< High power, charge pump on */
#define IFX_AUTANALOG_CTDAC_REF_BUF_PWR_ULTRA_HIGH_RAIL 10 /**< Ultra high, charge pump on */
/** @} */

/**
 * @name Output buffer power modes (cy_en_autanalog_dac_out_buf_pwr_t)
 * @{
 */
#define IFX_AUTANALOG_CTDAC_OUT_BUF_PWR_OFF             0  /**< Output buffer powered off */
#define IFX_AUTANALOG_CTDAC_OUT_BUF_PWR_ULTRA_LOW       1  /**< Ultra low power, charge pump off */
#define IFX_AUTANALOG_CTDAC_OUT_BUF_PWR_ULTRA_LOW_RAIL  2  /**< Ultra low power, charge pump on */
#define IFX_AUTANALOG_CTDAC_OUT_BUF_PWR_LOW_RAIL        4  /**< Low power, charge pump on */
#define IFX_AUTANALOG_CTDAC_OUT_BUF_PWR_MEDIUM_RAIL     6  /**< Medium power, charge pump on */
#define IFX_AUTANALOG_CTDAC_OUT_BUF_PWR_HIGH_RAIL       8  /**< High power, charge pump on */
#define IFX_AUTANALOG_CTDAC_OUT_BUF_PWR_ULTRA_HIGH_RAIL 10 /**< Ultra high, charge pump on */
/** @} */

/**
 * @name Vref source (combined vrefSel + vrefMux encoding)
 *
 * VDDA uses bit 8 as a flag. All other values map directly to the
 * `cy_en_autanalog_dac_vref_mux_t` PDL encoding.
 * @{
 */
#define IFX_AUTANALOG_CTDAC_VREF_VDDA     0x100 /**< VDDA supply */
#define IFX_AUTANALOG_CTDAC_VREF_VBGR     0     /**< Band-gap reference */
#define IFX_AUTANALOG_CTDAC_VREF_CTB0_OA0 1     /**< CTB0 OpAmp0 output */
#define IFX_AUTANALOG_CTDAC_VREF_CTB0_OA1 2     /**< CTB0 OpAmp1 output */
#define IFX_AUTANALOG_CTDAC_VREF_CTB1_OA0 3     /**< CTB1 OpAmp0 output */
#define IFX_AUTANALOG_CTDAC_VREF_CTB1_OA1 4     /**< CTB1 OpAmp1 output */
#define IFX_AUTANALOG_CTDAC_VREF_PRB_OUT0 6     /**< PRB output 0 */
#define IFX_AUTANALOG_CTDAC_VREF_PRB_OUT1 7     /**< PRB output 1 */
/** @} */

/**
 * @name Topology (cy_en_autanalog_dac_topo_cfg_t)
 * @{
 */
#define IFX_AUTANALOG_CTDAC_TOPO_DIRECT            0 /**< Direct output */
#define IFX_AUTANALOG_CTDAC_TOPO_DIRECT_TRACK_CAP  1 /**< Direct output with track capacitor */
#define IFX_AUTANALOG_CTDAC_TOPO_DIRECT_TRACK_HOLD 2 /**< Direct output with track-and-hold */
#define IFX_AUTANALOG_CTDAC_TOPO_BUFFERED_INTERNAL 3 /**< Internally buffered output */
#define IFX_AUTANALOG_CTDAC_TOPO_BUFFERED_EXTERNAL 4 /**< Externally buffered output */
/** @} */

/**
 * @name Waveform operating mode (cy_en_autanalog_dac_oper_mode_t)
 * @{
 */
#define IFX_AUTANALOG_CTDAC_OP_OS_ONE_QUAD    0 /**< One-shot, one quadrant */
#define IFX_AUTANALOG_CTDAC_OP_OS_TWO_QUAD    1 /**< One-shot, two quadrants */
#define IFX_AUTANALOG_CTDAC_OP_OS_FOUR_QUAD   2 /**< One-shot, four quadrants */
#define IFX_AUTANALOG_CTDAC_OP_CONT_ONE_QUAD  3 /**< Continuous, one quadrant */
#define IFX_AUTANALOG_CTDAC_OP_CONT_TWO_QUAD  4 /**< Continuous, two quadrants */
#define IFX_AUTANALOG_CTDAC_OP_CONT_FOUR_QUAD 5 /**< Continuous, four quadrants */
#define IFX_AUTANALOG_CTDAC_OP_ADDR           6 /**< Address waveform table mode */
#define IFX_AUTANALOG_CTDAC_OP_DATA           7 /**< Data waveform table mode */
/** @} */

/**
 * @name Step value selector (cy_en_autanalog_dac_step_sel_t)
 * @{
 */
#define IFX_AUTANALOG_CTDAC_STEP_SEL_DISABLED 0 /**< Step selection disabled */
#define IFX_AUTANALOG_CTDAC_STEP_SEL_0        1 /**< Use step value 0 */
#define IFX_AUTANALOG_CTDAC_STEP_SEL_1        2 /**< Use step value 1 */
#define IFX_AUTANALOG_CTDAC_STEP_SEL_2        3 /**< Use step value 2 */
/** @} */

/**
 * @name Status and range detection selector (cy_en_autanalog_dac_stat_sel_t)
 * @{
 */
#define IFX_AUTANALOG_CTDAC_STAT_SEL_DISABLED 0 /**< Status selection disabled */
#define IFX_AUTANALOG_CTDAC_STAT_SEL_0        1 /**< Use status selector 0 */
#define IFX_AUTANALOG_CTDAC_STAT_SEL_1        2 /**< Use status selector 1 */
#define IFX_AUTANALOG_CTDAC_STAT_SEL_2        3 /**< Use status selector 2 */
/** @} */

/**
 * @name Channel limit and range detection condition (cy_en_autanalog_dac_limit_t)
 * @{
 */
#define IFX_AUTANALOG_CTDAC_LIMIT_BELOW   0 /**< Result is below the configured range */
#define IFX_AUTANALOG_CTDAC_LIMIT_INSIDE  1 /**< Result is inside the configured range */
#define IFX_AUTANALOG_CTDAC_LIMIT_ABOVE   2 /**< Result is above the configured range */
#define IFX_AUTANALOG_CTDAC_LIMIT_OUTSIDE 3 /**< Result is outside the configured range */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DAC_INFINEON_AUTANALOG_CTDAC_H_ */
