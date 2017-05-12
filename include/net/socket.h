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

#if defined(CONFIG_NET_SOCKETS_POSIX_NAMES)
#define socket zsock_socket
#define close zsock_close
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NET_SOCKET_H */
