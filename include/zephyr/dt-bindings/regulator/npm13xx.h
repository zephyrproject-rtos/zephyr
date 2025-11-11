/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM13XX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM13XX_H_

/**
 * @defgroup regulator_npm13xx nPM13xx Devicetree helpers.
 * @ingroup regulator_interface
 * @{
 */

/**
 * @name nPM13xx Regulator modes
 * @{
 */
/* Buck modes */
#define NPM13XX_BUCK_MODE_AUTO 0x00U
#define NPM13XX_BUCK_MODE_PWM  0x01U
#define NPM13XX_BUCK_MODE_PFM  0x04U

/* LDSW / LDO modes */
#define NPM13XX_LDSW_MODE_LDO  0x02U
#define NPM13XX_LDSW_MODE_LDSW 0x03U

/* GPIO control configuration */
#define NPM13XX_GPIO_CHAN_NONE 0x00U
#define NPM13XX_GPIO_CHAN_0    0x01U
#define NPM13XX_GPIO_CHAN_1    0x02U
#define NPM13XX_GPIO_CHAN_2    0x03U
#define NPM13XX_GPIO_CHAN_3    0x04U
#define NPM13XX_GPIO_CHAN_4    0x05U

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM13XX_H_*/
