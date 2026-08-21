/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TCP server sink element for the MP net plugin.
 *
 * Opens a TCP server socket, accepts one client connection, and streams
 * every buffer it receives over the socket.
 */

#ifndef ZEPHYR_INCLUDE_MP_NET_MP_TCPSINK_H_
#define ZEPHYR_INCLUDE_MP_NET_MP_TCPSINK_H_

/**
 * @defgroup mp_net_sinks Sinks
 * @ingroup mp_net
 * @brief TCP-backed sink element.
 * @{
 */

#include <zephyr/mp/mp_sink.h>

/**
 * @brief TCP sink property identifiers.
 *
 * Extends the base sink properties defined in @ref mp_prop_sink.
 */
enum mp_prop_net_sink {
	/** TCP port to listen on (uint16_t). */
	MP_PROP_NET_SINK_PORT = MP_PROP_SINK_LAST,
};

/**
 * @brief TCP server sink element.
 *
 * Extends the base @ref mp_sink to forward received buffers to a remote TCP
 * client. The element listens on a configurable port and blocks in
 * MP_STATE_CHANGE_READY_TO_PAUSED until exactly one client connects.
 * When CONFIG_NET_IPV6 is enabled, a single IPv6 socket accepts both IPv4
 * and IPv6 clients via IPv4-mapped addresses (CONFIG_NET_IPV4_MAPPING_TO_IPV6).
 */
struct mp_tcpsink {
	/** Base sink element. */
	struct mp_sink sink;
	/** TCP port number. */
	uint16_t port;
	/** Listening socket file descriptor (-1 when closed). */
	int server_fd;
	/** Accepted client socket file descriptor (-1 when not connected). */
	int client_fd;
};

/**
 * @brief Initialize a TCP server sink element.
 *
 * @param self Pointer to the element to initialize.
 */
void mp_tcpsink_init(struct mp_element *self);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_NET_MP_TCPSINK_H_ */
