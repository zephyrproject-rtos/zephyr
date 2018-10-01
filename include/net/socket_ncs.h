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

/* NCS specific protocol/address families. */

#define PF_LTE 102 /**< Protocol family specific to LTE. */
#define AF_LTE PF_LTE /**< Address family specific to LTE. */

/* NCS specific protocol types. */

/** Protocol numbers for LTE protocols */
enum net_lte_protocol {
	NPROTO_AT = 513,
	NPROTO_PDN = 514
};

/** Protocol numbers for LOCAL protocols */
enum net_local_protocol {
	NPROTO_DFU = 515
};

/* NCS specific socket types. */

#define SOCK_MGMT 4 /**< Management socket type. */

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

/* NCS specific PDN options */

/** Protocol level for PDN. */
#define SOL_PDN 514

/* Socket options for SOL_PDN level */
#define SO_PDN_AF 1
#define SO_PDN_CONTEXT_ID 2
#define SO_PDN_STATE 3

/* NCS specific DFU options */

/** Protocol level for DFU. */
#define SOL_DFU 515

/* Socket options for SOL_DFU level */
#define SO_DFU_FW_VERSION 1
#define SO_DFU_RESOURCES 2
#define SO_DFU_TIMEO 3
#define SO_DFU_APPLY 4
#define SO_DFU_REVERT 5
#define SO_DFU_BACKUP_DELETE 6
#define SO_DFU_OFFSET 7
#define SO_DFU_ERROR 20

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
