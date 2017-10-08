/*
 * Copyright (c) 2017 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_SOC_H
#define __INC_SOC_H

/* GPIO */
#define GPIO_DW_0_BASE_ADDR			0x00080C00
#define GPIO_DW_0_BITS				32
#define GPIO_DW_PORT_0_INT_MASK			0
#define GPIO_DW_0_IRQ_FLAGS			0
#define GPIO_DW_0_IRQ				20

/* UART - UART0 */
#define UART_NS16550_PORT_0_BASE_ADDR		0x00080800
#define UART_NS16550_PORT_0_CLK_FREQ		38400000

#endif /* __INC_SOC_H */
