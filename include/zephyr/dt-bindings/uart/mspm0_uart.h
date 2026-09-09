/*
 * Copyright (c) 2026 Bang & Olufsen A/S, Denmark
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TI MSPM0 UART FIFO threshold level definitions
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_UART_MSPM0_UART_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_UART_MSPM0_UART_H_

/**
 * @name MSPM0 UART RX FIFO threshold levels
 * @{
 */

/** At least one entry is present */
#define MSPM0_UART_RX_FIFO_LEVEL_ONE_ENTRY 0
/** RX FIFO is 1/4 full */
#define MSPM0_UART_RX_FIFO_LEVEL_1_4_FULL  1
/** RX FIFO is 1/2 full */
#define MSPM0_UART_RX_FIFO_LEVEL_1_2_FULL  2
/** RX FIFO is 3/4 full */
#define MSPM0_UART_RX_FIFO_LEVEL_3_4_FULL  3
/** RX FIFO is full */
#define MSPM0_UART_RX_FIFO_LEVEL_FULL      4

/** @} */

/**
 * @name MSPM0 UART TX FIFO threshold levels
 * @{
 */

/** TX FIFO has at least one entry available */
#define MSPM0_UART_TX_FIFO_LEVEL_ONE_ENTRY 0
/** TX FIFO is 1/4 empty */
#define MSPM0_UART_TX_FIFO_LEVEL_1_4_EMPTY 1
/** TX FIFO is 1/2 empty */
#define MSPM0_UART_TX_FIFO_LEVEL_1_2_EMPTY 2
/** TX FIFO is 3/4 empty */
#define MSPM0_UART_TX_FIFO_LEVEL_3_4_EMPTY 3
/** TX FIFO is empty */
#define MSPM0_UART_TX_FIFO_LEVEL_EMPTY     4

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_UART_MSPM0_UART_H_ */
