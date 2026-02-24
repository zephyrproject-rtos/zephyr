/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_STM32F1_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_STM32F1_PINCTRL_H_

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

#endif	/* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_STM32F1_PINCTRL_H_ */
