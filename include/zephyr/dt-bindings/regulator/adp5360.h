/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_ADP5360_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_ADP5360_H_

/**
 * @defgroup regulator_adp5360 ADP5360 Devicetree helpers.
 * @ingroup regulator_interface
 * @{
 */

/**
 * @name ADP5360 Regulator modes
 * @{
 */
/** Hysteresis mode */
#define ADP5360_MODE_HYS 0
/** PWM mode */
#define ADP5360_MODE_PWM 1
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_ADP5360_H_*/
