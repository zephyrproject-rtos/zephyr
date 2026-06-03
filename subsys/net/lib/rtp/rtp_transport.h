/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file rtp_transport.h
 * @brief Internal functions to handle transport in RTP stack
 *
 */

#ifndef ZEPHYR_INCLUDE_NET_RTP_TRANSPORT_H_
#define ZEPHYR_INCLUDE_NET_RTP_TRANSPORT_H_

#include <zephyr/net/rtp.h>

typedef int (*rtp_transport_init_t)(struct rtp_session *session);

typedef int (*rtp_transport_start_rx_t)(struct rtp_session *session);

typedef int (*rtp_transport_start_tx_t)(struct rtp_session *session);

typedef int (*rtp_transport_stop_rx_t)(struct rtp_session *session);

typedef int (*rtp_transport_stop_tx_t)(struct rtp_session *session);

typedef int (*rtp_transport_send_t)(struct rtp_session *session, struct rtp_packet *rtp_pkt,
				    uint8_t padding);

struct rtp_transport_interface {
	rtp_transport_init_t init;
	rtp_transport_start_rx_t start_rx;
	rtp_transport_start_tx_t start_tx;
	rtp_transport_stop_rx_t stop_rx;
	rtp_transport_stop_tx_t stop_tx;
	rtp_transport_send_t send;
};

int rtp_transport_init(struct rtp_session *session);

int rtp_transport_start_rx(struct rtp_session *session);

int rtp_transport_start_tx(struct rtp_session *session);

int rtp_transport_stop_rx(struct rtp_session *session);

int rtp_transport_stop_tx(struct rtp_session *session);

int rtp_transport_send(struct rtp_session *session, struct rtp_packet *rtp_pkt, uint8_t padding);

#ifdef CONFIG_RTP_TRANSPORT_SOCKET

int rtp_transport_socket_init(struct rtp_session *session);

int rtp_transport_socket_start_rx(struct rtp_session *session);

int rtp_transport_socket_start_tx(struct rtp_session *session);

int rtp_transport_socket_stop_rx(struct rtp_session *session);

int rtp_transport_socket_stop_tx(struct rtp_session *session);

int rtp_transport_socket_send(struct rtp_session *session, struct rtp_packet *rtp_pkt,
			      uint8_t padding);
#endif /* CONFIG_RTP_TRANSPORT_SOCKET */

#endif /* ZEPHYR_INCLUDE_NET_RTP_TRANSPORT_H_ */
