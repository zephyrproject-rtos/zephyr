/* ieee802154_uart_pipe.h - Private header for UART PIPE fake radio driver */

/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_UART_PIPE_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_UART_PIPE_H_

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

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_UART_PIPE_H_ */
