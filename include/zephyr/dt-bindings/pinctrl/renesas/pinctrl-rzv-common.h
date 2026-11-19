/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree pin control helpers for Renesas RZ/V
 * @ingroup pinctrl_rzv
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZV_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZV_COMMON_H_

/**
 * @addtogroup renesas_pinctrl Renesas pin control helpers
 * @ingroup devicetree-pinctrl
 */

/**
 * @defgroup pinctrl_rzv Renesas RZ/V pin control helpers
 * @brief Macros for pin control configuration of Renesas RZ/V SoCs
 * @ingroup renesas_pinctrl
 *
 * General-purpose pins are configured with the RZV_PINMUX() macro, which takes
 * three fields:
 *
 * - @c port — the port identifier, one of the @c PORT_* values.
 * - @c pin — the pin number within that port.
 * - @c func — the alternate-function number selecting the peripheral signal
 *   routed to the pin, taken from the SoC's Pin Function Controller table; it
 *   has no symbolic macro and is passed as a plain integer.
 *
 * Dedicated-function pins that sit outside the general-purpose port matrix are
 * referenced directly through their @c BSP_IO_* identifiers.
 *
 * An optional digital noise filter can be applied to a pin group with
 * RZV_FILTER_SET(), built from a stage count (@c RZV_FILNUM_*) and a sampling
 * clock divider (@c RZV_FILCLKSEL_*).
 *
 * This header only holds the macros shared by all RZ/V SoCs. The ports and
 * the dedicated-function pins differ between SoCs and are provided by a header
 * per SoC, which includes this one:
 *
 * - @c pinctrl-rzv2l.h — RZ/V2L
 * - @c pinctrl-rzv2h.h — RZ/V2H
 * - @c pinctrl-rzv2n.h — RZ/V2N
 *
 * @code{.dts}
 * #include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-rzv2l.h>
 *
 * &pinctrl {
 *         scif0_default: scif0_default {
 *                 device-pinmux {
 *                         pinmux = <RZV_PINMUX(PORT_04, 0, 2)>,
 *                                  <RZV_PINMUX(PORT_04, 1, 2)>;
 *                         bias-pull-up;
 *                         renesas,filter =
 *                                 RZV_FILTER_SET(RZV_FILNUM_8_STAGE, RZV_FILCLKSEL_DIV_18000);
 *                 };
 *         };
 * };
 * @endcode
 *
 * @{
 */

/**
 * @brief Create an encoded value containing port/pin/function information.
 *
 * @param port Port identifier (PORT_*).
 * @param pin Pin number within the port.
 * @param func Pin function selector.
 *
 * @return Encoded pinmux value.
 */
#define RZV_PINMUX(port, pin, func) ((port) | (pin) | ((func) << 4))

/**
 * @name Renesas RZ/V digital noise filter options
 *
 * Set the number of filter stages (FILNUM) and the sampling clock divider
 * (FILCLKSEL).
 * @{
 */

#define RZV_FILNUM_4_STAGE  0 /**< FILNUM: 4-stage filter */
#define RZV_FILNUM_8_STAGE  1 /**< FILNUM: 8-stage filter */
#define RZV_FILNUM_12_STAGE 2 /**< FILNUM: 12-stage filter */
#define RZV_FILNUM_16_STAGE 3 /**< FILNUM: 16-stage filter */

#define RZV_FILCLKSEL_NOT_DIV   0 /**< FILCLKSEL: no division */
#define RZV_FILCLKSEL_DIV_9000  1 /**< FILCLKSEL: divided by 9000 */
#define RZV_FILCLKSEL_DIV_18000 2 /**< FILCLKSEL: divided by 18000 */
#define RZV_FILCLKSEL_DIV_36000 3 /**< FILCLKSEL: divided by 36000 */

/** @} */

/**
 * @brief Encode filter stage count and clock selection into one configuration value.
 *
 * Encoding:
 * - bits [3:2] = FILNUM (masked to 2 bits)
 * - bits [1:0] = FILCLKSEL (masked to 2 bits)
 *
 * @param filnum Filter stage selection (RZV_FILNUM_*).
 * @param filclksel Filter clock selection (RZV_FILCLKSEL_*).
 *
 * @return Encoded filter configuration value.
 */
#define RZV_FILTER_SET(filnum, filclksel) ((((filnum) & 0x3) << 0x2) | ((filclksel) & 0x3))

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZV_COMMON_H_ */
