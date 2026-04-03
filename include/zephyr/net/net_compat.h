/** @file
 * @brief Network namespace compatibility mode header
 *
 * Allows to use relevant network symbols without the "net_", "NET_" or
 * "ZSOCK_" prefixes.
 */

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_COMPAT_H_
#define ZEPHYR_INCLUDE_NET_NET_COMPAT_H_

/**
 * @brief Network namespace compatibility mode symbols
 * @defgroup net_compat Network namespace compatibility mode symbols
 * @since 4.4
 * @version 0.0.1
 * @ingroup networking
 * @{
 */

/* No need to generate documentation for the compatibility mode symbols */
/** @cond INTERNAL_HIDDEN */

#ifdef __cplusplus
extern "C" {
#endif

/* Prevent direct inclusion of this file. This is a private header
 * meant to be included via <net/net_ip.h>.
 */
#if !defined(ZEPHYR_INCLUDE_NET_COMPAT_MODE_SYMBOLS)
#error "This header file should not be included directly. " \
	"Include <zephyr/net/net_ip.h> instead."
#endif /* !defined(ZEPHYR_INCLUDE_NET_COMPAT_MODE_SYMBOLS) */

#define PF_UNSPEC   NET_PF_UNSPEC
#define PF_INET     NET_PF_INET
#define PF_INET6    NET_PF_INET6
#define PF_PACKET   NET_PF_PACKET
#define PF_CAN      NET_PF_CAN
#define PF_NET_MGMT NET_PF_NET_MGMT
#define PF_LOCAL    NET_PF_LOCAL
#define PF_UNIX     NET_PF_UNIX

#define AF_UNSPEC      NET_PF_UNSPEC
#define AF_INET        NET_PF_INET
#define AF_INET6       NET_PF_INET6
#define AF_PACKET      NET_PF_PACKET
#define AF_CAN         NET_PF_CAN
#define AF_NET_MGMT    NET_PF_NET_MGMT
#define AF_LOCAL       NET_PF_LOCAL
#define AF_UNIX        NET_PF_UNIX

#define IPPROTO_IP        NET_IPPROTO_IP
#define IPPROTO_ICMP      NET_IPPROTO_ICMP
#define IPPROTO_IGMP      NET_IPPROTO_IGMP
#define IPPROTO_ETH_P_ALL NET_IPPROTO_ETH_P_ALL
#define IPPROTO_IPIP      NET_IPPROTO_IPIP
#define IPPROTO_TCP       NET_IPPROTO_TCP
#define IPPROTO_UDP       NET_IPPROTO_UDP
#define IPPROTO_IPV6      NET_IPPROTO_IPV6
#define IPPROTO_ICMPV6    NET_IPPROTO_ICMPV6
#define IPPROTO_RAW       NET_IPPROTO_RAW
#define IPPROTO_TLS_1_0   NET_IPPROTO_TLS_1_0
#define IPPROTO_TLS_1_1   NET_IPPROTO_TLS_1_1
#define IPPROTO_TLS_1_2   NET_IPPROTO_TLS_1_2
#define IPPROTO_TLS_1_3   NET_IPPROTO_TLS_1_3
#define IPPROTO_DTLS_1_0  NET_IPPROTO_DTLS_1_0
#define IPPROTO_DTLS_1_2  NET_IPPROTO_DTLS_1_2

#define SOCK_STREAM NET_SOCK_STREAM
#define SOCK_DGRAM  NET_SOCK_DGRAM
#define SOCK_RAW    NET_SOCK_RAW

#define CAN_RAW NET_CAN_RAW
#define SOL_CAN_RAW  (NET_SOL_CAN_BASE + NET_CAN_RAW)
#define CAN_RAW_FILTER NET_CAN_RAW_FILTER

#define ntohs(x)  net_ntohs(x)
#define ntohl(x)  net_ntohl(x)
#define ntohll(x) net_ntohll(x)
#define htons(x)  net_htons(x)
#define htonl(x)  net_htonl(x)
#define htonll(x) net_htonll(x)

#define in_addr  net_in_addr
#define in6_addr net_in6_addr

#define sockaddr         net_sockaddr
#define sockaddr_in      net_sockaddr_in
#define sockaddr_in6     net_sockaddr_in6
#define sockaddr_ll      net_sockaddr_ll
#define sockaddr_can     net_sockaddr_can
#define sockaddr_un      net_sockaddr_un
#define sockaddr_nm      net_sockaddr_nm
#define sockaddr_ptr     net_sockaddr_ptr
#define sockaddr_in_ptr  net_sockaddr_in_ptr
#define sockaddr_in6_ptr net_sockaddr_in6_ptr
#define sockaddr_ll_ptr  net_sockaddr_ll_ptr
#define sockaddr_un_ptr  net_sockaddr_un_ptr
#define sockaddr_can_ptr net_sockaddr_can_ptr
#define sockaddr_storage net_sockaddr_storage

#define sa_family_t   net_sa_family_t
#define socklen_t     net_socklen_t

#define iovec                     net_iovec
#define msghdr                    net_msghdr
#define cmsghdr                   net_cmsghdr
#define ALIGN_H(x)                NET_ALIGN_H(x)
#define ALIGN_D(x)                NET_ALIGN_D(x)
#define CMSG_LEN(len)             NET_CMSG_LEN(len)
#define CMSG_SPACE(len)           NET_CMSG_SPACE(len)
#define CMSG_DATA(cmsg)           NET_CMSG_DATA(cmsg)
#define CMSG_FIRSTHDR(msghdr)     NET_CMSG_FIRSTHDR(msghdr)
#define CMSG_NXTHDR(msghdr, cmsg) NET_CMSG_NXTHDR(msghdr, cmsg)

#define PACKET_HOST       NET_PACKET_HOST
#define PACKET_BROADCAST  NET_PACKET_BROADCAST
#define PACKET_MULTICAST  NET_PACKET_MULTICAST
#define PACKET_OTHERHOST  NET_PACKET_OTHERHOST
#define PACKET_OUTGOING   NET_PACKET_OUTGOING
#define PACKET_LOOPBACK   NET_PACKET_LOOPBACK
#define PACKET_FASTROUTE  NET_PACKET_FASTROUTE

#define in6addr_any       net_in6addr_any
#define in6addr_loopback  net_in6addr_loopback

#define INADDR_ANY             NET_INADDR_ANY
#define INADDR_BROADCAST       NET_INADDR_BROADCAST
#define INADDR_ANY_INIT        NET_INADDR_ANY_INIT
#define INADDR_LOOPBACK_INIT   NET_INADDR_LOOPBACK_INIT
#define INET_ADDRSTRLEN        NET_INET_ADDRSTRLEN
#define INET6_ADDRSTRLEN       NET_INET6_ADDRSTRLEN
#define IN6ADDR_ANY_INIT       NET_IN6ADDR_ANY_INIT
#define IN6ADDR_LOOPBACK_INIT  NET_IN6ADDR_LOOPBACK_INIT

#if !defined(IFNAMSIZ)
#define IFNAMSIZ NET_IFNAMSIZ
#endif /* IFNAMSIZ */

#define in_pktinfo   net_in_pktinfo
#define ip_mreqn     net_ip_mreqn
#define ip_mreq      net_ip_mreq
#define ipv6_mreq    net_ipv6_mreq
#define in6_pktinfo  net_in6_pktinfo
#define ifreq        net_ifreq

#define SOL_TLS                           ZSOCK_SOL_TLS
#define TLS_SEC_TAG_LIST                  ZSOCK_TLS_SEC_TAG_LIST
#define TLS_HOSTNAME                      ZSOCK_TLS_HOSTNAME
#define TLS_CIPHERSUITE_LIST              ZSOCK_TLS_CIPHERSUITE_LIST
#define TLS_CIPHERSUITE_USED              ZSOCK_TLS_CIPHERSUITE_USED
#define TLS_PEER_VERIFY                   ZSOCK_TLS_PEER_VERIFY
#define TLS_DTLS_ROLE                     ZSOCK_TLS_DTLS_ROLE
#define TLS_ALPN_LIST                     ZSOCK_TLS_ALPN_LIST
#define TLS_DTLS_HANDSHAKE_TIMEOUT_MIN    ZSOCK_TLS_DTLS_HANDSHAKE_TIMEOUT_MIN
#define TLS_DTLS_HANDSHAKE_TIMEOUT_MAX    ZSOCK_TLS_DTLS_HANDSHAKE_TIMEOUT_MAX
#define TLS_CERT_NOCOPY                   ZSOCK_TLS_CERT_NOCOPY
#define TLS_NATIVE                        ZSOCK_TLS_NATIVE
#define TLS_SESSION_CACHE                 ZSOCK_TLS_SESSION_CACHE
#define TLS_SESSION_CACHE_PURGE           ZSOCK_TLS_SESSION_CACHE_PURGE
#define TLS_DTLS_CID                      ZSOCK_TLS_DTLS_CID
#define TLS_DTLS_CID_STATUS               ZSOCK_TLS_DTLS_CID_STATUS
#define TLS_DTLS_CID_VALUE                ZSOCK_TLS_DTLS_CID_VALUE
#define TLS_DTLS_PEER_CID_VALUE           ZSOCK_TLS_DTLS_PEER_CID_VALUE
#define TLS_DTLS_HANDSHAKE_ON_CONNECT     ZSOCK_TLS_DTLS_HANDSHAKE_ON_CONNECT
#define TLS_CERT_VERIFY_RESULT            ZSOCK_TLS_CERT_VERIFY_RESULT
#define TLS_CERT_VERIFY_CALLBACK          ZSOCK_TLS_CERT_VERIFY_CALLBACK
#define TLS_PEER_VERIFY_NONE              ZSOCK_TLS_PEER_VERIFY_NONE
#define TLS_PEER_VERIFY_OPTIONAL          ZSOCK_TLS_PEER_VERIFY_OPTIONAL
#define TLS_PEER_VERIFY_REQUIRED          ZSOCK_TLS_PEER_VERIFY_REQUIRED
#define TLS_DTLS_ROLE_CLIENT              ZSOCK_TLS_DTLS_ROLE_CLIENT
#define TLS_DTLS_ROLE_SERVER              ZSOCK_TLS_DTLS_ROLE_SERVER
#define TLS_CERT_NOCOPY_NONE              ZSOCK_TLS_CERT_NOCOPY_NONE
#define TLS_CERT_NOCOPY_OPTIONAL          ZSOCK_TLS_CERT_NOCOPY_OPTIONAL
#define TLS_SESSION_CACHE_DISABLED        ZSOCK_TLS_SESSION_CACHE_DISABLED
#define TLS_SESSION_CACHE_ENABLED         ZSOCK_TLS_SESSION_CACHE_ENABLED
#define TLS_DTLS_CID_DISABLED             ZSOCK_TLS_DTLS_CID_DISABLED
#define TLS_DTLS_CID_SUPPORTED            ZSOCK_TLS_DTLS_CID_SUPPORTED
#define TLS_DTLS_CID_ENABLED              ZSOCK_TLS_DTLS_CID_ENABLED
#define TLS_DTLS_CID_STATUS_DISABLED      ZSOCK_TLS_DTLS_CID_STATUS_DISABLED
#define TLS_DTLS_CID_STATUS_DOWNLINK      ZSOCK_TLS_DTLS_CID_STATUS_DOWNLINK
#define TLS_DTLS_CID_STATUS_UPLINK        ZSOCK_TLS_DTLS_CID_STATUS_UPLINK
#define TLS_DTLS_CID_STATUS_BIDIRECTIONAL ZSOCK_TLS_DTLS_CID_STATUS_BIDIRECTIONAL

#define tls_cert_verify_cb zsock_tls_cert_verify_cb

#define AI_PASSIVE      ZSOCK_AI_PASSIVE
#define AI_CANONNAME    ZSOCK_AI_CANONNAME
#define AI_NUMERICHOST  ZSOCK_AI_NUMERICHOST
#define AI_V4MAPPED     ZSOCK_AI_V4MAPPED
#define AI_ALL          ZSOCK_AI_ALL
#define AI_ADDRCONFIG   ZSOCK_AI_ADDRCONFIG
#define AI_NUMERICSERV  ZSOCK_AI_NUMERICSERV
#define AI_EXTFLAGS     ZSOCK_AI_EXTFLAGS

#define NI_NUMERICHOST  ZSOCK_NI_NUMERICHOST
#define NI_NUMERICSERV  ZSOCK_NI_NUMERICSERV
#define NI_NOFQDN       ZSOCK_NI_NOFQDN
#define NI_NAMEREQD     ZSOCK_NI_NAMEREQD
#define NI_DGRAM        ZSOCK_NI_DGRAM
#define NI_MAXHOST      ZSOCK_NI_MAXHOST

#define SOL_SOCKET                    ZSOCK_SOL_SOCKET
#define SO_DEBUG                      ZSOCK_SO_DEBUG
#define SO_REUSEADDR                  ZSOCK_SO_REUSEADDR
#define SO_TYPE                       ZSOCK_SO_TYPE
#define SO_ERROR                      ZSOCK_SO_ERROR
#define SO_DONTROUTE                  ZSOCK_SO_DONTROUTE
#define SO_BROADCAST                  ZSOCK_SO_BROADCAST
#define SO_SNDBUF                     ZSOCK_SO_SNDBUF
#define SO_RCVBUF                     ZSOCK_SO_RCVBUF
#define SO_KEEPALIVE                  ZSOCK_SO_KEEPALIVE
#define SO_OOBINLINE                  ZSOCK_SO_OOBINLINE
#define SO_PRIORITY                   ZSOCK_SO_PRIORITY
#define SO_LINGER                     ZSOCK_SO_LINGER
#define SO_REUSEPORT                  ZSOCK_SO_REUSEPORT
#define SO_RCVLOWAT                   ZSOCK_SO_RCVLOWAT
#define SO_SNDLOWAT                   ZSOCK_SO_SNDLOWAT
#define SO_RCVTIMEO                   ZSOCK_SO_RCVTIMEO
#define SO_SNDTIMEO                   ZSOCK_SO_SNDTIMEO
#define SO_BINDTODEVICE               ZSOCK_SO_BINDTODEVICE
#define SO_ACCEPTCONN                 ZSOCK_SO_ACCEPTCONN
#define SO_TIMESTAMPING               ZSOCK_SO_TIMESTAMPING
#define SO_PROTOCOL                   ZSOCK_SO_PROTOCOL
#define SO_DOMAIN                     ZSOCK_SO_DOMAIN
#define SO_SOCKS5                     ZSOCK_SO_SOCKS5
#define SO_TXTIME                     ZSOCK_SO_TXTIME
#define SCM_TXTIME                    ZSOCK_SCM_TXTIME
#define SOF_TIMESTAMPING_RX_HARDWARE  ZSOCK_SOF_TIMESTAMPING_RX_HARDWARE
#define SOF_TIMESTAMPING_TX_HARDWARE  ZSOCK_SOF_TIMESTAMPING_TX_HARDWARE

#define SHUT_RD   ZSOCK_SHUT_RD
#define SHUT_WR   ZSOCK_SHUT_WR
#define SHUT_RDWR ZSOCK_SHUT_RDWR

#define MSG_PEEK     ZSOCK_MSG_PEEK
#define MSG_TRUNC    ZSOCK_MSG_TRUNC
#define MSG_DONTWAIT ZSOCK_MSG_DONTWAIT
#define MSG_WAITALL  ZSOCK_MSG_WAITALL

#define TCP_NODELAY    ZSOCK_TCP_NODELAY
#define TCP_KEEPIDLE   ZSOCK_TCP_KEEPIDLE
#define TCP_KEEPINTVL  ZSOCK_TCP_KEEPINTVL
#define TCP_KEEPCNT    ZSOCK_TCP_KEEPCNT

#define IP_TOS               ZSOCK_IP_TOS
#define IP_TTL               ZSOCK_IP_TTL
#define IP_PKTINFO           ZSOCK_IP_PKTINFO
#define IP_RECVTTL           ZSOCK_IP_RECVTTL
#define IP_MTU               ZSOCK_IP_MTU
#define IP_MULTICAST_IF      ZSOCK_IP_MULTICAST_IF
#define IP_MULTICAST_TTL     ZSOCK_IP_MULTICAST_TTL
#define IP_MULTICAST_LOOP    ZSOCK_IP_MULTICAST_LOOP
#define IP_ADD_MEMBERSHIP    ZSOCK_IP_ADD_MEMBERSHIP
#define IP_DROP_MEMBERSHIP   ZSOCK_IP_DROP_MEMBERSHIP
#define IP_LOCAL_PORT_RANGE  ZSOCK_IP_LOCAL_PORT_RANGE

#define IPV6_UNICAST_HOPS               ZSOCK_IPV6_UNICAST_HOPS
#define IPV6_MULTICAST_IF               ZSOCK_IPV6_MULTICAST_IF
#define IPV6_MULTICAST_HOPS             ZSOCK_IPV6_MULTICAST_HOPS
#define IPV6_MULTICAST_LOOP             ZSOCK_IPV6_MULTICAST_LOOP
#define IPV6_ADD_MEMBERSHIP             ZSOCK_IPV6_ADD_MEMBERSHIP
#define IPV6_DROP_MEMBERSHIP            ZSOCK_IPV6_DROP_MEMBERSHIP
#define IPV6_JOIN_GROUP                 ZSOCK_IPV6_ADD_MEMBERSHIP
#define IPV6_LEAVE_GROUP                ZSOCK_IPV6_DROP_MEMBERSHIP
#define IPV6_MTU                        ZSOCK_IPV6_MTU
#define IPV6_V6ONLY                     ZSOCK_IPV6_V6ONLY
#define IPV6_RECVPKTINFO                ZSOCK_IPV6_RECVPKTINFO
#define IPV6_PKTINFO                    ZSOCK_IPV6_PKTINFO
#define IPV6_RECVHOPLIMIT               ZSOCK_IPV6_RECVHOPLIMIT
#define IPV6_HOPLIMIT                   ZSOCK_IPV6_HOPLIMIT
#define IPV6_ADDR_PREFERENCES           ZSOCK_IPV6_ADDR_PREFERENCES
#define IPV6_PREFER_SRC_TMP             ZSOCK_IPV6_PREFER_SRC_TMP
#define IPV6_PREFER_SRC_PUBLIC          ZSOCK_IPV6_PREFER_SRC_PUBLIC
#define IPV6_PREFER_SRC_PUBTMP_DEFAULT  ZSOCK_IPV6_PREFER_SRC_PUBTMP_DEFAULT
#define IPV6_PREFER_SRC_COA             ZSOCK_IPV6_PREFER_SRC_COA
#define IPV6_PREFER_SRC_HOME            ZSOCK_IPV6_PREFER_SRC_HOME
#define IPV6_PREFER_SRC_CGA             ZSOCK_IPV6_PREFER_SRC_CGA
#define IPV6_PREFER_SRC_NONCGA          ZSOCK_IPV6_PREFER_SRC_NONCGA
#define IPV6_TCLASS                     ZSOCK_IPV6_TCLASS

#define SOMAXCONN ZSOCK_SOMAXCONN

#define IN6_IS_ADDR_UNSPECIFIED(addr)   ZSOCK_IN6_IS_ADDR_UNSPECIFIED(addr)
#define IN6_IS_ADDR_LOOPBACK(addr)      ZSOCK_IN6_IS_ADDR_LOOPBACK(addr)
#define IN6_IS_ADDR_MULTICAST(addr)     ZSOCK_IN6_IS_ADDR_MULTICAST(addr)
#define IN6_IS_ADDR_LINKLOCAL(addr)     ZSOCK_IN6_IS_ADDR_LINKLOCAL(addr)
#define IN6_IS_ADDR_SITELOCAL(addr)     ZSOCK_IN6_IS_ADDR_SITELOCAL(addr)
#define IN6_IS_ADDR_V4MAPPED(addr)      ZSOCK_IN6_IS_ADDR_V4MAPPED(addr)
#define IN6_IS_ADDR_MC_GLOBAL(addr)     ZSOCK_IN6_IS_ADDR_MC_GLOBAL(addr)
#define IN6_IS_ADDR_MC_NODELOCAL(addr)  ZSOCK_IN6_IS_ADDR_MC_NODELOCAL(addr)
#define IN6_IS_ADDR_MC_LINKLOCAL(addr)  ZSOCK_IN6_IS_ADDR_MC_LINKLOCAL(addr)
#define IN6_IS_ADDR_MC_SITELOCAL(addr)  ZSOCK_IN6_IS_ADDR_MC_SITELOCAL(addr)
#define IN6_IS_ADDR_MC_ORGLOCAL(addr)   ZSOCK_IN6_IS_ADDR_MC_ORGLOCAL(addr)

#ifdef __cplusplus
}
#endif

/** @endcond */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_NET_COMPAT_H_ */
