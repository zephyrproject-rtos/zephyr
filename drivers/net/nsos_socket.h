/**
 * Copyright (c) 2024 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRIVERS_NET_NSOS_SOCKET_H__
#define __DRIVERS_NET_NSOS_SOCKET_H__

#include <stdint.h>

/**
 * @name Socket level options (NSOS_MID_SOL_SOCKET)
 * @{
 */
/** Socket-level option */
#define NSOS_MID_SOL_SOCKET 1

/* Socket options for NSOS_MID_SOL_SOCKET level */

/** Recording debugging information (ignored, for compatibility) */
#define NSOS_MID_SO_DEBUG 1
/** address reuse */
#define NSOS_MID_SO_REUSEADDR 2
/** Type of the socket */
#define NSOS_MID_SO_TYPE 3
/** Async error */
#define NSOS_MID_SO_ERROR 4
/** Bypass normal routing and send directly to host (ignored, for compatibility) */
#define NSOS_MID_SO_DONTROUTE 5
/** Transmission of broadcast messages is supported (ignored, for compatibility) */
#define NSOS_MID_SO_BROADCAST 6

/** Size of socket send buffer */
#define NSOS_MID_SO_SNDBUF 7
/** Size of socket recv buffer */
#define NSOS_MID_SO_RCVBUF 8

/** Enable sending keep-alive messages on connections */
#define NSOS_MID_SO_KEEPALIVE 9
/** Place out-of-band data into receive stream (ignored, for compatibility) */
#define NSOS_MID_SO_OOBINLINE 10
/** Socket priority */
#define NSOS_MID_SO_PRIORITY 12
/** Socket lingers on close (ignored, for compatibility) */
#define NSOS_MID_SO_LINGER 13
/** Allow multiple sockets to reuse a single port */
#define NSOS_MID_SO_REUSEPORT 15

/** Receive low watermark (ignored, for compatibility) */
#define NSOS_MID_SO_RCVLOWAT 18
/** Send low watermark (ignored, for compatibility) */
#define NSOS_MID_SO_SNDLOWAT 19

/**
 * Receive timeout
 * Applies to receive functions like recv(), but not to connect()
 */
#define NSOS_MID_SO_RCVTIMEO 20
/** Send timeout */
#define NSOS_MID_SO_SNDTIMEO 21

/** Bind a socket to an interface */
#define NSOS_MID_SO_BINDTODEVICE	25

/** Socket accepts incoming connections (ignored, for compatibility) */
#define NSOS_MID_SO_ACCEPTCONN 30

/** Timestamp TX packets */
#define NSOS_MID_SO_TIMESTAMPING 37
/** Protocol used with the socket */
#define NSOS_MID_SO_PROTOCOL 38

/** Domain used with SOCKET */
#define NSOS_MID_SO_DOMAIN 39

/** Enable SOCKS5 for Socket */
#define NSOS_MID_SO_SOCKS5 60

/** Socket TX time (when the data should be sent) */
#define NSOS_MID_SO_TXTIME 61

struct nsos_mid_timeval {
	int64_t tv_sec;
	int64_t tv_usec;
};

/** @} */

/**
 * @name TCP level options (NSOS_MID_IPPROTO_TCP)
 * @{
 */
/* Socket options for NSOS_MID_IPPROTO_TCP level */
/** Disable TCP buffering (ignored, for compatibility) */
#define NSOS_MID_TCP_NODELAY 1
/** Start keepalives after this period (seconds) */
#define NSOS_MID_TCP_KEEPIDLE 2
/** Interval between keepalives (seconds) */
#define NSOS_MID_TCP_KEEPINTVL 3
/** Number of keepalives before dropping connection */
#define NSOS_MID_TCP_KEEPCNT 4

/** @} */

/**
 * @name IPv6 level options (NSOS_MID_IPPROTO_IPV6)
 * @{
 */
/* Socket options for NSOS_MID_IPPROTO_IPV6 level */
/** Set the unicast hop limit for the socket. */
#define NSOS_MID_IPV6_UNICAST_HOPS	16

/** Set the multicast hop limit for the socket. */
#define NSOS_MID_IPV6_MULTICAST_HOPS 18

/** Join IPv6 multicast group. */
#define NSOS_MID_IPV6_ADD_MEMBERSHIP 20

/** Leave IPv6 multicast group. */
#define NSOS_MID_IPV6_DROP_MEMBERSHIP 21

/** Don't support IPv4 access */
#define NSOS_MID_IPV6_V6ONLY 26

/** Pass an IPV6_RECVPKTINFO ancillary message that contains a
 *  in6_pktinfo structure that supplies some information about the
 *  incoming packet. See RFC 3542.
 */
#define NSOS_MID_IPV6_RECVPKTINFO 49

/** @} */

#endif /* __DRIVERS_NET_NSOS_SOCKET_H__ */
