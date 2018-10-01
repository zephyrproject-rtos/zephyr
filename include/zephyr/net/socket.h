/**
 * @file
 * @brief BSD Sockets compatible API definitions
 *
 * An API for applications to use BSD Sockets like API.
 */

/*
 * Copyright (c) 2017-2018 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor
 * Copyright (c) 2025 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKET_H_
#define ZEPHYR_INCLUDE_NET_SOCKET_H_

/**
 * @brief BSD Sockets compatible API
 * @defgroup bsd_sockets BSD Sockets compatible API
 * @since 1.9
 * @version 1.0.0
 * @ingroup networking
 * @{
 */

#include <zephyr/kernel.h>
#include <sys/types.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket_select.h>
#include <zephyr/net/socket_poll.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/socket_ncs.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Options for poll()
 * @{
 */
/* ZSOCK_POLL* values are compatible with Linux */
/** zsock_poll: Poll for readability */
#define ZSOCK_POLLIN 1
/** zsock_poll: Poll for exceptional condition */
#define ZSOCK_POLLPRI 2
/** zsock_poll: Poll for writability */
#define ZSOCK_POLLOUT 4
/** zsock_poll: Poll results in error condition (output value only) */
#define ZSOCK_POLLERR 8
/** zsock_poll: Poll detected closed connection (output value only) */
#define ZSOCK_POLLHUP 0x10
/** zsock_poll: Invalid socket (output value only) */
#define ZSOCK_POLLNVAL 0x20
/** @} */

/**
 * @name Options for sending and receiving data
 * @{
 */
/** zsock_recv: Read data without removing it from socket input queue */
#define ZSOCK_MSG_PEEK 0x02
/** zsock_recvmsg: Control data buffer too small.
 */
#define ZSOCK_MSG_CTRUNC 0x08
/** zsock_recv: return the real length of the datagram, even when it was longer
 *  than the passed buffer
 */
#define ZSOCK_MSG_TRUNC 0x20
/** zsock_recv/zsock_send: Override operation to non-blocking */
#define ZSOCK_MSG_DONTWAIT 0x40
/** zsock_recv: block until the full amount of data can be returned */
#define ZSOCK_MSG_WAITALL 0x100
/** @} */

/**
 * @name Options for shutdown() function
 * @{
 */
/* Well-known values, e.g. from Linux man 2 shutdown:
 * "The constants SHUT_RD, SHUT_WR, SHUT_RDWR have the value 0, 1, 2,
 * respectively". Some software uses numeric values.
 */
/** zsock_shutdown: Shut down for reading */
#define ZSOCK_SHUT_RD 0
/** zsock_shutdown: Shut down for writing */
#define ZSOCK_SHUT_WR 1
/** zsock_shutdown: Shut down for both reading and writing */
#define ZSOCK_SHUT_RDWR 2
/** @} */

/**
 * @defgroup secure_sockets_options Socket options for TLS
 * @since 1.13
 * @version 0.8.0
 * @{
 */
/**
 * @name Socket options for TLS
 * @{
 */

/** Protocol level for TLS.
 *  Here, the same socket protocol level for TLS as in Linux was used.
 */
#define SOL_TLS 282

/** Socket option to select TLS credentials to use. It accepts and returns an
 *  array of sec_tag_t that indicate which TLS credentials should be used with
 *  specific socket.
 */
#define TLS_SEC_TAG_LIST 1
/** Write-only socket option to set hostname. It accepts a string containing
 *  the hostname (may be NULL to disable hostname verification). By default,
 *  hostname check is enforced for TLS clients.
 */
#define TLS_HOSTNAME 2
/** Socket option to select ciphersuites to use. It accepts and returns an array
 *  of integers with IANA assigned ciphersuite identifiers.
 *  If not set, socket will allow all ciphersuites available in the system
 *  (mbedTLS default behavior).
 */
#define TLS_CIPHERSUITE_LIST 3
/** Read-only socket option to read a ciphersuite chosen during TLS handshake.
 *  It returns an integer containing an IANA assigned ciphersuite identifier
 *  of chosen ciphersuite.
 */
#define TLS_CIPHERSUITE_USED 4
/** Write-only socket option to set peer verification level for TLS connection.
 *  This option accepts an integer with a peer verification level, compatible
 *  with mbedTLS values:
 *    - 0 - none
 *    - 1 - optional
 *    - 2 - required
 *
 *  If not set, socket will use mbedTLS defaults (none for servers, required
 *  for clients).
 */
#define TLS_PEER_VERIFY 5
/** Write-only socket option to set role for DTLS connection. This option
 *  is irrelevant for TLS connections, as for them role is selected based on
 *  connect()/listen() usage. By default, DTLS will assume client role.
 *  This option accepts an integer with a TLS role, compatible with
 *  mbedTLS values:
 *    - 0 - client
 *    - 1 - server
 */
#define TLS_DTLS_ROLE 6
/** Socket option for setting the supported Application Layer Protocols.
 *  It accepts and returns a const char array of NULL terminated strings
 *  representing the supported application layer protocols listed during
 *  the TLS handshake.
 */
#define TLS_ALPN_LIST 7
/** Socket option to set DTLS min handshake timeout. The timeout starts at min,
 *  and upon retransmission the timeout is doubled util max is reached.
 *  Min and max arguments are separate options. The time unit is ms.
 */
#define TLS_DTLS_HANDSHAKE_TIMEOUT_MIN 8

/** Socket option to set DTLS max handshake timeout. The timeout starts at min,
 *  and upon retransmission the timeout is doubled util max is reached.
 *  Min and max arguments are separate options. The time unit is ms.
 */
#define TLS_DTLS_HANDSHAKE_TIMEOUT_MAX 9

/** Socket option for preventing certificates from being copied to the mbedTLS
 *  heap if possible. The option is only effective for DER certificates and is
 *  ignored for PEM certificates.
 */
#define TLS_CERT_NOCOPY	       10
/** TLS socket option to use with offloading. The option instructs the network
 *  stack only to offload underlying TCP/UDP communication. The TLS/DTLS
 *  operation is handled by a native TLS/DTLS socket implementation from Zephyr.
 *
 *  Note, that this option is only applicable if socket dispatcher is used
 *  (CONFIG_NET_SOCKETS_OFFLOAD_DISPATCHER is enabled).
 *  In such case, it should be the first socket option set on a newly created
 *  socket. After that, the application may use SO_BINDTODEVICE to choose the
 *  dedicated network interface for the underlying TCP/UDP socket.
 */
#define TLS_NATIVE 11
/** Socket option to control TLS session caching on a socket. Accepted values:
 *  - 0 - Disabled.
 *  - 1 - Enabled.
 */
#define TLS_SESSION_CACHE 12
/** Write-only socket option to purge session cache immediately.
 *  This option accepts any value.
 */
#define TLS_SESSION_CACHE_PURGE 13
/** Write-only socket option to control DTLS CID.
 *  The option accepts an integer, indicating the setting.
 *  Accepted values for the option are: 0, 1 and 2.
 *  Effective when set before connecting to the socket.
 *  - 0 - DTLS CID will be disabled.
 *  - 1 - DTLS CID will be enabled, and a 0 length CID value to be sent to the
 *        peer.
 *  - 2 - DTLS CID will be enabled, and the most recent value set with
 *        TLS_DTLS_CID_VALUE will be sent to the peer. Otherwise, a random value
 *        will be used.
 */
#define TLS_DTLS_CID 14
/** Read-only socket option to get DTLS CID status.
 *  The option accepts a pointer to an integer, indicating the setting upon
 *  return.
 *  Returned values for the option are:
 *  - 0 - DTLS CID is disabled.
 *  - 1 - DTLS CID is received on the downlink.
 *  - 2 - DTLS CID is sent to the uplink.
 *  - 3 - DTLS CID is used in both directions.
 */
#define TLS_DTLS_CID_STATUS 15
/** Socket option to set or get the value of the DTLS connection ID to be
 *  used for the DTLS session.
 *  The option accepts a byte array, holding the CID value.
 */
#define TLS_DTLS_CID_VALUE 16
/** Read-only socket option to get the value of the DTLS connection ID
 *  received from the peer.
 *  The option accepts a pointer to a byte array, holding the CID value upon
 *  return. The optlen returned will be 0 if the peer did not provide a
 *  connection ID, otherwise will contain the length of the CID value.
 */
#define TLS_DTLS_PEER_CID_VALUE 17
/** Socket option to configure DTLS socket behavior on connect().
 *  If set, DTLS connect() will execute the handshake with the configured peer.
 *  This is the default behavior.
 *  Otherwise, DTLS connect() will only configure peer address (as with regular
 *  UDP socket) and will not attempt to execute DTLS handshake. The handshake
 *  will take place in consecutive send()/recv() call.
 */
#define TLS_DTLS_HANDSHAKE_ON_CONNECT 18

/* Valid values for @ref TLS_PEER_VERIFY option */
#define TLS_PEER_VERIFY_NONE 0     /**< Peer verification disabled. */
#define TLS_PEER_VERIFY_OPTIONAL 1 /**< Peer verification optional. */
#define TLS_PEER_VERIFY_REQUIRED 2 /**< Peer verification required. */

/* Valid values for @ref TLS_DTLS_ROLE option */
#define TLS_DTLS_ROLE_CLIENT 0 /**< Client role in a DTLS session. */
#define TLS_DTLS_ROLE_SERVER 1 /**< Server role in a DTLS session. */

/* Valid values for @ref TLS_CERT_NOCOPY option */
#define TLS_CERT_NOCOPY_NONE 0     /**< Cert duplicated in heap */
#define TLS_CERT_NOCOPY_OPTIONAL 1 /**< Cert not copied in heap if DER */

/* Valid values for @ref TLS_SESSION_CACHE option */
#define TLS_SESSION_CACHE_DISABLED 0 /**< Disable TLS session caching. */
#define TLS_SESSION_CACHE_ENABLED 1 /**< Enable TLS session caching. */

/* Valid values for @ref TLS_DTLS_CID (Connection ID) option */
#define TLS_DTLS_CID_DISABLED		0 /**< CID is disabled  */
#define TLS_DTLS_CID_SUPPORTED		1 /**< CID is supported */
#define TLS_DTLS_CID_ENABLED		2 /**< CID is enabled   */

/* Valid values for @ref TLS_DTLS_CID_STATUS option */
#define TLS_DTLS_CID_STATUS_DISABLED		0 /**< CID is disabled */
#define TLS_DTLS_CID_STATUS_DOWNLINK		1 /**< CID is in use by us */
#define TLS_DTLS_CID_STATUS_UPLINK		2 /**< CID is in use by peer */
#define TLS_DTLS_CID_STATUS_BIDIRECTIONAL	3 /**< CID is in use by us and peer */
/** @} */ /* for @name */
/** @} */ /* for @defgroup */

/**
 * @brief Definition used when querying address information.
 *
 * A linked list of these descriptors is returned by getaddrinfo(). The struct
 * is also passed as hints when calling the getaddrinfo() function.
 */
struct zsock_addrinfo {
	struct zsock_addrinfo *ai_next; /**< Pointer to next address entry */
	int ai_flags;             /**< Additional options */
	int ai_family;            /**< Address family of the returned addresses */
	int ai_socktype;          /**< Socket type, for example SOCK_STREAM or SOCK_DGRAM */
	int ai_protocol;          /**< Protocol for addresses, 0 means any protocol */
	int ai_eflags;            /**< Extended flags for special usage */
	socklen_t ai_addrlen;     /**< Length of the socket address */
	struct sockaddr *ai_addr; /**< Pointer to the address */
	char *ai_canonname;       /**< Optional official name of the host */

/** @cond INTERNAL_HIDDEN */
	struct sockaddr _ai_addr;
	char _ai_canonname[DNS_MAX_NAME_SIZE + 1];
/** @endcond */
};

/**
 * @brief Obtain a file descriptor's associated net context
 *
 * With CONFIG_USERSPACE enabled, the kernel's object permission system
 * must apply to socket file descriptors. When a socket is opened, by default
 * only the caller has permission, access by other threads will fail unless
 * they have been specifically granted permission.
 *
 * This is achieved by tagging data structure definitions that implement the
 * underlying object associated with a network socket file descriptor with
 * '__net_socket`. All pointers to instances of these will be known to the
 * kernel as kernel objects with type K_OBJ_NET_SOCKET.
 *
 * This API is intended for threads that need to grant access to the object
 * associated with a particular file descriptor to another thread. The
 * returned pointer represents the underlying K_OBJ_NET_SOCKET  and
 * may be passed to APIs like k_object_access_grant().
 *
 * In a system like Linux which has the notion of threads running in processes
 * in a shared virtual address space, this sort of management is unnecessary as
 * the scope of file descriptors is implemented at the process level.
 *
 * However in Zephyr the file descriptor scope is global, and MPU-based systems
 * are not able to implement a process-like model due to the lack of memory
 * virtualization hardware. They use discrete object permissions and memory
 * domains instead to define thread access scope.
 *
 * User threads will have no direct access to the returned object
 * and will fault if they try to access its memory; the pointer can only be
 * used to make permission assignment calls, which follow exactly the rules
 * for other kernel objects like device drivers and IPC.
 *
 * @param sock file descriptor
 * @return pointer to associated network socket object, or NULL if the
 *         file descriptor wasn't valid or the caller had no access permission
 */
__syscall void *zsock_get_context_object(int sock);

/**
 * @brief Create a network socket
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/socket.html
 * for normative description.
 * This function is also exposed as `socket()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 *
 * If CONFIG_USERSPACE is enabled, the caller will be granted access to the
 * context object associated with the returned file descriptor.
 * @see zsock_get_context_object()
 *
 */
__syscall int zsock_socket(int family, int type, int proto);

/**
 * @brief Create an unnamed pair of connected sockets
 *
 * @details
 * See POSIX.1-2017 article
 * https://pubs.opengroup.org/onlinepubs/009695399/functions/socketpair.html
 * for normative description.
 * This function is also exposed as `socketpair()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
__syscall int zsock_socketpair(int family, int type, int proto, int *sv);

/**
 * @brief Close a network socket
 *
 * @details
 * Close a network socket.
 * This function is also exposed as `close()`
 * if @kconfig{CONFIG_POSIX_API} is defined (in which case it
 * may conflict with generic POSIX `close()` function).
 */
__syscall int zsock_close(int sock);

/**
 * @brief Shutdown socket send/receive operations
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/shutdown.html
 * for normative description, but currently this function has no effect in
 * Zephyr and provided solely for compatibility with existing code.
 * This function is also exposed as `shutdown()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
__syscall int zsock_shutdown(int sock, int how);

/**
 * @brief Bind a socket to a local network address
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/bind.html
 * for normative description.
 * This function is also exposed as `bind()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
__syscall int zsock_bind(int sock, const struct sockaddr *addr,
			 socklen_t addrlen);

/**
 * @brief Connect a socket to a peer network address
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/connect.html
 * for normative description.
 * This function is also exposed as `connect()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
__syscall int zsock_connect(int sock, const struct sockaddr *addr,
			    socklen_t addrlen);

/**
 * @brief Set up a STREAM socket to accept peer connections
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/listen.html
 * for normative description.
 * This function is also exposed as `listen()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
__syscall int zsock_listen(int sock, int backlog);

/**
 * @brief Accept a connection on listening socket
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/accept.html
 * for normative description.
 * This function is also exposed as `accept()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
__syscall int zsock_accept(int sock, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Send data to an arbitrary network address
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/sendto.html
 * for normative description.
 * This function is also exposed as `sendto()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
__syscall ssize_t zsock_sendto(int sock, const void *buf, size_t len,
			       int flags, const struct sockaddr *dest_addr,
			       socklen_t addrlen);

/**
 * @brief Send data to a connected peer
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/send.html
 * for normative description.
 * This function is also exposed as `send()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
static inline ssize_t zsock_send(int sock, const void *buf, size_t len,
				 int flags)
{
	return zsock_sendto(sock, buf, len, flags, NULL, 0);
}

/**
 * @brief Send data to an arbitrary network address
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/sendmsg.html
 * for normative description.
 * This function is also exposed as `sendmsg()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
__syscall ssize_t zsock_sendmsg(int sock, const struct msghdr *msg,
				int flags);

/**
 * @brief Receive data from an arbitrary network address
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/recvfrom.html
 * for normative description.
 * This function is also exposed as `recvfrom()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
__syscall ssize_t zsock_recvfrom(int sock, void *buf, size_t max_len,
				 int flags, struct sockaddr *src_addr,
				 socklen_t *addrlen);

/**
 * @brief Receive a message from an arbitrary network address
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/recvmsg.html
 * for normative description.
 * This function is also exposed as `recvmsg()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
__syscall ssize_t zsock_recvmsg(int sock, struct msghdr *msg, int flags);

/**
 * @brief Receive data from a connected peer
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/recv.html
 * for normative description.
 * This function is also exposed as `recv()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
static inline ssize_t zsock_recv(int sock, void *buf, size_t max_len,
				 int flags)
{
	return zsock_recvfrom(sock, buf, max_len, flags, NULL, NULL);
}

/**
 * @brief Control blocking/non-blocking mode of a socket
 *
 * @details
 * This functions allow to (only) configure a socket for blocking or
 * non-blocking operation (O_NONBLOCK).
 * This function is also exposed as `fcntl()`
 * if @kconfig{CONFIG_POSIX_API} is defined (in which case
 * it may conflict with generic POSIX `fcntl()` function).
 */
__syscall int zsock_fcntl_impl(int sock, int cmd, int flags);

/** @cond INTERNAL_HIDDEN */

/*
 * Need this wrapper because newer GCC versions got too smart and "typecheck"
 * even macros.
 */
static inline int zsock_fcntl_wrapper(int sock, int cmd, ...)
{
	va_list args;
	int flags;

	va_start(args, cmd);
	flags = va_arg(args, int);
	va_end(args);
	return zsock_fcntl_impl(sock, cmd, flags);
}

#define zsock_fcntl zsock_fcntl_wrapper

/** @endcond */

/**
 * @brief Control underlying socket parameters
 *
 * @details
 * See POSIX.1-2017 article
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/ioctl.html
 * for normative description.
 * This function enables querying or manipulating underlying socket parameters.
 * Currently supported @p request values include `ZFD_IOCTL_FIONBIO`, and
 * `ZFD_IOCTL_FIONREAD`, to set non-blocking mode, and query the number of
 * bytes available to read, respectively.
 * This function is also exposed as `ioctl()`
 * if @kconfig{CONFIG_POSIX_API} is defined (in which case
 * it may conflict with generic POSIX `ioctl()` function).
 */
__syscall int zsock_ioctl_impl(int sock, unsigned long request, va_list ap);

/** @cond INTERNAL_HIDDEN */

static inline int zsock_ioctl_wrapper(int sock, unsigned long request, ...)
{
	int ret;
	va_list args;

	va_start(args, request);
	ret = zsock_ioctl_impl(sock, request, args);
	va_end(args);

	return ret;
}

#define zsock_ioctl zsock_ioctl_wrapper

/** @endcond */

/**
 * @brief Efficiently poll multiple sockets for events
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/poll.html
 * for normative description.
 * This function is also exposed as `poll()`
 * if @kconfig{CONFIG_POSIX_API} is defined (in which case
 * it may conflict with generic POSIX `poll()` function).
 */
static inline int zsock_poll(struct zsock_pollfd *fds, int nfds, int timeout)
{
	return zvfs_poll(fds, nfds, timeout);
}

/**
 * @brief Get various socket options
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/getsockopt.html
 * for normative description. In Zephyr this function supports a subset of
 * socket options described by POSIX, but also some additional options
 * available in Linux (some options are dummy and provided to ease porting
 * of existing code).
 * This function is also exposed as `getsockopt()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
__syscall int zsock_getsockopt(int sock, int level, int optname,
			       void *optval, socklen_t *optlen);

/**
 * @brief Set various socket options
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/setsockopt.html
 * for normative description. In Zephyr this function supports a subset of
 * socket options described by POSIX, but also some additional options
 * available in Linux (some options are dummy and provided to ease porting
 * of existing code).
 * This function is also exposed as `setsockopt()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
__syscall int zsock_setsockopt(int sock, int level, int optname,
			       const void *optval, socklen_t optlen);

/**
 * @brief Get peer name
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/getpeername.html
 * for normative description.
 * This function is also exposed as `getpeername()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
__syscall int zsock_getpeername(int sock, struct sockaddr *addr,
				socklen_t *addrlen);

/**
 * @brief Get socket name
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/getsockname.html
 * for normative description.
 * This function is also exposed as `getsockname()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
__syscall int zsock_getsockname(int sock, struct sockaddr *addr,
				socklen_t *addrlen);

/**
 * @brief Get local host name
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/gethostname.html
 * for normative description.
 * This function is also exposed as `gethostname()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
__syscall int zsock_gethostname(char *buf, size_t len);

/**
 * @brief Convert network address from internal to numeric ASCII form
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/inet_ntop.html
 * for normative description.
 * This function is also exposed as `inet_ntop()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
static inline char *zsock_inet_ntop(sa_family_t family, const void *src,
				    char *dst, size_t size)
{
	return net_addr_ntop(family, src, dst, size);
}

/**
 * @brief Convert network address from numeric ASCII form to internal representation
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/inet_pton.html
 * for normative description.
 * This function is also exposed as `inet_pton()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
__syscall int zsock_inet_pton(sa_family_t family, const char *src, void *dst);

/** @cond INTERNAL_HIDDEN */
__syscall int z_zsock_getaddrinfo_internal(const char *host,
					   const char *service,
					   const struct zsock_addrinfo *hints,
					   struct zsock_addrinfo *res);
/** @endcond */

/* Flags for getaddrinfo() hints. */

/**
 * @name Flags for getaddrinfo() hints
 * @{
 */
/** Address for bind() (vs for connect()) */
#define AI_PASSIVE 0x1
/** Fill in ai_canonname */
#define AI_CANONNAME 0x2
/** Assume host address is in numeric notation, don't DNS lookup */
#define AI_NUMERICHOST 0x4
/** May return IPv4 mapped address for IPv6  */
#define AI_V4MAPPED 0x8
/** May return both native IPv6 and mapped IPv4 address for IPv6 */
#define AI_ALL 0x10
/** IPv4/IPv6 support depends on local system config */
#define AI_ADDRCONFIG 0x20
/** Assume service (port) is numeric */
#define AI_NUMERICSERV 0x400
/** Extra flags present (see RFC 5014) */
#define AI_EXTFLAGS 0x800
/** @} */

/**
 * @brief Resolve a domain name to one or more network addresses
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/getaddrinfo.html
 * for normative description.
 * This function is also exposed as `getaddrinfo()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
int zsock_getaddrinfo(const char *host, const char *service,
		      const struct zsock_addrinfo *hints,
		      struct zsock_addrinfo **res);

/**
 * @brief Free results returned by zsock_getaddrinfo()
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/freeaddrinfo.html
 * for normative description.
 * This function is also exposed as `freeaddrinfo()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
void zsock_freeaddrinfo(struct zsock_addrinfo *ai);

/**
 * @brief Convert zsock_getaddrinfo() error code to textual message
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/gai_strerror.html
 * for normative description.
 * This function is also exposed as `gai_strerror()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
const char *zsock_gai_strerror(int errcode);

/**
 * @name Flags for getnameinfo()
 * @{
 */
/** zsock_getnameinfo(): Resolve to numeric address. */
#define NI_NUMERICHOST 1
/** zsock_getnameinfo(): Resolve to numeric port number. */
#define NI_NUMERICSERV 2
/** zsock_getnameinfo(): Return only hostname instead of FQDN */
#define NI_NOFQDN 4
/** zsock_getnameinfo(): Dummy option for compatibility */
#define NI_NAMEREQD 8
/** zsock_getnameinfo(): Dummy option for compatibility */
#define NI_DGRAM 16

/* POSIX extensions */

/** zsock_getnameinfo(): Max supported hostname length */
#ifndef NI_MAXHOST
#define NI_MAXHOST 64
#endif
/** @} */

/**
 * @brief Resolve a network address to a domain name or ASCII address
 *
 * @details
 * See POSIX.1-2017 article
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/getnameinfo.html
 * for normative description.
 * This function is also exposed as `getnameinfo()`
 * if @kconfig{CONFIG_POSIX_API} is defined.
 */
int zsock_getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
		      char *host, socklen_t hostlen,
		      char *serv, socklen_t servlen, int flags);

/**
 * @name Network interface name description
 * @{
 */
/** Network interface name length */
#if defined(CONFIG_NET_INTERFACE_NAME)
#define IFNAMSIZ CONFIG_NET_INTERFACE_NAME_LEN
#else
#define IFNAMSIZ Z_DEVICE_MAX_NAME_LEN
#endif

/** Interface description structure */
struct ifreq {
	char ifr_name[IFNAMSIZ]; /**< Network interface name */
};
/** @} */

/**
 * @name Socket level options (SOL_SOCKET)
 * @{
 */
/** Socket-level option */
#define SOL_SOCKET 1

/* Socket options for SOL_SOCKET level */

/** Recording debugging information (ignored, for compatibility) */
#define SO_DEBUG 1
/** address reuse */
#define SO_REUSEADDR 2
/** Type of the socket */
#define SO_TYPE 3
/** Async error */
#define SO_ERROR 4
/** Bypass normal routing and send directly to host (ignored, for compatibility) */
#define SO_DONTROUTE 5
/** Transmission of broadcast messages is supported (ignored, for compatibility) */
#define SO_BROADCAST 6

/** Size of socket send buffer */
#define SO_SNDBUF 7
/** Size of socket recv buffer */
#define SO_RCVBUF 8

/** Enable sending keep-alive messages on connections */
#define SO_KEEPALIVE 9
/** Place out-of-band data into receive stream (ignored, for compatibility) */
#define SO_OOBINLINE 10
/** Socket priority */
#define SO_PRIORITY 12
/** Socket lingers on close (ignored, for compatibility) */
#define SO_LINGER 13
/** Allow multiple sockets to reuse a single port */
#define SO_REUSEPORT 15

/** Receive low watermark (ignored, for compatibility) */
#define SO_RCVLOWAT 18
/** Send low watermark (ignored, for compatibility) */
#define SO_SNDLOWAT 19

/**
 * Receive timeout
 * Applies to receive functions like recv(), but not to connect()
 */
#define SO_RCVTIMEO 20
/** Send timeout */
#define SO_SNDTIMEO 21

/** Bind a socket to an interface */
#define SO_BINDTODEVICE	25

/** Socket accepts incoming connections (ignored, for compatibility) */
#define SO_ACCEPTCONN 30

/** Timestamp TX RX or both packets. Supports multiple timestamp sources. */
#define SO_TIMESTAMPING 37

/** Protocol used with the socket */
#define SO_PROTOCOL 38

/** Domain used with SOCKET */
#define SO_DOMAIN 39

/** Enable SOCKS5 for Socket */
#define SO_SOCKS5 60

/** Socket TX time (when the data should be sent) */
#define SO_TXTIME 61
/** Socket TX time (same as SO_TXTIME) */
#define SCM_TXTIME SO_TXTIME

/** Timestamp generation flags */

/** Request RX timestamps generated by network adapter. */
#define SOF_TIMESTAMPING_RX_HARDWARE BIT(0)
/**
 * Request TX timestamps generated by network adapter.
 * This can be enabled via socket option or control messages.
 */
#define SOF_TIMESTAMPING_TX_HARDWARE BIT(1)

/** */

/** @} */

/**
 * @name TCP level options (IPPROTO_TCP)
 * @{
 */
/* Socket options for IPPROTO_TCP level */
/** Disable TCP buffering (ignored, for compatibility) */
#define TCP_NODELAY 1
/** Start keepalives after this period (seconds) */
#define TCP_KEEPIDLE 2
/** Interval between keepalives (seconds) */
#define TCP_KEEPINTVL 3
/** Number of keepalives before dropping connection */
#define TCP_KEEPCNT 4

/** @} */

/**
 * @name IPv4 level options (IPPROTO_IP)
 * @{
 */
/* Socket options for IPPROTO_IP level */
/** Set or receive the Type-Of-Service value for an outgoing packet. */
#define IP_TOS 1

/** Set or receive the Time-To-Live value for an outgoing packet. */
#define IP_TTL 2

/** Pass an IP_PKTINFO ancillary message that contains a
 *  pktinfo structure that supplies some information about the
 *  incoming packet.
 */
#define IP_PKTINFO 8

/**
 * @brief Incoming IPv4 packet information.
 *
 * Used as ancillary data when calling recvmsg() and IP_PKTINFO socket
 * option is set.
 */
struct in_pktinfo {
	unsigned int   ipi_ifindex;  /**< Network interface index */
	struct in_addr ipi_spec_dst; /**< Local address */
	struct in_addr ipi_addr;     /**< Header Destination address */
};

/** Retrieve the current known path MTU of the current socket. Returns an
 *  integer. IP_MTU is valid only for getsockopt and can be employed only when
 *  the socket has been connected.
 */
#define IP_MTU 14

/** Set IPv4 multicast datagram network interface. */
#define IP_MULTICAST_IF 32
/** Set IPv4 multicast TTL value. */
#define IP_MULTICAST_TTL 33
/** Set IPv4 multicast loop value. */
#define IP_MULTICAST_LOOP 34
/** Join IPv4 multicast group. */
#define IP_ADD_MEMBERSHIP 35
/** Leave IPv4 multicast group. */
#define IP_DROP_MEMBERSHIP 36

/**
 * @brief Struct used when joining or leaving a IPv4 multicast group.
 */
struct ip_mreqn {
	struct in_addr imr_multiaddr; /**< IP multicast group address */
	struct in_addr imr_address;   /**< IP address of local interface */
	int            imr_ifindex;   /**< Network interface index */
};

/**
 * @brief Struct used when setting a IPv4 multicast network interface.
 */
struct ip_mreq  {
	struct in_addr imr_multiaddr;   /**< IP multicast group address */
	struct in_addr imr_interface;   /**< IP address of local interface */
};

/** Clamp down the global port range for a given socket */
#define IP_LOCAL_PORT_RANGE 51

/** @} */

/**
 * @name IPv6 level options (IPPROTO_IPV6)
 * @{
 */
/* Socket options for IPPROTO_IPV6 level */
/** Set the unicast hop limit for the socket. */
#define IPV6_UNICAST_HOPS	16

/** Set multicast output network interface index for the socket. */
#define IPV6_MULTICAST_IF       17

/** Set the multicast hop limit for the socket. */
#define IPV6_MULTICAST_HOPS 18

/** Set the multicast loop bit for the socket. */
#define IPV6_MULTICAST_LOOP 19

/** Join IPv6 multicast group. */
#define IPV6_ADD_MEMBERSHIP 20

/** Leave IPv6 multicast group. */
#define IPV6_DROP_MEMBERSHIP 21

/** Join IPv6 multicast group. */
#define IPV6_JOIN_GROUP IPV6_ADD_MEMBERSHIP

/** Leave IPv6 multicast group. */
#define IPV6_LEAVE_GROUP IPV6_DROP_MEMBERSHIP

/**
 * @brief Struct used when joining or leaving a IPv6 multicast group.
 */
struct ipv6_mreq {
	/** IPv6 multicast address of group */
	struct in6_addr ipv6mr_multiaddr;

	/** Network interface index of the local IPv6 address */
	int ipv6mr_ifindex;
};

/** For getsockopt(), retrieve the current known IPv6 path MTU of the given socket.
 * Valid only when the socket has been connected.
 * For setsockopt(), set the MTU to be used for the socket. The MTU is limited by
 * the device MTU or the path MTU when path MTU discovery is enabled.
 */
#define IPV6_MTU 24

/** Don't support IPv4 access */
#define IPV6_V6ONLY 26

/** Pass an IPV6_RECVPKTINFO ancillary message that contains a
 *  in6_pktinfo structure that supplies some information about the
 *  incoming packet. See RFC 3542.
 */
#define IPV6_RECVPKTINFO 49

/** RFC5014: Source address selection. */
#define IPV6_ADDR_PREFERENCES   72

/** Prefer temporary address as source. */
#define IPV6_PREFER_SRC_TMP             0x0001
/** Prefer public address as source. */
#define IPV6_PREFER_SRC_PUBLIC          0x0002
/** Either public or temporary address is selected as a default source
 *  depending on the output interface configuration (this is the default value).
 *  This is Linux specific option not found in the RFC.
 */
#define IPV6_PREFER_SRC_PUBTMP_DEFAULT  0x0100
/** Prefer Care-of address as source. Ignored in Zephyr. */
#define IPV6_PREFER_SRC_COA             0x0004
/** Prefer Home address as source. Ignored in Zephyr. */
#define IPV6_PREFER_SRC_HOME            0x0400
/** Prefer CGA (Cryptographically Generated Address) address as source. Ignored in Zephyr. */
#define IPV6_PREFER_SRC_CGA             0x0008
/** Prefer non-CGA address as source. Ignored in Zephyr. */
#define IPV6_PREFER_SRC_NONCGA          0x0800

/**
 * @brief Incoming IPv6 packet information.
 *
 * Used as ancillary data when calling recvmsg() and IPV6_RECVPKTINFO socket
 * option is set.
 */
struct in6_pktinfo {
	struct in6_addr ipi6_addr;    /**< Destination IPv6 address */
	unsigned int    ipi6_ifindex; /**< Receive interface index */
};

/** Set or receive the traffic class value for an outgoing packet. */
#define IPV6_TCLASS 67
/** @} */

/**
 * @name Backlog size for listen()
 * @{
 */
/** listen: The maximum backlog queue length */
#define SOMAXCONN 128
/** @} */

/**
 * @name Macros for checking special IPv6 addresses.
 * @{
 */
/** Check unspecified IPv6 address. */
#define IN6_IS_ADDR_UNSPECIFIED(addr) \
	net_ipv6_addr_cmp(net_ipv6_unspecified_address(), addr)

/** Check loopback IPv6 address. */
#define IN6_IS_ADDR_LOOPBACK(addr) net_ipv6_is_addr_loopback(addr)

/** Check IPv6 multicast address */
#define IN6_IS_ADDR_MULTICAST(addr) net_ipv6_is_addr_mcast(addr)

/** Check IPv6 link local address */
#define IN6_IS_ADDR_LINKLOCAL(addr) net_ipv6_is_ll_addr(addr)

/** Check IPv6 site local address */
#define IN6_IS_ADDR_SITELOCAL(addr) net_ipv6_is_sl_addr(addr)

/** Check IPv6 v4 mapped address */
#define IN6_IS_ADDR_V4MAPPED(addr) net_ipv6_addr_is_v4_mapped(addr)

/** Check IPv6 multicast global address */
#define IN6_IS_ADDR_MC_GLOBAL(addr) net_ipv6_is_addr_mcast_global(addr)

/** Check IPv6 multicast node local address */
#define IN6_IS_ADDR_MC_NODELOCAL(addr) net_ipv6_is_addr_mcast_iface(addr)

/** Check IPv6 multicast link local address */
#define IN6_IS_ADDR_MC_LINKLOCAL(addr) net_ipv6_is_addr_mcast_link(addr)

/** Check IPv6 multicast site local address */
#define IN6_IS_ADDR_MC_SITELOCAL(addr) net_ipv6_is_addr_mcast_site(addr)

/** Check IPv6 multicast organization local address */
#define IN6_IS_ADDR_MC_ORGLOCAL(addr) net_ipv6_is_addr_mcast_org(addr)

/** @} */

/** @cond INTERNAL_HIDDEN */
/**
 * @brief Registration information for a given BSD socket family.
 */
struct net_socket_register {
	int family;
	bool is_offloaded;
	bool (*is_supported)(int family, int type, int proto);
	int (*handler)(int family, int type, int proto);
#if defined(CONFIG_NET_SOCKETS_OBJ_CORE)
	/* Store also the name of the socket type in order to be able to
	 * print it later.
	 */
	const char * const name;
#endif
};

#define NET_SOCKET_DEFAULT_PRIO CONFIG_NET_SOCKETS_PRIORITY_DEFAULT

#define NET_SOCKET_GET_NAME(socket_name, prio)	\
	__net_socket_register_##prio##_##socket_name

#if defined(CONFIG_NET_SOCKETS_OBJ_CORE)
#define K_OBJ_TYPE_SOCK  K_OBJ_TYPE_ID_GEN("SOCK")

#define NET_SOCKET_REGISTER_NAME(_name)		\
	.name = STRINGIFY(_name),
#else
#define NET_SOCKET_REGISTER_NAME(_name)
#endif

#define _NET_SOCKET_REGISTER(socket_name, prio, _family, _is_supported, _handler, _is_offloaded) \
	static const STRUCT_SECTION_ITERABLE(net_socket_register,	\
			NET_SOCKET_GET_NAME(socket_name, prio)) = {	\
		.family = _family,					\
		.is_offloaded = _is_offloaded,				\
		.is_supported = _is_supported,				\
		.handler = _handler,					\
		NET_SOCKET_REGISTER_NAME(socket_name)			\
	}

#define NET_SOCKET_REGISTER(socket_name, prio, _family, _is_supported, _handler) \
	_NET_SOCKET_REGISTER(socket_name, prio, _family, _is_supported, _handler, false)

#define NET_SOCKET_OFFLOAD_REGISTER(socket_name, prio, _family, _is_supported, _handler) \
	_NET_SOCKET_REGISTER(socket_name, prio, _family, _is_supported, _handler, true)

/** @endcond */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/socket.h>

/**
 * @}
 */

/* Avoid circular loops with POSIX socket headers.
 * We have these includes here so that we do not need
 * to change the applications that were only including
 * zephyr/net/socket.h header file.
 *
 * Additionally, if non-zephyr-prefixed headers are used here,
 * native_sim pulls in those from the host rather than Zephyr's.
 */
#if defined(CONFIG_POSIX_API)
#if !defined(ZEPHYR_INCLUDE_POSIX_ARPA_INET_H_)
#include <zephyr/posix/arpa/inet.h>
#endif
#if !defined(ZEPHYR_INCLUDE_POSIX_NETDB_H_)
#include <zephyr/posix/netdb.h>
#endif
#if !defined(ZEPHYR_INCLUDE_POSIX_UNISTD_H_)
#include <zephyr/posix/unistd.h>
#endif
#if !defined(ZEPHYR_INCLUDE_POSIX_POLL_H_)
#include <zephyr/posix/poll.h>
#endif
#if !defined(ZEPHYR_INCLUDE_POSIX_SYS_SOCKET_H_)
#include <zephyr/posix/sys/socket.h>
#endif
#endif /* CONFIG_POSIX_API */

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_H_ */
