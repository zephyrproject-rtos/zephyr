/*
 * Copyright (c) 2022 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc_pinmap.h>

#define USB_PU_CTRL_PAD		23
#define USB_DN_PAD		28
#define USB_DP_PAD		31
#define USB_PAD_CFG		(PAD_E_4MA | PAD_P_Z | PAD_OEN_NORMAL | PAD_SMT_DISABLE \
					   | PAD_REN_DISABLE | PAD_SR_SLOW | PAD_CTRL_SEL_FPGA)

#define UART_TX_PAD		44
#define UART_TX_PAD_CFG		UART_TXD_PAD44
#define UART_RX_PAD		45
#define UART_RX_PAD_CFG		UART_RXD_PAD45

#define UART_RX_SEL		UART_RXD_SEL_PAD45

#endif /* __INC_BOARD_H */
