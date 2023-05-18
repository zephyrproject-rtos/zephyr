/*
 * Copyright (c) 2023 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MAX32_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MAX32_PINCTRL_H_

/* Adapted from Linux: include/dt-bindings/pinctrl/stm32-pinfunc.h */

/**
 * @brief Pin modes
 */
#define MAX32_GPIO 0x00
#define MAX32_AF1  0x01
#define MAX32_AF2  0x02
#define MAX32_AF3  0x03
#define MAX32_AF4  0x04
/**
 * @brief Macro to generate pinmux int using port, pin number and mode arguments
 * This is inspired from Linux equivalent st,stm32f429-pinctrl binding
 */

#define MAX32_MODE_SHIFT 0U
#define MAX32_MODE_MASK  0x0FU
#define MAX32_PORT_SHIFT 4U
#define MAX32_PORT_MASK  0x0FU
#define MAX32_LINE_SHIFT 8U
#define MAX32_LINE_MASK  0xFFU

/**
 * @brief Pin configuration configuration bit field.
 *
 * Fields:
 *
 * - mode [ 0 : 3 ]
 * - port [ 4 : 7 ]
 * - line [ 8 : 15 ]
 *
 * @param port Port (0 .. 15)
 * @param line Pin (0..31)
 * @param mode Mode (ANALOG, GPIO_IN, ALTERNATE).
 */
#define MAX32_PINMUX(port, line, mode)                                         \
	((((port) & MAX32_PORT_MASK) << MAX32_PORT_SHIFT) |                    \
	 (((line) & MAX32_LINE_MASK) << MAX32_LINE_SHIFT) |                    \
	 (((MAX32_##mode) & MAX32_MODE_MASK) << MAX32_MODE_SHIFT))

#define MAX32_PINMUX_PORT(pinmux) (((pinmux) >> MAX32_PORT_SHIFT) & MAX32_PORT_MASK)
#define MAX32_PINMUX_PIN(pinmux)  (((pinmux) >> MAX32_LINE_SHIFT) & MAX32_LINE_MASK)
#define MAX32_PINMUX_MODE(pinmux) (((pinmux) >> MAX32_MODE_SHIFT) & MAX32_MODE_MASK)

/*
 *	MAX32666 GPIO Configs:
 *	VSSEL: 1 bit
 *	Pull Strength (PS): 1 bit, weak or strong
 *	PAD: 2 bits, 1 for pullup, 2 for pulldown. may be disabled (HiZ). both is reserved.
 *	Output Drive Strength (ODS): 2 bits. 0 to 3
 *
 *	Input and Output buffer enables. one bit each.
 *
 */

#define MAX32_VDDIO  0
#define MAX32_VDDIOH 1

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_STM32_PINCTRL_H_ */
