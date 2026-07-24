/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp
 * @brief TCP server sink element for the MP znet plugin.
 *
 * Opens a TCP server socket, accepts one client connection, and streams
 * every buffer it receives over the socket.  Designed for raw MJPEG
 * streaming on native_sim or any Zephyr target with BSD-socket support.
 */

#ifndef ZEPHYR_INCLUDE_MP_ZNET_MP_ZTCPSINK_H_
#define ZEPHYR_INCLUDE_MP_ZNET_MP_ZTCPSINK_H_

#include <zephyr/mp/core/mp_property.h>
#include <zephyr/mp/core/mp_sink.h>

/** Cast a generic element pointer to @ref mp_ztcpsink. */
#define MP_ZTCPSINK(self) ((struct mp_ztcpsink *)(self))

/**
 * @brief TCP sink property identifiers.
 *
 * Extends the base sink properties defined in @ref mp_property.h.
 */
enum prop_ztcpsink {
	/** TCP port to listen on (uint16_t). Default: 5000. */
	PROP_ZTCPSINK_PORT = PROP_SINK_LAST + 1,
};

/**
 * @brief TCP server sink element.
 *
 * Extends @ref mp_sink to forward pipeline buffers to a remote TCP client.
 * The element listens on a configurable port and blocks in
 * MP_STATE_CHANGE_READY_TO_PAUSED until exactly one client connects.
 * When CONFIG_NET_IPV6 is enabled, a single IPv6 socket accepts both IPv4
 * and IPv6 clients via IPv4-mapped addresses (CONFIG_NET_IPV4_MAPPING_TO_IPV6).
 */
struct mp_ztcpsink {
	/** Base sink element. */
	struct mp_sink sink;
	/** TCP port number (default 5000). */
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
void mp_ztcpsink_init(struct mp_element *self);

#endif /* ZEPHYR_INCLUDE_MP_ZNET_MP_ZTCPSINK_H_ */
