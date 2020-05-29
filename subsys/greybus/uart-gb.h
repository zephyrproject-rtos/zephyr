/*
 * Copyright (c) 2015 Google, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _GREYBUS_UART_H_
#define _GREYBUS_UART_H_

#include <greybus/types.h>

/* UART protocol IDs */
#define GB_UART_PROTOCOL_INVALID                0x00
#define GB_UART_PROTOCOL_VERSION                0x01
#define GB_UART_PROTOCOL_SEND_DATA              0x02
#define GB_UART_PROTOCOL_RECEIVE_DATA           0x03
#define GB_UART_PROTOCOL_SET_LINE_CODING        0x04
#define GB_UART_PROTOCOL_SET_CONTROL_LINE_STATE 0x05
#define GB_UART_PROTOCOL_SEND_BREAK             0x06
#define GB_UART_PROTOCOL_SERIAL_STATE           0x07

/* recv-data-request flags */
#define GB_UART_RECV_FLAG_FRAMING       0x01    /* Framing error */
#define GB_UART_RECV_FLAG_PARITY        0x02    /* Parity error */
#define GB_UART_RECV_FLAG_OVERRUN       0x04    /* Overrun error */
#define GB_UART_RECV_FLAG_BREAK         0x08    /* Break */

/* stop bits */
#define GB_SERIAL_1_STOP_BITS       0
#define GB_SERIAL_1_5_STOP_BITS     1
#define GB_SERIAL_2_STOP_BITS       2

/* parity */
#define GB_SERIAL_NO_PARITY         0
#define GB_SERIAL_ODD_PARITY        1
#define GB_SERIAL_EVEN_PARITY       2
#define GB_SERIAL_MARK_PARITY       3
#define GB_SERIAL_SPACE_PARITY      4

/* output control lines */
#define GB_UART_CTRL_DTR            0x01
#define GB_UART_CTRL_RTS            0x02

/* input control lines and line errors */
#define GB_UART_CTRL_DCD            0x01
#define GB_UART_CTRL_DSR            0x02
#define GB_UART_CTRL_RI             0x04

#define GB_UART_CTRL_FRAMING        0x10
#define GB_UART_CTRL_PARITY         0x20
#define GB_UART_CTRL_OVERRUN        0x40

/* version request has no payload */
struct gb_uart_proto_version_response {
    __u8    major;
    __u8    minor;
} __packed;

struct gb_uart_send_data_request {
    __le16  size;
    __u8    data[0];
} __packed;

struct gb_uart_receive_data_request {
    __le16  size;
    __u8    flags;
    __u8    data[0];
} __packed;

struct gb_serial_line_coding_request {
    __le32  rate __packed;
    __u8    format;     /* stop bits */
    __u8    parity;
    __u8    data;       /* data bits */
} __packed;

struct gb_uart_set_control_line_state_request {
    __u8    control;
} __packed;

struct gb_uart_set_break_request {
    __u8    state;
} __packed;

struct gb_uart_serial_state_request {
    __u8    control;
} __packed;

#endif /* _GREYBUS_UART_H_ */

