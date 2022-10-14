/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_STM32_PINCTRLF1_H_
#define ZEPHYR_STM32_PINCTRLF1_H_

#include <zephyr/dt-bindings/pinctrl/stm32-pinctrl-common.h>
#include <zephyr/dt-bindings/pinctrl/stm32f1-afio.h>

/* Adapted from Linux: include/dt-bindings/pinctrl/stm32-pinfunc.h */

/**
 * @brief Macro to generate pinmux int using port, pin number and mode arguments
 * This is adapted from Linux equivalent st,stm32f429-pinctrl binding
 */

#define STM32_MODE_SHIFT  0U
#define STM32_MODE_MASK   0x3U
#define STM32_LINE_SHIFT  2U
#define STM32_LINE_MASK   0xFU
#define STM32_PORT_SHIFT  6U
#define STM32_PORT_MASK   0xFU
#define STM32_REMAP_SHIFT 10U
#define STM32_REMAP_MASK  0x3FFU

/**
 * @brief Pin configuration configuration bit field.
 *
 * Fields:
 *
 * - mode  [ 0 : 1 ]
 * - line  [ 2 : 5 ]
 * - port  [ 6 : 9 ]
 * - remap [ 10 : 19 ]
 *
 * @param port Port ('A'..'K')
 * @param line Pin (0..15)
 * @param mode Pin mode (ANALOG, GPIO_IN, ALTERNATE).
 * @param remap Pin remapping configuration (NO_REMAP, REMAP_1, ...)
 */
#define STM32F1_PINMUX(port, line, mode, remap)				       \
		(((((port) - 'A') & STM32_PORT_MASK) << STM32_PORT_SHIFT) |    \
		(((line) & STM32_LINE_MASK) << STM32_LINE_SHIFT) |	       \
		(((mode) & STM32_MODE_MASK) << STM32_MODE_SHIFT) |	       \
		(((remap) & STM32_REMAP_MASK) << STM32_REMAP_SHIFT))

/**
 * @brief Pin modes
 */

#define ALTERNATE	0x0  /* Alternate function output */
#define GPIO_IN		0x1  /* Input */
#define ANALOG		0x2  /* Analog */
#define GPIO_OUT	0x3  /* Output */

/**
 * @brief PIN configuration bitfield
 *
 * Pin configuration is coded with the following
 * fields
 *    GPIO I/O Mode       [ 0 ]
 *    GPIO Input config   [ 1 : 2 ]
 *    GPIO Output speed   [ 3 : 4 ]
 *    GPIO Output PP/OD   [ 5 ]
 *    GPIO Output AF/GP   [ 6 ]
 *    GPIO PUPD Config    [ 7 : 8 ]
 *    GPIO ODR            [ 9 ]
 *
 * Applicable to STM32F1 series
 */

/* Port Mode */
#define STM32_MODE_INPUT		(0x0 << STM32_MODE_INOUT_SHIFT)
#define STM32_MODE_OUTPUT		(0x1 << STM32_MODE_INOUT_SHIFT)
#define STM32_MODE_INOUT_MASK		0x1
#define STM32_MODE_INOUT_SHIFT		0

/* Input Port configuration */
#define STM32_CNF_IN_ANALOG		(0x0 << STM32_CNF_IN_SHIFT)
#define STM32_CNF_IN_FLOAT		(0x1 << STM32_CNF_IN_SHIFT)
#define STM32_CNF_IN_PUPD		(0x2 << STM32_CNF_IN_SHIFT)
#define STM32_CNF_IN_MASK		0x3
#define STM32_CNF_IN_SHIFT		1

/* Output Port configuration */
#define STM32_MODE_OUTPUT_MAX_10	(0x0 << STM32_MODE_OSPEED_SHIFT)
#define STM32_MODE_OUTPUT_MAX_2		(0x1 << STM32_MODE_OSPEED_SHIFT)
#define STM32_MODE_OUTPUT_MAX_50	(0x2 << STM32_MODE_OSPEED_SHIFT)
#define STM32_MODE_OSPEED_MASK		0x3
#define STM32_MODE_OSPEED_SHIFT		3

#define STM32_CNF_PUSH_PULL		(0x0 << STM32_CNF_OUT_0_SHIFT)
#define STM32_CNF_OPEN_DRAIN		(0x1 << STM32_CNF_OUT_0_SHIFT)
#define STM32_CNF_OUT_0_MASK		0x1
#define STM32_CNF_OUT_0_SHIFT		5

#define STM32_CNF_GP_OUTPUT		(0x0 << STM32_CNF_OUT_1_SHIFT)
#define STM32_CNF_ALT_FUNC		(0x1 << STM32_CNF_OUT_1_SHIFT)
#define STM32_CNF_OUT_1_MASK		0x1
#define STM32_CNF_OUT_1_SHIFT		6

/* GPIO High impedance/Pull-up/Pull-down */
#define STM32_PUPD_NO_PULL		(0x0 << STM32_PUPD_SHIFT)
#define STM32_PUPD_PULL_UP		(0x1 << STM32_PUPD_SHIFT)
#define STM32_PUPD_PULL_DOWN		(0x2 << STM32_PUPD_SHIFT)
#define STM32_PUPD_MASK			0x3
#define STM32_PUPD_SHIFT		7

/* GPIO plain output value */
#define STM32_ODR_0			(0x0 << STM32_ODR_SHIFT)
#define STM32_ODR_1			(0x1 << STM32_ODR_SHIFT)
#define STM32_ODR_MASK			0x1
#define STM32_ODR_SHIFT			9

#endif	/* ZEPHYR_STM32_PINCTRLF1_H_ */
