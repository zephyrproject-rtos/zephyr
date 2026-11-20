/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Deere & Company
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup regulator_tle927x
 * @brief Header file for TLE927x Devicetree helpers.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_TLE927X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_TLE927X_H_

/**
 * @defgroup regulator_tle927x TLE927x Devicetree helpers
 * @brief Infineon TLE927x VCC2 regulator driver Devicetree helpers
 * @ingroup devicetree-regulator
 * @{
 */

/**
 * @name TLE927x VCC2 regulator modes
 *
 * The mode selects the set of SBC modes the VCC2 rail stays on in. The values
 * are the M_S_CTRL.VCC2_ON field encoding; 0 is the OFF encoding and is
 * therefore not a mode.
 * @{
 */
#define TLE927X_VCC2_MODE_NORMAL      1 /**< VCC2 on in Normal mode */
#define TLE927X_VCC2_MODE_NORMAL_STOP 2 /**< VCC2 on in Normal and Stop mode */
#define TLE927X_VCC2_MODE_ALL         3 /**< VCC2 always on, Sleep mode included */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_TLE927X_H_ */
