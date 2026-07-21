/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup regulator_npm1100
 * @brief Header file for nPM1100 Devicetree helpers.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM1100_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM1100_H_

/**
 * @defgroup regulator_npm1100 nPM1100 Devicetree helpers
 * @brief Nordic nPM1100 PMIC regulator driver Devicetree helpers
 * @ingroup devicetree-regulator
 * @{
 */

/**
 * @name nPM1100 regulator modes
 * @{
 */
/** Automatic mode */
#define NPM1100_MODE_AUTO 0
/** PWM mode */
#define NPM1100_MODE_PWM 1
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM1100_H_*/
