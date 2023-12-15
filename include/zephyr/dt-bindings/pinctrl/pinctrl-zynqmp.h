/*
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ZYNQMP_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ZYNQMP_PINCTRL_H_

#define UART_FUNCTION 0x1

#define UART0_RX_38 (38U | (UART_FUNCTION << 8))
#define UART0_TX_39 (39U | (UART_FUNCTION << 8))


#endif
