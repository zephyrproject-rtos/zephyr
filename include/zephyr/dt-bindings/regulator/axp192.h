/*
 * Copyright (c) 2023 Martin Kiepfer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup regulator_axp192
 * @brief Header file for AXP192 Devicetree helpers.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_AXP192_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_AXP192_H_

/**
 * @defgroup regulator_axp192 AXP192 Devicetree helpers
 * @brief X-Powers AXP192 PMIC regulator driver Devicetree helpers
 * @ingroup devicetree-regulator
 * @{
 */

/**
 * @name AXP192 DCDC modes
 * @{
 */
/** Automatic mode */
#define AXP192_DCDC_MODE_AUTO 0x00U
/** PWM mode */
#define AXP192_DCDC_MODE_PWM  0x01U

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_AXP192_H_ */
