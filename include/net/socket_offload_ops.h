/*
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Socket Offload Redirect API
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKET_OFFLOAD_OPS_H_
#define ZEPHYR_INCLUDE_NET_SOCKET_OFFLOAD_OPS_H_

#include <sys/types.h>
#include <net/net_ip.h>
#include <net/socket.h>  /* needed for struct pollfd */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Socket Offload Redirect API
 * @defgroup socket_offload Socket offloading interface
 * @ingroup networking
 * @{
 */

/**
 * @brief An offloaded Socket API interface
 *
 * It is assumed that these offload functions follow the
 * POSIX socket API standard for arguments, return values and setting of errno.
 */
struct socket_offload {
	/* POSIX Socket Functions: */
	int (*socket)(int family, int type, int proto);
	int (*close)(int sock);
	int (*accept)(int sock, struct sockaddr *addr, socklen_t *addrlen);
	int (*bind)(int sock, const struct sockaddr *addr, socklen_t addrlen);
	int (*listen)(int sock, int backlog);
	int (*connect)(int sock, const struct sockaddr *addr,
		       socklen_t addrlen);
	int (*poll)(struct pollfd *fds, int nfds, int timeout);
	int (*setsockopt)(int sock, int level, int optname,
			  const void *optval, socklen_t optlen);
	int (*getsockopt)(int sock, int level, int optname, void *optval,
			  socklen_t *optlen);
	ssize_t (*recv)(int sock, void *buf, size_t max_len, int flags);
	ssize_t (*recvfrom)(int sock, void *buf, short int len,
			    short int flags, struct sockaddr *from,
			    socklen_t *fromlen);
	ssize_t (*send)(int sock, const void *buf, size_t len, int flags);
	ssize_t (*sendto)(int sock, const void *buf, size_t len, int flags,
			  const struct sockaddr *to, socklen_t tolen);
	int (*getaddrinfo)(const char *node, const char *service,
			   const struct addrinfo *hints,
			   struct addrinfo **res);
	void (*freeaddrinfo)(struct addrinfo *res);
	int (*fcntl)(int fd, int cmd, va_list args);
};

/**
 * @brief Register an offloaded socket API interface.
 *
 * @param ops A pointer to the offloaded socket API interface.
 */
extern void socket_offload_register(const struct socket_offload *ops);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_OFFLOAD_OPS_H_ */
