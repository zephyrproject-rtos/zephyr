/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_B91_PINCTRL_COMMON_H_
#define ZEPHYR_B91_PINCTRL_COMMON_H_

/* IDs for GPIO functions */

#define B91_FUNC_A       0x00
#define B91_FUNC_B       0x01
#define B91_FUNC_C       0x02

/* IDs for GPIO Ports  */

#define B91_PORT_A       0x00
#define B91_PORT_B       0x01
#define B91_PORT_C       0x02
#define B91_PORT_D       0x03
#define B91_PORT_E       0x04

/* IDs for GPIO Pins */

#define B91_PIN_0        0x01
#define B91_PIN_1        0x02
#define B91_PIN_2        0x04
#define B91_PIN_3        0x08
#define B91_PIN_4        0x10
#define B91_PIN_5        0x20
#define B91_PIN_6        0x40
#define B91_PIN_7        0x80

/* B91 pinctrl pull-up/down */

#define B91_PULL_NONE    0
#define B91_PULL_DOWN    2
#define B91_PULL_UP      3

/* Pin function positions */

#define B91_PIN_0_FUNC_POS    0x00
#define B91_PIN_1_FUNC_POS    0x02
#define B91_PIN_2_FUNC_POS    0x04
#define B91_PIN_3_FUNC_POS    0x06
#define B91_PIN_4_FUNC_POS    0x00
#define B91_PIN_5_FUNC_POS    0x02
#define B91_PIN_6_FUNC_POS    0x04
#define B91_PIN_7_FUNC_POS    0x06

/* B91 pin configuration bit field positions and masks */

#define B91_PULL_POS     19
#define B91_PULL_MSK     0x3
#define B91_FUNC_POS     16
#define B91_FUNC_MSK     0x3
#define B91_PORT_POS     8
#define B91_PORT_MSK     0xFF
#define B91_PIN_POS      0
#define B91_PIN_MSK      0xFFFF
#define B91_PIN_ID_MSK   0xFF

/* Setters and getters */

#define B91_PINMUX_SET(port, pin, func)   ((func << B91_FUNC_POS) | \
					   (port << B91_PORT_POS) | \
					   (pin << B91_PIN_POS))
#define B91_PINMUX_GET_PULL(pinmux)       ((pinmux >> B91_PULL_POS) & B91_PULL_MSK)
#define B91_PINMUX_GET_FUNC(pinmux)       ((pinmux >> B91_FUNC_POS) & B91_FUNC_MSK)
#define B91_PINMUX_GET_PIN(pinmux)        ((pinmux >> B91_PIN_POS) & B91_PIN_MSK)
#define B91_PINMUX_GET_PIN_ID(pinmux)     ((pinmux >> B91_PIN_POS) & B91_PIN_ID_MSK)

#endif  /* ZEPHYR_B91_PINCTRL_COMMON_H_ */
