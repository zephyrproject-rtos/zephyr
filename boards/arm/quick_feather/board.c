/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <soc.h>
#include <board.h>

static int eos_s3_board_init(void)
{

	/* IO MUX setup for UART */
	eos_s3_io_mux(UART_TX_PAD, UART_TX_PAD_CFG);
	eos_s3_io_mux(UART_RX_PAD, UART_RX_PAD_CFG);

	IO_MUX->UART_rxd_SEL = UART_RX_SEL;

	return 0;
}

SYS_INIT(eos_s3_board_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
