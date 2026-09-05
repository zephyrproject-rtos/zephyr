/*
 * Copyright (c) 2026 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB106X_UART_H
#define ENE_KB106X_UART_H

/**
 * Structure type to access MialBox1/2 (MBX).
 */

struct uart_regs {
	volatile uint32_t uart_cfg;     /*Configuration Register */
	volatile uint8_t uart_ie;       /*Interrupt Enable Register */
	volatile uint8_t reserved_0[3]; /*Reserved */
	volatile uint8_t uart_sirq;     /*SIRQ Register */
	volatile uint8_t reserved_1[3]; /*Reserved */
};

#define UART_FUNCTION_ENABLE  0x01
#define UART_IO_DECODE_ENABLE 0x04

#define UART_IRQ_NUM_POS 4

#define UART_IO_BASE_POS  16
#define UART_IO_BASE_MASK 0xFFFF0000

#endif /* ENE_KB106X_UART_H */
