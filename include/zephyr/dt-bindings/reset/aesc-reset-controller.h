/*
 * SPDX-FileCopyrightText: 2026 Aesc Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Reset domain identifiers for the Aesc Silicon ElemRV-H SoC
 * @ingroup aesc_reset_controller
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_AESC_RESET_CONTROLLER_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_AESC_RESET_CONTROLLER_H_

/**
 * @defgroup aesc_reset_controller Aesc Silicon reset domain identifiers
 * @brief Reset domain identifiers for the Aesc Silicon ElemRV-H SoC.
 * @ingroup devicetree-reset-controller
 *
 * These identifiers are used as the single reset specifier cell of an
 * @c aesc,reset-controller node, e.g.
 *
 *     resets = <&rstctl HYDROGEN_RESET_FLASH>;
 *
 * Each identifier is the zero-based index of the reset domain and matches the
 * order of the domain list in the SoC.
 *
 * @{
 */

/** @brief System reset domain, resets the CPU and the peripherals. */
#define HYDROGEN_RESET_SYSTEM 0
/** @brief Debug reset domain, resets the debug module. */
#define HYDROGEN_RESET_DEBUG  1
/** @brief Flash reset domain, resets the external SPI flash. */
#define HYDROGEN_RESET_FLASH  2

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_AESC_RESET_CONTROLLER_H_ */
