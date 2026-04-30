/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for Infineon AutAnalog CTB.
 *
 * These constants match the PDL enum values and are intended for use in
 * devicetree properties that configure the AutAnalog continuous-time block
 * opamps and comparator features.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_OPAMP_INFINEON_AUTANALOG_CTB_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_OPAMP_INFINEON_AUTANALOG_CTB_H_

/**
 * @name Opamp power modes (cy_en_autanalog_ctb_oa_pwr_t)
 * @{
 */
#define IFX_AUTANALOG_CTB_OA_PWR_OFF             0  /**< Opamp powered off */
#define IFX_AUTANALOG_CTB_OA_PWR_ULTRA_LOW       1  /**< Ultra low power, charge pump off */
#define IFX_AUTANALOG_CTB_OA_PWR_ULTRA_LOW_RAIL  2  /**< Ultra low power, charge pump on */
#define IFX_AUTANALOG_CTB_OA_PWR_LOW_RAIL        4  /**< Low power, charge pump on */
#define IFX_AUTANALOG_CTB_OA_PWR_MEDIUM_RAIL     6  /**< Medium power, charge pump on */
#define IFX_AUTANALOG_CTB_OA_PWR_HIGH_RAIL       8  /**< High power, charge pump on */
#define IFX_AUTANALOG_CTB_OA_PWR_ULTRA_HIGH_RAIL 10 /**< Ultra high power, charge pump on */
/** @} */

/**
 * @name Opamp topologies (cy_en_autanalog_ctb_oa_topo_t)
 * @{
 */
#define IFX_AUTANALOG_CTB_OA_TOPO_COMPARATOR      0 /**< Comparator topology */
#define IFX_AUTANALOG_CTB_OA_TOPO_PGA             1 /**< Programmable gain amplifier topology */
#define IFX_AUTANALOG_CTB_OA_TOPO_TIA             2 /**< Transimpedance amplifier topology */
#define IFX_AUTANALOG_CTB_OA_TOPO_OPEN_LOOP_OPAMP 3 /**< Open-loop opamp topology */
#define IFX_AUTANALOG_CTB_OA_TOPO_DIFF_AMPLIFIER  4 /**< Differential amplifier topology */
#define IFX_AUTANALOG_CTB_OA_TOPO_HYST_COMPARATOR 5 /**< Hysteresis comparator topology */
#define IFX_AUTANALOG_CTB_OA_TOPO_BUFFER          6 /**< Voltage follower / buffer topology */
/** @} */

/**
 * @name Comparator interrupt and edge detection modes (cy_en_autanalog_ctb_comp_int_t)
 * @{
 */
#define IFX_AUTANALOG_CTB_COMP_INT_DISABLED     0 /**< Interrupt disabled */
#define IFX_AUTANALOG_CTB_COMP_INT_EDGE_RISING  1 /**< Rising edge detection */
#define IFX_AUTANALOG_CTB_COMP_INT_EDGE_FALLING 2 /**< Falling edge detection */
#define IFX_AUTANALOG_CTB_COMP_INT_EDGE_BOTH    3 /**< Both-edge detection */
/** @} */

/**
 * @name Feedback capacitor (cy_en_autanalog_ctb_oa_fb_cap_t)
 * @{
 */
#define IFX_AUTANALOG_CTB_OA_FB_CAP_0_pF    0  /**< 0.0 pF feedback capacitor */
#define IFX_AUTANALOG_CTB_OA_FB_CAP_0_7_pF  1  /**< 0.7 pF feedback capacitor */
#define IFX_AUTANALOG_CTB_OA_FB_CAP_1_4_pF  2  /**< 1.4 pF feedback capacitor */
#define IFX_AUTANALOG_CTB_OA_FB_CAP_2_1_pF  3  /**< 2.1 pF feedback capacitor */
#define IFX_AUTANALOG_CTB_OA_FB_CAP_2_8_pF  4  /**< 2.8 pF feedback capacitor */
#define IFX_AUTANALOG_CTB_OA_FB_CAP_3_5_pF  5  /**< 3.5 pF feedback capacitor */
#define IFX_AUTANALOG_CTB_OA_FB_CAP_4_2_pF  6  /**< 4.2 pF feedback capacitor */
#define IFX_AUTANALOG_CTB_OA_FB_CAP_4_9_pF  7  /**< 4.9 pF feedback capacitor */
#define IFX_AUTANALOG_CTB_OA_FB_CAP_5_6_pF  8  /**< 5.6 pF feedback capacitor */
#define IFX_AUTANALOG_CTB_OA_FB_CAP_6_3_pF  9  /**< 6.3 pF feedback capacitor */
#define IFX_AUTANALOG_CTB_OA_FB_CAP_7_0_pF  10 /**< 7.0 pF feedback capacitor */
#define IFX_AUTANALOG_CTB_OA_FB_CAP_7_7_pF  11 /**< 7.7 pF feedback capacitor */
#define IFX_AUTANALOG_CTB_OA_FB_CAP_8_4_pF  12 /**< 8.4 pF feedback capacitor */
#define IFX_AUTANALOG_CTB_OA_FB_CAP_9_1_pF  13 /**< 9.1 pF feedback capacitor */
#define IFX_AUTANALOG_CTB_OA_FB_CAP_9_8_pF  14 /**< 9.8 pF feedback capacitor */
#define IFX_AUTANALOG_CTB_OA_FB_CAP_10_5_pF 15 /**< 10.5 pF feedback capacitor */
/** @} */

/**
 * @name Compensation capacitor (cy_en_autanalog_ctb_oa_cc_cap_t)
 * @{
 */
#define IFX_AUTANALOG_CTB_OA_CC_CAP_0_1_pF   0 /**< 0.1 pF compensation capacitor */
#define IFX_AUTANALOG_CTB_OA_CC_CAP_1_1_pF   1 /**< 1.1 pF compensation capacitor */
#define IFX_AUTANALOG_CTB_OA_CC_CAP_2_1_pF   2 /**< 2.1 pF compensation capacitor */
#define IFX_AUTANALOG_CTB_OA_CC_CAP_3_1_pF   3 /**< 3.1 pF compensation capacitor */
#define IFX_AUTANALOG_CTB_OA_CC_CAP_4_1_pF   4 /**< 4.1 pF compensation capacitor */
#define IFX_AUTANALOG_CTB_OA_CC_CAP_5_1_pF   5 /**< 5.1 pF compensation capacitor */
#define IFX_AUTANALOG_CTB_OA_CC_CAP_6_1_pF   6 /**< 6.1 pF compensation capacitor */
#define IFX_AUTANALOG_CTB_OA_CC_CAP_7_1_pF   7 /**< 7.1 pF compensation capacitor */
#define IFX_AUTANALOG_CTB_OA_CC_CAP_DISABLED 8 /**< Compensation capacitor disabled */
/** @} */

/**
 * @name Non-inverting input pin connection (cy_en_autanalog_ctb_oa_ninv_pin_t)
 * @{
 */
#define IFX_AUTANALOG_CTB_OA_NINV_PIN_DISCONNECT    0 /**< Pin disconnected */
#define IFX_AUTANALOG_CTB_OA_NINV_PIN_OA0_P0_OA1_P5 1 /**< OA0 P0 or OA1 P5 pin */
#define IFX_AUTANALOG_CTB_OA_NINV_PIN_OA0_P1_OA1_P4 2 /**< OA0 P1 or OA1 P4 pin */
/** @} */

/**
 * @name Non-inverting input reference connection (cy_en_autanalog_ctb_oa_ninv_ref_t)
 * @{
 */
#define IFX_AUTANALOG_CTB_OA_NINV_REF_DISCONNECT  0 /**< Reference disconnected */
#define IFX_AUTANALOG_CTB_OA_NINV_REF_DAC0        1 /**< DAC0 output */
#define IFX_AUTANALOG_CTB_OA_NINV_REF_DAC1        2 /**< DAC1 output */
#define IFX_AUTANALOG_CTB_OA_NINV_REF_PRB_OUT0    3 /**< PRB output 0 */
#define IFX_AUTANALOG_CTB_OA_NINV_REF_PRB_OUT1    4 /**< PRB output 1 */
#define IFX_AUTANALOG_CTB_OA_NINV_REF_VBGR        5 /**< Band-gap reference */
#define IFX_AUTANALOG_CTB_OA_NINV_REF_CTB_OA0_OUT 6 /**< CTB opamp 0 output */
#define IFX_AUTANALOG_CTB_OA_NINV_REF_CTB_OA1_OUT 7 /**< CTB opamp 1 output */
/** @} */

/**
 * @name Inverting input pin connection (cy_en_autanalog_ctb_oa_inv_pin_t)
 * @{
 */
#define IFX_AUTANALOG_CTB_OA_INV_PIN_DISCONNECT    0 /**< Pin disconnected */
#define IFX_AUTANALOG_CTB_OA_INV_PIN_OA0_P0_OA1_P5 1 /**< OA0 P0 or OA1 P5 pin */
#define IFX_AUTANALOG_CTB_OA_INV_PIN_OA0_P1_OA1_P4 2 /**< OA0 P1 or OA1 P4 pin */
/** @} */

/**
 * @name Resistor ladder bottom pin connection (cy_en_autanalog_ctb_oa_res_pin_t)
 * @{
 */
#define IFX_AUTANALOG_CTB_OA_RES_PIN_DISCONNECT    0 /**< Pin disconnected */
#define IFX_AUTANALOG_CTB_OA_RES_PIN_OA0_P0_OA1_P5 1 /**< OA0 P0 or OA1 P5 pin */
#define IFX_AUTANALOG_CTB_OA_RES_PIN_OA0_P1_OA1_P4 2 /**< OA0 P1 or OA1 P4 pin */
/** @} */

/**
 * @name Resistor ladder bottom reference connection (cy_en_autanalog_ctb_oa_res_ref_t)
 * @{
 */
#define IFX_AUTANALOG_CTB_OA_RES_REF_DISCONNECT  0 /**< Reference disconnected */
#define IFX_AUTANALOG_CTB_OA_RES_REF_DAC0        1 /**< DAC0 output */
#define IFX_AUTANALOG_CTB_OA_RES_REF_DAC1        2 /**< DAC1 output */
#define IFX_AUTANALOG_CTB_OA_RES_REF_CTB_OA0_OUT 3 /**< CTB opamp 0 output */
#define IFX_AUTANALOG_CTB_OA_RES_REF_CTB_OA1_OUT 4 /**< CTB opamp 1 output */
#define IFX_AUTANALOG_CTB_OA_RES_REF_VSSA        5 /**< Analog ground reference */
/** @} */

/**
 * @name Input multiplexer connection (cy_en_autanalog_ctb_oa_mux_in_t)
 * @{
 */
#define IFX_AUTANALOG_CTB_OA_MUX_IN_DISCONNECT 0 /**< Input mux disconnected */
#define IFX_AUTANALOG_CTB_OA_MUX_IN_P0         1 /**< Connect pin P0 */
#define IFX_AUTANALOG_CTB_OA_MUX_IN_P1         2 /**< Connect pin P1 */
#define IFX_AUTANALOG_CTB_OA_MUX_IN_P2         3 /**< Connect pin P2 */
#define IFX_AUTANALOG_CTB_OA_MUX_IN_P3         4 /**< Connect pin P3 */
#define IFX_AUTANALOG_CTB_OA_MUX_IN_P4         5 /**< Connect pin P4 */
#define IFX_AUTANALOG_CTB_OA_MUX_IN_P5         6 /**< Connect pin P5 */
#define IFX_AUTANALOG_CTB_OA_MUX_IN_P6         7 /**< Connect pin P6 */
#define IFX_AUTANALOG_CTB_OA_MUX_IN_P7         8 /**< Connect pin P7 */
/** @} */

/**
 * @name Output multiplexer connection (cy_en_autanalog_ctb_oa_mux_out_t)
 * @{
 */
#define IFX_AUTANALOG_CTB_OA_MUX_OUT_DISCONNECT 0 /**< Output mux disconnected */
#define IFX_AUTANALOG_CTB_OA_MUX_OUT_NINV       1 /**< Route non-inverting input */
#define IFX_AUTANALOG_CTB_OA_MUX_OUT_INV        2 /**< Route inverting input */
#define IFX_AUTANALOG_CTB_OA_MUX_OUT_RES        3 /**< Route resistor ladder node */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_OPAMP_INFINEON_AUTANALOG_CTB_H_ */
