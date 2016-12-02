/* ieee802154_uart_pipe.h - Private header for UART PIPE fake radio driver */

/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __IEEE802154_UART_PIPE_H__
#define __IEEE802154_UART_PIPE_H__

#define UART_PIPE_RADIO_15_4_FRAME_TYPE		0xF0

struct upipe_context {
	struct net_if *iface;
	uint8_t mac_addr[8];
	bool stopped;
	/** RX specific attributes */
	uint8_t uart_pipe_buf[1];
	bool rx;
	uint8_t rx_len;
	uint8_t rx_off;
	uint8_t rx_buf[127];
};

#endif /* __IEEE802154_UART_PIPE_H__ */
