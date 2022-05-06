/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include "soc.h"

static int board_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* See table 2-4 from the Data sheet*/
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay)
	/* Set muxing, for UART 0 and power up */
	PCR_INST->CLK_REQ_2_b.UART_0_CLK_REQ = 1;
	UART0_INST->CONFIG = 0;
	UART0_INST->ACTIVATE = 1;
	GPIO_100_137_INST->GPIO_104_PIN_CONTROL_b.MUX_CONTROL = 1;
	GPIO_100_137_INST->GPIO_105_PIN_CONTROL_b.MUX_CONTROL = 1;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	/* Set muxing, for UART 1 and power up */
	PCR_INST->CLK_REQ_2_b.UART_1_CLK_REQ = 1;
	UART1_INST->CONFIG = 0;
	UART1_INST->ACTIVATE = 1;
	GPIO_140_176_INST->GPIO_170_PIN_CONTROL_b.MUX_CONTROL = 2;
	GPIO_140_176_INST->GPIO_171_PIN_CONTROL_b.MUX_CONTROL = 2;
	GPIO_100_137_INST->GPIO_113_PIN_CONTROL_b.GPIO_DIRECTION = 1;
#endif
	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
