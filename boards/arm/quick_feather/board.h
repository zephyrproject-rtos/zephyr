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

#ifdef CONFIG_SPI
#define SPI_CLK_PAD		34
#define SPI_MISO_PAD		36
#define SPI_MOSI_PAD		38
#define SPI_SS1_PAD		39

#define SPI_CLK_PAD_CFG		SPI_CLK_PAD34
#define SPI_MISO_PAD_CFG	SPI_MISO_PAD36
#define SPI_MOSI_PAD_CFG	SPI_MOSI_PAD38
#define SPI_SS1_PAD_CFG		SPI_SS1_PAD39
#endif /* CONFIG_SPI_EOS_S3 */

#endif /* __INC_BOARD_H */
