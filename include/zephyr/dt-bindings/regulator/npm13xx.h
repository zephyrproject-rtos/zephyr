/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup regulator_npm13xx
 * @brief Header file for nPM13xx Devicetree helpers.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM13XX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM13XX_H_

/**
 * @defgroup regulator_npm13xx nPM13xx Devicetree helpers
 * @brief Nordic nPM13 Series PMIC regulator driver Devicetree helpers
 * @ingroup devicetree-regulator
 * @{
 */

/**
 * @name nPM13xx BUCK regulator modes
 * @{
 */
/** Automatic mode */
#define NPM13XX_BUCK_MODE_AUTO 0x00U
/** PWM mode */
#define NPM13XX_BUCK_MODE_PWM  0x01U
/** PFM mode */
#define NPM13XX_BUCK_MODE_PFM  0x04U
/** @} */

/**
 * @name nPM13xx LDSW/LDO regulator modes
 * @{
 */
/** LDO mode */
#define NPM13XX_LDSW_MODE_LDO  0x02U
/** Load switch mode */
#define NPM13XX_LDSW_MODE_LDSW 0x03U
/** @} */

/**
 * @name nPM13xx GPIO channel selection
 * @{
 */
/** No GPIO channel */
#define NPM13XX_GPIO_CHAN_NONE 0x00U
/** GPIO channel 0 */
#define NPM13XX_GPIO_CHAN_0    0x01U
/** GPIO channel 1 */
#define NPM13XX_GPIO_CHAN_1    0x02U
/** GPIO channel 2 */
#define NPM13XX_GPIO_CHAN_2    0x03U
/** GPIO channel 3 */
#define NPM13XX_GPIO_CHAN_3    0x04U
/** GPIO channel 4 */
#define NPM13XX_GPIO_CHAN_4    0x05U
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM13XX_H_*/
