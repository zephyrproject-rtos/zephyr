/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree pin control helpers for Ambiq Apollo5
 * @ingroup pinctrl_apollo5
 */

#ifndef __APOLLO5_PINCTRL_H__
#define __APOLLO5_PINCTRL_H__

/**
 * @addtogroup ambiq_pinctrl Ambiq pin control helpers
 * @ingroup devicetree-pinctrl
 */

/**
 * @defgroup pinctrl_apollo5 Ambiq Apollo5 pin control helpers
 * @brief Macros for pin control configuration of Ambiq Apollo5
 * @ingroup ambiq_pinctrl
 *
 * Use APOLLO5_PINMUX() to encode a pin number and alternative function for
 * the @p pinmux devicetree property.
 *
 * @code{.dts}
 * #include <zephyr/dt-bindings/pinctrl/ambiq-apollo5-pinctrl.h>
 *
 * &pinctrl {
 *         uart0_default: uart0_default {
 *                 group1 {
 *                         pinmux = <APOLLO5_PINMUX(60, 4)>;
 *                 };
 *                 group2 {
 *                         pinmux = <APOLLO5_PINMUX(47, 4)>;
 *                         input-enable;
 *                 };
 *         };
 * };
 * @endcode
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */

#define APOLLO5_ALT_FUNC_POS 0
#define APOLLO5_ALT_FUNC_MASK 0xf

#define APOLLO5_PIN_NUM_POS 4
#define APOLLO5_PIN_NUM_MASK 0xff

#define APOLLO5_PINMUX(pin_num, alt_func)       \
	(pin_num << APOLLO5_PIN_NUM_POS |       \
	 alt_func << APOLLO5_ALT_FUNC_POS)

/** @endcond */

/** @} */

#endif /* __APOLLO5_PINCTRL_H__ */
