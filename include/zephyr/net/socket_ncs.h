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

/* NCS specific TLS level socket options */

/** Socket option to set DTLS handshake timeout, specifically for nRF sockets.
 *  The option accepts an integer, indicating the total handshake timeout,
 *  including retransmissions, in seconds.
 *  Accepted values for the option are: 1, 3, 7, 15, 31, 63, 123.
 */
#define TLS_DTLS_HANDSHAKE_TIMEO 18

/** Socket option to save DTLS connection, specifically for nRF sockets.
 */
#define TLS_DTLS_CONN_SAVE 19

/** Socket option to load DTLS connection, specifically for nRF sockets.
 */
#define TLS_DTLS_CONN_LOAD 20

/** Socket option to get result of latest TLS/DTLS completed handshakes end status,
 *  specifically for nRF sockets.
 *  The option accepts an integer, indicating the setting.
 *  Accepted vaules for the option are: 0 and 1.
 */
#define TLS_DTLS_HANDSHAKE_STATUS 21

/* Valid values for TLS_SESSION_CACHE option */
#define TLS_SESSION_CACHE_DISABLED 0 /**< Disable TLS session caching. */
#define TLS_SESSION_CACHE_ENABLED 1 /**< Enable TLS session caching. */

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
#define SO_EXCEPTIONAL_DATA 33
/** sockopt: Keep socket open when its PDN connection is lost
 *           or the device is put into flight mode.
 */
#define SO_KEEPOPEN 34
/** sockopt: bind to PDN */
#define SO_BINDTOPDN 40
/** sockopt: Release Assistance Indication feature: This will indicate that the
 *  application will not send any more data.
 *
 *  @note This socket option requires the socket to be connected.
 *
 *  @deprecated use @ref SO_RAI with value @ref RAI_NO_DATA instead.
 */
#define SO_RAI_NO_DATA 50
/** sockopt: Release Assistance Indication feature: This will indicate that the
 *  next call to send/sendto will be the last one for some time.
 *
 *  @deprecated use @ref SO_RAI with value @ref RAI_LAST instead.
 */
#define SO_RAI_LAST 51
/** sockopt: Release Assistance Indication feature: This will indicate that
 *  after the next call to send/sendto, the application is expecting to receive
 *  one more data packet before this socket will not be used again for some time.
 *
 *  @deprecated use @ref SO_RAI with value @ref RAI_ONE_RESP instead.
 */
#define SO_RAI_ONE_RESP 52
/** sockopt: Release Assistance Indication feature: If a client application
 *  expects to use the socket more it can indicate that by setting this socket
 *  option before the next send call which will keep the network up longer.
 *
 *  @deprecated use @ref SO_RAI with value @ref RAI_ONGOING instead.
 */
#define SO_RAI_ONGOING 53
/** sockopt: Release Assistance Indication feature: If a server application
 *  expects to use the socket more it can indicate that by setting this socket
 *  option before the next send call.
 *
 *  @deprecated use @ref SO_RAI with value @ref RAI_WAIT_MORE instead.
 */
#define SO_RAI_WAIT_MORE 54

/** sockopt: Release assistance indication (RAI).
 *  The option accepts an integer, indicating the type of RAI.
 *  Accepted values for the option are: @ref RAI_NO_DATA, @ref RAI_LAST, @ref RAI_ONE_RESP,
 *  @ref RAI_ONGOING, @ref RAI_WAIT_MORE.
 */
#define SO_RAI 61

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
#define SO_SILENCE_ALL 30

/* NCS specific IPPROTO_IP level socket options */

/** sockopt: enable IPv4 ICMP replies */
#define SO_IP_ECHO_REPLY 31

/* NCS specific IPPROTO_IPV6 level socket options */

/** sockopt: enable IPv6 ICMP replies */
#define SO_IPV6_ECHO_REPLY 32

/* NCS specific TCP level socket options */

/** sockopt: Configurable TCP server session timeout in minutes.
 * Range is 0 to 135. 0 is no timeout and 135 is 2 h 15 min. Default is 0 (no timeout).
 */
#define SO_TCP_SRV_SESSTIMEO 55

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
