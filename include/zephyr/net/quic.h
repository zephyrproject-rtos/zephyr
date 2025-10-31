/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief QUIC implementation for Zephyr.
 */

#ifndef ZEPHYR_INCLUDE_NET_QUIC_H_
#define ZEPHYR_INCLUDE_NET_QUIC_H_

/**
 * @brief QUIC library
 * @defgroup quic QUIC Library
 * @since 4.5
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Stream direction */
enum quic_stream_direction {
	QUIC_STREAM_BIDIRECTIONAL  = 0x00, /**< Bidirectional stream */
	QUIC_STREAM_UNIDIRECTIONAL = 0x02, /**< Unidirectional stream */
};

/** Stream initiator */
enum quic_stream_initiator {
	QUIC_STREAM_CLIENT = 0x00, /**< Client initiated stream */
	QUIC_STREAM_SERVER = 0x01, /**< Server initiated stream */
};

/** QUIC socket option level for getsockopt/setsockopt */
#define ZSOCK_SOL_QUIC 284

/** QUIC socket options for use with getsockopt/setsockopt at ZSOCK_SOL_QUIC level */
enum {
	/** Get stream type (returns combination of direction | initiator bits) */
	ZSOCK_QUIC_SO_STREAM_TYPE = 1,
	/**
	 * Add an intermediate certificate to the certificate chain.
	 * Option value is pointer to DER-encoded certificate data.
	 * Option length is the certificate data length.
	 * Call multiple times to add multiple intermediate certificates.
	 */
	ZSOCK_QUIC_SO_CERT_CHAIN_ADD = 2,
};

/**
 * @brief Creates a new QUIC connection socket.
 *
 * @param remote_addr Remote connection endpoint address.
 * @param local_addr Local connection endpoint address. If set to NULL,
 *        the local address is selected according to the remote address.
 *
 * @return New QUIC connection socket on success, <0 on failure.
 */
int quic_connection_open(const struct net_sockaddr *remote_addr,
			 const struct net_sockaddr *local_addr);

/**
 * @brief Closes the QUIC socket.
 *
 * @param  sock QUIC connection socket to close.
 *
 * @return 0 success, <0 on failure.
 */
int quic_connection_close(int sock);

/**
 * @brief Creates a new QUIC stream socket.
 *
 * @param  connection_sock Connection to create the stream on.
 * @param  initiator       Stream initiator (client or server).
 * @param  direction       Stream direction (uni- or bidirectional).
 * @param  priority        Stream priority (0-255).
 *
 * @return New QUIC stream socket on success, <0 on failure.
 */
int quic_stream_open(int connection_sock,
		     enum quic_stream_initiator initiator,
		     enum quic_stream_direction direction,
		     uint8_t priority);

/**
 * @brief Closes the QUIC stream socket.
 *
 * @param  sock QUIC stream socket to close.
 *
 * @return 0 success, <0 on failure.
 */
int quic_stream_close(int sock);

/**
 * @brief Checks if the given socket is a QUIC stream socket.
 *
 * @param  sock Socket to check.
 *
 * @return true if the socket is a QUIC stream socket, false otherwise.
 */
bool quic_is_stream_socket(int sock);

/**
 * @brief Checks if the given socket is a QUIC connection socket.
 *
 * @param  sock Socket to check.
 *
 * @return true if the socket is a QUIC connection socket, false otherwise.
 */
bool quic_is_connection_socket(int sock);

/**
 * @brief Return the stream id associated with the given QUIC stream socket.
 *
 * @param sock Socket to check.
 * @param stream_id Pointer to store the stream id.
 *
 * @return 0 on success, <0 on failure.
 */
int quic_stream_get_id(int sock, uint64_t *stream_id);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_QUIC_H_ */
