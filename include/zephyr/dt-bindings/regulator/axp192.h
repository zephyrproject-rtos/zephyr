/*
 * Copyright (c) 2023 Martin Kiepfer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_AXP192_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_AXP192_H_

/**
 * @defgroup regulator_axp192 AXP192 Devicetree helpers.
 * @ingroup regulator_interface
 * @{
 */

/**
 * @name AXP192 Regulator modes
 * @{
 */
/* DCDCs */
#define AXP192_DCDC_MODE_AUTO 0x00U
#define AXP192_DCDC_MODE_PWM  0x01U

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_AXP192_H_ */
