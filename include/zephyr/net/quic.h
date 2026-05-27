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
 *
 * @note Server mode sends Version Negotiation for unsupported versions and can
 * enforce the RFC 9000 anti-amplification limit before peer address
 * validation, including Retry and NEW_TOKEN-based address-validation tokens.
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

	/**
	 * Export or import resumable client session state.
	 *
	 * The option value is a pointer to struct quic_session_state. Use
	 * getsockopt() on a connected client connection socket after a
	 * NewSessionTicket has been received, then pass the returned state to
	 * setsockopt() on a new client connection socket before opening the first
	 * stream to attempt session resumption.
	 */
	ZSOCK_QUIC_SO_SESSION_STATE = 5,

	/**
	 * Enable or disable server-side session ticket issuance.
	 *
	 * The option value is a pointer to an int. Set a non-zero value on a
	 * listening or server-side connection socket before the handshake to have
	 * the server send NewSessionTicket after the handshake completes.
	 */
	ZSOCK_QUIC_SO_SESSION_TICKET_ENABLE = 6,
};

/** Version of struct quic_session_state. */
#define QUIC_SESSION_STATE_VERSION 2U

/** Maximum serialized ticket length exported through struct quic_session_state. */
#define QUIC_MAX_SESSION_TICKET_LEN 256U

/** Maximum resumable PSK length exported through struct quic_session_state. */
#define QUIC_MAX_RESUMPTION_PSK_LEN 48U

/**
 * @brief Remembered peer transport parameters for resumption.
 *
 * These are the server limits associated with the stored ticket. They are used
 * to initialize early flow-control state and to validate a later 0-RTT-capable
 * resumption attempt.
 */
struct quic_session_transport_params {
	/** True when the remembered transport-parameter snapshot is present. */
	bool valid;
	/** Remembered initial_max_data. */
	uint64_t initial_max_data;
	/** Remembered initial_max_stream_data_bidi_local. */
	uint64_t initial_max_stream_data_bidi_local;
	/** Remembered initial_max_stream_data_bidi_remote. */
	uint64_t initial_max_stream_data_bidi_remote;
	/** Remembered initial_max_stream_data_uni. */
	uint64_t initial_max_stream_data_uni;
	/** Remembered initial_max_streams_bidi. */
	uint64_t initial_max_streams_bidi;
	/** Remembered initial_max_streams_uni. */
	uint64_t initial_max_streams_uni;
	/** Remembered max_idle_timeout in milliseconds. */
	uint64_t max_idle_timeout;
	/** Remembered max_udp_payload_size. */
	uint16_t max_udp_payload_size;
};

/**
 * @brief Resumable QUIC client session state.
 *
 * The structure is exported with getsockopt() and imported with setsockopt()
 * using ZSOCK_QUIC_SO_SESSION_STATE on a connection socket.
 */
struct quic_session_state {
	/** Structure format version, must be QUIC_SESSION_STATE_VERSION. */
	uint16_t version;
	/** TLS 1.3 cipher suite associated with the resumable state. */
	uint16_t cipher_suite;
	/** Ticket lifetime in seconds from NewSessionTicket. */
	uint32_t ticket_lifetime;
	/** Random ticket_age_add value from NewSessionTicket. */
	uint32_t ticket_age_add;
	/** Max early data size from NewSessionTicket, or 0 when not permitted. */
	uint32_t max_early_data_size;
	/** Local monotonic timestamp in milliseconds when the ticket was received. */
	uint64_t issue_time_ms;
	/** Length of the opaque ticket identity. */
	uint16_t ticket_len;
	/** Length of the derived resumption PSK. */
	uint16_t psk_len;
	/** Opaque ticket identity from NewSessionTicket. */
	uint8_t ticket[QUIC_MAX_SESSION_TICKET_LEN];
	/** Derived resumption PSK used for the next resumed handshake. */
	uint8_t psk[QUIC_MAX_RESUMPTION_PSK_LEN];
	/** Remembered peer transport parameters associated with the ticket. */
	struct quic_session_transport_params transport_params;
};

/**
 * @brief Creates a new QUIC connection socket.
 *
 * @details Creates a new QUIC connection context. This serves as a foundation
 *          for all subsequent communication.
 *
 * @param remote_addr Remote connection endpoint address. In client mode, this
 *        is the server address to connect to. In server mode, this is NULL
 *        or unspecified if binding a listener.
 * @param local_addr Local connection endpoint address. In client mode, if set
 *        to NULL, the system will auto-bind the socket to an ephemeral port
 *        and select the local address. In server mode, this is the address to
 *        listen on.
 *
 * @return New QUIC connection socket on success, <0 on failure.
 */
int quic_connection_open(const struct net_sockaddr *remote_addr,
			 const struct net_sockaddr *local_addr);

/**
 * @brief Closes the QUIC socket.
 *
 * @details Closes the connection and terminates the TLS session.
 *          Does the same thing as ``zsock_close`` for a connection socket.
 *
 * @param  sock QUIC connection socket to close.
 *
 * @return 0 success, <0 on failure.
 */
int quic_connection_close(int sock);

/**
 * @brief Creates a new QUIC stream socket within an established QUIC connection.
 *
 * @param  connection_sock Connection to create the stream on. This is the
 *         socket id returned by quic_connection_open().
 * @param  initiator Stream initiator (client or server). This is either
 *         QUIC_STREAM_CLIENT or QUIC_STREAM_SERVER.
 * @param  direction Stream direction (uni- or bidirectional). If set to
 *         QUIC_STREAM_BIDIRECTIONAL, then both sides can read/write.
 *         If set to QUIC_STREAM_UNIDIRECTIONAL, then only the initiator can write.
 * @param  priority Priority level (0-255) for scheduling stream data.
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
 * @details Closes a specific stream without closing the underlying connection.
 *          Does the same thing as ``zsock_close`` for a stream socket.
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
