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

/* I2C - I2C0 */
#define I2C_DW_0_BASE_ADDR			0x00080400
#define I2C_DW_0_IRQ				20
#define I2C_DW_IRQ_FLAGS			0
#define I2C_DW_CLOCK_SPEED			38

#endif /* __INC_SOC_H */
