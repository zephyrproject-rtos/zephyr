/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc_pinmap.h>

#define UART_TX_PAD		44
#define UART_TX_PAD_CFG		UART_TXD_PAD44
#define UART_RX_PAD		45
#define UART_RX_PAD_CFG		UART_RXD_PAD45

#define UART_RX_SEL		UART_RXD_SEL_PAD45

#endif /* __INC_BOARD_H */
