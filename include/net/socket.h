/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_SOCKET_H
#define __NET_SOCKET_H

#include <zephyr/types.h>
#include <net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

int zsock_socket(int family, int type, int proto);
int zsock_close(int sock);
int zsock_bind(int sock, const struct sockaddr *addr, socklen_t addrlen);
int zsock_connect(int sock, const struct sockaddr *addr, socklen_t addrlen);
int zsock_listen(int sock, int backlog);
int zsock_accept(int sock, struct sockaddr *addr, socklen_t *addrlen);

#if defined(CONFIG_NET_SOCKETS_POSIX_NAMES)
#define socket zsock_socket
#define close zsock_close
#define bind zsock_bind
#define connect zsock_connect
#define listen zsock_listen
#define accept zsock_accept
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NET_SOCKET_H */
