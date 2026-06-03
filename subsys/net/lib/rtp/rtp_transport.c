/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

/** @file rtp_transport.c
 *
 * @brief Internal functions to handle transport in RTP stack.
 */

#include "rtp_transport.h"

const struct rtp_transport_interface interfaces[RTP_TRANSPORT_NUM] = {
#ifdef CONFIG_RTP_TRANSPORT_SOCKET
	{
		.init = rtp_transport_socket_init,
		.start_rx = rtp_transport_socket_start_rx,
		.start_tx = rtp_transport_socket_start_tx,
		.stop_rx = rtp_transport_socket_stop_rx,
		.stop_tx = rtp_transport_socket_stop_tx,
		.send = rtp_transport_socket_send,
	},
#endif /* CONFIG_RTP_TRANSPORT_SOCKET */
#ifdef CONFIG_RTP_TRANSPORT_NET_PKT
	{
		.init = rtp_transport_net_pkt_init,
		.start_rx = rtp_transport_net_pkt_start_rx,
		.start_tx = rtp_transport_net_pkt_start_tx,
		.stop_rx = rtp_transport_net_pkt_stop_rx,
		.stop_tx = rtp_transport_net_pkt_stop_tx,
		.send = rtp_transport_net_pkt_send,
	},
#endif /* CONFIG_RTP_TRANSPORT_NET_PKT */
};

int rtp_transport_init(struct rtp_session *session)
{
	__ASSERT_NO_MSG(session != NULL);

	return interfaces[session->transport.type].init(session);
}

int rtp_transport_start_rx(struct rtp_session *session)
{
	__ASSERT_NO_MSG(session != NULL);

	return interfaces[session->transport.type].start_rx(session);
}

int rtp_transport_start_tx(struct rtp_session *session)
{
	__ASSERT_NO_MSG(session != NULL);

	return interfaces[session->transport.type].start_tx(session);
}

int rtp_transport_stop_rx(struct rtp_session *session)
{
	__ASSERT_NO_MSG(session != NULL);

	return interfaces[session->transport.type].stop_rx(session);
}

int rtp_transport_stop_tx(struct rtp_session *session)
{
	__ASSERT_NO_MSG(session != NULL);

	return interfaces[session->transport.type].stop_tx(session);
}

int rtp_transport_send(struct rtp_session *session, struct rtp_packet *rtp_pkt, uint8_t padding)
{
	__ASSERT_NO_MSG(session != NULL);
	__ASSERT_NO_MSG(rtp_pkt != NULL);

	return interfaces[session->transport.type].send(session, rtp_pkt, padding);
}
