/*
 * Copyright 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_MSPM0_VREF_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_MSPM0_VREF_H

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
#define MSPM0_VREF_MODE_NORMAL 0
/** Sample and hold mode */
#define MSPM0_VREF_MODE_SHMODE 1

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_MSPM0_VREF_H */
