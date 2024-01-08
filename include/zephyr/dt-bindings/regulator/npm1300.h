/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM1300_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM1300_H_

/**
 * @defgroup regulator_npm1300 NPM1300 Devicetree helpers.
 * @ingroup regulator_interface
 * @{
 */

/**
 * @name NPM1300 Regulator modes
 * @{
 */
/* Buck modes */
#define NPM1300_BUCK_MODE_AUTO 0x00U
#define NPM1300_BUCK_MODE_PWM  0x01U
#define NPM1300_BUCK_MODE_PFM  0x04U

/* LDSW / LDO modes */
#define NPM1300_LDSW_MODE_LDO  0x02U
#define NPM1300_LDSW_MODE_LDSW 0x03U

/* GPIO control configuration */
#define NPM1300_GPIO_CHAN_NONE 0x00U
#define NPM1300_GPIO_CHAN_0    0x01U
#define NPM1300_GPIO_CHAN_1    0x02U
#define NPM1300_GPIO_CHAN_2    0x03U
#define NPM1300_GPIO_CHAN_3    0x04U
#define NPM1300_GPIO_CHAN_4    0x05U

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM1300_H_*/
