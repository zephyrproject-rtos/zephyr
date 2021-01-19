/**
 * @file
 * @brief BSD Sockets compatible API definitions
 *
 * An API for applications to use BSD Sockets like API.
 */

/*
 * Copyright (c) 2017-2018 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKET_H_
#define ZEPHYR_INCLUDE_NET_SOCKET_H_

/**
 * @brief BSD Sockets compatible API
 * @defgroup bsd_sockets BSD Sockets compatible API
 * @ingroup networking
 * @{
 */

#include <sys/types.h>
#include <zephyr/types.h>
#include <net/net_ip.h>
#include <net/dns_resolve.h>
#include <net/socket_select.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct zsock_pollfd {
	int fd;
	short events;
	short revents;
};

/* ZSOCK_POLL* values are compatible with Linux */
/** zsock_poll: Poll for readability */
#define ZSOCK_POLLIN 1
/** zsock_poll: Compatibility value, ignored */
#define ZSOCK_POLLPRI 2
/** zsock_poll: Poll for writability */
#define ZSOCK_POLLOUT 4
/** zsock_poll: Poll results in error condition (output value only) */
#define ZSOCK_POLLERR 8
/** zsock_poll: Poll detected closed connection (output value only) */
#define ZSOCK_POLLHUP 0x10
/** zsock_poll: Invalid socket (output value only) */
#define ZSOCK_POLLNVAL 0x20

/** zsock_recv: Read data without removing it from socket input queue */
#define ZSOCK_MSG_PEEK 0x02
/** zsock_recv/zsock_send: Override operation to non-blocking */
#define ZSOCK_MSG_DONTWAIT 0x40

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

/** Protocol level for TLS.
 *  Here, the same socket protocol level for TLS as in Linux was used.
 */
#define SOL_TLS 282

/**
 *  @defgroup secure_sockets_options Socket options for TLS
 *  @{
 */

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
 *  (mebdTLS default behavior).
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

/** @} */

/* Valid values for TLS_PEER_VERIFY option */
#define TLS_PEER_VERIFY_NONE 0     /**< Peer verification disabled. */
#define TLS_PEER_VERIFY_OPTIONAL 1 /**< Peer verification optional. */
#define TLS_PEER_VERIFY_REQUIRED 2 /**< Peer verification required. */

/* Valid values for TLS_DTLS_ROLE option */
#define TLS_DTLS_ROLE_CLIENT 0 /**< Client role in a DTLS session. */
#define TLS_DTLS_ROLE_SERVER 1 /**< Server role in a DTLS session. */

struct zsock_addrinfo {
	struct zsock_addrinfo *ai_next;
	int ai_flags;
	int ai_family;
	int ai_socktype;
	int ai_protocol;
	socklen_t ai_addrlen;
	struct sockaddr *ai_addr;
	char *ai_canonname;

	struct sockaddr _ai_addr;
	char _ai_canonname[DNS_MAX_NAME_SIZE + 1];
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
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/socket.html>`__
 * for normative description.
 * This function is also exposed as ``socket()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
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
 * @rst
 * See `POSIX.1-2017 article
 * <https://pubs.opengroup.org/onlinepubs/009695399/functions/socketpair.html>`__
 * for normative description.
 * This function is also exposed as ``socketpair()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
__syscall int zsock_socketpair(int family, int type, int proto, int *sv);

/**
 * @brief Close a network socket
 *
 * @details
 * @rst
 * Close a network socket.
 * This function is also exposed as ``close()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined (in which case it
 * may conflict with generic POSIX ``close()`` function).
 * @endrst
 */
__syscall int zsock_close(int sock);

/**
 * @brief Shutdown socket send/receive operations
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/shutdown.html>`__
 * for normative description, but currently this function has no effect in
 * Zephyr and provided solely for compatibility with existing code.
 * This function is also exposed as ``shutdown()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
__syscall int zsock_shutdown(int sock, int how);

/**
 * @brief Bind a socket to a local network address
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/bind.html>`__
 * for normative description.
 * This function is also exposed as ``bind()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
__syscall int zsock_bind(int sock, const struct sockaddr *addr,
			 socklen_t addrlen);

/**
 * @brief Connect a socket to a peer network address
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/connect.html>`__
 * for normative description.
 * This function is also exposed as ``connect()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
__syscall int zsock_connect(int sock, const struct sockaddr *addr,
			    socklen_t addrlen);

/**
 * @brief Set up a STREAM socket to accept peer connections
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/listen.html>`__
 * for normative description.
 * This function is also exposed as ``listen()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
__syscall int zsock_listen(int sock, int backlog);

/**
 * @brief Accept a connection on listening socket
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/accept.html>`__
 * for normative description.
 * This function is also exposed as ``accept()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
__syscall int zsock_accept(int sock, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Send data to an arbitrary network address
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/sendto.html>`__
 * for normative description.
 * This function is also exposed as ``sendto()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
__syscall ssize_t zsock_sendto(int sock, const void *buf, size_t len,
			       int flags, const struct sockaddr *dest_addr,
			       socklen_t addrlen);

/**
 * @brief Send data to a connected peer
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/send.html>`__
 * for normative description.
 * This function is also exposed as ``send()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
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
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/sendmsg.html>`__
 * for normative description.
 * This function is also exposed as ``sendmsg()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
__syscall ssize_t zsock_sendmsg(int sock, const struct msghdr *msg,
				int flags);

/**
 * @brief Receive data from an arbitrary network address
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/recvfrom.html>`__
 * for normative description.
 * This function is also exposed as ``recvfrom()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
__syscall ssize_t zsock_recvfrom(int sock, void *buf, size_t max_len,
				 int flags, struct sockaddr *src_addr,
				 socklen_t *addrlen);

/**
 * @brief Receive data from a connected peer
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/recv.html>`__
 * for normative description.
 * This function is also exposed as ``recv()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
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
 * @rst
 * This functions allow to (only) configure a socket for blocking or
 * non-blocking operation (O_NONBLOCK).
 * This function is also exposed as ``fcntl()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined (in which case
 * it may conflict with generic POSIX ``fcntl()`` function).
 * @endrst
 */
__syscall int zsock_fcntl(int sock, int cmd, int flags);

/**
 * @brief Efficiently poll multiple sockets for events
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/poll.html>`__
 * for normative description. (In Zephyr this function works only with
 * sockets, not arbitrary file descriptors.)
 * This function is also exposed as ``poll()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined (in which case
 * it may conflict with generic POSIX ``poll()`` function).
 * @endrst
 */
__syscall int zsock_poll(struct zsock_pollfd *fds, int nfds, int timeout);

/**
 * @brief Get various socket options
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/getsockopt.html>`__
 * for normative description. In Zephyr this function supports a subset of
 * socket options described by POSIX, but also some additional options
 * available in Linux (some options are dummy and provided to ease porting
 * of existing code).
 * This function is also exposed as ``getsockopt()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
__syscall int zsock_getsockopt(int sock, int level, int optname,
			       void *optval, socklen_t *optlen);

/**
 * @brief Set various socket options
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/setsockopt.html>`__
 * for normative description. In Zephyr this function supports a subset of
 * socket options described by POSIX, but also some additional options
 * available in Linux (some options are dummy and provided to ease porting
 * of existing code).
 * This function is also exposed as ``setsockopt()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
__syscall int zsock_setsockopt(int sock, int level, int optname,
			       const void *optval, socklen_t optlen);

/**
 * @brief Get socket name
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/getsockname.html>`__
 * for normative description.
 * This function is also exposed as ``getsockname()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
__syscall int zsock_getsockname(int sock, struct sockaddr *addr,
				socklen_t *addrlen);

/**
 * @brief Get local host name
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/gethostname.html>`__
 * for normative description.
 * This function is also exposed as ``gethostname()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
__syscall int zsock_gethostname(char *buf, size_t len);

/**
 * @brief Convert network address from internal to numeric ASCII form
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/inet_ntop.html>`__
 * for normative description.
 * This function is also exposed as ``inet_ntop()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
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
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/inet_pton.html>`__
 * for normative description.
 * This function is also exposed as ``inet_pton()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
__syscall int zsock_inet_pton(sa_family_t family, const char *src, void *dst);

/** @cond INTERNAL_HIDDEN */
__syscall int z_zsock_getaddrinfo_internal(const char *host,
					   const char *service,
					   const struct zsock_addrinfo *hints,
					   struct zsock_addrinfo *res);
/** @endcond */

/* Flags for getaddrinfo() hints. */

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

/**
 * @brief Resolve a domain name to one or more network addresses
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/getaddrinfo.html>`__
 * for normative description.
 * This function is also exposed as ``getaddrinfo()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
int zsock_getaddrinfo(const char *host, const char *service,
		      const struct zsock_addrinfo *hints,
		      struct zsock_addrinfo **res);

/**
 * @brief Free results returned by zsock_getaddrinfo()
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/freeaddrinfo.html>`__
 * for normative description.
 * This function is also exposed as ``freeaddrinfo()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
void zsock_freeaddrinfo(struct zsock_addrinfo *ai);

/**
 * @brief Convert zsock_getaddrinfo() error code to textual message
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/gai_strerror.html>`__
 * for normative description.
 * This function is also exposed as ``gai_strerror()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
const char *zsock_gai_strerror(int errcode);

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

/**
 * @brief Resolve a network address to a domain name or ASCII address
 *
 * @details
 * @rst
 * See `POSIX.1-2017 article
 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/getnameinfo.html>`__
 * for normative description.
 * This function is also exposed as ``getnameinfo()``
 * if :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` is defined.
 * @endrst
 */
int zsock_getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
		      char *host, socklen_t hostlen,
		      char *serv, socklen_t servlen, int flags);

#if defined(CONFIG_NET_SOCKETS_POSIX_NAMES)

#define pollfd zsock_pollfd

static inline int socket(int family, int type, int proto)
{
	return zsock_socket(family, type, proto);
}

static inline int socketpair(int family, int type, int proto, int sv[2])
{
	return zsock_socketpair(family, type, proto, sv);
}

static inline int close(int sock)
{
	return zsock_close(sock);
}

static inline int shutdown(int sock, int how)
{
	return zsock_shutdown(sock, how);
}

static inline int bind(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	return zsock_bind(sock, addr, addrlen);
}

static inline int connect(int sock, const struct sockaddr *addr,
			  socklen_t addrlen)
{
	return zsock_connect(sock, addr, addrlen);
}

static inline int listen(int sock, int backlog)
{
	return zsock_listen(sock, backlog);
}

static inline int accept(int sock, struct sockaddr *addr, socklen_t *addrlen)
{
	return zsock_accept(sock, addr, addrlen);
}

static inline ssize_t send(int sock, const void *buf, size_t len, int flags)
{
	return zsock_send(sock, buf, len, flags);
}

static inline ssize_t recv(int sock, void *buf, size_t max_len, int flags)
{
	return zsock_recv(sock, buf, max_len, flags);
}

/* This conflicts with fcntl.h, so code must include fcntl.h before socket.h: */
#define fcntl zsock_fcntl

static inline ssize_t sendto(int sock, const void *buf, size_t len, int flags,
			     const struct sockaddr *dest_addr,
			     socklen_t addrlen)
{
	return zsock_sendto(sock, buf, len, flags, dest_addr, addrlen);
}

static inline ssize_t sendmsg(int sock, const struct msghdr *message,
			      int flags)
{
	return zsock_sendmsg(sock, message, flags);
}

static inline ssize_t recvfrom(int sock, void *buf, size_t max_len, int flags,
			       struct sockaddr *src_addr, socklen_t *addrlen)
{
	return zsock_recvfrom(sock, buf, max_len, flags, src_addr, addrlen);
}

static inline int poll(struct zsock_pollfd *fds, int nfds, int timeout)
{
	return zsock_poll(fds, nfds, timeout);
}

static inline int getsockopt(int sock, int level, int optname,
			     void *optval, socklen_t *optlen)
{
	return zsock_getsockopt(sock, level, optname, optval, optlen);
}

static inline int setsockopt(int sock, int level, int optname,
			     const void *optval, socklen_t optlen)
{
	return zsock_setsockopt(sock, level, optname, optval, optlen);
}

static inline int getsockname(int sock, struct sockaddr *addr,
			      socklen_t *addrlen)
{
	return zsock_getsockname(sock, addr, addrlen);
}

static inline int getaddrinfo(const char *host, const char *service,
			      const struct zsock_addrinfo *hints,
			      struct zsock_addrinfo **res)
{
	return zsock_getaddrinfo(host, service, hints, res);
}

static inline void freeaddrinfo(struct zsock_addrinfo *ai)
{
	zsock_freeaddrinfo(ai);
}

static inline const char *gai_strerror(int errcode)
{
	return zsock_gai_strerror(errcode);
}

static inline int getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
			      char *host, socklen_t hostlen,
			      char *serv, socklen_t servlen, int flags)
{
	return zsock_getnameinfo(addr, addrlen, host, hostlen,
				 serv, servlen, flags);
}

#define addrinfo zsock_addrinfo

static inline int gethostname(char *buf, size_t len)
{
	return zsock_gethostname(buf, len);
}

static inline int inet_pton(sa_family_t family, const char *src, void *dst)
{
	return zsock_inet_pton(family, src, dst);
}

static inline char *inet_ntop(sa_family_t family, const void *src, char *dst,
			      size_t size)
{
	return zsock_inet_ntop(family, src, dst, size);
}

#define POLLIN ZSOCK_POLLIN
#define POLLOUT ZSOCK_POLLOUT
#define POLLERR ZSOCK_POLLERR
#define POLLHUP ZSOCK_POLLHUP
#define POLLNVAL ZSOCK_POLLNVAL

#define MSG_PEEK ZSOCK_MSG_PEEK
#define MSG_DONTWAIT ZSOCK_MSG_DONTWAIT

#define SHUT_RD ZSOCK_SHUT_RD
#define SHUT_WR ZSOCK_SHUT_WR
#define SHUT_RDWR ZSOCK_SHUT_RDWR

#define EAI_BADFLAGS DNS_EAI_BADFLAGS
#define EAI_NONAME DNS_EAI_NONAME
#define EAI_AGAIN DNS_EAI_AGAIN
#define EAI_FAIL DNS_EAI_FAIL
#define EAI_NODATA DNS_EAI_NODATA
#define EAI_MEMORY DNS_EAI_MEMORY
#define EAI_SYSTEM DNS_EAI_SYSTEM
#define EAI_SERVICE DNS_EAI_SERVICE
#define EAI_SOCKTYPE DNS_EAI_SOCKTYPE
#define EAI_FAMILY DNS_EAI_FAMILY
#endif /* defined(CONFIG_NET_SOCKETS_POSIX_NAMES) */

/** sockopt: Socket-level option */
#define SOL_SOCKET 1

/* Socket options for SOL_SOCKET level */
/** sockopt: Enable server address reuse (ignored, for compatibility) */
#define SO_REUSEADDR 2
/** sockopt: Type of the socket */
#define SO_TYPE 3
/** sockopt: Async error (ignored, for compatibility) */
#define SO_ERROR 4

/**
 * sockopt: Receive timeout
 * Applies to receive functions like recv(), but not to connect()
 */
#define SO_RCVTIMEO 20
/** sockopt: Send timeout */
#define SO_SNDTIMEO 21

/** sockopt: Timestamp TX packets */
#define SO_TIMESTAMPING 37
/** sockopt: Protocol used with the socket */
#define SO_PROTOCOL 38

/* Socket options for IPPROTO_TCP level */
/** sockopt: Disable TCP buffering (ignored, for compatibility) */
#define TCP_NODELAY 1

/* Socket options for IPPROTO_IPV6 level */
/** sockopt: Don't support IPv4 access (ignored, for compatibility) */
#define IPV6_V6ONLY 26

/** sockopt: Socket priority */
#define SO_PRIORITY 12

/** sockopt: Socket TX time (when the data should be sent) */
#define SO_TXTIME 61
#define SCM_TXTIME SO_TXTIME

/* Socket options for SOCKS5 proxy */
/** sockopt: Enable SOCKS5 for Socket */
#define SO_SOCKS5 60

/** @cond INTERNAL_HIDDEN */
/**
 * @brief Registration information for a given BSD socket family.
 */
struct net_socket_register {
	int family;
	bool (*is_supported)(int family, int type, int proto);
	int (*handler)(int family, int type, int proto);
};

#define NET_SOCKET_GET_NAME(socket_name)	\
	(__net_socket_register_##socket_name)

#define NET_SOCKET_REGISTER(socket_name, _family, _is_supported, _handler) \
	static const Z_STRUCT_SECTION_ITERABLE(net_socket_register,	\
			NET_SOCKET_GET_NAME(socket_name)) = {		\
		.family = _family,					\
		.is_supported = _is_supported,				\
		.handler = _handler,					\
	}

/** @endcond */

#ifdef __cplusplus
}
#endif

#include <syscalls/socket.h>

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_H_ */
