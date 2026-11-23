/*
 * Copyright (c) 2023 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup regulator_max20335
 * @brief Header file for MAX20335 Devicetree helpers.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_MAX20335_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_MAX20335_H_

/**
 * @defgroup regulator_max20335 MAX20335 Devicetree helpers
 * @brief Maxim MAX20335 PMIC regulator driver Devicetree helpers
 * @ingroup devicetree-regulator
 * @{
 */

/**
 * @name MAX20335 regulator modes
 * @{
 */
/** LDO mode */
#define MAX20335_LDO_MODE 0
/** Load switch mode */
#define MAX20335_LOAD_SWITCH_MODE 1
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_MAX20335_H_*/
