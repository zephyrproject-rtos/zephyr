/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for Infineon AutAnalog PRB regulators.
 *
 * These constants match the PDL enum values and are intended for use in
 * devicetree properties that configure the PRB voltage reference outputs.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_INFINEON_AUTANALOG_PRB_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_INFINEON_AUTANALOG_PRB_H_

/**
 * @name Source voltage selectors (cy_en_autanalog_prb_src_t)
 * @{
 */
#define IFX_AUTANALOG_PRB_SRC_VBGR 0 /**< VBGR source, nominal 0.9 V */
#define IFX_AUTANALOG_PRB_SRC_VDDA 1 /**< VDDA source, max 1.8 V */
/** @} */

/**
 * @name Tap position selectors (cy_en_autanalog_prb_tap_t)
 *
 * PRB output voltage is computed as `((tap + 1) / 16) * Vsrc`.
 * @{
 */
#define IFX_AUTANALOG_PRB_TAP_0  0  /**< 0.0625 * Vsrc */
#define IFX_AUTANALOG_PRB_TAP_1  1  /**< 0.1250 * Vsrc */
#define IFX_AUTANALOG_PRB_TAP_2  2  /**< 0.1875 * Vsrc */
#define IFX_AUTANALOG_PRB_TAP_3  3  /**< 0.2500 * Vsrc */
#define IFX_AUTANALOG_PRB_TAP_4  4  /**< 0.3125 * Vsrc */
#define IFX_AUTANALOG_PRB_TAP_5  5  /**< 0.3750 * Vsrc */
#define IFX_AUTANALOG_PRB_TAP_6  6  /**< 0.4375 * Vsrc */
#define IFX_AUTANALOG_PRB_TAP_7  7  /**< 0.5000 * Vsrc */
#define IFX_AUTANALOG_PRB_TAP_8  8  /**< 0.5625 * Vsrc */
#define IFX_AUTANALOG_PRB_TAP_9  9  /**< 0.6250 * Vsrc */
#define IFX_AUTANALOG_PRB_TAP_10 10 /**< 0.6875 * Vsrc */
#define IFX_AUTANALOG_PRB_TAP_11 11 /**< 0.7500 * Vsrc */
#define IFX_AUTANALOG_PRB_TAP_12 12 /**< 0.8125 * Vsrc */
#define IFX_AUTANALOG_PRB_TAP_13 13 /**< 0.8750 * Vsrc */
#define IFX_AUTANALOG_PRB_TAP_14 14 /**< 0.9375 * Vsrc */
#define IFX_AUTANALOG_PRB_TAP_15 15 /**< 1.0000 * Vsrc */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_INFINEON_AUTANALOG_PRB_H_ */
