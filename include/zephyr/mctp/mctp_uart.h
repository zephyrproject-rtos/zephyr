/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_MCTP_UART_H_
#define ZEPHYR_MCTP_UART_H_

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <libmctp.h>

/**
 * @brief An MCTP binding for Zephyr's asynchronous UART interface
 */
struct mctp_binding_uart {
	/** @cond INTERNAL_HIDDEN */
	struct mctp_binding binding;
	const struct device *dev;

	/* receive buffers and state */
	uint8_t rx_buf[2][256];
	bool rx_buf_used[2];
	struct mctp_pktbuf *rx_pkt;
	uint8_t rx_exp_len;
	uint16_t rx_fcs;
	uint16_t rx_fcs_calc;
	enum {
		STATE_WAIT_SYNC_START,
		STATE_WAIT_REVISION,
		STATE_WAIT_LEN,
		STATE_DATA,
		STATE_DATA_ESCAPED,
		STATE_WAIT_FCS1,
		STATE_WAIT_FCS2,
		STATE_WAIT_SYNC_END,
	} rx_state;
	int rx_res;

	/* staging buffer for tx */
	uint8_t tx_buf[256];
	int tx_res;

	/** @endcond INTERNAL_HIDDEN */
};

/**
 * @brief Start the receive of a single mctp message
 *
 * Will read a single mctp message from the uart.
 *
 * @param uart MCTP UART binding
 */
void mctp_uart_start_rx(struct mctp_binding_uart *uart);

/** @cond INTERNAL_HIDDEN */
int mctp_uart_start(struct mctp_binding *binding);
int mctp_uart_tx(struct mctp_binding *binding, struct mctp_pktbuf *pkt);
/** @endcond INTERNAL_HIDDEN */

/**
 * @brief Statically define a MCTP bus binding for a UART
 *
 * @param _name Symbolic name of the bus binding variable
 * @param _dev UART device
 */
#define MCTP_UART_DT_DEFINE(_name, _dev)                                                           \
	struct mctp_binding_uart _name = {                                                         \
		.binding =                                                                         \
			{                                                                          \
				.name = STRINGIFY(_name), .version = 1,                            \
						  .pkt_size = MCTP_PACKET_SIZE(MCTP_BTU),          \
						  .pkt_header = 0, .pkt_trailer = 0,               \
						  .start = mctp_uart_start, .tx = mctp_uart_tx,    \
				},                                                                 \
				.dev = _dev,                                                       \
				.rx_state = STATE_WAIT_SYNC_START,                                 \
				.rx_pkt = NULL,                                                    \
				.rx_res = 0,                                                       \
				.tx_res = 0,                                                       \
	};

#endif /* ZEPHYR_MCTP_UART_H_ */
