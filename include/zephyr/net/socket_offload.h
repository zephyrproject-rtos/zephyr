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
	/** DNS getaddrinfo offloaded implementation API */
	int (*getaddrinfo)(const char *node, const char *service,
			   const struct zsock_addrinfo *hints,
			   struct zsock_addrinfo **res);
	/** DNS freeaddrinfo offloaded implementation API */
	void (*freeaddrinfo)(struct zsock_addrinfo *res);
};

/**
 * @brief Register an offloaded socket DNS API interface.
 *
 * @param ops A pointer to the offloaded socket DNS API interface.
 */
void socket_offload_dns_register(const struct socket_dns_offload *ops);

/**
 * @brief Deregister an offloaded socket DNS API interface.
 *
 * @param ops A pointer to the offloaded socket DNS API interface.
 *
 * @retval 0 On success
 * @retval -EINVAL Offloaded DNS API was not regsitered.
 */
int socket_offload_dns_deregister(const struct socket_dns_offload *ops);

/**
 * @brief Enable/disable DNS offloading at runtime.
 *
 * @param enable Whether to enable or disable the DNS offloading.
 */
void socket_offload_dns_enable(bool enable);

/**
 * @brief Check if DNS offloading is enabled.
 *
 * @retval true DNS offloaded API is registered and enabled.
 * @retval false DNS offloading is disabled.
 */
#if defined(CONFIG_NET_SOCKETS_OFFLOAD)
bool socket_offload_dns_is_enabled(void);
#else
#define socket_offload_dns_is_enabled() false
#endif /* defined(CONFIG_NET_SOCKETS_OFFLOAD) */


/** @cond INTERNAL_HIDDEN */

int socket_offload_getaddrinfo(const char *node, const char *service,
			       const struct zsock_addrinfo *hints,
			       struct zsock_addrinfo **res);

void socket_offload_freeaddrinfo(struct zsock_addrinfo *res);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_OFFLOAD_H_ */
