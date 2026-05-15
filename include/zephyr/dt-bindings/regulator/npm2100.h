/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup regulator_npm2100
 * @brief Header file for nPM2100 Devicetree helpers.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM2100_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM2100_H_

/**
 * @defgroup regulator_npm2100 nPM2100 Devicetree helpers
 * @brief Nordic nPM2100 PMIC regulator driver Devicetree helpers
 * @ingroup devicetree-regulator
 * @{
 */

/**
 * @name nPM2100 LDOSW regulator modes
 * @{
 */
/** Enable load switch */
#define NPM2100_REG_LDSW_EN 0x01U
/** @} */

/**
 * @name nPM2100 BOOST DPS modes
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define NPM2100_REG_DPS_MASK 0x03U
/** @endcond */
/** Allow dynamic power scaling */
#define NPM2100_REG_DPS_ALLOW   0x01U
/** Allow dynamic power scaling in low power */
#define NPM2100_REG_DPS_ALLOWLP 0x02U
/** @} */

/**
 * @name nPM2100 regulator operating modes
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define NPM2100_REG_OPER_MASK 0x1CU
/** @endcond */
/** Automatic mode */
#define NPM2100_REG_OPER_AUTO 0x00U
/** High power mode */
#define NPM2100_REG_OPER_HP   0x04U
/** Low power mode */
#define NPM2100_REG_OPER_LP   0x08U
/** Ultra-low power mode */
#define NPM2100_REG_OPER_ULP  0x0CU
/** Pass-through mode */
#define NPM2100_REG_OPER_PASS 0x10U
/** No high power mode */
#define NPM2100_REG_OPER_NOHP 0x14U
/** Off mode */
#define NPM2100_REG_OPER_OFF  0x18U
/** @} */

/**
 * @name nPM2100 regulator forced modes when GPIO active
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define NPM2100_REG_FORCE_MASK 0xE0U
/** @endcond */
/** Force high performance mode */
#define NPM2100_REG_FORCE_HP   0x20U
/** Force low power mode */
#define NPM2100_REG_FORCE_LP   0x40U
/** Force ultra-low power mode */
#define NPM2100_REG_FORCE_ULP  0x60U
/** Force pass-through mode */
#define NPM2100_REG_FORCE_PASS 0x80U
/** Force no high performance mode */
#define NPM2100_REG_FORCE_NOHP 0xA0U
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM2100_H_*/
