/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp
 * @brief TCP server source element for the MP znet plugin.
 *
 * Opens a TCP server socket, accepts one client connection, and emits
 * every received byte chunk as a downstream buffer. Designed for raw
 * byte-stream ingestion (e.g. screen mirror demo: Android phone streams
 * MJPEG frames to a Zephyr device for decode and display).
 */

#ifndef ZEPHYR_INCLUDE_MP_ZNET_MP_ZTCPSRC_H_
#define ZEPHYR_INCLUDE_MP_ZNET_MP_ZTCPSRC_H_

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_property.h>
#include <zephyr/mp/core/mp_src.h>

/** Cast a generic element pointer to @ref mp_ztcpsrc. */
#define MP_ZTCPSRC(self) ((struct mp_ztcpsrc *)(self))

/**
 * @brief TCP source property identifiers.
 *
 * Extends the base source properties defined in @ref mp_property.h.
 */
enum prop_ztcpsrc {
	/** TCP port to listen on (uint16_t). Default: CONFIG_MP_PLUGIN_ZNET_DEFAULT_SRC_PORT. */
	PROP_ZTCPSRC_PORT = PROP_SRC_LAST + 1,
};

/**
 * @brief TCP server source element.
 *
 * Extends @ref mp_src to receive pipeline buffers from a remote TCP client.
 * The element listens on a configurable port and blocks in
 * MP_STATE_CHANGE_READY_TO_PAUSED until exactly one client connects.
 * When CONFIG_NET_IPV6 is enabled, a single IPv6 socket accepts both IPv4
 * and IPv6 clients via IPv4-mapped addresses (CONFIG_NET_IPV4_MAPPING_TO_IPV6).
 */
struct mp_ztcpsrc {
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
void mp_ztcpsrc_init(struct mp_element *self);

#endif /* ZEPHYR_INCLUDE_MP_ZNET_MP_ZTCPSRC_H_ */
