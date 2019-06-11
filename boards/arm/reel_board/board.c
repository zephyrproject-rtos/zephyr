/*
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <soc.h>

/* Peripheral voltage ON/OFF GPIO */
#define PERIPH_PON_PIN		0

static int board_reel_board_init(struct device *dev)
{
	ARG_UNUSED(dev);
	volatile NRF_GPIO_Type *gpio = NRF_P1;

	/*
	 * Workaround to enable peripheral voltage.
	 */
	gpio->PIN_CNF[PERIPH_PON_PIN] =
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
		(GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);

	gpio->OUTSET = BIT(PERIPH_PON_PIN);

	/*
	 * Enable pull-up on UART RX pin to reduce power consumption.
	 * If the board is powered by battery and the debugger is
	 * not connected via USB to the host, the SoC consumes up
	 * to 2mA more than expected.
	 * The consumption increases because RX pin is floating
	 * (High-Impedance state of pin B from Dual-Supply Bus Transceiver).
	 */
	gpio = NRF_P0;
	gpio->PIN_CNF[DT_INST_0_NORDIC_NRF_UART_RX_PIN] =
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
		(GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos) |
		(GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos);
	gpio->PIN_CNF[DT_INST_0_NORDIC_NRF_UART_TX_PIN] =
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
		(GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos) |
		(GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos);


	return 0;
}

SYS_INIT(board_reel_board_init, PRE_KERNEL_2,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
