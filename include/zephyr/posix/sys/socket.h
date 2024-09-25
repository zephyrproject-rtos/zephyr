/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_SYS_SOCKET_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_SOCKET_H_

#include <sys/types.h>
#include <zephyr/net/socket.h>

#define SHUT_RD   ZSOCK_SHUT_RD
#define SHUT_WR   ZSOCK_SHUT_WR
#define SHUT_RDWR ZSOCK_SHUT_RDWR

#define MSG_PEEK     ZSOCK_MSG_PEEK
#define MSG_TRUNC    ZSOCK_MSG_TRUNC
#define MSG_DONTWAIT ZSOCK_MSG_DONTWAIT
#define MSG_WAITALL  ZSOCK_MSG_WAITALL

#ifdef __cplusplus
extern "C" {
#endif

struct linger {
	int  l_onoff;
	int  l_linger;
};

int accept(int sock, struct sockaddr *addr, socklen_t *addrlen);
int bind(int sock, const struct sockaddr *addr, socklen_t addrlen);
int connect(int sock, const struct sockaddr *addr, socklen_t addrlen);
int getpeername(int sock, struct sockaddr *addr, socklen_t *addrlen);
int getsockname(int sock, struct sockaddr *addr, socklen_t *addrlen);
int getsockopt(int sock, int level, int optname, void *optval, socklen_t *optlen);
int listen(int sock, int backlog);
ssize_t recv(int sock, void *buf, size_t max_len, int flags);
ssize_t recvfrom(int sock, void *buf, size_t max_len, int flags, struct sockaddr *src_addr,
		 socklen_t *addrlen);
ssize_t recvmsg(int sock, struct msghdr *msg, int flags);
ssize_t send(int sock, const void *buf, size_t len, int flags);
ssize_t sendmsg(int sock, const struct msghdr *message, int flags);
ssize_t sendto(int sock, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr,
	       socklen_t addrlen);
int setsockopt(int sock, int level, int optname, const void *optval, socklen_t optlen);
int shutdown(int sock, int how);
int sockatmark(int s);
int socket(int family, int type, int proto);
int socketpair(int family, int type, int proto, int sv[2]);

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_SYS_SOCKET_H_ */
