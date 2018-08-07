/*
 * Copyright (c) 2018 Madani Lainani.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

#define CONFIG_SPI_SAM0_SERCOM1_PADS				\
	(SERCOM_SPI_CTRLA_DIPO(3) | SERCOM_SPI_CTRLA_DOPO(0))

#define CONFIG_UART_SAM0_SERCOM5_PADS \
	(SERCOM_USART_CTRLA_RXPO(3) | SERCOM_USART_CTRLA_TXPO(1))

#endif /* __INC_BOARD_H */
