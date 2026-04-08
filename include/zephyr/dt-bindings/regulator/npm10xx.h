/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM10XX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM10XX_H_

#include <zephyr/sys/util_macro.h>

/**
 * @file npm10xx.h
 * @brief Nordic's nPM10 Series PMIC regulator driver Devicetree helpers
 * @defgroup regulator_npm10xx nPM10xx Devicetree helpers.
 * @ingroup regulator_interface
 * @{
 */

/**
 * @name nPM10xx BUCK regulator modes
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define NPM10XX_BUCK_PWRMODE_Msk  BIT_MASK(2)
#define NPM10XX_BUCK_PASSTHRU_Msk (BIT_MASK(2) << 2)
/** @endcond */
/** BUCK Low Power mode */
#define NPM10XX_BUCK_PWRMODE_LP   BIT(0)
/** BUCK Ultra-Low Power mode */
#define NPM10XX_BUCK_PWRMODE_ULP  BIT(1)
/** BUCK disable Pass-through mode */
#define NPM10XX_BUCK_PASSTHRU_OFF BIT(2)
/** BUCK enable Pass-through mode */
#define NPM10XX_BUCK_PASSTHRU_ON  BIT(3)
/** @} */

/**
 * @name nPM10xx LDO regulator modes
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define NPM10XX_LDO_MODE_Msk BIT_MASK(2)
/** @endcond */
/** LDO mode */
#define NPM10XX_LDO_MODE_LDO BIT(0)
/** Load Switch mode */
#define NPM10XX_LDO_MODE_LS  BIT(1)
/** @} */

/**
 * @name nPM10xx regulator GPIO control
 * @{
 *
 * Multiple GPIO control flags can be combined on the BUCK regulator.
 */
/** @cond INTERNAL_HIDDEN */
#define NPM10XX_REG_GPIO_Msk      (BIT_MASK(4) << 4)
/** @endcond */
/** Turn on enable GPIO control of a regulator */
#define NPM10XX_REG_GPIO_EN       BIT(4)
/** Turn on power mode switching GPIO control of BUCK */
#define NPM10XX_BUCK_GPIO_PWRMODE BIT(5)
/** Turn on alternate VOUT switching GPIO control of BUCK */
#define NPM10XX_BUCK_GPIO_ALTVOUT BIT(6)
/** Disable all GPIO control of a regulator. Cannot be combined with other GPIO flags */
#define NPM10XX_REG_GPIO_NONE     BIT(7)
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM10XX_H_ */
