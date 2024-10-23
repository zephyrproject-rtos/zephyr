/*
 * Copyright (c) 2021-2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_B91_PINCTRL_COMMON_H_
#define ZEPHYR_B91_PINCTRL_COMMON_H_

/* IDs for B91 GPIO functions */

#define B91_FUNC_A       0x00
#define B91_FUNC_B       0x01
#define B91_FUNC_C       0x02

/* IDs for GPIO Ports  */

#define B9x_PORT_A       0x00
#define B9x_PORT_B       0x01
#define B9x_PORT_C       0x02
#define B9x_PORT_D       0x03
#define B9x_PORT_E       0x04

/* IDs for GPIO Pins */

#define B9x_PIN_0        0x01
#define B9x_PIN_1        0x02
#define B9x_PIN_2        0x04
#define B9x_PIN_3        0x08
#define B9x_PIN_4        0x10
#define B9x_PIN_5        0x20
#define B9x_PIN_6        0x40
#define B9x_PIN_7        0x80

/* B9x pinctrl pull-up/down */

#define B9x_PULL_NONE    0
#define B9x_PULL_DOWN    2
#define B9x_PULL_UP      3

/* Pin pull up and function positions */

#define B9x_PIN_0_PULL_UP_EN_POS    0x00
#define B9x_PIN_1_PULL_UP_EN_POS    0x02
#define B9x_PIN_2_PULL_UP_EN_POS    0x04
#define B9x_PIN_3_PULL_UP_EN_POS    0x06
#define B9x_PIN_4_PULL_UP_EN_POS    0x00
#define B9x_PIN_5_PULL_UP_EN_POS    0x02
#define B9x_PIN_6_PULL_UP_EN_POS    0x04
#define B9x_PIN_7_PULL_UP_EN_POS    0x06

/* B91 pin configuration bit field positions and masks */

#define B9x_PULL_POS     19
#define B9x_PULL_MSK     0x3
#define B9x_FUNC_POS     16
#define B91_FUNC_MSK     0x3
#define B9x_PORT_POS     8
#define B9x_PORT_MSK     0xFF

#define B9x_PIN_POS      0
#define B9x_PIN_MSK      0xFFFF
#define B9x_PIN_ID_MSK   0xFF

/* Setters and getters */

#define B9x_PINMUX_SET(port, pin, func)   ((func << B9x_FUNC_POS) | \
					   (port << B9x_PORT_POS) | \
					   (pin << B9x_PIN_POS))
#define B9x_PINMUX_GET_PULL(pinmux)       ((pinmux >> B9x_PULL_POS) & B9x_PULL_MSK)
#define B9x_PINMUX_GET_FUNC(pinmux)       ((pinmux >> B9x_FUNC_POS) & B91_FUNC_MSK)
#define B9x_PINMUX_GET_PIN(pinmux)        ((pinmux >> B9x_PIN_POS) & B9x_PIN_MSK)
#define B9x_PINMUX_GET_PIN_ID(pinmux)     ((pinmux >> B9x_PIN_POS) & B9x_PIN_ID_MSK)

#endif  /* ZEPHYR_B91_PINCTRL_COMMON_H_ */
