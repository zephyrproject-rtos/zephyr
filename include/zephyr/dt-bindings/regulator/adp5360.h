/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup regulator_adp5360
 * @brief Header file for ADP5360 Devicetree helpers.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_ADP5360_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_ADP5360_H_

/**
 * @defgroup regulator_adp5360 ADP5360 Devicetree helpers
 * @brief Analog Devices ADP5360 PMIC regulator driver Devicetree helpers
 * @ingroup devicetree-regulator
 * @{
 */

/**
 * @name ADP5360 regulator modes
 * @{
 */
/** Hysteresis mode */
#define ADP5360_MODE_HYS 0
/** PWM mode */
#define ADP5360_MODE_PWM 1
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_ADP5360_H_*/
