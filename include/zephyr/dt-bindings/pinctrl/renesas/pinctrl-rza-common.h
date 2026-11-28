/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree pin control helpers common to the Renesas RZ/A SoCs
 * @ingroup pinctrl_rza
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZA_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZA_COMMON_H_

/**
 * @addtogroup renesas_pinctrl Renesas pin control helpers
 * @ingroup devicetree-pinctrl
 */

/**
 * @defgroup pinctrl_rza Renesas RZ/A pin control helpers
 * @brief Macros for pin control configuration of Renesas RZ/A SoCs
 * @ingroup renesas_pinctrl
 *
 * General-purpose pins are configured with the RZA_PINMUX() macro, which takes
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
 * RZA_FILTER_SET(), built from a stage count (@c RZA_FILNUM_*) and a sampling
 * clock divider (@c RZA_FILCLKSEL_*).
 *
 * This header only holds the macros shared by all RZ/A SoCs. The ports, the
 * dedicated-function pins and RZA_PINMUX() differ between SoCs and are
 * provided by a header per SoC, which includes this one:
 *
 * - @c pinctrl-rza3ul.h — RZ/A3UL
 * - @c pinctrl-rza3m.h — RZ/A3M
 *
 * @code{.dts}
 * #include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-rza3ul.h>
 *
 * &pinctrl {
 *         scif0_default: scif0_default {
 *                 device-pinmux {
 *                         pinmux = <RZA_PINMUX(PORT_08, 1, 5)>,
 *                                  <RZA_PINMUX(PORT_08, 2, 5)>;
 *                         bias-pull-up;
 *                         renesas,filter =
 *                                 RZA_FILTER_SET(RZA_FILNUM_8_STAGE, RZA_FILCLKSEL_DIV_18000);
 *                 };
 *         };
 * };
 * @endcode
 *
 * @{
 */

/**
 * @name Renesas RZ/A digital noise filter options
 *
 * Set the number of filter stages (FILNUM) and the sampling clock divider
 * (FILCLKSEL).
 * @{
 */

#define RZA_FILNUM_4_STAGE  0 /**< FILNUM: 4-stage filter */
#define RZA_FILNUM_8_STAGE  1 /**< FILNUM: 8-stage filter */
#define RZA_FILNUM_12_STAGE 2 /**< FILNUM: 12-stage filter */
#define RZA_FILNUM_16_STAGE 3 /**< FILNUM: 16-stage filter */

#define RZA_FILCLKSEL_NOT_DIV   0 /**< FILCLKSEL: no division */
#define RZA_FILCLKSEL_DIV_9000  1 /**< FILCLKSEL: divided by 9000 */
#define RZA_FILCLKSEL_DIV_18000 2 /**< FILCLKSEL: divided by 18000 */
#define RZA_FILCLKSEL_DIV_36000 3 /**< FILCLKSEL: divided by 36000 */

/** @} */

/**
 * @brief Encode filter stage count and clock selection into one configuration value.
 *
 * Encoding:
 * - bits [3:2] = FILNUM (masked to 2 bits)
 * - bits [1:0] = FILCLKSEL (masked to 2 bits)
 *
 * @param filnum Filter stage selection (RZA_FILNUM_*).
 * @param filclksel Filter clock selection (RZA_FILCLKSEL_*).
 *
 * @return Encoded filter configuration value.
 */
#define RZA_FILTER_SET(filnum, filclksel) ((((filnum) & 0x3) << 0x2) | ((filclksel) & 0x3))

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RZA_COMMON_H_ */
