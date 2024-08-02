/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __RENESAS_RZT2M_PINCTRL_H__
#define __RENESAS_RZT2M_PINCTRL_H__

#define RZT2M_PINMUX(port, pin, func) ((port << 16) | (pin << 8) | func)

#define UART0TX_P16_5	RZT2M_PINMUX(16, 5, 1)
#define UART0RX_P16_6	RZT2M_PINMUX(16, 6, 2)

#define UART3TX_P18_0	RZT2M_PINMUX(18, 0, 4)
#define UART3RX_P17_7	RZT2M_PINMUX(17, 7, 4)

#endif /* __RENESAS_RZT2M_PINCTRL_H__ */
