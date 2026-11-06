/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mpipe_tcp_server_src.h @brief TCP server source element of the mpipe net plugin. */

#ifndef ZEPHYR_INCLUDE_MPIPE_NET_MPIPE_TCP_SERVER_SRC_H_
#define ZEPHYR_INCLUDE_MPIPE_NET_MPIPE_TCP_SERVER_SRC_H_

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_src.h>

/** @addtogroup mpipe_plugins */
/** @{ */
/** @defgroup mpipe_net Network */
/** @} */

/** @addtogroup mpipe_net */
/** @{ */

/** TCP server source property identifiers, extending @ref mpipe_prop_src. */
enum mpipe_prop_tcp_server_src {
	/** TCP port to listen on (uint16_t). */
	MPIPE_PROP_TCP_SERVER_SRC_PORT = MPIPE_PROP_SRC_LAST,
};

/** TCP server source: emits what one client sends, EOS when it disconnects. */
struct mpipe_tcp_server_src {
	/** Base source element. */
	struct mpipe_src src;
	/** Receive buffer pool. */
	struct mpipe_buffer_pool pool;
	/** TCP port number. */
	uint16_t port;
	/** Listening socket, -1 when closed. */
	int server_fd;
	/** Client socket, -1 when not connected. */
	int client_fd;
};

/** Initialize a TCP server source. @param tsrc Element. @param id Id. @return 0 or -errno. */
int mpipe_tcp_server_src_init(struct mpipe_tcp_server_src *tsrc, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_NET_MPIPE_TCP_SERVER_SRC_H_ */
