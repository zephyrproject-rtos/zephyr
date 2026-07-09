/*
 * Copyright (c) 2026 Embracecactus
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BK7258_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BK7258_PINCTRL_H_

#define BK7258_GPIO_PIN_MASK 0x3fU
#define BK7258_FUNC_MASK     0x0fU
#define BK7258_FUNC_POS      8

#define BK7258_PINMUX(pin, func)                                                                   \
	(((pin) & BK7258_GPIO_PIN_MASK) | (((func) & BK7258_FUNC_MASK) << BK7258_FUNC_POS))

#define BK7258_FLAG_OUTPUT      0x00001000
#define BK7258_FLAG_INPUT       0x00002000
#define BK7258_FLAG_PULL_UP     0x00004000
#define BK7258_FLAG_PULL_DOWN   0x00008000
#define BK7258_FLAG_SECOND_FUNC 0x00010000

#define FUNC_UART1_TX 0
#define FUNC_UART1_RX 0

#define BK7258_PIN_0 0
#define BK7258_PIN_1 1

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BK7258_PINCTRL_H_ */
