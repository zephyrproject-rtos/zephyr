/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree pin control helpers common to the Renesas RZ/G SoCs
 * @ingroup pinctrl_rzg
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZG_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZG_COMMON_H_

/**
 * @addtogroup renesas_pinctrl Renesas pin control helpers
 * @ingroup devicetree-pinctrl
 */

/**
 * @defgroup pinctrl_rzg Renesas RZ/G pin control helpers
 * @brief Macros for pin control configuration of Renesas RZ/G SoCs
 * @ingroup renesas_pinctrl
 *
 * General-purpose pins are configured with the RZG_PINMUX() macro, which takes
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
 * RZG_FILTER_SET(), built from a stage count (@c RZG_FILNUM_*) and a sampling
 * clock divider (@c RZG_FILCLKSEL_*).
 *
 * This header only holds the macros shared by all RZ/G SoCs. The ports, the
 * dedicated-function pins and the sampling clock dividers differ between SoCs
 * and are provided by a header per SoC, which includes this one:
 *
 * - @c pinctrl-rzg2.h — RZ/G2L, RZ/G2LC, RZ/G2UL
 * - @c pinctrl-rzg3s.h — RZ/G3S
 * - @c pinctrl-rzg3e.h — RZ/G3E
 *
 * @code{.dts}
 * #include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-rzg3s.h>
 *
 * &pinctrl {
 *         scif0_default: scif0_default {
 *                 device-pinmux {
 *                         pinmux = <RZG_PINMUX(PORT_08, 1, 5)>,
 *                                  <RZG_PINMUX(PORT_08, 2, 5)>;
 *                         bias-pull-up;
 *                         renesas,filter =
 *                                 RZG_FILTER_SET(RZG_FILNUM_8_STAGE, RZG_FILCLKSEL_DIV_18000);
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
#define RZG_PINMUX(port, pin, func) ((port) | (pin) | ((func) << 4))

/**
 * @name Renesas RZ/G digital noise filter stages
 *
 * Set the number of filter stages (FILNUM).
 * @{
 */

#define RZG_FILNUM_4_STAGE  0 /**< FILNUM: 4-stage filter */
#define RZG_FILNUM_8_STAGE  1 /**< FILNUM: 8-stage filter */
#define RZG_FILNUM_12_STAGE 2 /**< FILNUM: 12-stage filter */
#define RZG_FILNUM_16_STAGE 3 /**< FILNUM: 16-stage filter */

/** @} */

/**
 * @brief Encode filter stage count and clock selection into one configuration value.
 *
 * Encoding:
 * - bits [3:2] = FILNUM (masked to 2 bits)
 * - bits [1:0] = FILCLKSEL (masked to 2 bits)
 *
 * @param filnum Filter stage selection (RZG_FILNUM_*).
 * @param filclksel Filter clock selection (RZG_FILCLKSEL_* of the SoC header).
 *
 * @return Encoded filter configuration value.
 */
#define RZG_FILTER_SET(filnum, filclksel) ((((filnum) & 0x3) << 0x2) | ((filclksel) & 0x3))

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZG_COMMON_H_ */
