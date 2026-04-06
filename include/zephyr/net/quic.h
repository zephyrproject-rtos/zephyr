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

/** Quic statistics */
struct net_stats_quic {
	/** Number of handshake init packets received */
	uint32_t handshake_init_rx;
	/** Number of handshake init packets sent */
	uint32_t handshake_init_tx;
	/** Number of handshake response packets received */
	uint32_t handshake_resp_rx;
	/** Number of handshake response packets sent */
	uint32_t handshake_resp_tx;
	/** Number of invalid handshake errors */
	uint32_t invalid_handshake;
	/** Number of peer not found errors */
	uint32_t peer_not_found;
	/** Number of invalid packets */
	uint32_t invalid_packet;
	/** Number of invalid key errors */
	uint32_t invalid_key;
	/** Number of invalid packet length errors */
	uint32_t invalid_packet_len;
	/** Number of decrypt failed errors */
	uint32_t decrypt_failed;
	/** Number of dropped RX packets */
	uint32_t drop_rx;
	/** Number of dropped TX packets */
	uint32_t drop_tx;
	/** Number of allocation failures */
	uint32_t alloc_failed;
	/** Number of valid packets received */
	uint32_t valid_rx;
	/** Number of valid packets sent */
	uint32_t valid_tx;
};

/** Quic global statistics */
struct net_stats_quic_global {
	/** Number of any (valid or invalid) packets received */
	uint32_t packets_rx;
	/** Total number of connections opened */
	uint32_t connections_opened;
	/** Total number of connections closed */
	uint32_t connections_closed;
	/** Total number of streams opened */
	uint32_t streams_opened;
	/** Total number of streams closed */
	uint32_t streams_closed;
	/** Total number of connection open failures */
	uint32_t connection_open_failed;
	/** Total number of stream open failures */
	uint32_t stream_open_failed;
	/** Total number of connection close failures */
	uint32_t connection_close_failed;
	/** Total number of stream close failures */
	uint32_t stream_close_failed;
};

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
	 * Option value is a pointer to a sec_tag_t referencing a credential
	 * previously registered via tls_credential_add() (with type
	 * TLS_CREDENTIAL_CA_CERTIFICATE or TLS_CREDENTIAL_PUBLIC_CERTIFICATE).
	 * Option length must be sizeof(sec_tag_t).
	 * Call multiple times to add multiple intermediate certificates.
	 */
	ZSOCK_QUIC_SO_CERT_CHAIN_ADD = 2,

	/**
	 * Delete an intermediate certificate from the certificate chain.
	 * Option value is a pointer to a sec_tag_t referencing a credential
	 * previously added by ZSOCK_QUIC_SO_CERT_CHAIN_ADD option.
	 * Option length must be sizeof(sec_tag_t) or set to 0. If set to 0,
	 * the option value can be omitted by setting it to NULL.
	 * If set to 0, all intermediate certificates are removed.
	 */
	ZSOCK_QUIC_SO_CERT_CHAIN_DEL = 3,

	/** Set the error code to use when sending STOP_SENDING frame on stream close */
	ZSOCK_QUIC_SO_STOP_SENDING_CODE = 4,
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
