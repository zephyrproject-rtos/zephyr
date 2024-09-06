/*
 * Copyright (c) 2024 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_RTS5817_INTC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_RTS5817_INTC_H_

#define IRQ_NUM_SIE        0
#define IRQ_NUM_MC         1
#define IRQ_NUM_SENSOR_SPI 4
#define IRQ_NUM_SPI_MASTER 5
#define IRQ_NUM_AES        6
#define IRQ_NUM_SHA        7
#define IRQ_NUM_GPIO       11
#define IRQ_NUM_SP_GPIO    12
#define IRQ_NUM_UART0      15
#define IRQ_NUM_UART1      16
#define IRQ_NUM_SPI_SLAVE  23

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_RTS5817_INTC_H_ */
