/*
 * Copyright (c) 2025 Bavariamatic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock identifiers for Microchip SmartFusion2
 * @ingroup microchip_smartfusion2_clocks
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MICROCHIP_SMARTFUSION2_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MICROCHIP_SMARTFUSION2_CLOCK_H_

/**
 * @defgroup microchip_smartfusion2_clocks Microchip SmartFusion2 clock identifiers
 * @brief Clock identifiers for the Microchip SmartFusion2 SoC.
 * @ingroup devicetree-clocks
 * @{
 */

/** @brief Main CPU clock. */
#define SMARTFUSION2_CLOCK_CPU             0
/** @brief MSS peripheral clock. */
#define SMARTFUSION2_CLOCK_MSS_PERIPHERAL  1

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MICROCHIP_SMARTFUSION2_CLOCK_H_ */
