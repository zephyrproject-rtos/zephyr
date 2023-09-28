/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NXP_VREF_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NXP_VREF_H

/**
 * @defgroup regulator_nxp_vref Devicetree helpers
 * @ingroup regulator_interface
 * @{
 */

/**
 * @name NXP VREF Regulator API Modes
 * @{
 */
#define NXP_VREF_MODE_STANDBY 0
#define NXP_VREF_MODE_LOW_POWER 1
#define NXP_VREF_MODE_HIGH_POWER 2
#define NXP_VREF_MODE_INTERNAL_REGULATOR 3

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NXP_VREF_H */
