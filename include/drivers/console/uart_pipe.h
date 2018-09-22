/** @file
 *  @brief Pipe UART driver header file.
 *
 *  A pipe UART driver that allows applications to handle all aspects of
 *  received protocol data.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CONSOLE_UART_PIPE_H_
#define ZEPHYR_INCLUDE_DRIVERS_CONSOLE_UART_PIPE_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Received data callback.
 *
 *  This function is called when new data is received on UART. The off parameter
 *  can be used to alter offset at which received data is stored. Typically,
 *  when the complete data is received and a new buffer is provided off should
 *  be set to 0.
 *
 *  @param buf Buffer with received data.
 *  @param off Data offset on next received and accumulated data length.
 *
 *  @return Buffer to be used on next receive.
 */
typedef u8_t *(*uart_pipe_recv_cb)(u8_t *buf, size_t *off);

/** @brief Register UART application.
 *
 *  This function is used to register new UART application.
 *
 *  @param buf Initial buffer for received data.
 *  @param len Size of buffer.
 *  @param cb Callback to be called on data reception.
 */
void uart_pipe_register(u8_t *buf, size_t len, uart_pipe_recv_cb cb);

/** @brief Send data over UART.
 *
 *  This function is used to send data over UART.
 *
 *  @param data Buffer with data to be send.
 *  @param len Size of data.
 *
 *  @return 0 on success or negative error
 */
int uart_pipe_send(const u8_t *data, int len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CONSOLE_UART_PIPE_H_ */
