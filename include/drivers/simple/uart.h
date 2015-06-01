/*! @file
 *  @brief Simple UART driver header file.
 *
 *  A simple UART driver that allows applications to handle all aspects of
 *  received protocol data.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>

/*! @brief Received data callback.
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
typedef uint8_t *(*uart_simple_recv_cb)(uint8_t *buf, size_t *off);

/*! @brief Register UART application.
 *
 *  This function is used to register new UART application.
 *
 *  @param buf Initial buffer for received data.
 *  @param len Size of buffer.
 *  @param cb Callback to be called on data reception.
 */
void uart_simple_register(uint8_t *buf, size_t len, uart_simple_recv_cb cb);

/*! @brief Send data over UART.
 *
 *  This function is used to send data over UART.
 *
 *  @param data Buffer with data to be send.
 *  @param len Size of data.
 *
 *  @return Number of bytes sent.
 */
int uart_simple_send(const uint8_t *data, int len);

/*! @brief Simple UART interrupt handler.
 *
 *  This function is called from an interrupt and should not be called by
 *  an application directly.
 *
 *  @param unused unused
 */
void uart_simple_isr(void *unused);
