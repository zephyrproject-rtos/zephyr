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

#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief An offloaded Socket DNS API interface
 *
 * It is assumed that these offload functions follow the
 * POSIX socket API standard for arguments, return values and setting of errno.
 */
struct socket_dns_offload {
	int (*getaddrinfo)(const char *node, const char *service,
			   const struct zsock_addrinfo *hints,
			   struct zsock_addrinfo **res);
	void (*freeaddrinfo)(struct zsock_addrinfo *res);
};

/**
 * @brief Register an offloaded socket DNS API interface.
 *
 * @param ops A pointer to the offloaded socket DNS API interface.
 */
void socket_offload_dns_register(const struct socket_dns_offload *ops);

int socket_offload_getaddrinfo(const char *node, const char *service,
			       const struct zsock_addrinfo *hints,
			       struct zsock_addrinfo **res);

void socket_offload_freeaddrinfo(struct zsock_addrinfo *res);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_OFFLOAD_H_ */
