/*
 * Copyright 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup regulator_mspm0_vref
 * @brief Header file for MSPM0 VREF Devicetree helpers.*
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_MSPM0_VREF_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_MSPM0_VREF_H

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @defgroup regulator_mspm0_vref MSPM0 VREF Devicetree helpers
 * @brief Texas Instruments MSPM0 VREF regulator driver Devicetree helpers
 * @ingroup devicetree-regulator
 * @{
 */

/**
 * @name MSPM0 VREF regulator modes
 * @{
 */
/** Normal operating mode */
#define MSPM0_VREF_MODE_NORMAL     0
/** Sample and hold mode */
#define MSPM0_VREF_MODE_SHMODE     BIT(0)
/** Voltage-to-current buffer mode. */
#define MSPM0_VREF_MODE_V2I        BIT(1)
/** Sample and hold combined with voltage-to-current buffer mode */
#define MSPM0_VREF_MODE_SHMODE_V2I (MSPM0_VREF_MODE_SHMODE | MSPM0_VREF_MODE_V2I)

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_MSPM0_VREF_H */
