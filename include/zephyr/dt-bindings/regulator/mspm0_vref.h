/*
 * Copyright 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_MSPM0_VREF_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_MSPM0_VREF_H

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @file mspm0_vref.h
 * @brief MSPM0 VREF regulator devicetree helpers
 * @defgroup regulator_mspm0_vref Devicetree helpers
 * @ingroup regulator_interface
 * @{
 */

/**
 * @name MSPM0 VREF Regulator API Modes
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
