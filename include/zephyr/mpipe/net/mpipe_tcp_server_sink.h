/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mpipe_tcp_server_sink.h @brief TCP server sink element of the mpipe net plugin. */

#ifndef ZEPHYR_INCLUDE_MPIPE_NET_MPIPE_TCP_SERVER_SINK_H_
#define ZEPHYR_INCLUDE_MPIPE_NET_MPIPE_TCP_SERVER_SINK_H_

#include <zephyr/mpipe/mpipe_sink.h>

/** @addtogroup mpipe_net */
/** @{ */

/** TCP server sink property identifiers, extending @ref mpipe_prop_sink. */
enum mpipe_prop_tcp_server_sink {
	/** TCP port to listen on (uint16_t). */
	MPIPE_PROP_TCP_SERVER_SINK_PORT = MPIPE_PROP_SINK_LAST,
};

/** TCP server sink: writes every buffer it receives to one client. */
struct mpipe_tcp_server_sink {
	/** Base sink element. */
	struct mpipe_sink sink;
	/** TCP port number. */
	uint16_t port;
	/** Listening socket, -1 when closed. */
	int server_fd;
	/** Client socket, -1 when not connected. */
	int client_fd;
};

/** Initialize a TCP server sink. @param tsink Element. @param id Id. @return 0 or -errno. */
int mpipe_tcp_server_sink_init(struct mpipe_tcp_server_sink *tsink, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_NET_MPIPE_TCP_SERVER_SINK_H_ */
