/*
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Socket Offload Redirect API
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKET_OFFLOAD_H_
#define ZEPHYR_INCLUDE_NET_SOCKET_OFFLOAD_H_

#include <net/socket_offload_ops.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct socket_offload *socket_ops;

static inline int socket(int family, int type, int proto)
{
	__ASSERT_NO_MSG(socket_ops);
	__ASSERT_NO_MSG(socket_ops->socket);

	return socket_ops->socket(family, type, proto);
}

static inline int close(int sock)
{
	__ASSERT_NO_MSG(socket_ops);
	__ASSERT_NO_MSG(socket_ops->close);

	return socket_ops->close(sock);
}

static inline int accept(int sock, struct sockaddr *addr,
			 socklen_t *addrlen)
{
	__ASSERT_NO_MSG(socket_ops);
	__ASSERT_NO_MSG(socket_ops->accept);

	return socket_ops->accept(sock, addr, addrlen);
}


static inline int bind(int sock, const struct sockaddr *addr,
		       socklen_t addrlen)
{
	__ASSERT_NO_MSG(socket_ops);
	__ASSERT_NO_MSG(socket_ops->bind);

	return socket_ops->bind(sock, addr, addrlen);
}

static inline int listen(int sock, int backlog)
{
	__ASSERT_NO_MSG(socket_ops);
	__ASSERT_NO_MSG(socket_ops->listen);

	return socket_ops->listen(sock, backlog);
}

static inline int connect(int sock, const struct sockaddr *addr,
			  socklen_t addrlen)
{
	__ASSERT_NO_MSG(socket_ops);
	__ASSERT_NO_MSG(socket_ops->connect);

	return socket_ops->connect(sock, addr, addrlen);
}

static inline int poll(struct pollfd *fds, int nfds, int timeout)
{
	__ASSERT_NO_MSG(socket_ops);
	__ASSERT_NO_MSG(socket_ops->poll);

	return socket_ops->poll(fds, nfds, timeout);
}

static inline int setsockopt(int sock, int level, int optname,
			     const void *optval,
			     socklen_t optlen)
{
	__ASSERT_NO_MSG(socket_ops);
	__ASSERT_NO_MSG(socket_ops->setsockopt);

	return socket_ops->setsockopt(sock, level, optname, optval, optlen);
}

static inline int getsockopt(int sock, int level, int optname,
			     void *optval, socklen_t *optlen)
{
	__ASSERT_NO_MSG(socket_ops);
	__ASSERT_NO_MSG(socket_ops->getsockopt);

	return socket_ops->getsockopt(sock, level, optname, optval, optlen);
}

static inline ssize_t recv(int sock, void *buf, size_t max_len,
				      int flags)
{
	__ASSERT_NO_MSG(socket_ops);
	__ASSERT_NO_MSG(socket_ops->recv);

	return socket_ops->recv(sock, buf, max_len, flags);
}

static inline ssize_t recvfrom(int sock, void *buf,
			       short int len,
			       short int flags,
			       struct sockaddr *from,
			       socklen_t *fromlen)
{
	__ASSERT_NO_MSG(socket_ops);
	__ASSERT_NO_MSG(socket_ops->recvfrom);

	return socket_ops->recvfrom(sock, buf, len, flags, from, fromlen);
}

static inline ssize_t send(int sock, const void *buf, size_t len,
			   int flags)
{
	__ASSERT_NO_MSG(socket_ops);
	__ASSERT_NO_MSG(socket_ops->send);

	return socket_ops->send(sock, buf, len, flags);
}

static inline ssize_t sendto(int sock, const void *buf,
			     size_t len, int flags,
			     const struct sockaddr *to,
			     socklen_t tolen)
{
	__ASSERT_NO_MSG(socket_ops);
	__ASSERT_NO_MSG(socket_ops->sendto);

	return socket_ops->sendto(sock, buf, len, flags, to, tolen);
}

static inline int getaddrinfo(const char *node, const char *service,
			      const struct addrinfo *hints,
			      struct addrinfo **res)
{
	__ASSERT_NO_MSG(socket_ops);
	__ASSERT_NO_MSG(socket_ops->getaddrinfo);

	return socket_ops->getaddrinfo(node, service, hints, res);
}

static inline void freeaddrinfo(struct addrinfo *res)
{
	__ASSERT_NO_MSG(socket_ops);
	__ASSERT_NO_MSG(socket_ops->freeaddrinfo);

	return socket_ops->freeaddrinfo(res);
}

int fcntl(int fd, int cmd, ...);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_OFFLOAD_H_ */
