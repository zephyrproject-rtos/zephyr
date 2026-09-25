/*
 * SPDX-FileCopyrightText: 2026 Aesc Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock domain identifiers for the Aesc Silicon ElemRV-H SoC
 * @ingroup aesc_clock_controller
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AESC_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AESC_CLOCK_H_

/**
 * @defgroup aesc_clock_controller Aesc Silicon clock domain identifiers
 * @brief Clock domain identifiers for the Aesc Silicon ElemRV-H SoC.
 * @ingroup devicetree-clocks
 *
 * These identifiers are used as the single clock specifier cell of an
 * @c aesc,clock-controller node, e.g.
 *
 *     clocks = <&clkctrl HYDROGEN_CLK_SYSTEM>;
 *
 * Each identifier is the zero-based index of the clock domain and matches the
 * order of the domain list in the SoC.
 *
 * @{
 */

/** @brief System clock domain, drives the CPU and the peripheral bus. */
#define HYDROGEN_CLK_SYSTEM 0
/** @brief Debug clock domain, drives the debug module. */
#define HYDROGEN_CLK_DEBUG  1

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AESC_CLOCK_H_ */
