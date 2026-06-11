/*
 * Copyright (c) 2025 Bavariamatic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file microchip-smartfusion2-clock.h
 * @brief SmartFusion2 clock binding identifiers.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MICROCHIP_SMARTFUSION2_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MICROCHIP_SMARTFUSION2_CLOCK_H_

/**
 * @defgroup smartfusion2_clock SmartFusion2 Clock Bindings
 * @brief SmartFusion2 SoC clock assignments.
 * @{
 */

/** @brief Main CPU clock. */
#define SMARTFUSION2_CLOCK_CPU             0
/** @brief MSS peripheral clock. */
#define SMARTFUSION2_CLOCK_MSS_PERIPHERAL  1

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MICROCHIP_SMARTFUSION2_CLOCK_H_ */
