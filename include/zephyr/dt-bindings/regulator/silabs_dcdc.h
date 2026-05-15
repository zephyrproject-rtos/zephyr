/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup regulator_silabs_dcdc
 * @brief Header file for Silicon Labs DCDC regulator Devicetree helpers.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_SILABS_DCDC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_SILABS_DCDC_H_

/**
 * @defgroup regulator_silabs_dcdc Silicon Labs DCDC Devicetree helpers
 * @brief Silicon Labs DCDC regulator driver Devicetree helpers
 * @ingroup devicetree-regulator
 * @{
 */

/**
 * @name Silicon Labs DCDC modes
 * @{
 */
/** Buck mode */
#define SILABS_DCDC_MODE_BUCK  0
/** Boost mode */
#define SILABS_DCDC_MODE_BOOST 1
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_SILABS_DCDC_H_ */
