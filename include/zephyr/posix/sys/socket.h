/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main sockets header.
 * @ingroup posix
 *
 * Provides the BSD socket interface: creating sockets, binding, connecting,
 * sending, receiving, and socket options.
 *
 * @posix_header{sys_socket.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_SOCKET_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_SOCKET_H_

#include <sys/types.h>

#undef ZEPHYR_INCLUDE_NET_COMPAT_MODE_SYMBOLS
/** @cond INTERNAL_HIDDEN */
#define ZEPHYR_INCLUDE_NET_COMPAT_MODE_SYMBOLS
/** @endcond */
#include <zephyr/net/socket.h>
#undef ZEPHYR_INCLUDE_NET_COMPAT_MODE_SYMBOLS

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Socket linger option structure.
 */
struct linger {
	/**
	 * @brief Indicates whether linger option is enabled.
	 */
	int  l_onoff;
	/**
	 * @brief Linger time, in seconds.
	 */
	int  l_linger;
};

#if !defined(CONFIG_NET_NAMESPACE_COMPAT_MODE)
/**
 * @brief Type for socket address length values.
 */
typedef uint32_t socklen_t;
struct msghdr;
struct sockaddr;

/**
 * @brief Peek at incoming data without removing it from the queue.
 */
#define MSG_PEEK     ZSOCK_MSG_PEEK

/**
 * @brief Return the real length of the datagram even if it was truncated.
 */
#define MSG_TRUNC    ZSOCK_MSG_TRUNC

/**
 * @brief Enable non-blocking operation for this call only.
 */
#define MSG_DONTWAIT ZSOCK_MSG_DONTWAIT

/**
 * @brief Block until all requested data has been received.
 */
#define MSG_WAITALL  ZSOCK_MSG_WAITALL

/**
 * @brief Disables further receive operations.
 */
#define SHUT_RD   ZSOCK_SHUT_RD

/**
 * @brief Disables further send operations.
 */
#define SHUT_WR   ZSOCK_SHUT_WR

/**
 * @brief Disables further send and receive operations.
 */
#define SHUT_RDWR ZSOCK_SHUT_RDWR
#endif

/**
 * @brief Accept a new connection on a socket.
 *
 * @param sock    Listening socket file descriptor.
 * @param addr    Output: address of the connecting peer, or NULL.
 * @param addrlen Input: size of @p addr; output: actual address size.
 *
 * @return New socket file descriptor on success, or -1 with errno set on failure.
 *
 * @posix_func{accept}
 */
int accept(int sock, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Bind a name to a socket.
 *
 * @param sock    Socket file descriptor.
 * @param addr    Local address to bind.
 * @param addrlen Size of @p addr in bytes.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{bind}
 */
int bind(int sock, const struct sockaddr *addr, socklen_t addrlen);

/**
 * @brief Connect a socket.
 *
 * @param sock    Socket file descriptor.
 * @param addr    Remote address to connect to.
 * @param addrlen Size of @p addr in bytes.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{connect}
 */
int connect(int sock, const struct sockaddr *addr, socklen_t addrlen);

/**
 * @brief Get the name of the peer socket.
 *
 * @param sock    Socket file descriptor.
 * @param addr    Output: peer address.
 * @param addrlen Input: size of @p addr; output: actual address size.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{getpeername}
 */
int getpeername(int sock, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Get the socket name.
 *
 * @param sock    Socket file descriptor.
 * @param addr    Output: local address bound to the socket.
 * @param addrlen Input: size of @p addr; output: actual address size.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{getsockname}
 */
int getsockname(int sock, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief Get the socket options.
 *
 * @param sock    Socket file descriptor.
 * @param level   Protocol level (@c SOL_SOCKET, @c IPPROTO_TCP, etc.).
 * @param optname Option name (@c SO_REUSEADDR, @c SO_KEEPALIVE, etc.).
 * @param optval  Output: option value.
 * @param optlen  Input: size of @p optval; output: actual size.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{getsockopt}
 */
int getsockopt(int sock, int level, int optname, void *optval, socklen_t *optlen);

/**
 * @brief Listen for socket connections and limit the queue of incoming connections.
 *
 * @param sock    Socket file descriptor.
 * @param backlog Maximum number of pending connections to queue.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{listen}
 */
int listen(int sock, int backlog);

/**
 * @brief Receive a message from a connected socket.
 *
 * @param sock    Socket file descriptor.
 * @param buf     Buffer to receive data into.
 * @param max_len Maximum number of bytes to receive.
 * @param flags   Flags (e.g. @c MSG_PEEK, @c MSG_DONTWAIT, @c MSG_WAITALL).
 *
 * @return Number of bytes received, 0 when the peer has closed the connection,
 *         or -1 with errno set on failure.
 *
 * @posix_func{recv}
 */
ssize_t recv(int sock, void *buf, size_t max_len, int flags);

/**
 * @brief Receive a message from a socket.
 *
 * @param sock     Socket file descriptor.
 * @param buf      Buffer to receive data into.
 * @param max_len  Maximum number of bytes to receive.
 * @param flags    Flags (e.g. @c MSG_PEEK, @c MSG_DONTWAIT).
 * @param src_addr Output: sender's address, or NULL.
 * @param addrlen  Input: size of @p src_addr; output: actual address size.
 *
 * @return Number of bytes received on success, or -1 with errno set on failure.
 *
 * @posix_func{recvfrom}
 */
ssize_t recvfrom(int sock, void *buf, size_t max_len, int flags, struct sockaddr *src_addr,
		 socklen_t *addrlen);

/**
 * @brief Receive a message from a socket.
 *
 * @param sock  Socket file descriptor.
 * @param msg   Message header specifying I/O vectors and address buffer.
 * @param flags Flags (e.g. @c MSG_PEEK, @c MSG_DONTWAIT).
 *
 * @return Number of bytes received on success, or -1 with errno set on failure.
 *
 * @posix_func{recvmsg}
 */
ssize_t recvmsg(int sock, struct msghdr *msg, int flags);

/**
 * @brief Send a message on a socket.
 *
 * @param sock  Socket file descriptor.
 * @param buf   Data to send.
 * @param len   Number of bytes to send.
 * @param flags Flags (e.g. @c MSG_DONTWAIT).
 *
 * @return Number of bytes sent on success, or -1 with errno set on failure.
 *
 * @posix_func{send}
 */
ssize_t send(int sock, const void *buf, size_t len, int flags);

/**
 * @brief Send a message on a socket using a message structure.
 *
 * @param sock    Socket file descriptor.
 * @param message Message header specifying I/O vectors and destination.
 * @param flags   Flags (e.g. @c MSG_DONTWAIT).
 *
 * @return Number of bytes sent on success, or -1 with errno set on failure.
 *
 * @posix_func{sendmsg}
 */
ssize_t sendmsg(int sock, const struct msghdr *message, int flags);

/**
 * @brief Send a message on a socket.
 *
 * @param sock      Socket file descriptor.
 * @param buf       Data to send.
 * @param len       Number of bytes to send.
 * @param flags     Flags (e.g. @c MSG_DONTWAIT).
 * @param dest_addr Destination address.
 * @param addrlen   Size of @p dest_addr in bytes.
 *
 * @return Number of bytes sent on success, or -1 with errno set on failure.
 *
 * @posix_func{sendto}
 */
ssize_t sendto(int sock, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr,
	       socklen_t addrlen);

/**
 * @brief Set the socket options.
 *
 * @param sock    Socket file descriptor.
 * @param level   Protocol level (@c SOL_SOCKET, @c IPPROTO_TCP, etc.).
 * @param optname Option name (@c SO_REUSEADDR, @c SO_KEEPALIVE, etc.).
 * @param optval  Pointer to the new option value.
 * @param optlen  Size of @p optval in bytes.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{setsockopt}
 */
int setsockopt(int sock, int level, int optname, const void *optval, socklen_t optlen);

/**
 * @brief Shut down socket send and receive operations.
 *
 * @param sock Socket file descriptor.
 * @param how  @c SHUT_RD, @c SHUT_WR, or @c SHUT_RDWR.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{shutdown}
 */
int shutdown(int sock, int how);

/**
 * @brief Determine whether a socket is at the out-of-band mark.
 *
 * @param s Socket file descriptor.
 *
 * @return 1 if at the mark, 0 if not, or -1 with errno set on failure.
 *
 * @posix_func{sockatmark}
 */
int sockatmark(int s);

/**
 * @brief Create an endpoint for communication.
 *
 * @param family Protocol family (@c AF_INET, @c AF_INET6, @c AF_UNIX, etc.).
 * @param type   Socket type (@c SOCK_STREAM, @c SOCK_DGRAM, @c SOCK_RAW, etc.).
 * @param proto  Protocol number (@c IPPROTO_TCP, @c IPPROTO_UDP, or 0 for the default).
 *
 * @return New socket file descriptor on success, or -1 with errno set on failure.
 *
 * @posix_func{socket}
 */
int socket(int family, int type, int proto);

/**
 * @brief Create a pair of connected sockets.
 *
 * @param family Protocol family (typically @c AF_UNIX).
 * @param type   Socket type.
 * @param proto  Protocol number.
 * @param sv     Output: two-element array receiving the socket descriptors.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{socketpair}
 */
int socketpair(int family, int type, int proto, int sv[2]);

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_SYS_SOCKET_H_ */
