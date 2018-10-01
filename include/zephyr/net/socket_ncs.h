/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKET_NCS_H_
#define ZEPHYR_INCLUDE_NET_SOCKET_NCS_H_

/**
 * @file
 * @brief NCS specific additions to the BSD sockets API definitions
 */

#ifdef __cplusplus
extern "C" {
#endif

/* When CONFIG_NET_SOCKETS_OFFLOAD is enabled, offloaded sockets take precedence
 * when creating a new socket. Combine this flag with a socket type when
 * creating a socket, to enforce native socket creation (e. g. SOCK_STREAM | SOCK_NATIVE).
 * If it's desired to create a native TLS socket, but still offload the
 * underlying TCP/UDP socket, use e. g. SOCK_STREAM | SOCK_NATIVE_TLS.
 */
#define SOCK_NATIVE 0x80000000
#define SOCK_NATIVE_TLS 0x40000000

/** Define a base for NCS specific socket options to prevent overlaps with Zephyr's socket options.
 */
#define NET_SOCKET_NCS_BASE 1000

/* NCS specific TLS level socket options */

/** Socket option to set DTLS handshake timeout, specifically for nRF sockets.
 *  The option accepts an integer, indicating the total handshake timeout,
 *  including retransmissions, in seconds.
 *  Accepted values for the option are: 1, 3, 7, 15, 31, 63, 123.
 */
#define TLS_DTLS_HANDSHAKE_TIMEO (NET_SOCKET_NCS_BASE + 18)

/** Socket option to save DTLS connection, specifically for nRF sockets.
 */
#define TLS_DTLS_CONN_SAVE (NET_SOCKET_NCS_BASE + 19)

/** Socket option to load DTLS connection, specifically for nRF sockets.
 */
#define TLS_DTLS_CONN_LOAD (NET_SOCKET_NCS_BASE + 20)

/** Socket option to get result of latest TLS/DTLS completed handshakes end status,
 *  specifically for nRF sockets.
 *  The option accepts an integer, indicating the setting.
 *  Accepted vaules for the option are: 0 and 1.
 */
#define TLS_DTLS_HANDSHAKE_STATUS (NET_SOCKET_NCS_BASE + 21)

/* Valid values for TLS_DTLS_HANDSHAKE_TIMEO option */
#define TLS_DTLS_HANDSHAKE_TIMEO_NONE 0 /**< No timeout */
#define TLS_DTLS_HANDSHAKE_TIMEO_1S 1 /**< 1 second */
#define TLS_DTLS_HANDSHAKE_TIMEO_3S 3 /**< 1s + 2s */
#define TLS_DTLS_HANDSHAKE_TIMEO_7S 7 /**< 1s + 2s + 4s */
#define TLS_DTLS_HANDSHAKE_TIMEO_15S 15 /**< 1s + 2s + 4s + 8s */
#define TLS_DTLS_HANDSHAKE_TIMEO_31S 31 /**< 1s + 2s + 4s + 8s + 16s */
#define TLS_DTLS_HANDSHAKE_TIMEO_63S 63 /**< 1s + 2s + 4s + 8s + 16s + 32s */
#define TLS_DTLS_HANDSHAKE_TIMEO_123S 123 /**< 1s + 2s + 4s + 8s + 16s + 32s + 60s */

/* Valid values for TLS_DTLS_HANDSHAKE_STATUS option */
#define TLS_DTLS_HANDSHAKE_STATUS_FULL		0
#define TLS_DTLS_HANDSHAKE_STATUS_CACHED	1

/* NCS specific socket options */

/** sockopt: enable sending data as part of exceptional events */
#define SO_EXCEPTIONAL_DATA (NET_SOCKET_NCS_BASE + 33)
/** sockopt: Keep socket open when its PDN connection is lost
 *           or the device is put into flight mode.
 */
#define SO_KEEPOPEN (NET_SOCKET_NCS_BASE + 34)
/** sockopt: bind to PDN */
#define SO_BINDTOPDN (NET_SOCKET_NCS_BASE + 40)

/** sockopt: Release assistance indication (RAI).
 *  The option accepts an integer, indicating the type of RAI.
 *  Accepted values for the option are: @ref RAI_NO_DATA, @ref RAI_LAST, @ref RAI_ONE_RESP,
 *  @ref RAI_ONGOING, @ref RAI_WAIT_MORE.
 */
#define SO_RAI (NET_SOCKET_NCS_BASE + 61)

/** Release assistance indication (RAI).
 *  Indicate that the application does not intend to send more data.
 *  This applies immediately and lets the modem exit connected mode more
 *  quickly.
 *
 *  @note This requires the socket to be connected.
 */
#define RAI_NO_DATA 1
/** Release assistance indication (RAI).
 *  Indicate that the application does not intend to send more data
 *  after the next call to send() or sendto().
 *  This lets the modem exit connected mode more quickly after sending the data.
 */
#define RAI_LAST 2
/** Release assistance indication (RAI).
 *  Indicate that the application is expecting to receive just one data packet
 *  after the next call to send() or sendto().
 *  This lets the modem exit connected mode more quickly after having received the data.
 */
#define RAI_ONE_RESP 3
/** Release assistance indication (RAI).
 *  Indicate that the socket is in active use by a client application.
 *  This lets the modem stay in connected mode longer.
 */
#define RAI_ONGOING 4
/** Release assistance indication (RAI).
 *  Indicate that the socket is in active use by a server application.
 *  This lets the modem stay in connected mode longer.
 */
#define RAI_WAIT_MORE 5

/* NCS specific IPPROTO_ALL level socket options */

/** IPv4 and IPv6 protocol level (pseudo-val) for nRF sockets. */
#define IPPROTO_ALL 512
/** sockopt: disable all replies to unexpected traffics */
#define SO_SILENCE_ALL (NET_SOCKET_NCS_BASE + 30)

/* NCS specific IPPROTO_IP level socket options */

/** sockopt: enable IPv4 ICMP replies */
#define SO_IP_ECHO_REPLY (NET_SOCKET_NCS_BASE + 31)

/* NCS specific IPPROTO_IPV6 level socket options */

/** sockopt: enable IPv6 ICMP replies */
#define SO_IPV6_ECHO_REPLY (NET_SOCKET_NCS_BASE + 32)

/** sockopt: Delay IPv6 address refresh during power saving mode  */
#define SO_IPV6_DELAYED_ADDR_REFRESH (NET_SOCKET_NCS_BASE + 62)

/* NCS specific TCP level socket options */

/** sockopt: Configurable TCP server session timeout in minutes.
 * Range is 0 to 135. 0 is no timeout and 135 is 2 h 15 min. Default is 0 (no timeout).
 */
#define SO_TCP_SRV_SESSTIMEO (NET_SOCKET_NCS_BASE + 55)

/* NCS specific gettaddrinfo() flags */

/** Assume `service` contains a Packet Data Network (PDN) ID.
 *  When specified together with the AI_NUMERICSERV flag,
 *  `service` shall be formatted as follows: "port:pdn_id"
 *  where "port" is the port number and "pdn_id" is the PDN ID.
 *  Example: "8080:1", port 8080 PDN ID 1.
 *  Example: "42:0", port 42 PDN ID 0.
 */
#define AI_PDNSERV 0x1000

/* NCS specific send() and sendto() flags */

/** Request a blocking send operation until the request is acknowledged.
 *  When used in send() or sendto(), the request will not return until the
 *  send operation is completed by lower layers, or until the timeout, given by the SO_SNDTIMEO
 *  socket option, is reached. Valid timeout values are 1 to 600 seconds.
 */
#define MSG_WAITACK 0x200

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_NCS_H_ */
