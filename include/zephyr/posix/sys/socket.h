/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_SYS_SOCKET_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_SOCKET_H_

#include <sys/types.h>
#include <zephyr/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

struct linger {
	int  l_onoff;
	int  l_linger;
};

#ifndef CONFIG_NET_SOCKETS_POSIX_NAMES

static inline int socket(int family, int type, int proto)
{
	return zsock_socket(family, type, proto);
}

static inline int socketpair(int family, int type, int proto, int sv[2])
{
	return zsock_socketpair(family, type, proto, sv);
}

#define SHUT_RD ZSOCK_SHUT_RD
#define SHUT_WR ZSOCK_SHUT_WR
#define SHUT_RDWR ZSOCK_SHUT_RDWR

#define MSG_PEEK ZSOCK_MSG_PEEK
#define MSG_TRUNC ZSOCK_MSG_TRUNC
#define MSG_DONTWAIT ZSOCK_MSG_DONTWAIT
#define MSG_WAITALL ZSOCK_MSG_WAITALL

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

static inline int getpeername(int sock, struct sockaddr *addr,
			      socklen_t *addrlen)
{
	return zsock_getpeername(sock, addr, addrlen);
}

static inline int getsockname(int sock, struct sockaddr *addr,
			      socklen_t *addrlen)
{
	return zsock_getsockname(sock, addr, addrlen);
}

#endif /* CONFIG_NET_SOCKETS_POSIX_NAMES */

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_SYS_SOCKET_H_ */
