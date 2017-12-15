/*
 * Copyright (c) 2017 Crypta Labs Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>

#include "soc.h"

static int board_init(struct device *dev)
{
#ifdef CONFIG_UART_NS16550_PORT_0
	/* Set clock request, muxing and power up UART0 */
	PCR_INST->CLK_REQ_2_b.UART_0_CLK_REQ = 1;
	GPIO_100_137_INST->GPIO_104_PIN_CONTROL_b.MUX_CONTROL = 1;
	GPIO_100_137_INST->GPIO_105_PIN_CONTROL_b.MUX_CONTROL = 1;
	UART0_INST->CONFIG = 0;
	UART0_INST->ACTIVATE = 1;
#endif
#ifdef CONFIG_UART_NS16550_PORT_1
	/* Set clock request, muxing, UART1_RX_EN and power up UART1 */
	PCR_INST->CLK_REQ_2_b.UART_1_CLK_REQ = 1;
	GPIO_140_176_INST->GPIO_170_PIN_CONTROL_b.MUX_CONTROL = 2;
	GPIO_140_176_INST->GPIO_171_PIN_CONTROL_b.MUX_CONTROL = 2;
	GPIO_100_137_INST->GPIO_113_PIN_CONTROL_b.GPIO_DIRECTION = 1;
	UART1_INST->CONFIG = 0;
	UART1_INST->ACTIVATE = 1;
#endif
	return 0;
}

SYS_INIT(board_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
