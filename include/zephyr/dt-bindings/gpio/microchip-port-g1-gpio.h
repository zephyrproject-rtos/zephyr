/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree GPIO flag definitions for Microchip PORT (G1).
 *
 * Provides Microchip-specific GPIO DT flags used in addition to standard Zephyr
 * GPIO binding flags.
 */

#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_MICROCHIP_PORT_G1_GPIO_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_MICROCHIP_PORT_G1_GPIO_H_

/**
 * @def MCHP_GPIO_DEBOUNCE
 * @brief Enable hardware debouncing for a GPIO pin.
 *
 * Zephyr-specific devicetree flag for Microchip SoCs. When set in GPIO DT flags, the
 * driver enables the SoC's debounce feature for the configured pin/interrupt line.
 */
#define MCHP_GPIO_DEBOUNCE (1U << 8)

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_MICROCHIP_PORT_G1_GPIO_H_ */
