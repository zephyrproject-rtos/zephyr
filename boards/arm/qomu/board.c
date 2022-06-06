/*
 * Copyright (c) 2022 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <soc.h>
#include "board.h"

static int qomu_board_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	/* IO MUX setup for UART */
	eos_s3_io_mux(UART_TX_PAD, UART_TX_PAD_CFG);
	eos_s3_io_mux(UART_RX_PAD, UART_RX_PAD_CFG);

	IO_MUX->UART_rxd_SEL = UART_RX_SEL;

	/* IO MUX setup for USB */
	eos_s3_io_mux(USB_PU_CTRL_PAD, USB_PAD_CFG);
	eos_s3_io_mux(USB_DN_PAD, USB_PAD_CFG);
	eos_s3_io_mux(USB_DP_PAD, USB_PAD_CFG);

	return 0;
}

SYS_INIT(qomu_board_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
