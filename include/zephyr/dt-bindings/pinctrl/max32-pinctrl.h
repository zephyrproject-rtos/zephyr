/*
 * Copyright (c) 2023-2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MAX32_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MAX32_PINCTRL_H_

/**
 * @brief Pin modes
 */
#define MAX32_MODE_GPIO 0x00
#define MAX32_MODE_AF1  0x01
#define MAX32_MODE_AF2  0x02
#define MAX32_MODE_AF3  0x03
#define MAX32_MODE_AF4  0x04
#define MAX32_MODE_AF5  0x05

/**
 * @brief Mode, port, pin shift number
 */
#define MAX32_MODE_SHIFT 0U
#define MAX32_MODE_MASK  0x0FU
#define MAX32_PORT_SHIFT 4U
#define MAX32_PORT_MASK  0x0FU
#define MAX32_PIN_SHIFT  8U
#define MAX32_PIN_MASK   0xFFU

/**
 * @brief Pin configuration bit field.
 *
 * Fields:
 *
 * - mode [ 0 : 3 ]
 * - port [ 4 : 7 ]
 * - pin [ 8 : 15 ]
 *
 * @param port Port (0 .. 15)
 * @param pin Pin (0..31)
 * @param mode Mode (GPIO, AF1, AF2...).
 */
#define MAX32_PINMUX(port, pin, mode)                                                              \
	((((port)&MAX32_PORT_MASK) << MAX32_PORT_SHIFT) |                                          \
	 (((pin)&MAX32_PIN_MASK) << MAX32_PIN_SHIFT) |                                             \
	 (((MAX32_MODE_##mode) & MAX32_MODE_MASK) << MAX32_MODE_SHIFT))

#define MAX32_PINMUX_PORT(pinmux) (((pinmux) >> MAX32_PORT_SHIFT) & MAX32_PORT_MASK)
#define MAX32_PINMUX_PIN(pinmux)  (((pinmux) >> MAX32_PIN_SHIFT) & MAX32_PIN_MASK)
#define MAX32_PINMUX_MODE(pinmux) (((pinmux) >> MAX32_MODE_SHIFT) & MAX32_MODE_MASK)

/* Selects the voltage rail used for the pin */
#define MAX32_VSEL_VDDIO  0
#define MAX32_VSEL_VDDIOH 1

/**
 * @brief Pin configuration
 */
#define MAX32_INPUT_ENABLE_SHIFT   0x00
#define MAX32_BIAS_PULL_UP_SHIFT   0x01
#define MAX32_BIAS_PULL_DOWN_SHIFT 0x02
#define MAX32_OUTPUT_ENABLE_SHIFT  0x03
#define MAX32_POWER_SOURCE_SHIFT   0x04
#define MAX32_OUTPUT_HIGH_SHIFT    0x05
#define MAX32_DRV_STRENGTH_SHIFT   0x06 /* 2 bits */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MAX32_PINCTRL_H_ */
