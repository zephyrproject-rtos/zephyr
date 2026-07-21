/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup regulator_npm6001
 * @brief Header file for nPM6001 Devicetree helpers
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM6001_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM6001_H_

/**
 * @defgroup regulator_npm6001 nPM6001 Devicetree helpers
 * @brief Nordic nPM6001 PMIC regulator driver Devicetree helpers
 * @ingroup devicetree-regulator
 * @{
 */

/**
 * @name nPM6001 regulator modes
 * @{
 */
/** Hysteretic mode */
#define NPM6001_MODE_HYS 0
/** PWM mode */
#define NPM6001_MODE_PWM 1
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM6001_H_*/
