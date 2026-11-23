/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_RZT2M_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_RZT2M_PINCTRL_H_

#define RZT2M_PINMUX(port, pin, func) ((port << 16) | (pin << 8) | func)

#define UART0TX_P16_5	RZT2M_PINMUX(16, 5, 1)
#define UART0RX_P16_6	RZT2M_PINMUX(16, 6, 2)

#define UART3TX_P18_0	RZT2M_PINMUX(18, 0, 4)
#define UART3RX_P17_7	RZT2M_PINMUX(17, 7, 4)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_RZT2M_PINCTRL_H_ */
