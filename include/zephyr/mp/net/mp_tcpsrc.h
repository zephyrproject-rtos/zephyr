/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TCP server source element for the MP net plugin.
 *
 * Opens a TCP server socket, accepts one client connection, and emits
 * every received byte chunk as a downstream buffer.
 */

#ifndef ZEPHYR_INCLUDE_MP_NET_MP_TCPSRC_H_
#define ZEPHYR_INCLUDE_MP_NET_MP_TCPSRC_H_

/**
 * @defgroup mp_net_srcs Sources
 * @ingroup mp_net
 * @brief TCP-backed source element.
 * @{
 */

#include <zephyr/mp/mp_buffer.h>
#include <zephyr/mp/mp_src.h>

/**
 * @brief TCP source property identifiers.
 *
 * Extends the base source properties defined in @ref mp_prop_src.
 */
enum mp_prop_net_src {
	/** TCP port to listen on (uint16_t). */
	MP_PROP_NET_SRC_PORT = MP_PROP_SRC_LAST,
};

/**
 * @brief TCP server source element.
 *
 * Extends the base @ref mp_src to receive pipeline buffers from a remote TCP
 * client. The element listens on a configurable port and blocks in
 * MP_STATE_CHANGE_READY_TO_PAUSED until exactly one client connects.
 * When CONFIG_NET_IPV6 is enabled, a single IPv6 socket accepts both IPv4
 * and IPv6 clients via IPv4-mapped addresses (CONFIG_NET_IPV4_MAPPING_TO_IPV6).
 */
struct mp_tcpsrc {
	/** Base source element. */
	struct mp_src src;
	/** Internal receive buffer pool. */
	struct mp_buffer_pool pool;
	/** TCP port number. */
	uint16_t port;
	/** Listening socket file descriptor (-1 when closed). */
	int server_fd;
	/** Accepted client socket file descriptor (-1 when not connected). */
	int client_fd;
};

/**
 * @brief Initialize a TCP server source element.
 *
 * @param self Pointer to the element to initialize.
 */
void mp_tcpsrc_init(struct mp_element *self);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_NET_MP_TCPSRC_H_ */
