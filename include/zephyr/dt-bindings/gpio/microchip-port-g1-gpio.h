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

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_MICROCHIP_PORT_G1_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_MICROCHIP_PORT_G1_GPIO_H_

/**
 * @def MCHP_GPIO_DEBOUNCE
 * @brief Enable hardware debouncing for a GPIO pin.
 *
 * Zephyr-specific devicetree flag for Microchip SoCs. When set in GPIO DT flags, the
 * driver enables the SoC's debounce feature for the configured pin/interrupt line.
 */
#define MCHP_GPIO_DEBOUNCE (1U << 8)

/**
 * @def MCHP_GPIO_EVGEN_ENABLE
 * @brief Enable the gpio to generate event
 *
 * Zephyr-specific devicetree flag for Microchip SoCs. When set in GPIO DT flags, the
 * driver enables the pin to be an event generator. Inorder to use this flag, the interrupt
 * configure api has to be called
 */
#define MCHP_GPIO_EVGEN_ENABLE (1U << 7)

/**
 * @def MCHP_GPIO_SET_ON_EVENT
 * @brief Set the GPIO output when an event is received.
 */
#define MCHP_GPIO_SET_ON_EVENT (1U << 6)

/**
 * @def MCHP_GPIO_OUT_ON_EVENT
 * @brief Drive the GPIO output according to the event signal.
 */
#define MCHP_GPIO_OUT_ON_EVENT (1U << 5)

/**
 * @def MCHP_GPIO_CLR_ON_EVENT
 * @brief Clear the GPIO output when an event is received.
 */
#define MCHP_GPIO_CLR_ON_EVENT (1U << 4)

/**
 * @def MCHP_GPIO_TGL_ON_EVENT
 * @brief Toggle the GPIO output when an event is received.
 */
#define MCHP_GPIO_TGL_ON_EVENT (1U << 3)

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_MICROCHIP_PORT_G1_GPIO_H_ */
