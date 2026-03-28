/*
 * Copyright (c) ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ENE_KB1200_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ENE_KB1200_PINCTRL_H_

#include <zephyr/dt-bindings/dt-util.h>

#define PINMUX_FUNC_GPIO 0x00
#define PINMUX_FUNC_A    0x00
#define PINMUX_FUNC_B    0x01
#define PINMUX_FUNC_C    0x02
#define PINMUX_FUNC_D    0x03
#define PINMUX_FUNC_MAX  0x04

#define ENE_KB1200_NO_PUD_POS        12
#define ENE_KB1200_PD_POS            13
#define ENE_KB1200_PU_POS            14
#define ENE_KB1200_PUSH_PULL_POS     15
#define ENE_KB1200_OPEN_DRAIN_POS    16
#define ENE_KB1200_OUT_DIS_POS       17
#define ENE_KB1200_OUT_EN_POS        18
#define ENE_KB1200_OUT_HI_POS        19
#define ENE_KB1200_OUT_LO_POS        20
#define ENE_KB1200_PIN_LOW_POWER_POS 21

#define ENE_KB1200_PINMUX_PORT_POS 5
#define ENE_KB1200_PINMUX_PORT_MSK 0x7
#define ENE_KB1200_PINMUX_PIN_POS  0
#define ENE_KB1200_PINMUX_PIN_MSK  0x1f
#define ENE_KB1200_PINMUX_FUNC_POS 8
#define ENE_KB1200_PINMUX_FUNC_MSK 0xf

/*
 * f is function number
 * b[7:5] = pin bank
 * b[4:0] = pin position in bank
 * b[11:8] = function
 */
#define ENE_KB1200_PINMUX(n, f)                                                                    \
	(((((n) >> 5) & ENE_KB1200_PINMUX_PORT_MSK) << ENE_KB1200_PINMUX_PORT_POS) |               \
	 (((n) & ENE_KB1200_PINMUX_PIN_MSK) << ENE_KB1200_PINMUX_PIN_POS) |                        \
	 (((f) & ENE_KB1200_PINMUX_FUNC_MSK) << ENE_KB1200_PINMUX_FUNC_POS))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ENE_KB1200_PINCTRL_H_ */
