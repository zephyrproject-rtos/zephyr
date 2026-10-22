/*
 * Copyright 2023, 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup regulator_nxp_vref
 * @brief Header file for NXP VREF Devicetree helpers.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NXP_VREF_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NXP_VREF_H

/**
 * @defgroup regulator_nxp_vref NXP VREF Devicetree helpers
 * @brief NXP VREF regulator driver Devicetree helpers
 * @ingroup devicetree-regulator
 * @{
 */

/**
 * @name NXP VREF regulator modes
 * @{
 */
/** Standby mode */
#define NXP_VREF_MODE_STANDBY 0
/** Low power mode */
#define NXP_VREF_MODE_LOW_POWER 1
/** High power mode */
#define NXP_VREF_MODE_HIGH_POWER 2

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NXP_VREF_H */
