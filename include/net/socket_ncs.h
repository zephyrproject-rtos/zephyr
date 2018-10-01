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

/* NCS specific TLS options */

/** Socket option to control TLS session caching. Accepted values:
 *  - 0 - Disabled.
 *  - 1 - Enabled.
 */
#define TLS_SESSION_CACHE 10
/** Socket option to purge session cache immediately.
 *  This option accepts any value.
 */
#define TLS_SESSION_CACHE_PURGE 11
/** Socket option to set DTLS handshake timeout, specifically for nRF sockets.
 *  The option accepts an integer, indicating the total handshake timeout,
 *  including retransmissions, in seconds.
 *  Accepted values for the option are: 1, 3, 7, 15, 31, 63, 123.
 */
#define TLS_DTLS_HANDSHAKE_TIMEO 12

/* Valid values for TLS_SESSION_CACHE option */
#define TLS_SESSION_CACHE_DISABLED 0 /**< Disable TLS session caching. */
#define TLS_SESSION_CACHE_ENABLED 1 /**< Enable TLS session caching. */

/* Valid values for TLS_DTLS_HANDSHAKE_TIMEO option */
#define TLS_DTLS_HANDSHAKE_TIMEO_1S 1 /**< 1 second */
#define TLS_DTLS_HANDSHAKE_TIMEO_3S 3 /**< 1s + 2s */
#define TLS_DTLS_HANDSHAKE_TIMEO_7S 7 /**< 1s + 2s + 4s */
#define TLS_DTLS_HANDSHAKE_TIMEO_15S 15 /**< 1s + 2s + 4s + 8s */
#define TLS_DTLS_HANDSHAKE_TIMEO_31S 31 /**< 1s + 2s + 4s + 8s + 16s */
#define TLS_DTLS_HANDSHAKE_TIMEO_63S 63 /**< 1s + 2s + 4s + 8s + 16s + 32s */
#define TLS_DTLS_HANDSHAKE_TIMEO_123S 123 /**< 1s + 2s + 4s + 8s + 16s + 32s + 60s */

/* NCS specific socket options */

/** sockopt: disable all replies to unexpected traffics */
#define SO_SILENCE_ALL 30
/** sockopt: disable IPv4 ICMP replies */
#define SO_IP_ECHO_REPLY 31
/** sockopt: disable IPv6 ICMP replies */
#define SO_IPV6_ECHO_REPLY 32
/** sockopt: Release Assistance Indication feature: This will indicate that the
 *  next call to send/sendto will be the last one for some time.
 */
#define SO_RAI_LAST 50
/** sockopt: Release Assistance Indication feature: This will indicate that the
 *  application will not send any more data.
 */
#define SO_RAI_NO_DATA 51
/** sockopt: Release Assistance Indication feature: This will indicate that
 *  after the next call to send/sendto, the application is expecting to receive
 *  one more data packet before this socket will not be used again for some time.
 */
#define SO_RAI_ONE_RESP 52
/** sockopt: Release Assistance Indication feature: If a client application
 *  expects to use the socket more it can indicate that by setting this socket
 *  option before the next send call which will keep the network up longer.
 */
#define SO_RAI_ONGOING 53
/** sockopt: Release Assistance Indication feature: If a server application
 *  expects to use the socket more it can indicate that by setting this socket
 *  option before the next send call.
 */
#define SO_RAI_WAIT_MORE 54
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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_NCS_H_ */
